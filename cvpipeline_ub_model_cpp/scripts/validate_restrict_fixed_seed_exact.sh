#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ORACLE_ROOT="${MODULE_DIR}/output/fixed_seed_oracle_restrict"

[[ -f "${ORACLE_ROOT}/summary.tsv" ]] || {
  echo "[ERROR] restrict oracle is missing; run dump_restrict_fixed_seed_oracles.sh" >&2
  exit 2
}

export PLANMEMORY_FIXED_SEED_ORACLE_ROOT="${ORACLE_ROOT}"
export PLANMEMORY_FIXED_SEED_ORACLE_SUMMARY="${ORACLE_ROOT}/summary.tsv"
export PLANMEMORY_FIXED_SEED_EXPECTED_RUNS="${PLANMEMORY_FIXED_SEED_EXPECTED_RUNS:-3320}"
export PLANMEMORY_FIXED_SEED_SNAPSHOT_ROOT="${MODULE_DIR}/output/snapshots_restrict"
export PLANMEMORY_FIXED_SEED_REPORT_TAG="restrict"

bash "${MODULE_DIR}/scripts/validate_fixed_seed_exact.sh"
