#!/usr/bin/env python3
"""Compare lightweight HIVMOptSinglePoint with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


HIVM_OPT_SINGLE_POINT = StageSpec(
    name="hivm-opt-single-point",
    input_checkpoint="09_after_extended_canonicalizer_func_1.mlir",
    oracle_checkpoint="10_after_hivm_opt_single_point.mlir",
    cumulative_flags=(
        "--apply-model",
        "--apply-outer-canonicalizer",
        "--apply-arith-to-affine",
        "--apply-canonicalize-iter-arg",
        "--apply-module-canonicalizer",
        "--apply-scf-for-loop-canonicalization",
        "--apply-cse",
        "--apply-first-func-canonicalizer",
        "--apply-hivm-opt-single-point",
    ),
    single_flag="--apply-hivm-opt-single-point",
)


if __name__ == "__main__":
    raise SystemExit(main(HIVM_OPT_SINGLE_POINT))
