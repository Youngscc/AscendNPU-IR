#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VALIDATOR="$("${MODULE_DIR}/scripts/build_dev_tools.sh")"
REPORT_DIR="${MODULE_DIR}/output/reports"
SUFFIX_ROOT="${CVPIPELINE_SUFFIX_ROOT:-${MODULE_DIR}/input/suffix_compile}"

mkdir -p "${REPORT_DIR}"

"${VALIDATOR}" \
  --action=validate-memory-plan \
  --root="${SUFFIX_ROOT}" \
  --output="${REPORT_DIR}/memory_plan_validation.tsv"
