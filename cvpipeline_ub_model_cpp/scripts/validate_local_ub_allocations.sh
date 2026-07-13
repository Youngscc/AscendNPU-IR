#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VALIDATOR="$("${MODULE_DIR}/scripts/build_dev_tools.sh")"
REPORT_DIR="${MODULE_DIR}/output/reports"
SUFFIX_ROOT="${CVPIPELINE_SUFFIX_ROOT:-${MODULE_DIR}/input/suffix_compile}"

mkdir -p "${REPORT_DIR}"

"${VALIDATOR}" \
  --action=validate-local-ub-allocations \
  --root="${SUFFIX_ROOT}" \
  --expected-summary="${SUFFIX_ROOT}/ub_peak_summary.tsv" \
  --output="${REPORT_DIR}/local_ub_allocations_validation.tsv"
