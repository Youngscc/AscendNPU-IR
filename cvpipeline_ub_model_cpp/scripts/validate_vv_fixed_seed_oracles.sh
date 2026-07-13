#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_ROOT="${1:-${MODULE_DIR}/output/vv_fixed_seed_oracle}"

bash "${MODULE_DIR}/scripts/validate_generated_config_oracles.sh" "${OUTPUT_ROOT}"

echo "VV_FIXED_SEED_VALIDATION=PASS"
