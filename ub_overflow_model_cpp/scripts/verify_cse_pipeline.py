#!/usr/bin/env python3
"""Compare lightweight module CSE with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


CSE = StageSpec(
    name="cse",
    input_checkpoint="07_after_scf_for_loop_canonicalization.mlir",
    oracle_checkpoint="08_after_cse.mlir",
    cumulative_flags=(
        "--apply-model",
        "--apply-outer-canonicalizer",
        "--apply-arith-to-affine",
        "--apply-canonicalize-iter-arg",
        "--apply-module-canonicalizer",
        "--apply-scf-for-loop-canonicalization",
        "--apply-cse",
    ),
    single_flag="--apply-cse",
)


if __name__ == "__main__":
    raise SystemExit(main(CSE))
