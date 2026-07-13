#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

bash "${MODULE_DIR}/scripts/dump_fixed_seed_oracles.sh" \
  --output-root "${MODULE_DIR}/output/fixed_seed_oracle_restrict" \
  --config-id restrict-inplace-as-isa \
  --restrict-inplace-as-isa \
  "$@"
