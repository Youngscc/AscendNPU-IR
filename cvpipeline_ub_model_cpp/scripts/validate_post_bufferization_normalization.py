#!/usr/bin/env python3
"""Validate the connected post-OneShotBufferize allocation normalization."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    objects = repo / "Output/experiments/c2_c3_pass_oracles/objects"
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    failures: list[str] = []
    allocations = 0
    sources = sorted(objects.glob(
        "*/pass_tree/builtin_module_no-symbol-name/0_one-shot-bufferize.mlir"))
    with tempfile.TemporaryDirectory(prefix="cvub-post-buffer-") as temporary:
        temp = Path(temporary)
        for source in sources:
            tree = source.parent
            candidates = sorted(tree.glob("func_*/3_0_hivm-decompose-op.mlir"))
            if not candidates:
                failures.append(f"missing decompose entry: {source}")
                continue
            before_decompose = candidates[0]
            model = temp / "model.tsv"
            oracle = temp / "oracle.tsv"
            model_drops = temp / "model-drops.tsv"
            oracle_drops = temp / "oracle-drops.tsv"
            after_one_shot = source.with_name("1_one-shot-bufferize.mlir")
            commands = (
                [str(tool), "--action=model-post-bufferization-allocations",
                 "--root", str(source), "--output", str(model)],
                [str(tool), "--action=dump-c2-allocation-oracle",
                 "--root", str(before_decompose), "--output", str(oracle)],
                [str(tool), "--action=model-canonicalized-iter-arg-results",
                 "--root", str(source), "--output", str(model_drops)],
                [str(tool),
                 "--action=dump-canonicalized-iter-arg-results-oracle",
                 "--root", str(after_one_shot), "--after-ir",
                 str(before_decompose), "--output", str(oracle_drops)],
            )
            errors = []
            for command in commands:
                run = subprocess.run(command, text=True, capture_output=True)
                if run.returncode:
                    errors.append(run.stderr.strip() or run.stdout.strip())
            if errors:
                failures.append(f"{source}: {'; '.join(errors)}")
                continue
            allocations += oracle.read_bytes().count(b"\nALLOC\t")
            if model.read_bytes() != oracle.read_bytes():
                failures.append(f"allocation normalization mismatch: {source}")
            if model_drops.read_bytes() != oracle_drops.read_bytes():
                failures.append(f"dropped-result mismatch: {source}")

    if failures:
        print(f"POST_BUFFERIZATION_NORMALIZATION=FAIL objects={len(sources)} "
              f"failures={len(failures)}")
        print("\n".join(failures[:30]))
        return 1
    print(f"POST_BUFFERIZATION_NORMALIZATION=PASS objects={len(sources)} "
          f"allocations={allocations}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
