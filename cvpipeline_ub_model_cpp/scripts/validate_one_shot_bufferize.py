#!/usr/bin/env python3
"""Validate lightweight OneShotAnalysis and UB allocation materialization."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


def run(tool: Path, action: str, source: Path, output: Path,
        after: Path | None = None) -> str | None:
    command = [str(tool), f"--action={action}", "--root", str(source),
               "--output", str(output)]
    if after is not None:
        command.extend(("--after-ir", str(after)))
    result = subprocess.run(command, text=True, capture_output=True)
    if result.returncode:
        return result.stderr.strip() or result.stdout.strip()
    return None


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--objects", type=Path,
        default=repo / "Output/experiments/one_shot_bufferize_hivm_decompose_op/objects")
    parser.add_argument(
        "--analysis", type=Path,
        default=repo / "Output/experiments/one_shot_analysis")
    parser.add_argument(
        "--tool", type=Path,
        default=module / "output/bin/cvpipeline_ub_model_dev_validate")
    args = parser.parse_args()

    inputs = sorted(args.objects.glob(
        "*/pass_tree/builtin_module_no-symbol-name/0_one-shot-bufferize.mlir"))
    if not inputs:
        print(f"[ERROR] no OneShotBufferize oracle objects under {args.objects}")
        return 2
    failures: list[str] = []
    allocation_count = 0
    checked_values = 0
    checked_accesses = 0
    with tempfile.TemporaryDirectory(prefix="cvub-bufferize-") as temporary:
        temp = Path(temporary)
        for source in inputs:
            object_hash = source.parents[2].name
            after = source.with_name("1_one-shot-bufferize.mlir")
            annotated = args.analysis / object_hash / "analysis.mlir"
            paths = {name: temp / name for name in
                     ("model_analysis", "oracle_analysis",
                      "model_allocations", "oracle_allocations",
                      "semantic_validation")}
            commands = (
                ("model-one-shot-analysis", source, paths["model_analysis"]),
                ("dump-one-shot-analysis-oracle", annotated,
                 paths["oracle_analysis"]),
                ("model-one-shot-bufferize-allocations", source,
                 paths["model_allocations"]),
                ("dump-one-shot-bufferize-allocation-oracle", after,
                 paths["oracle_allocations"]),
            )
            errors = [error for action, root, output in commands
                      if (error := run(args.tool, action, root, output))]
            semantic_error = run(
                args.tool, "validate-bufferized-semantic-ir", source,
                paths["semantic_validation"], after)
            if semantic_error:
                errors.append(semantic_error)
            if errors:
                failures.append(f"{object_hash}: {'; '.join(errors)}")
                continue
            if paths["model_analysis"].read_bytes() != \
                    paths["oracle_analysis"].read_bytes():
                failures.append(f"{object_hash}: analysis mismatch")
            if paths["model_allocations"].read_bytes() != \
                    paths["oracle_allocations"].read_bytes():
                failures.append(f"{object_hash}: allocation mismatch")
            allocation_count += paths["oracle_allocations"].read_bytes().count(
                b"\nALLOC\t")
            summary = paths["semantic_validation"].read_text().splitlines()[1]
            _, values, accesses, semantic_failures = summary.split("\t")
            checked_values += int(values)
            checked_accesses += int(accesses)
            if int(semantic_failures):
                failures.append(f"{object_hash}: semantic IR mismatch")

    if failures:
        print("\n".join(failures[:40]))
        print(f"ONE_SHOT_BUFFERIZE=FAIL objects={len(inputs)} "
              f"failures={len(failures)}")
        return 1
    print(f"ONE_SHOT_BUFFERIZE=PASS objects={len(inputs)} "
          f"allocations={allocation_count} values={checked_values} "
          f"accesses={checked_accesses}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
