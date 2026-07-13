#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VALIDATOR="$("${MODULE_DIR}/scripts/build_dev_tools.sh")"
REPORT_DIR="${MODULE_DIR}/output/reports"
SUFFIX_ROOT="${CVPIPELINE_SUFFIX_ROOT:-${MODULE_DIR}/input/suffix_compile}"

mkdir -p "${REPORT_DIR}"

"${VALIDATOR}" \
  --action=validate-lifetimes \
  --root="${SUFFIX_ROOT}" \
  --try-all-seeds \
  --output="${REPORT_DIR}/lifetime_validation.tsv"
