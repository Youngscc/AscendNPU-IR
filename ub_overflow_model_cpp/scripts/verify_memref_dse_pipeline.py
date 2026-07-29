#!/usr/bin/env python3
"""Compare lightweight pre-CV MemRef DSE with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


MEMREF_DSE = StageSpec(
    name="memref-dse",
    input_checkpoint="11_after_extended_canonicalizer_func_2.mlir",
    oracle_checkpoint="12_after_memref_dead_store_elimination.mlir",
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
        "--apply-memref-dse",
    ),
    single_flag="--apply-memref-dse",
)


if __name__ == "__main__":
    raise SystemExit(main(MEMREF_DSE))
