#!/usr/bin/env python3
"""Validate AllocExtraBuffer scope, physical bits and extra buffers."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--objects", type=Path,
        default=repo / "Output/experiments/one_shot_bufferize_hivm_decompose_op/objects")
    parser.add_argument(
        "--tool", type=Path,
        default=module / "output/bin/cvpipeline_ub_model_dev_validate")
    args = parser.parse_args()
    objects = args.objects.resolve()
    tool = args.tool.resolve()
    failures: list[str] = []
    checked = buffers = 0
    with tempfile.TemporaryDirectory(prefix="cvub-alloc-extra-buffer-") as temporary:
        temp = Path(temporary)
        for source in sorted(objects.glob(
                "*/pass_tree/builtin_module_no-symbol-name/"
                "0_one-shot-bufferize.mlir")):
            tree = source.parents[1]
            after_hivm_decompose_op = source.with_name(
                "5_convert-non-contiguous-reshape-to-copy.mlir")
            after_infer_hivm_mem_scope = source.with_name("9_hivm-infer-mem-scope.mlir")
            model = temp / "model.tsv"
            oracle = temp / "oracle.tsv"
            commands = (
                [str(tool), "--action=model-alloc-extra-buffer-projection",
                 "--root", str(source), "--output", str(model)],
                [str(tool), "--action=dump-alloc-extra-buffer-projection-oracle",
                 "--root", str(source), "--attempt-root", str(after_hivm_decompose_op),
                 "--after-ir", str(after_infer_hivm_mem_scope),
                 "--expected-summary", str(tree),
                 "--output", str(oracle)],
            )
            errors = []
            for command in commands:
                run = subprocess.run(command, text=True, capture_output=True)
                if run.returncode:
                    errors.append(run.stderr.strip() or run.stdout.strip())
            if errors:
                failures.append(f"{source}: {'; '.join(errors)}")
                continue
            checked += 1
            buffers += oracle.read_bytes().count(b"\nBUFFER\t")
            if model.read_bytes() != oracle.read_bytes():
                model_lines = model.read_text().splitlines()
                oracle_lines = oracle.read_text().splitlines()
                different = next(
                    (index for index, pair in enumerate(
                        zip(model_lines, oracle_lines), 1)
                     if pair[0] != pair[1]),
                    min(len(model_lines), len(oracle_lines)) + 1)
                model_value = (model_lines[different - 1]
                               if different <= len(model_lines) else "<eof>")
                oracle_value = (oracle_lines[different - 1]
                                if different <= len(oracle_lines) else "<eof>")
                failures.append(
                    f"mismatch line={different}: {source}\n"
                    f"  oracle={oracle_value}\n  model={model_value}")

    if failures:
        print(f"ALLOC_EXTRA_BUFFER_PROJECTION=FAIL objects={checked} "
              f"failures={len(failures)}")
        print("\n".join(failures[:30]))
        return 1
    print(f"ALLOC_EXTRA_BUFFER_PROJECTION=PASS objects={checked} buffers={buffers}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
