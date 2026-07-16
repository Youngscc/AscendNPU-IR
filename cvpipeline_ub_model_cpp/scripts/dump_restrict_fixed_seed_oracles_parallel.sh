#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
MANIFEST="${REPO_ROOT}/Output/index/planmemory_ttadapter_168/final_planmemory_status.tsv"
OUTPUT_ROOT="${MODULE_DIR}/output/fixed_seed_oracle_restrict"
JOBS="${PLANMEMORY_ORACLE_JOBS:-8}"

if [[ ! "${JOBS}" =~ ^[1-9][0-9]*$ ]]; then
  echo "[ERROR] PLANMEMORY_ORACLE_JOBS must be positive: ${JOBS}" >&2
  exit 2
fi
if [[ ! -f "${MANIFEST}" ]]; then
  echo "[ERROR] manifest not found: ${MANIFEST}" >&2
  exit 2
fi

SHARD_ROOT="${OUTPUT_ROOT}/shards"
mkdir -p "${SHARD_ROOT}"

pids=()
for ((shard = 0; shard < JOBS; ++shard)); do
  shard_dir="${SHARD_ROOT}/$(printf '%02d' "${shard}")"
  shard_manifest="${shard_dir}/manifest.tsv"
  mkdir -p "${shard_dir}"
  awk -F '\t' -v OFS='\t' -v shard="${shard}" -v jobs="${JOBS}" '
    NR == 1 { print; next }
    (NR - 2) % jobs == shard { print }
  ' "${MANIFEST}" >"${shard_manifest}"
  (
    bash "${MODULE_DIR}/scripts/dump_fixed_seed_oracles.sh" \
      --manifest "${shard_manifest}" \
      --output-root "${shard_dir}/oracle" \
      --expected-cases 0 \
      --config-id restrict-inplace-as-isa \
      --restrict-inplace-as-isa
  ) >"${shard_dir}/run.log" 2>&1 &
  pids+=("$!")
done

failed=0
for pid in "${pids[@]}"; do
  if ! wait "${pid}"; then
    failed=$((failed + 1))
  fi
done
if [[ "${failed}" -ne 0 ]]; then
  echo "[ERROR] ${failed} oracle shards failed; inspect ${SHARD_ROOT}/*/run.log" >&2
  exit 1
fi

combined_tmp="${OUTPUT_ROOT}/summary.tsv.tmp"
first_summary="${SHARD_ROOT}/00/oracle/summary.tsv"
head -n 1 "${first_summary}" >"${combined_tmp}"
for ((shard = 0; shard < JOBS; ++shard)); do
  shard_summary="${SHARD_ROOT}/$(printf '%02d' "${shard}")/oracle/summary.tsv"
  tail -n +2 "${shard_summary}"
done | LC_ALL=C sort -t $'\t' -k1,1 -k2,2n >>"${combined_tmp}"
mv "${combined_tmp}" "${OUTPUT_ROOT}/summary.tsv"

expected_cases=$(awk -F '\t' 'NR > 1 && $2 == "reached_after_plan_memory" { count++ } END { print count + 0 }' "${MANIFEST}")
expected_runs=$((expected_cases * 20))
actual_runs=$(awk 'END { print (NR > 0 ? NR - 1 : 0) }' "${OUTPUT_ROOT}/summary.tsv")
unique_runs=$(awk -F '\t' 'NR > 1 { print $1 "\t" $2 "\t" $3 }' "${OUTPUT_ROOT}/summary.tsv" | LC_ALL=C sort -u | wc -l | tr -d ' ')
incomplete_runs=$(awk -F '\t' 'NR > 1 && $5 != "complete" { count++ } END { print count + 0 }' "${OUTPUT_ROOT}/summary.tsv")

if [[ "${actual_runs}" -ne "${expected_runs}" ||
      "${unique_runs}" -ne "${expected_runs}" ||
      "${incomplete_runs}" -ne 0 ]]; then
  echo "[ERROR] restrict oracle matrix is incomplete: expected=${expected_runs} actual=${actual_runs} unique=${unique_runs} incomplete=${incomplete_runs}" >&2
  exit 1
fi

echo "RESTRICT_FIXED_SEED_ORACLES=PASS"
echo "cases=${expected_cases} seeds=20 runs=${actual_runs} jobs=${JOBS}"
echo "${OUTPUT_ROOT}/summary.tsv"
