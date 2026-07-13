#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
TOOL="${PLANMEMORY_ORACLE_TOOL:-${REPO_ROOT}/build/bin/bishengir-cvpipeline-suffix-compile}"
INPUT_ROOT="${1:-${MODULE_DIR}/input/suffix_compile}"
OUTPUT_ROOT="${2:-${MODULE_DIR}/output/planmemory_attempt_oracle}"
PLAN_MEMORY_SEED="${PLAN_MEMORY_SEED:--1}"
RESTRICT_INPLACE_AS_ISA="${RESTRICT_INPLACE_AS_ISA:-0}"

INPUT_ROOT="$(cd "${INPUT_ROOT}" && pwd)"
mkdir -p "${OUTPUT_ROOT}"
OUTPUT_ROOT="$(cd "${OUTPUT_ROOT}" && pwd)"

if [[ ! -x "${TOOL}" ]]; then
  echo "missing oracle tool: ${TOOL}" >&2
  echo "build it with: cmake --build ${REPO_ROOT}/build --target bishengir-cvpipeline-suffix-compile -j8" >&2
  exit 2
fi

summary="${OUTPUT_ROOT}/summary.tsv"
printf 'case\tstatus\tseed\trestrict_inplace_as_isa\tinput\tattempt_dump\n' >"${summary}"

while IFS= read -r -d '' input; do
  case_name="$(basename "$(dirname "${input}")")"
  case_dir="${OUTPUT_ROOT}/${case_name}"
  mkdir -p "${case_dir}"
  dump="${case_dir}/planmemory_attempts.tsv"
  output_ir="${case_dir}/after_local_plan_memory.mlir"
  status=0
  seed_args=()
  if [[ "${PLAN_MEMORY_SEED}" != "-1" ]]; then
    seed_args+=(--plan-memory-seed "${PLAN_MEMORY_SEED}")
  fi
  if [[ "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
    seed_args+=(--restrict-inplace-as-isa)
  fi
  (
    cd "${case_dir}"
    BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS=1 \
      "${TOOL}" "${input}" \
      --local-plan-memory-only \
      --enable-memory-display \
      --mlir-disable-threading \
      "${seed_args[@]}" \
      -o "${output_ir}" \
      >stdout.log 2>"${dump}"
  ) || status=$?
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
    "${case_name}" "${status}" "${PLAN_MEMORY_SEED}" \
    "${RESTRICT_INPLACE_AS_ISA}" "${input}" "${dump}" >>"${summary}"
done < <(find -L "${INPUT_ROOT}" -type f \
  \( -name 'before_local_plan_memory.mlir' \
  -o -name '*.before_local_plan_memory.mlir' \) -print0)

echo "${summary}"
