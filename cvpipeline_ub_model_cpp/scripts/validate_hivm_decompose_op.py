#!/usr/bin/env python3
"""Validate connected C2-to-HIVMDecomposeOp UB buffer materialization."""

from __future__ import annotations

import argparse
import re
import subprocess
import tempfile
from pathlib import Path


HEADER = re.compile(r"operation: @([^)]*)\)")


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--objects", type=Path,
        default=repo / "Output/experiments/c2_c3_pass_oracles/objects")
    parser.add_argument(
        "--tool", type=Path,
        default=module / "output/bin/cvpipeline_ub_model_dev_validate")
    args = parser.parse_args()

    failures: list[str] = []
    checked = 0
    allocations = 0
    with tempfile.TemporaryDirectory(prefix="cvub-decompose-") as temporary:
        temp = Path(temporary)
        for before in sorted(args.objects.glob(
                "*/pass_tree/**/3_0_hivm-decompose-op.mlir")):
            after = before.with_name("3_1_hivm-decompose-op.mlir")
            match = HEADER.search(before.open(errors="replace").readline())
            if not after.is_file() or not match:
                failures.append(f"malformed boundary: {before}")
                continue
            function = match.group(1)
            object_root = before
            while object_root.name != "pass_tree":
                object_root = object_root.parent
            source = (object_root /
                      "builtin_module_no-symbol-name/0_one-shot-bufferize.mlir")
            model = temp / "model.tsv"
            oracle = temp / "oracle.tsv"
            model_delta = temp / "model-delta.tsv"
            oracle_delta = temp / "oracle-delta.tsv"
            rewrite_report = temp / "rewrite-report.tsv"
            commands = (
                [str(args.tool), "--action=model-hivm-decompose-op",
                 "--root", str(source), "--function", function,
                 "--output", str(model)],
                [str(args.tool), "--action=dump-hivm-decompose-op-oracle",
                 "--root", str(before), "--after-ir", str(after),
                 "--function", function, "--output", str(oracle)],
                [str(args.tool),
                 "--action=model-hivm-decompose-operation-delta",
                 "--root", str(source), "--function", function,
                 "--output", str(model_delta)],
                [str(args.tool),
                 "--action=dump-hivm-decompose-operation-delta-oracle",
                 "--root", str(before), "--after-ir", str(after),
                 "--function", function, "--output", str(oracle_delta)],
                [str(args.tool), "--action=validate-c3-operation-rewrites",
                 "--root", str(source), "--attempt-root", str(before),
                 "--after-ir", str(after), "--function", function,
                 "--output", str(rewrite_report)],
            )
            errors = []
            for command in commands:
                run = subprocess.run(command, text=True, capture_output=True)
                if run.returncode:
                    errors.append(run.stderr.strip() or run.stdout.strip())
            if errors:
                failures.append(f"{before}: {'; '.join(errors)}")
                continue
            checked += 1
            allocations += oracle.read_bytes().count(b"\nALLOC\t")
            if model.read_bytes() != oracle.read_bytes():
                failures.append(
                    f"{before}\n  model={model.read_text().strip()}\n"
                    f"  oracle={oracle.read_text().strip()}")
            if model_delta.read_bytes() != oracle_delta.read_bytes():
                failures.append(
                    f"{before} operation delta\n"
                    f"  model={model_delta.read_text().strip()}\n"
                    f"  oracle={oracle_delta.read_text().strip()}")

    if failures:
        print(f"HIVM_DECOMPOSE_OP=FAIL pairs={checked} "
              f"failures={len(failures)}")
        print("\n".join(failures[:30]))
        return 1
    print(f"HIVM_DECOMPOSE_OP=PASS pairs={checked} "
          f"allocations={allocations}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
