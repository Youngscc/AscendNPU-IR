#!/usr/bin/env bash

set -euo pipefail

# Legacy Python memory-modeling validation entry. The current PlanMemory work
# does not use it by default, but this keeps the old one-liner discoverable.
python3 -m memory_modeling.run_adapters \
  --data-dir data \
  --output-root Output \
  --pretty
