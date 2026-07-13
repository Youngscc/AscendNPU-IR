#!/usr/bin/env python3
"""Capture native MarkMultiBuffer snapshots for the C6 strategy matrix."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


PASS_NAME = "hivm-mark-multi-buffer"
CONFIGS = (
    ("disabled", False, "no-l0c", "only-cube"),
    ("auto_nol0c_cube", True, "no-l0c", "only-cube"),
    ("auto_all_cube", True, "no-limit", "only-cube"),
    ("auto_nol0c_vector", True, "no-l0c", "only-vector"),
    ("auto_all_vector", True, "no-limit", "only-vector"),
    ("auto_nol0c_all", True, "no-l0c", "no-limit"),
    ("auto_all_all", True, "no-limit", "no-limit"),
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--c0-summary", type=Path,
                        default=repo / "Output/index/c0_cvpipeline_after/summary.tsv")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--output-root", type=Path, default=repo / "Output")
    parser.add_argument("--max-objects", type=int, default=0)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--no-resume", action="store_true")
    return parser.parse_args()


def pass_pairs(tree: Path) -> int:
    before = list(tree.rglob(f"*_0_{PASS_NAME}.mlir"))
    after = list(tree.rglob(f"*_1_{PASS_NAME}.mlir"))
    return len(before) if len(before) == len(after) and before else 0


def run_one(tool: Path, input_hash: str, input_path: Path,
            config: tuple[str, bool, str, str], object_root: Path,
            tool_hash: str, resume: bool) -> dict[str, str]:
    config_id, enabled, local, mix = config
    directory = object_root / input_hash / config_id
    tree = directory / "pass_tree"
    provenance = directory / "provenance.json"
    fingerprint = {
        "schema_version": 1, "input_sha256": input_hash,
        "oracle_tool_sha256": tool_hash, "pass": PASS_NAME,
        "enable_auto": enabled, "local_strategy": local,
        "mix_strategy": mix,
    }
    reusable = False
    if resume and provenance.is_file() and tree.is_dir():
        try:
            reusable = json.loads(provenance.read_text()) == fingerprint
            reusable = reusable and pass_pairs(tree) > 0
        except (OSError, json.JSONDecodeError):
            reusable = False
    status = 0
    if not reusable:
        if tree.exists():
            shutil.rmtree(tree)
        directory.mkdir(parents=True, exist_ok=True)
        command = [
            str(tool), str(input_path), "--disable-cv-pipelining",
            "--enable-memory-display=false", "--mlir-disable-threading",
            "--mlir-print-op-generic", "--mlir-print-ir-module-scope",
            f"--mlir-print-ir-tree-dir={tree}",
            f"--mlir-print-ir-before={PASS_NAME}",
            f"--mlir-print-ir-after={PASS_NAME}",
            f"--enable-auto-multi-buffer={'true' if enabled else 'false'}",
            f"--limit-auto-multi-buffer-of-local-buffer={local}",
            f"--limit-auto-multi-buffer-buffer={mix}",
            "-o", str(directory / "suffix.mlir"),
        ]
        with (directory / "stdout.log").open("wb") as stdout, \
                (directory / "stderr.log").open("wb") as stderr:
            status = subprocess.run(command, stdout=stdout, stderr=stderr).returncode
        if pass_pairs(tree):
            provenance.write_text(json.dumps(fingerprint, sort_keys=True) + "\n")
    pairs = pass_pairs(tree) if tree.is_dir() else 0
    return {
        "after_cvpipeline_sha256": input_hash,
        "after_cvpipeline_ir": str(input_path), "config_id": config_id,
        "enable_auto": str(int(enabled)), "local_strategy": local,
        "mix_strategy": mix,
        "status": "complete" if pairs else "incomplete",
        "compiler_status": str(status), "snapshot_pairs": str(pairs),
        "pass_tree": str(tree), "provenance": str(provenance),
        "oracle_tool_sha256": tool_hash,
    }


def main() -> int:
    args = parse_args()
    with args.c0_summary.resolve().open(newline="") as stream:
        rows = [row for row in csv.DictReader(stream, delimiter="\t")
                if row["status"] == "complete"]
    inputs: dict[str, Path] = {}
    for row in rows:
        inputs.setdefault(row["after_cvpipeline_sha256"],
                          Path(row["after_cvpipeline_ir"]))
    selected = dict(sorted(inputs.items()))
    if args.max_objects:
        selected = dict(list(selected.items())[:args.max_objects])
    output = args.output_root.resolve()
    object_root = output / "experiments/c6_pass_oracles/objects"
    index_root = output / "index/c6_pass_oracles"
    object_root.mkdir(parents=True, exist_ok=True)
    index_root.mkdir(parents=True, exist_ok=True)
    tool = args.tool.resolve()
    tool_hash = sha256(tool)
    futures = []
    results = []
    with ThreadPoolExecutor(max_workers=max(1, args.workers)) as pool:
        for input_hash, input_path in selected.items():
            for config in CONFIGS:
                futures.append(pool.submit(
                    run_one, tool, input_hash, input_path, config, object_root,
                    tool_hash, not args.no_resume))
        for future in as_completed(futures):
            results.append(future.result())
    results.sort(key=lambda row: (row["after_cvpipeline_sha256"],
                                  row["config_id"]))
    fields = list(results[0]) if results else []
    with (index_root / "summary.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(results)
    complete = sum(row["status"] == "complete" for row in results)
    print(f"C6_ORACLES objects={len(selected)} configs={len(CONFIGS)} "
          f"tuples={len(results)} complete={complete} "
          f"function_pairs={sum(int(row['snapshot_pairs']) for row in results)}")
    return 0 if complete == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
