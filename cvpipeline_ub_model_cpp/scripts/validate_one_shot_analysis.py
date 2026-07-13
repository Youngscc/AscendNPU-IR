#!/usr/bin/env python3
"""Compare lightweight OneShotAnalysis decisions with annotated upstream IR."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--objects", type=Path, default=repo / (
        "Output/experiments/c2_c3_pass_oracles/objects"))
    parser.add_argument("--analysis", type=Path, default=repo / (
        "Output/experiments/one_shot_analysis"))
    parser.add_argument("--tool", type=Path, default=module / (
        "output/bin/cvpipeline_ub_model_dev_validate"))
    parser.add_argument("--max-objects", type=int, default=0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    inputs = sorted(args.objects.glob(
        "*/pass_tree/builtin_module_no-symbol-name/0_one-shot-bufferize.mlir"))
    if args.max_objects:
        inputs = inputs[:args.max_objects]
    failures = []
    with tempfile.TemporaryDirectory(prefix="cvub-one-shot-") as tmp:
        tmpdir = Path(tmp)
        for index, source in enumerate(inputs):
            object_hash = source.parents[2].name
            annotated = args.analysis / object_hash / "analysis.mlir"
            model = tmpdir / "model.tsv"
            oracle = tmpdir / "oracle.tsv"
            commands = (
                [str(args.tool), "--action=model-one-shot-analysis",
                 "--root", str(source), "--output", str(model)],
                [str(args.tool), "--action=dump-one-shot-analysis-oracle",
                 "--root", str(annotated), "--output", str(oracle)],
            )
            errors = []
            for command in commands:
                run = subprocess.run(command, text=True, capture_output=True)
                if run.returncode:
                    errors.append(run.stderr.strip() or run.stdout.strip())
            if errors:
                failures.append(f"{object_hash}: {'; '.join(errors)}")
            elif model.read_bytes() != oracle.read_bytes():
                model_lines = model.read_text().splitlines()
                oracle_lines = oracle.read_text().splitlines()
                difference = next((
                    (line_number, expected, actual)
                    for line_number, (expected, actual) in enumerate(
                        zip(oracle_lines, model_lines), start=1)
                    if expected != actual), None)
                failures.append(f"{object_hash}: first_difference={difference}")
            if index and index % 50 == 0:
                print(f"checked={index}", flush=True)
    if failures:
        print(f"ONE_SHOT_ANALYSIS=FAIL objects={len(inputs)} "
              f"failures={len(failures)}")
        print("\n".join(failures[:40]))
        return 1
    print(f"ONE_SHOT_ANALYSIS=PASS objects={len(inputs)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
