#!/usr/bin/env python3
"""Compare the in-pipeline module ExtendedCanonicalizer with BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


MODULE_EXTENDED_CANONICALIZER = StageSpec(
    name="module-extended-canonicalizer",
    input_checkpoint="05_after_canonicalize_iter_arg.mlir",
    oracle_checkpoint="06_after_extended_canonicalizer_module.mlir",
    cumulative_flags=(
        "--apply-model",
        "--apply-outer-canonicalizer",
        "--apply-arith-to-affine",
        "--apply-canonicalize-iter-arg",
        "--apply-module-canonicalizer",
    ),
    single_flag="--apply-module-canonicalizer",
)


if __name__ == "__main__":
    raise SystemExit(main(MODULE_EXTENDED_CANONICALIZER))
