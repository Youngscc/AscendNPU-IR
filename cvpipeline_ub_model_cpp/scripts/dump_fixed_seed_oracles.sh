#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
TOOL="${PLANMEMORY_ORACLE_TOOL:-${REPO_ROOT}/build/bin/bishengir-cvpipeline-suffix-compile}"
MANIFEST="${REPO_ROOT}/Output/index/planmemory_ttadapter_168/final_planmemory_status.tsv"
INPUT_ROOT=""
OUTPUT_ROOT="${MODULE_DIR}/output/fixed_seed_oracle"
EXPECTED_CASES=166
CONFIG_ID="default"
RESTRICT_INPLACE_AS_ISA=0
RESUME=1
MAX_CASES=0
SEEDS=()

usage() {
  cat <<'EOF'
Usage: bash cvpipeline_ub_model_cpp/scripts/dump_fixed_seed_oracles.sh [options]

Generate one real PlanMemory oracle for every input/config/seed tuple.

Options:
  --manifest FILE       final_planmemory_status.tsv containing accepted inputs.
  --input-root DIR      Scan before_local_plan_memory.mlir files instead.
  --output-root DIR     Output directory.
  --expected-cases N    Require exactly N inputs. Defaults to 166; 0 disables.
  --seed N              Add one seed in [0, 19]. Repeatable; defaults to all 20.
  --config-id NAME      Stable config label written to summary.tsv.
  --restrict-inplace-as-isa
                        Pass the matching PlanMemory config option to the oracle.
  --max-cases N         Limit inputs for a development smoke test.
  --no-resume           Re-run tuples with an existing complete dump.
  -h, --help            Show this help.
EOF
}

die() {
  echo "[ERROR] $*" >&2
  exit 2
}

