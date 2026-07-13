#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
MANIFEST="${REPO_ROOT}/Output/index/planmemory_vv/final_planmemory_status.tsv"
OUTPUT_ROOT="${1:-${MODULE_DIR}/output/vv_fixed_seed_oracle}"

mkdir -p "${OUTPUT_ROOT}"

bash "${MODULE_DIR}/scripts/dump_fixed_seed_oracles.sh" \
  --manifest "${MANIFEST}" \
  --output-root "${OUTPUT_ROOT}/default" \
  --expected-cases 5 \
  --config-id vv-default

bash "${MODULE_DIR}/scripts/dump_fixed_seed_oracles.sh" \
  --manifest "${MANIFEST}" \
  --output-root "${OUTPUT_ROOT}/restrict" \
  --expected-cases 5 \
  --config-id vv-restrict \
  --restrict-inplace-as-isa

combined="${OUTPUT_ROOT}/oracle_summary.tsv"
head -n 1 "${OUTPUT_ROOT}/default/summary.tsv" >"${combined}"
tail -n +2 "${OUTPUT_ROOT}/default/summary.tsv" >>"${combined}"
tail -n +2 "${OUTPUT_ROOT}/restrict/summary.tsv" >>"${combined}"

runs=$(awk 'END { print (NR > 0 ? NR - 1 : 0) }' "${combined}")
[[ "${runs}" -eq 200 ]]

echo "VV_FIXED_SEED_ORACLES=PASS"
echo "cases=5 seeds=20 configs=2 runs=${runs}"
echo "${combined}"
