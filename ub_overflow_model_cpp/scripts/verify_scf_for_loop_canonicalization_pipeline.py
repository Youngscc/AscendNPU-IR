#!/usr/bin/env python3
"""Compare lightweight SCFForLoopCanonicalization with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


SCF_FOR_LOOP_CANONICALIZATION = StageSpec(
    name="scf-for-loop-canonicalization",
    input_checkpoint="06_after_extended_canonicalizer_module.mlir",
    oracle_checkpoint="07_after_scf_for_loop_canonicalization.mlir",
    cumulative_flags=(
        "--apply-model",
        "--apply-outer-canonicalizer",
        "--apply-arith-to-affine",
        "--apply-canonicalize-iter-arg",
        "--apply-module-canonicalizer",
        "--apply-scf-for-loop-canonicalization",
    ),
    single_flag="--apply-scf-for-loop-canonicalization",
)


if __name__ == "__main__":
    raise SystemExit(main(SCF_FOR_LOOP_CANONICALIZATION))
