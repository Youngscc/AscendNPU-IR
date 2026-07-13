#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VALIDATOR="$("${MODULE_DIR}/scripts/build_dev_tools.sh")"
REPORT_DIR="${MODULE_DIR}/output/reports"
SUFFIX_ROOT="${CVPIPELINE_SUFFIX_ROOT:-${MODULE_DIR}/input/suffix_compile}"
ATTEMPT_ROOT="${PLANMEMORY_ATTEMPT_ROOT:-${MODULE_DIR}/output/planmemory_attempt_oracle}"

mkdir -p "${REPORT_DIR}"
"${VALIDATOR}" \
  --action=validate-planmemory-attempts \
  --root="${SUFFIX_ROOT}" \
  --attempt-root="${ATTEMPT_ROOT}" \
  --output="${REPORT_DIR}/planmemory_attempt_validation.tsv"
