#!/usr/bin/env bash

set -euo pipefail

# Run the saved Triton Python stage and ask Triton to dump generated IR under
# ./data. This is a legacy helper for regenerating .ttadapter-like inputs.
TRITON_DEBUG=1 \
TRITON_ALWAYS_COMPILE=1 \
MLIR_ENABLE_DUMP=1 \
TRITON_KERNEL_DUMP=1 \
TRITON_DUMP_DIR=./data \
python extracted_stages/s0_original_baseline_5800us.py
