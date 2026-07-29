#!/usr/bin/env python3
"""Compare lightweight AutoBlockify with checkpoints from one native attempt."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import asdict, dataclass
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_FLAGS = (
    "--enable-hfusion-compile=true",
    "--enable-triton-kernel-compile=true",
    "--enable-auto-blockify-loop=true",
    "--mlir-disable-threading",
)


@dataclass
class Result:
    adapter: str
    status: str
    detail: str = ""
    artifacts: str = ""


def run(command: list[str], environment: dict[str, str] | None = None):
    return subprocess.run(
        command,
        cwd=ROOT,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def discover_adapters(adapter_root: Path, selection_root: Path) -> list[Path]:
    names = sorted(path.name for path in selection_root.iterdir() if path.is_dir())
    adapters = [adapter_root / name for name in names]
    missing = [path for path in adapters if not path.is_file()]
    if missing:
        raise RuntimeError(
            "missing adapters selected by before-CV corpus: "
            + ", ".join(str(path) for path in missing)
        )
    return adapters


def preserve_failure(work: Path, failure_root: Path | None, ordinal: int) -> str:
    if failure_root is None:
        return ""
    destination = failure_root / f"{ordinal:04d}_{work.name}"
    shutil.copytree(work, destination)
    return str(destination)


def verify_one(
    ordinal: int,
    adapter: Path,
    compiler: Path,
    runner: Path,
    temporary_root: Path,
    failure_root: Path | None,
) -> Result:
    work = temporary_root / f"case_{ordinal:04d}_{adapter.name}"
    checkpoints = work / "checkpoints"
    work.mkdir()
    environment = os.environ.copy()
    environment["BISHENGIR_UB_PREFIX_CHECKPOINT_DIR"] = str(checkpoints)
    environment["BISHENGIR_STOP_AFTER_UB_PREFIX_CHECKPOINTS"] = "1"
    environment.pop("BISHENGIR_UB_PREFIX_CHECKPOINT_STAGE", None)
    native = run(
        [
            str(compiler),
            str(adapter),
            *DEFAULT_FLAGS,
            "-o",
            str(work / "unused.o"),
        ],
        environment,
    )
    attempt = checkpoints / "attempt-1"
    before = attempt / "00_before_auto_blockify.mlir"
    after = attempt / "01_after_auto_blockify.mlir"
    if native.returncode != 0 or not before.is_file() or not after.is_file():
        artifacts = preserve_failure(work, failure_root, ordinal)
        diagnostic = (native.stderr or native.stdout).strip()[-2000:]
        return Result(str(adapter), "NATIVE_FAILED", diagnostic, artifacts)

    model = run([str(runner), "--apply-model", str(before)])
    oracle = run([str(runner), str(after)])
    if model.returncode != 0:
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(adapter), "MODEL_FAILED", model.stderr.strip()[-2000:], artifacts
        )
    if oracle.returncode != 0:
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(adapter),
            "ORACLE_SNAPSHOT_FAILED",
            oracle.stderr.strip()[-2000:],
            artifacts,
        )
    if model.stdout != oracle.stdout:
        (work / "model.snapshot.tsv").write_text(model.stdout, encoding="utf-8")
        (work / "native.snapshot.tsv").write_text(oracle.stdout, encoding="utf-8")
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(adapter),
            "MISMATCH",
            "structural snapshots differ",
            artifacts,
        )
    return Result(str(adapter), "PASS")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--adapter-root", default="ub_overflow_model_cpp/data/adapter"
    )
    parser.add_argument(
        "--selection-root",
        default="ub_overflow_model_cpp/data/before_cvpipelining",
        help="directory names select the de-duplicated adapter set",
    )
    parser.add_argument("--compiler", default="build/bin/bishengir-compile")
    parser.add_argument(
        "--model-runner",
        default="ub_overflow_model_cpp/output/tests/auto_blockify_model_runner",
    )
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--limit", type=int)
    parser.add_argument("--failure-dir")
    parser.add_argument("--json-report")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    compiler = Path(arguments.compiler).resolve()
    runner = Path(arguments.model_runner).resolve()
    if not compiler.is_file() or not runner.is_file():
        raise RuntimeError("compiler and model runner must already be built")
    adapters = discover_adapters(
        Path(arguments.adapter_root), Path(arguments.selection_root)
    )
    if arguments.limit is not None:
        adapters = adapters[: arguments.limit]
    if arguments.jobs < 1:
        raise RuntimeError("--jobs must be positive")
    failure_root = (
        Path(arguments.failure_dir).resolve() if arguments.failure_dir else None
    )
    if failure_root:
        failure_root.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="auto-blockify-pipeline-") as temp:
        temporary_root = Path(temp)
        with ThreadPoolExecutor(max_workers=arguments.jobs) as executor:
            futures = [
                executor.submit(
                    verify_one,
                    ordinal,
                    adapter.resolve(),
                    compiler,
                    runner,
                    temporary_root,
                    failure_root,
                )
                for ordinal, adapter in enumerate(adapters, start=1)
            ]
            results = [future.result() for future in futures]

    counts: dict[str, int] = {}
    for result in results:
        counts[result.status] = counts.get(result.status, 0) + 1
        suffix = f": {result.detail}" if result.detail else ""
        print(f"[{result.status}] {result.adapter}{suffix}")
    report = {
        "compiler": str(compiler),
        "model_runner": str(runner),
        "total": len(results),
        "counts": counts,
        "results": [asdict(result) for result in results],
    }
    if arguments.json_report:
        report_path = Path(arguments.json_report)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8"
        )
    print(f"summary: {json.dumps(counts, sort_keys=True)}")
    return 0 if counts == {"PASS": len(results)} else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"verify_auto_blockify_pipeline: {error}", file=sys.stderr)
        raise SystemExit(2)
