#!/usr/bin/env python3
"""Ensure C1 exact comparison rejects semantic field mutations."""

from __future__ import annotations

import csv
import hashlib
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def mutate_field(text: str, column: int) -> str:
    lines = text.splitlines(keepends=True)
    for index, line in enumerate(lines):
        fields = line.rstrip("\n").split("\t")
        if fields[0] == "OP" and len(fields) > column and fields[column]:
            fields[column] = fields[column] + "0"
            lines[index] = "\t".join(fields) + "\n"
            return "".join(lines)
    raise RuntimeError(f"no mutable OP field in column {column}")


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    source = repo / "Output/index/c1_semantic_ir"
    validator = Path(__file__).with_name("validate_c1_semantic_oracles.py")
    # name, operand identity, and memory effect columns.
    for column in (6, 8, 14):
        with tempfile.TemporaryDirectory(prefix="c1-semantic-sensitivity-") as tmp:
            root = Path(tmp)
            shutil.copy2(source / "summary.tsv", root / "summary.tsv")
            shutil.copy2(source / "dataset.json", root / "dataset.json")
            with (root / "summary.tsv").open(newline="") as stream:
                reader = csv.DictReader(stream, delimiter="\t")
                rows = list(reader)
                fields = reader.fieldnames
            if not rows or fields is None:
                return 2
            target_hash = rows[0]["after_cvpipeline_sha256"]
            source_oracle = Path(rows[0]["oracle_semantic"])
            mutated = root / f"mutated-column-{column}.tsv"
            mutated.write_text(mutate_field(source_oracle.read_text(), column))
            mutated_hash = hashlib.sha256(mutated.read_bytes()).hexdigest()
            for row in rows:
                if row["after_cvpipeline_sha256"] == target_hash:
                    row["oracle_semantic"] = str(mutated)
                    row["oracle_sha256"] = mutated_hash
            with (root / "summary.tsv").open("w", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
                writer.writeheader()
                writer.writerows(rows)
            result = subprocess.run(
                [sys.executable, str(validator), "--root", str(root)],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if result.returncode == 0:
                print(f"[ERROR] C1 validator accepted mutation in column {column}")
                return 1
    print("C1_SEMANTIC_SENSITIVITY=PASS mutations=3")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
