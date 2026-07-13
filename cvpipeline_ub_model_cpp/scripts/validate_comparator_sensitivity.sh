#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VALIDATOR="$("${MODULE_DIR}/scripts/build_dev_tools.sh")"
SOURCE_SUMMARY="${PLANMEMORY_FIXED_SEED_ORACLE_SUMMARY:-${MODULE_DIR}/output/fixed_seed_oracle/summary.tsv}"

[[ -f "${SOURCE_SUMMARY}" ]] || {
  echo "[ERROR] oracle summary is missing: ${SOURCE_SUMMARY}" >&2
  exit 2
}

WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/cvub-comparator-sensitivity.XXXXXX")"
trap 'rm -rf "${WORK_DIR}"' EXIT

make_single_run_summary() {
  local dump="$1"
  local output="$2"
  awk -F '\t' -v OFS='\t' -v dump="${dump}" '
    NR == 1 {
      for (i = 1; i <= NF; ++i)
        if ($i == "dump")
          dump_column = i
      if (!dump_column)
        exit 2
      print
      next
    }
    NR == 2 {
      $dump_column = dump
      print
      exit
    }
  ' "${SOURCE_SUMMARY}" >"${output}"
}

ORIGINAL_DUMP="$(awk -F '\t' '
  NR == 1 {
    for (i = 1; i <= NF; ++i)
      if ($i == "dump")
        dump_column = i
    next
  }
  NR == 2 { print $dump_column; exit }
' "${SOURCE_SUMMARY}")"
[[ -f "${ORIGINAL_DUMP}" ]] || {
  echo "[ERROR] first oracle dump is missing: ${ORIGINAL_DUMP}" >&2
  exit 2
}

ORIGINAL_SUMMARY="${WORK_DIR}/original.tsv"
make_single_run_summary "${ORIGINAL_DUMP}" "${ORIGINAL_SUMMARY}"
"${VALIDATOR}" --quiet --action=validate-fixed-seed-lifetimes \
  --oracle-summary="${ORIGINAL_SUMMARY}" --expected-runs=1 \
  --output="${WORK_DIR}/original-lifetime-report.tsv"
"${VALIDATOR}" --quiet --action=validate-fixed-seed-plans \
  --oracle-summary="${ORIGINAL_SUMMARY}" --expected-runs=1 \
  --output="${WORK_DIR}/original-plan-report.tsv"

LIFETIME_DUMP="${WORK_DIR}/mutated-lifetime.tsv"
awk -F '\t' -v OFS='\t' '
  !changed && $1 == "PLANMEM_EXACT_BUFFER" {
    $7 = $7 + 1
    changed = 1
  }
  { print }
  END { if (!changed) exit 2 }
' "${ORIGINAL_DUMP}" >"${LIFETIME_DUMP}"
LIFETIME_SUMMARY="${WORK_DIR}/mutated-lifetime-summary.tsv"
make_single_run_summary "${LIFETIME_DUMP}" "${LIFETIME_SUMMARY}"
if "${VALIDATOR}" --quiet --action=validate-fixed-seed-lifetimes \
    --oracle-summary="${LIFETIME_SUMMARY}" --expected-runs=1 \
    --output="${WORK_DIR}/mutated-lifetime-report.tsv"; then
  echo "[ERROR] lifetime comparator accepted a modified allocTime" >&2
  exit 1
fi

PLAN_DUMP="${WORK_DIR}/mutated-plan.tsv"
awk -F '\t' -v OFS='\t' '
  !changed && $1 == "PLANMEM_EXACT_PLANNED_BUFFER" {
    $6 = $6 + 1
    changed = 1
  }
  { print }
  END { if (!changed) exit 2 }
' "${ORIGINAL_DUMP}" >"${PLAN_DUMP}"
PLAN_SUMMARY="${WORK_DIR}/mutated-plan-summary.tsv"
make_single_run_summary "${PLAN_DUMP}" "${PLAN_SUMMARY}"
if "${VALIDATOR}" --quiet --action=validate-fixed-seed-plans \
    --oracle-summary="${PLAN_SUMMARY}" --expected-runs=1 \
    --output="${WORK_DIR}/mutated-plan-report.tsv"; then
  echo "[ERROR] plan comparator accepted a modified offset" >&2
  exit 1
fi

echo "PLANMEMORY_COMPARATOR_SENSITIVITY=PASS"
