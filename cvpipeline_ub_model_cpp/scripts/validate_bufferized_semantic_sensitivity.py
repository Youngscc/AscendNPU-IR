#!/usr/bin/env python3
"""Ensure buffer identity and loop ownership mutations are rejected."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


def validate(tool: Path, before: Path, after: Path, report: Path) -> int:
    return subprocess.run(
        [str(tool), "--action=validate-bufferized-semantic-ir",
         "--root", str(before), "--after-ir", str(after),
         "--output", str(report)], stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL).returncode


def mutate_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise RuntimeError(f"expected one mutation anchor: {old}")
    return text.replace(old, new)


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    object_hash = (
        "18ad09d908eda36dd9a539576b94e575752cb0e09af03f34cccab83310f39026")
    tree = (repo / "Output/experiments/c2_c3_pass_oracles/objects" /
            object_hash / "pass_tree/builtin_module_no-symbol-name")
    before = tree / "0_one-shot-bufferize.mlir"
    after = tree / "1_one-shot-bufferize.mlir"
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"

    with tempfile.TemporaryDirectory(prefix="cvub-c2-sensitivity-") as tmp:
        temporary = Path(tmp)
        report = temporary / "report.tsv"
        if validate(tool, before, after, report):
            print("BUFFERIZED_SEMANTIC_SENSITIVITY=FAIL baseline")
            return 1

        original = after.read_text()
        mutations = {
            "dps_root": mutate_once(
                original, '"hivm.hir.vbrc"(%2, %10)',
                '"hivm.hir.vbrc"(%2, %9)'),
            "loop_yield": mutate_once(
                original, '"scf.yield"(%23)', '"scf.yield"(%22)'),
        }
        for name, text in mutations.items():
            mutated = temporary / f"{name}.mlir"
            mutated.write_text(text)
            if validate(tool, before, mutated, report) == 0:
                print(f"BUFFERIZED_SEMANTIC_SENSITIVITY=FAIL accepted={name}")
                return 1

    print("BUFFERIZED_SEMANTIC_SENSITIVITY=PASS mutations=2")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
