#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${MODULE_DIR}/scripts/build_incremental_common.sh"
OUT_DIR="${MODULE_DIR}/output/bin"
CORE_SRC="${MODULE_DIR}/src/main.cpp"
CORE_OUT="${OUT_DIR}/cvpipeline_ub_model"

compile_if_needed "${CORE_SRC}" "${CORE_OUT}" "${MODULE_DIR}"
