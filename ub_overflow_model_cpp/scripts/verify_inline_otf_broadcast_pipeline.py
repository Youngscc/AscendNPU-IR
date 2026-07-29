#!/usr/bin/env python3
"""Compare lightweight pre-CV InlineOTFBroadcast with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


INLINE_OTF_BROADCAST = StageSpec(
    name="inline-otf-broadcast",
    input_checkpoint="12_after_memref_dead_store_elimination.mlir",
    oracle_checkpoint="13_after_inline_otf_broadcast.mlir",
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
        "--apply-inline-otf-broadcast",
    ),
    single_flag="--apply-inline-otf-broadcast",
)


if __name__ == "__main__":
    raise SystemExit(main(INLINE_OTF_BROADCAST))
