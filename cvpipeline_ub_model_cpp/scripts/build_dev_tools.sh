#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${MODULE_DIR}/scripts/build_incremental_common.sh"
OUT_DIR="${MODULE_DIR}/output/bin"
DEV_SRC="${MODULE_DIR}/src/cli/dev_validate.cpp"
DEV_OUT="${OUT_DIR}/cvpipeline_ub_model_dev_validate"

compile_if_needed "${DEV_SRC}" "${DEV_OUT}" "${MODULE_DIR}"
