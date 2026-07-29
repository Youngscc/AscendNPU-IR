#!/usr/bin/env python3
"""Compare the first func-scoped ExtendedCanonicalizer with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


FIRST_FUNC_EXTENDED_CANONICALIZER = StageSpec(
    name="first-func-extended-canonicalizer",
    input_checkpoint="08_after_cse.mlir",
    oracle_checkpoint="09_after_extended_canonicalizer_func_1.mlir",
    cumulative_flags=(
        "--apply-model",
        "--apply-outer-canonicalizer",
        "--apply-arith-to-affine",
        "--apply-canonicalize-iter-arg",
        "--apply-module-canonicalizer",
        "--apply-scf-for-loop-canonicalization",
        "--apply-cse",
        "--apply-first-func-canonicalizer",
    ),
    single_flag="--apply-first-func-canonicalizer",
)


if __name__ == "__main__":
    raise SystemExit(main(FIRST_FUNC_EXTENDED_CANONICALIZER))
