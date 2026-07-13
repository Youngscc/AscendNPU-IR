#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
ROOT="${CVPIPELINE_EXTENDED_ROOT:-${REPO_ROOT}/Output/views/planmemory_all}"
REPORT_DIR="${MODULE_DIR}/output/reports"
VALIDATOR="$("${MODULE_DIR}/scripts/build_dev_tools.sh")"

mkdir -p "${REPORT_DIR}"

"${VALIDATOR}" \
  --action=validate-memory-plan \
  --root="${ROOT}" \
  --output="${REPORT_DIR}/extended_memory_plan_validation.tsv"

case_count="$(awk 'END {print NR - 1}' \
  "${REPORT_DIR}/extended_memory_plan_validation.tsv")"
expected_count="${CVPIPELINE_EXTENDED_EXPECTED_CASES:-144}"
if [[ "${case_count}" -ne "${expected_count}" ]]; then
  echo "[ERROR] expected ${expected_count} extended cases, got ${case_count}" >&2
  exit 1
fi
