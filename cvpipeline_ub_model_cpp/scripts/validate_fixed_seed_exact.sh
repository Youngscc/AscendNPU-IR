#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

lifetime_status=0
bash "${MODULE_DIR}/scripts/validate_fixed_seed_lifetimes.sh" || lifetime_status=$?

plan_status=0
bash "${MODULE_DIR}/scripts/validate_fixed_seed_plans.sh" || plan_status=$?

if [[ "${lifetime_status}" -ne 0 || "${plan_status}" -ne 0 ]]; then
  echo "[ERROR] fixed-seed validation failed: lifetime_status=${lifetime_status} plan_status=${plan_status}" >&2
  exit 1
fi

echo "PLANMEMORY_FIXED_SEED_EXACT_VALIDATION=PASS"
