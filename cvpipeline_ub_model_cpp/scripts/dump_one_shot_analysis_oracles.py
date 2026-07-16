#!/usr/bin/env python3
"""Dump upstream OneShotAnalysis decisions for source-forward validation."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--objects", type=Path,
        default=repo / "Output/experiments/one_shot_bufferize_hivm_decompose_op/objects")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-opt")
    parser.add_argument("--output", type=Path,
                        default=repo / "Output/experiments/one_shot_analysis")
    parser.add_argument("--max-objects", type=int, default=0)
    parser.add_argument("--no-resume", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    inputs = sorted(args.objects.glob(
        "*/pass_tree/builtin_module_no-symbol-name/0_one-shot-bufferize.mlir"))
    if args.max_objects:
        inputs = inputs[:args.max_objects]
    args.output.mkdir(parents=True, exist_ok=True)
    tool_hash = sha256(args.tool)
    failures = []
    for index, source in enumerate(inputs):
        object_hash = source.parents[2].name
        directory = args.output / object_hash
        output = directory / "analysis.mlir"
        provenance = directory / "provenance.json"
        fingerprint = {
            "schema_version": 1,
            "input": str(source.resolve()),
            "input_sha256": sha256(source),
            "tool_sha256": tool_hash,
            "options": {
                "allow_return_allocs_from_loops": True,
                "allow_unknown_ops": True,
                "bufferize_function_boundaries": True,
                "function_boundary_type_conversion": "identity-layout-map",
                "test_analysis_only": True,
                "print_conflicts": True,
                "dump_alias_sets": True,
            },
        }
        reusable = False
        if not args.no_resume and output.is_file() and provenance.is_file():
            try:
                reusable = json.loads(provenance.read_text()) == fingerprint
            except json.JSONDecodeError:
                reusable = False
        if reusable:
            continue
        directory.mkdir(parents=True, exist_ok=True)
        options = (
            "allow-return-allocs-from-loops allow-unknown-ops "
            "bufferize-function-boundaries "
            "function-boundary-type-conversion=identity-layout-map "
            "test-analysis-only print-conflicts dump-alias-sets"
        )
        run = subprocess.run(
            [str(args.tool), str(source), f"--one-shot-bufferize={options}",
             "--mlir-print-op-generic", "--mlir-disable-threading",
             "-o", str(output)],
            text=True, capture_output=True)
        if run.returncode:
            failures.append(f"{object_hash}: {run.stderr.strip()}")
            continue
        provenance.write_text(json.dumps(fingerprint, sort_keys=True) + "\n")
        if index and index % 50 == 0:
            print(f"dumped={index}", flush=True)
    if failures:
        print(f"ONE_SHOT_ANALYSIS_ORACLE=FAIL failures={len(failures)}")
        print("\n".join(failures[:20]))
        return 1
    print(f"ONE_SHOT_ANALYSIS_ORACLE=PASS objects={len(inputs)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
