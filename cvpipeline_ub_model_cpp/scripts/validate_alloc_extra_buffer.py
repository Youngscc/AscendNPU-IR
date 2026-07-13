#!/usr/bin/env python3
"""Validate the lightweight AllocExtraBuffer port at every real pass boundary."""

from __future__ import annotations

import argparse
import re
import subprocess
import tempfile
from pathlib import Path


HEADER = re.compile(r"operation: @([^)]*)\)")


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--objects",
        type=Path,
        default=module.parent / "Output/experiments/c2_c3_pass_oracles/objects",
    )
    parser.add_argument(
        "--tool",
        type=Path,
        default=module / "output/bin/cvpipeline_ub_model_dev_validate",
    )
    parser.add_argument("--max-pairs", type=int, default=0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    pairs: list[tuple[Path, Path, str]] = []
    for before in sorted(args.objects.glob(
            "*/pass_tree/**/7_18_hivm-alloc-extra-buffer.mlir")):
        after = before.with_name("7_19_hivm-alloc-extra-buffer.mlir")
        header = before.open(errors="replace").readline()
        match = HEADER.search(header)
        if not after.is_file() or not match:
            raise RuntimeError(f"malformed AllocExtraBuffer boundary: {before}")
        pairs.append((before, after, match.group(1)))
    if args.max_pairs:
        pairs = pairs[:args.max_pairs]

    failures: list[str] = []
    allocation_count = 0
    with tempfile.TemporaryDirectory(prefix="cvub-extra-buffer-") as tmp:
        temporary = Path(tmp)
        for index, (before, after, function) in enumerate(pairs):
            model = temporary / "model.tsv"
            oracle = temporary / "oracle.tsv"
            commands = (
                [str(args.tool), "--action=model-alloc-extra-buffer",
                 "--root", str(before), "--function", function,
                 "--output", str(model)],
                [str(args.tool), "--action=dump-alloc-extra-buffer-oracle",
                 "--root", str(before), "--after-ir", str(after),
                 "--output", str(oracle)],
            )
            errors = []
            for command in commands:
                run = subprocess.run(command, text=True, capture_output=True)
                if run.returncode:
                    errors.append(run.stderr.strip() or run.stdout.strip())
            if errors:
                failures.append(f"{before}: {'; '.join(errors)}")
                continue
            model_bytes = model.read_bytes()
            oracle_bytes = oracle.read_bytes()
            allocation_count += oracle_bytes.count(b"\nALLOC\t")
            if model_bytes != oracle_bytes:
                failures.append(
                    f"{before} function={function}\n"
                    f"  oracle={oracle.read_text().strip()}\n"
                    f"  model={model.read_text().strip()}"
                )
            if len(failures) >= 30:
                break
            if index and index % 100 == 0:
                print(f"checked={index}", flush=True)

    if failures:
        print(f"ALLOC_EXTRA_BUFFER=FAIL pairs={len(pairs)} "
              f"failures={len(failures)}")
        print("\n".join(failures))
        return 1
    print(f"ALLOC_EXTRA_BUFFER=PASS pairs={len(pairs)} "
          f"allocations={allocation_count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
