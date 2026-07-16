#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
FIXTURE_ROOT="${MODULE_DIR}/testdata/plan_memory"
OUTPUT_ROOT="${1:-${MODULE_DIR}/output/plan_memory_fixtures}"
EXACT_ROOT="${FIXTURE_ROOT}/exact"
TOOL="${PLANMEMORY_ORACLE_TOOL:-${REPO_ROOT}/build/bin/bishengir-cvpipeline-suffix-compile}"
EXACT_CASES=$(find "${EXACT_ROOT}" -type f -name 'b_*.mlir' | wc -l | tr -d ' ')
EXPECTED_RUNS=$((EXACT_CASES * 20 * 2))

mkdir -p "${OUTPUT_ROOT}"

bash "${MODULE_DIR}/scripts/dump_fixed_seed_oracles.sh" \
  --input-root "${EXACT_ROOT}" \
  --output-root "${OUTPUT_ROOT}/default" \
  --expected-cases "${EXACT_CASES}" \
  --config-id b-fixture-default

bash "${MODULE_DIR}/scripts/dump_fixed_seed_oracles.sh" \
  --input-root "${EXACT_ROOT}" \
  --output-root "${OUTPUT_ROOT}/restrict" \
  --expected-cases "${EXACT_CASES}" \
  --config-id b-fixture-restrict \
  --restrict-inplace-as-isa

combined="${OUTPUT_ROOT}/oracle_summary.tsv"
head -n 1 "${OUTPUT_ROOT}/default/summary.tsv" >"${combined}"
tail -n +2 "${OUTPUT_ROOT}/default/summary.tsv" >>"${combined}"
tail -n +2 "${OUTPUT_ROOT}/restrict/summary.tsv" >>"${combined}"

echo "PLAN_MEMORY_FIXTURE_ORACLES=PASS"
echo "exact_cases=${EXACT_CASES} exact_runs=${EXPECTED_RUNS}"
echo "${combined}"
