#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

bash "${MODULE_DIR}/scripts/validate_fixed_seed_exact.sh"
bash "${MODULE_DIR}/scripts/validate_restrict_fixed_seed_exact.sh"

echo "PLANMEMORY_CONFIG_MATRIX_VALIDATION=PASS"
