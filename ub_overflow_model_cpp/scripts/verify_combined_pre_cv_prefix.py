#!/usr/bin/env python3
"""Compare the combined lightweight pre-CV prefix with native BiSheng."""

from verify_canonicalize_iter_arg_pipeline import StageSpec, main


COMBINED_PRE_CV_PREFIX = StageSpec(
    name="combined-pre-cv-prefix",
    input_checkpoint="00_before_auto_blockify.mlir",
    oracle_checkpoint="13_after_inline_otf_broadcast.mlir",
    cumulative_flags=("--apply-combined-prefix",),
    single_flag="--apply-combined-prefix",
    single_uses_profile=True,
)


if __name__ == "__main__":
    raise SystemExit(main(COMBINED_PRE_CV_PREFIX))
