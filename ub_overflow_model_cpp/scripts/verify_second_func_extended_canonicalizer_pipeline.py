#!/usr/bin/env python3
"""Compare the second func-scoped ExtendedCanonicalizer with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


SECOND_FUNC_EXTENDED_CANONICALIZER = StageSpec(
    name="second-func-extended-canonicalizer",
    input_checkpoint="10_after_hivm_opt_single_point.mlir",
    oracle_checkpoint="11_after_extended_canonicalizer_func_2.mlir",
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
        "--apply-second-func-canonicalizer",
    ),
    single_flag="--apply-second-func-canonicalizer",
)


if __name__ == "__main__":
    raise SystemExit(main(SECOND_FUNC_EXTENDED_CANONICALIZER))
