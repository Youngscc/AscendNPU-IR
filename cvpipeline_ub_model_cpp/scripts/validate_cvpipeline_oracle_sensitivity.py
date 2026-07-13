#!/usr/bin/env python3
"""Ensure the C0 validator rejects provenance and artifact mutations."""

from __future__ import annotations

import argparse
import csv
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--oracle-root", type=Path,
                        default=repo / "Output/index/c0_cvpipeline_after")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    args = parser.parse_args()
    source = args.oracle_root.resolve()
    validator = Path(__file__).with_name("validate_cvpipeline_after_oracles.py")
    mutations = ("input_sha256", "after_cvpipeline_sha256", "config_sha256")

    for field in mutations:
        with tempfile.TemporaryDirectory(prefix="cvpipeline-c0-sensitivity-") as tmp:
            root = Path(tmp)
            shutil.copy2(source / "summary.tsv", root / "summary.tsv")
            shutil.copy2(source / "dataset.json", root / "dataset.json")
            with (root / "summary.tsv").open(newline="") as stream:
                reader = csv.DictReader(stream, delimiter="\t")
                rows = list(reader)
                names = reader.fieldnames
            if not rows or not names or field not in names:
                print(f"[ERROR] cannot mutate missing field: {field}")
                return 2
            rows[0][field] = "0" * 64
            with (root / "summary.tsv").open("w", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=names, delimiter="\t")
                writer.writeheader()
                writer.writerows(rows)
            result = subprocess.run(
                [sys.executable, str(validator), "--oracle-root", str(root),
                 "--tool", str(args.tool.resolve())],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            )
            if result.returncode == 0:
                print(f"[ERROR] validator accepted mutated {field}")
                return 1
    print(f"C0_ORACLE_SENSITIVITY=PASS mutations={len(mutations)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
