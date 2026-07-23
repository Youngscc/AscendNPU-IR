#!/usr/bin/env python3
"""Differentially verify the replicated AutoBlockifyParallelLoop transform.

Each input is first round-tripped through bishengir-opt in Generic MLIR form.
That exact IR is then transformed independently by:

  1. the real `-auto-blockify-parallel-loop` pass, and
  2. `RunAutoBlockifyParallelLoop` in the standalone C++ model.

Both results are parsed by the same model-side parser, compacted, serialized as
a structural TSV snapshot, and compared byte-for-byte.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass
class Result:
    input: str
    status: str
    detail: str = ""
    artifacts: str = ""
    compatibility_rewrites: int = 0


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, text=True, capture_output=True, check=False)


def diagnostic(process: subprocess.CompletedProcess[str]) -> str:
    message = process.stderr.strip() or process.stdout.strip()
    return message[-2000:]


def failure_class(process: subprocess.CompletedProcess[str]) -> str:
    message = (process.stderr + "\n" + process.stdout).lower()
    if "logical block number not found" in message or "logical_block_num mark not found" in message:
        return "LOGICAL_BLOCK_NOT_FOUND"
    if "physical block num cannot be inferred" in message:
        return "PHYSICAL_BLOCK_NOT_INFERRED"
    return "UNKNOWN"


def discover_inputs(args: argparse.Namespace) -> list[Path]:
    paths = [Path(path) for path in args.input]
    if args.corpus_root:
        paths.extend(sorted(Path(args.corpus_root).rglob("*.mlir")))
    unique = sorted({path.resolve() for path in paths})
    if args.only_block_idx:
        unique = [
            path
            for path in unique
            if '"hivm.hir.get_block_idx"' in path.read_text(errors="replace")
        ]
    if args.limit is not None:
        unique = unique[: args.limit]
    if not unique:
        raise ValueError("no MLIR inputs were selected")
    return unique


def preserve_failure(work: Path, root: Path | None, ordinal: int) -> str:
    if root is None:
        return ""
    destination = root / f"{ordinal:04d}_{work.name}"
    shutil.copytree(work, destination)
    return str(destination)


def repair_known_schema_drift(source: Path, destination: Path) -> int:
    """Repair corpus syntax that predates the current vcmp segment schema.

    The third segment describes optional mask operands. Adding the missing zero
    changes no SSA use or AutoBlockify behavior; it only makes the old generic
    op acceptable to the current dialect parser.
    """
    lines = source.read_text().splitlines(keepends=True)
    needle = "operandSegmentSizes = array<i32: 2, 1>"
    replacement = "operandSegmentSizes = array<i32: 2, 1, 0>"
    rewrites = 0
    repaired: list[str] = []
    for line in lines:
        if '"hivm.hir.vcmp"' in line and needle in line:
            line = line.replace(needle, replacement)
            rewrites += 1
        repaired.append(line)
    destination.write_text("".join(repaired))
    return rewrites


def verify_one(
    source: Path,
    compiler: Path,
    runner: Path,
    work: Path,
    failure_root: Path | None,
    ordinal: int,
    repair_schema_drift: bool,
) -> Result:
    generic_before = work / "before.generic.mlir"
    oracle_after = work / "oracle.after.generic.mlir"
    oracle_snapshot = work / "oracle.snapshot.tsv"
    model_snapshot = work / "model.snapshot.tsv"

    compiler_input = source
    compatibility_rewrites = 0
    normalized = run(
        [str(compiler), str(source), "--mlir-print-op-generic", "-o", str(generic_before)]
    )
    if normalized.returncode != 0 and repair_schema_drift:
        compiler_input = work / "before.compatible.mlir"
        compatibility_rewrites = repair_known_schema_drift(source, compiler_input)
        if compatibility_rewrites:
            normalized = run(
                [
                    str(compiler),
                    str(compiler_input),
                    "--mlir-print-op-generic",
                    "-o",
                    str(generic_before),
                ]
            )
    if normalized.returncode != 0:
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(source),
            "INPUT_REJECTED",
            diagnostic(normalized),
            artifacts,
            compatibility_rewrites,
        )

    oracle = run(
        [
            str(compiler),
            str(generic_before),
            "-auto-blockify-parallel-loop",
            "-verify-each",
            "--mlir-print-op-generic",
            "-o",
            str(oracle_after),
        ]
    )
    model = run([str(runner), "--apply-model", str(generic_before)])
    if oracle.returncode != 0:
        oracle_failure = failure_class(oracle)
        model_failure = failure_class(model) if model.returncode != 0 else "NONE"
        if oracle_failure != "UNKNOWN" and oracle_failure == model_failure:
            return Result(
                str(source),
                "PASS_EXPECTED_FAILURE",
                oracle_failure,
                compatibility_rewrites=compatibility_rewrites,
            )
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(source),
            "FAILURE_MISMATCH",
            f"oracle={oracle_failure}; model={model_failure}; "
            f"oracle diagnostic: {diagnostic(oracle)}; "
            f"model diagnostic: {diagnostic(model)}",
            artifacts,
            compatibility_rewrites,
        )

    if model.returncode != 0:
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(source),
            "MODEL_FAILED",
            diagnostic(model),
            artifacts,
            compatibility_rewrites,
        )
    model_snapshot.write_text(model.stdout)

    snapshot = run([str(runner), str(oracle_after)])
    if snapshot.returncode != 0:
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(source),
            "SNAPSHOT_FAILED",
            diagnostic(snapshot),
            artifacts,
            compatibility_rewrites,
        )
    oracle_snapshot.write_text(snapshot.stdout)

    if model.stdout != snapshot.stdout:
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(source),
            "MISMATCH",
            "model and oracle structural snapshots differ",
            artifacts,
            compatibility_rewrites,
        )
    detail = (
        f"applied {compatibility_rewrites} vcmp schema compatibility rewrite(s)"
        if compatibility_rewrites
        else ""
    )
    return Result(
        str(source), "PASS", detail, compatibility_rewrites=compatibility_rewrites
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", action="append", default=[], help="input MLIR; repeatable")
    parser.add_argument("--corpus-root", help="recursively test all MLIR files below this path")
    parser.add_argument(
        "--only-block-idx",
        action="store_true",
        help="select only files containing a generic hivm.hir.get_block_idx op",
    )
    parser.add_argument("--limit", type=int, help="test only the first N selected files")
    parser.add_argument("--compiler", default="build/bin/bishengir-opt")
    parser.add_argument(
        "--model-runner",
        default="ub_overflow_model_cpp/output/tests/auto_blockify_model_runner",
    )
    parser.add_argument("--failure-dir", help="preserve full artifacts for failing cases")
    parser.add_argument("--json-report", help="write machine-readable results here")
    parser.add_argument(
        "--repair-known-schema-drift",
        action="store_true",
        help="repair old vcmp operand segment metadata in a temporary copy",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        inputs = discover_inputs(args)
    except (OSError, ValueError) as error:
        print(f"verify_auto_blockify: {error}", file=sys.stderr)
        return 2

    compiler = Path(args.compiler).resolve()
    runner = Path(args.model_runner).resolve()
    for executable in (compiler, runner):
        if not executable.is_file():
            print(f"verify_auto_blockify: executable not found: {executable}", file=sys.stderr)
            return 2

    failure_root = Path(args.failure_dir).resolve() if args.failure_dir else None
    if failure_root:
        failure_root.mkdir(parents=True, exist_ok=True)

    results: list[Result] = []
    with tempfile.TemporaryDirectory(prefix="auto-blockify-verify-") as temp:
        temp_root = Path(temp)
        for ordinal, source in enumerate(inputs, start=1):
            work = temp_root / f"case_{ordinal:04d}"
            work.mkdir()
            result = verify_one(
                source,
                compiler,
                runner,
                work,
                failure_root,
                ordinal,
                args.repair_known_schema_drift,
            )
            results.append(result)
            suffix = f": {result.detail}" if result.detail else ""
            print(f"[{result.status}] {source}{suffix}")

    counts: dict[str, int] = {}
    for result in results:
        counts[result.status] = counts.get(result.status, 0) + 1
    report = {
        "compiler": str(compiler),
        "model_runner": str(runner),
        "total": len(results),
        "counts": counts,
        "results": [asdict(result) for result in results],
    }
    if args.json_report:
        report_path = Path(args.json_report)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    print(f"summary: {json.dumps(counts, sort_keys=True)}")
    passing_statuses = {"PASS", "PASS_EXPECTED_FAILURE"}
    return 0 if all(result.status in passing_statuses for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
