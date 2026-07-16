#!/usr/bin/env python3
"""Ensure HIVMDecomposeOp semantic mutations are rejected."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


def run(command: list[str]) -> int:
    return subprocess.run(
        command, stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL).returncode


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise RuntimeError(f"expected one mutation anchor: {old}")
    return text.replace(old, new)


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    object_hash = (
        "661e79369d12d8abbb793c9cab475aa9b4c5706a7154051d18050c1da987560a")
    tree = (repo / "Output/experiments/one_shot_bufferize_hivm_decompose_op/objects" /
            object_hash / "pass_tree/builtin_module_no-symbol-name")
    source = tree / "0_one-shot-bufferize.mlir"
    boundary = tree / "func_func_atomic_cas"
    before = boundary / "3_0_hivm-decompose-op.mlir"
    after = boundary / "3_1_hivm-decompose-op.mlir"
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"

    with tempfile.TemporaryDirectory(prefix="cvub-hivm-decompose-op-sensitivity-") as tmp:
        temporary = Path(tmp)
        report = temporary / "report.tsv"
        validate = [
            str(tool), "--action=validate-hivm-decompose-op-rewrites",
            "--root", str(source), "--attempt-root", str(before),
            "--after-ir", str(after), "--function", "atomic_cas",
            "--output", str(report),
        ]
        if run(validate):
            print("HIVM_DECOMPOSE_OP_SEMANTIC_SENSITIVITY=FAIL baseline")
            return 1

        original = after.read_text()
        type_mutation = replace_once(
            original,
            '%30 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> '
            ': () -> memref<512xi1>',
            '%30 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> '
            ': () -> memref<513xi1>')
        mutated_type = temporary / "allocation_type.mlir"
        mutated_type.write_text(type_mutation)
        model = temporary / "model.tsv"
        oracle = temporary / "oracle.tsv"
        if (run([str(tool), "--action=model-hivm-decompose-op",
                 "--root", str(source), "--function", "atomic_cas",
                 "--output", str(model)]) or
                run([str(tool), "--action=dump-hivm-decompose-op-oracle",
                     "--root", str(before), "--after-ir", str(mutated_type),
                     "--function", "atomic_cas",
                     "--output", str(oracle)])):
            print("HIVM_DECOMPOSE_OP_SEMANTIC_SENSITIVITY=FAIL allocation setup")
            return 1
        if model.read_bytes() == oracle.read_bytes():
            print("HIVM_DECOMPOSE_OP_SEMANTIC_SENSITIVITY=FAIL accepted=allocation_type")
            print(model.read_text().strip())
            return 1

        operand_mutation = replace_once(
            original, '"hivm.hir.vsel"(%30, %14, %29, %31)',
            '"hivm.hir.vsel"(%30, %14, %23, %31)')
        mutated_operand = temporary / "operation_buffer.mlir"
        mutated_operand.write_text(operand_mutation)
        validate[validate.index(str(after))] = str(mutated_operand)
        if run(validate) == 0:
            print("HIVM_DECOMPOSE_OP_SEMANTIC_SENSITIVITY=FAIL accepted=operation_buffer")
            return 1

    print("HIVM_DECOMPOSE_OP_SEMANTIC_SENSITIVITY=PASS mutations=2")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
