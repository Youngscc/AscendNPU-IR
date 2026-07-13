#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_ROOT="${1:-${MODULE_DIR}/output/config_matrix}"
SUMMARY="${OUTPUT_ROOT}/oracle_summary.tsv"
REPORT_DIR="${OUTPUT_ROOT}/reports"
SNAPSHOT_ROOT="${OUTPUT_ROOT}/snapshots"

if [[ ! -f "${SUMMARY}" ]]; then
  echo "[ERROR] oracle summary not found: ${SUMMARY}" >&2
  exit 2
fi

expected_runs=$(awk 'END { print (NR > 0 ? NR - 1 : 0) }' "${SUMMARY}")
if [[ "${expected_runs}" -le 0 ]]; then
  echo "[ERROR] oracle summary has no runs: ${SUMMARY}" >&2
  exit 2
fi

mkdir -p "${REPORT_DIR}"
VALIDATOR="$(bash "${MODULE_DIR}/scripts/build_dev_tools.sh")"

lifetime_status=0
"${VALIDATOR}" \
  --action=validate-fixed-seed-lifetimes \
  --oracle-summary="${SUMMARY}" \
  --expected-runs="${expected_runs}" \
  --snapshot-root="${SNAPSHOT_ROOT}/lifetime" \
  --output="${REPORT_DIR}/lifetime.tsv" || lifetime_status=$?

plan_status=0
"${VALIDATOR}" \
  --action=validate-fixed-seed-plans \
  --oracle-summary="${SUMMARY}" \
  --expected-runs="${expected_runs}" \
  --snapshot-root="${SNAPSHOT_ROOT}/plan" \
  --output="${REPORT_DIR}/plan.tsv" || plan_status=$?

if [[ "${lifetime_status}" -ne 0 || "${plan_status}" -ne 0 ]]; then
  echo "[ERROR] generated oracle validation failed: lifetime_status=${lifetime_status} plan_status=${plan_status}" >&2
  exit 1
fi

echo "GENERATED_CONFIG_ORACLE_VALIDATION=PASS"
echo "validated_runs=${expected_runs}"
