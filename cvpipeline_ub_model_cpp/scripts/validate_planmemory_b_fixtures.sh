#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_ROOT="${1:-${MODULE_DIR}/output/planmemory_b_fixtures}"

bash "${MODULE_DIR}/scripts/validate_generated_config_oracles.sh" "${OUTPUT_ROOT}"

echo "PLANMEMORY_B_FIXTURE_VALIDATION=PASS"
exact_runs=$(awk 'END { print (NR > 0 ? NR - 1 : 0) }' \
  "${OUTPUT_ROOT}/oracle_summary.tsv")
echo "exact_runs=${exact_runs}"
