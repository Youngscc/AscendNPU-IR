#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VALIDATOR="$("${MODULE_DIR}/scripts/build_dev_tools.sh")"
ORACLE_ROOT="${PLANMEMORY_FIXED_SEED_ORACLE_ROOT:-${MODULE_DIR}/output/fixed_seed_oracle}"
ORACLE_SUMMARY="${PLANMEMORY_FIXED_SEED_ORACLE_SUMMARY:-${ORACLE_ROOT}/summary.tsv}"
EXPECTED_RUNS="${PLANMEMORY_FIXED_SEED_EXPECTED_RUNS:-3320}"
REPORT_DIR="${MODULE_DIR}/output/reports"
SNAPSHOT_ROOT="${PLANMEMORY_FIXED_SEED_SNAPSHOT_ROOT:-${MODULE_DIR}/output/snapshots}/plan"
REPORT_TAG="${PLANMEMORY_FIXED_SEED_REPORT_TAG:-}"
REPORT_NAME="fixed_seed_plan_validation${REPORT_TAG:+_${REPORT_TAG}}.tsv"

mkdir -p "${REPORT_DIR}"
"${VALIDATOR}" \
  --action=validate-fixed-seed-plans \
  --oracle-summary="${ORACLE_SUMMARY}" \
  --expected-runs="${EXPECTED_RUNS}" \
  --snapshot-root="${SNAPSHOT_ROOT}" \
  --output="${REPORT_DIR}/${REPORT_NAME}"