abs_path() {
  local path="$1"
  if [[ "${path}" == /* ]]; then
    printf '%s' "${path}"
  else
    printf '%s/%s' "$(cd "$(dirname "${path}")" && pwd -P)" "$(basename "${path}")"
  fi
}

file_sha256() {
  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$1" | awk '{print $1}'
  else
    sha256sum "$1" | awk '{print $1}'
  fi
}

dump_is_complete() {
  local dump="$1"
  [[ -f "${dump}" ]] || return 1
  local config_lines result_lines liveness_attempts liveness_ends
  local plan_attempts plan_ends buffers exact_buffers gens exact_gens
  local kills exact_kills inplace exact_inplace multi exact_multi
  local planned exact_planned
  config_lines=$(grep -c $'^PLANMEM_RUN_CONFIG\t' "${dump}" || true)
  result_lines=$(grep -c $'^PLANMEM_RUN_RESULT\t' "${dump}" || true)
  liveness_attempts=$(grep -c $'^PLANMEM_LIVENESS_ATTEMPT\t' "${dump}" || true)
  liveness_ends=$(grep -c $'^PLANMEM_LIVENESS_ATTEMPT_END\t' "${dump}" || true)
  plan_attempts=$(grep -c $'^PLANMEM_PLAN_ATTEMPT\t' "${dump}" || true)
  plan_ends=$(grep -c $'^PLANMEM_PLAN_ATTEMPT_END\t' "${dump}" || true)
  buffers=$(grep -c $'^PLANMEM_BUFFER\t' "${dump}" || true)
  exact_buffers=$(grep -c $'^PLANMEM_EXACT_BUFFER\t' "${dump}" || true)
  gens=$(grep -c $'^PLANMEM_GEN\t' "${dump}" || true)
  exact_gens=$(grep -c $'^PLANMEM_EXACT_GEN\t' "${dump}" || true)
  kills=$(grep -c $'^PLANMEM_KILL\t' "${dump}" || true)
  exact_kills=$(grep -c $'^PLANMEM_EXACT_KILL\t' "${dump}" || true)
  inplace=$(grep -c $'^PLANMEM_INPLACE\t' "${dump}" || true)
  exact_inplace=$(grep -c $'^PLANMEM_EXACT_INPLACE\t' "${dump}" || true)
  multi=$(grep -c $'^PLANMEM_MULTI\t' "${dump}" || true)
  exact_multi=$(grep -c $'^PLANMEM_EXACT_MULTI\t' "${dump}" || true)
  planned=$(grep -c $'^PLANMEM_PLANNED_BUFFER\t' "${dump}" || true)
  exact_planned=$(grep -c $'^PLANMEM_EXACT_PLANNED_BUFFER\t' "${dump}" || true)
  [[ "${config_lines}" -eq 1 && "${result_lines}" -eq 1 &&
     "${liveness_attempts}" -gt 0 &&
     "${liveness_attempts}" -eq "${liveness_ends}" &&
     "${plan_attempts}" -eq "${plan_ends}" &&
     "${plan_attempts}" -eq "${liveness_attempts}" &&
     "${buffers}" -eq "${exact_buffers}" &&
     "${gens}" -eq "${exact_gens}" &&
     "${kills}" -eq "${exact_kills}" &&
     "${inplace}" -eq "${exact_inplace}" &&
     "${multi}" -eq "${exact_multi}" &&
     "${planned}" -eq "${exact_planned}" ]]
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --manifest)
    shift
    [[ $# -gt 0 ]] || die "missing value for --manifest"
    MANIFEST="$1"
    ;;
  --input-root)
    shift
    [[ $# -gt 0 ]] || die "missing value for --input-root"
    INPUT_ROOT="$1"
    MANIFEST=""
    ;;
  --output-root)
    shift
    [[ $# -gt 0 ]] || die "missing value for --output-root"
    OUTPUT_ROOT="$1"
    ;;
  --expected-cases)
    shift
    [[ $# -gt 0 ]] || die "missing value for --expected-cases"
    EXPECTED_CASES="$1"
    ;;
  --seed)
    shift
    [[ $# -gt 0 ]] || die "missing value for --seed"
    [[ "$1" =~ ^([0-9]|1[0-9])$ ]] || die "seed must be in [0, 19]: $1"
    SEEDS+=("$1")
    ;;
  --config-id)
    shift
    [[ $# -gt 0 ]] || die "missing value for --config-id"
    CONFIG_ID="$1"
    ;;
  --restrict-inplace-as-isa)
    RESTRICT_INPLACE_AS_ISA=1
    ;;
  --max-cases)
    shift
    [[ $# -gt 0 ]] || die "missing value for --max-cases"
    MAX_CASES="$1"
    ;;
  --no-resume)
    RESUME=0
    ;;
  -h | --help)
    usage
    exit 0
    ;;
  *)
    die "unknown option: $1"
    ;;
  esac
  shift
done

[[ -x "${TOOL}" ]] || die "oracle tool is not executable: ${TOOL}"
TOOL="$(abs_path "${TOOL}")"
TOOL_SHA256="$(file_sha256 "${TOOL}")"

if [[ ${#SEEDS[@]} -eq 0 ]]; then
  for seed in {0..19}; do
    SEEDS+=("${seed}")
  done
fi

INPUT_NAMES=()
INPUT_FILES=()
if [[ -n "${MANIFEST}" ]]; then
  [[ -f "${MANIFEST}" ]] || die "manifest not found: ${MANIFEST}"
  while IFS=$'\t' read -r input classification _ _ _ case_dir; do
    [[ "${input}" != "input" ]] || continue
    [[ "${classification}" == "reached_after_planmemory" ]] || continue
    before_ir="${case_dir}/before_local_plan_memory.mlir"
    [[ -f "${before_ir}" ]] || die "missing PlanMemory-before IR: ${before_ir}"
    INPUT_NAMES+=("$(basename "${input}")")
    INPUT_FILES+=("$(abs_path "${before_ir}")")
  done <"${MANIFEST}"
else
  [[ -n "${INPUT_ROOT}" && -d "${INPUT_ROOT}" ]] ||
    die "--input-root must name a directory"
  while IFS= read -r before_ir; do
    before_name="$(basename "${before_ir}")"
    if [[ "${before_name}" == b_*.mlir ]]; then
      INPUT_NAMES+=("${before_name%.mlir}")
    else
      INPUT_NAMES+=("$(basename "$(dirname "${before_ir}")")")
    fi
    INPUT_FILES+=("$(abs_path "${before_ir}")")
  done < <(find -L "${INPUT_ROOT}" -type f \
    \( -name 'before_local_plan_memory.mlir' \
    -o -name '*.before_local_plan_memory.mlir' \
    -o -name 'b_*.mlir' \) -print | LC_ALL=C sort)
fi

if [[ "${MAX_CASES}" -gt 0 && ${#INPUT_FILES[@]} -gt "${MAX_CASES}" ]]; then
  INPUT_NAMES=("${INPUT_NAMES[@]:0:${MAX_CASES}}")
  INPUT_FILES=("${INPUT_FILES[@]:0:${MAX_CASES}}")
fi

case_count=${#INPUT_FILES[@]}
[[ "${case_count}" -gt 0 ]] || die "no PlanMemory-before inputs found"
if [[ "${EXPECTED_CASES}" -gt 0 && "${MAX_CASES}" -eq 0 &&
      "${case_count}" -ne "${EXPECTED_CASES}" ]]; then
  die "expected ${EXPECTED_CASES} cases, found ${case_count}"
fi

mkdir -p "${OUTPUT_ROOT}"
OUTPUT_ROOT="$(abs_path "${OUTPUT_ROOT}")"
summary="${OUTPUT_ROOT}/summary.tsv"
printf 'case\tseed\tconfig_id\trestrict_inplace_as_isa\tstatus\tcompiler_status\tliveness_attempts\tplan_attempts\tinput\tdump\tinput_sha256\toracle_tool_sha256\n' >"${summary}"

complete_runs=0
failed_runs=0
for index in "${!INPUT_FILES[@]}"; do
  case_name="${INPUT_NAMES[$index]}"
  input="${INPUT_FILES[$index]}"
  input_sha256="$(file_sha256 "${input}")"
  for seed in "${SEEDS[@]}"; do
    seed_dir="${OUTPUT_ROOT}/${case_name}/seed_$(printf '%02d' "${seed}")"
    mkdir -p "${seed_dir}"
    dump="${seed_dir}/oracle.tsv"
    output_ir="${seed_dir}/after_local_plan_memory.mlir"
    stdout_log="${seed_dir}/stdout.log"
    status_file="${seed_dir}/compiler_status.txt"
    complete_marker="${seed_dir}/complete.marker"
    fingerprint="input=${input_sha256} tool=${TOOL_SHA256} config=${CONFIG_ID} restrict=${RESTRICT_INPLACE_AS_ISA} seed=${seed}"

    if [[ -f "${complete_marker}" ]] && ! dump_is_complete "${dump}"; then
      rm -f "${complete_marker}"
    fi
    if [[ -f "${complete_marker}" &&
          "$(cat "${complete_marker}")" != "${fingerprint}" ]]; then
      rm -f "${complete_marker}"
    fi
    if [[ "${RESUME}" != 1 || ! -f "${complete_marker}" ]]; then
      compiler_status=0
      args=(
        "${TOOL}" "${input}"
        --local-plan-memory-only
        --enable-memory-display
        --mlir-disable-threading
        --plan-memory-seed "${seed}"
        -o "${output_ir}"
      )
      if [[ "${RESTRICT_INPLACE_AS_ISA}" == 1 ]]; then
        args+=(--restrict-inplace-as-isa)
      fi
      (
        cd "${seed_dir}"
        BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS=1 "${args[@]}"
      ) >"${stdout_log}" 2>"${dump}" || compiler_status=$?
      printf '%s\n' "${compiler_status}" >"${status_file}"

      if dump_is_complete "${dump}"; then
        printf '%s\n' "${fingerprint}" >"${complete_marker}"
      else
        rm -f "${complete_marker}"
      fi
    fi

    compiler_status="$(cat "${status_file}" 2>/dev/null || printf '%s' -1)"
    liveness_attempts=$(grep -c $'^PLANMEM_LIVENESS_ATTEMPT\t' "${dump}" || true)
    plan_attempts=$(grep -c $'^PLANMEM_PLAN_ATTEMPT\t' "${dump}" || true)
    if [[ -f "${complete_marker}" ]]; then
      run_status=complete
      complete_runs=$((complete_runs + 1))
    else
      run_status=incomplete
      failed_runs=$((failed_runs + 1))
    fi
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "${case_name}" "${seed}" "${CONFIG_ID}" \
      "${RESTRICT_INPLACE_AS_ISA}" "${run_status}" "${compiler_status}" \
      "${liveness_attempts}" "${plan_attempts}" "${input}" "${dump}" \
      "${input_sha256}" "${TOOL_SHA256}" \
      >>"${summary}"
  done
done

expected_runs=$((case_count * ${#SEEDS[@]}))
echo "cases=${case_count} seeds=${#SEEDS[@]} expected_runs=${expected_runs} complete_runs=${complete_runs} incomplete_runs=${failed_runs}"
echo "${summary}"
[[ "${complete_runs}" -eq "${expected_runs}" && "${failed_runs}" -eq 0 ]]
