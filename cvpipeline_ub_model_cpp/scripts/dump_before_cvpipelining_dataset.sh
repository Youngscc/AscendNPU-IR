#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
ADAPTER_ROOT="${MODULE_DIR}/data/adapter"
DATASET_ROOT="${MODULE_DIR}/data/before_cvpipelining"
WORK_ROOT="${REPO_ROOT}/Output/dataset_before_cvpipelining"
COMPILER="${BISHENGIR_COMPILER:-${REPO_ROOT}/build/bin/bishengir-compile}"
SUFFIX_TOOL="${CVPIPELINING_SUFFIX_TOOL:-${REPO_ROOT}/build/bin/bishengir-cvpipeline-suffix-compile}"
MAX_FILES=0

usage() {
  cat <<'EOF'
Usage: bash cvpipeline_ub_model_cpp/scripts/dump_before_cvpipelining_dataset.sh [options]

Generate the checked-in CVPipelining-before dataset from all source adapters.
The full compiler locates the real pass boundary; the suffix tool prints the
same module in generic form for the lightweight model.

Options:
  --max-files N       Limit adapters for a smoke run. 0 means all adapters.
  --compiler FILE     Override build/bin/bishengir-compile.
  --suffix-tool FILE  Override bishengir-cvpipeline-suffix-compile.
  -h, --help          Show this help.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --max-files)
    MAX_FILES="$2"
    shift 2
    ;;
  --compiler)
    COMPILER="$2"
    shift 2
    ;;
  --suffix-tool)
    SUFFIX_TOOL="$2"
    shift 2
    ;;
  -h | --help)
    usage
    exit 0
    ;;
  *)
    echo "[ERROR] unknown option: $1" >&2
    usage >&2
    exit 2
    ;;
  esac
done

[[ -d "${ADAPTER_ROOT}" ]] || { echo "[ERROR] adapter root is missing: ${ADAPTER_ROOT}" >&2; exit 2; }
[[ -x "${COMPILER}" ]] || { echo "[ERROR] compiler is not executable: ${COMPILER}" >&2; exit 2; }
[[ -x "${SUFFIX_TOOL}" ]] || { echo "[ERROR] suffix tool is not executable: ${SUFFIX_TOOL}" >&2; exit 2; }
[[ "${MAX_FILES}" =~ ^[0-9]+$ ]] || { echo "[ERROR] --max-files must be non-negative" >&2; exit 2; }

dump_args=(
  --data-dir "${ADAPTER_ROOT}"
  --output-root "${WORK_ROOT}"
  --run-id compiler_default
  --compiler "${COMPILER}"
  --allow-failures
  --raw
)
if [[ "${MAX_FILES}" -gt 0 ]]; then
  dump_args+=(--max-files "${MAX_FILES}")
fi

bash "${REPO_ROOT}/tools/dump_before_cvpipelining_ir.sh" "${dump_args[@]}"

summary="${WORK_ROOT}/index/cvpipelining_compiler_default/summary.tsv"
[[ -f "${summary}" ]] || { echo "[ERROR] dump summary is missing: ${summary}" >&2; exit 2; }
mkdir -p "${DATASET_ROOT}"

manifest="${DATASET_ROOT}/manifest.tsv"
suffix_tool_sha256="$(shasum -a 256 "${SUFFIX_TOOL}" | awk '{print $1}')"
printf 'adapter\tdump_status\tcompile_status\tsnapshot_count\tadapter_sha256\tsuffix_tool_sha256\tbefore_cvpipelining_files\n' >"${manifest}"
"${COMPILER}" --version >"${DATASET_ROOT}/compiler_version.txt" 2>&1 || true

total=0
complete=0
failed=0
while IFS=$'\t' read -r input compiler_status _ case_dir _ _ _ _; do
  [[ "${input}" != "input" ]] || continue
  adapter="$(basename "${input}")"
  destination="${DATASET_ROOT}/${adapter}"
  rm -rf "${destination}"
  mkdir -p "${destination}"
  paths=()
  while IFS= read -r source; do
    [[ -f "${source}" ]] || continue
    output="${destination}/$(basename "${source}")"
    temporary="${case_dir}/generic.tmp.mlir"
    generic_log="${case_dir}/generic.stderr.log"
    if "${SUFFIX_TOOL}" "${source}" \
        --disable-cv-pipelining \
        --stop-after-cvpipelining \
        --dump-after-cvpipelining-generic-ir="${output}" \
        --mlir-disable-threading \
        -o "${temporary}" >/dev/null 2>"${generic_log}"; then
      rm -f "${temporary}" "${generic_log}"
      paths+=("${output#"${MODULE_DIR}/"}")
    else
      rm -f "${temporary}" "${output}"
    fi
  done < <(find "${case_dir}" -maxdepth 1 -type f -name 'before_cvpipelining_*.mlir' ! -name '*.raw.mlir' | LC_ALL=C sort)

  adapter_hash="$(shasum -a 256 "${input}" | awk '{print $1}')"
  if [[ ${#paths[@]} -gt 0 ]]; then
    dump_status=complete
    complete=$((complete + 1))
  else
    dump_status=failed
    failed=$((failed + 1))
    rmdir "${destination}" 2>/dev/null || true
  fi
  joined="$(IFS=,; echo "${paths[*]}")"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "${adapter}" "${dump_status}" "${compiler_status}" "${#paths[@]}" \
    "${adapter_hash}" "${suffix_tool_sha256}" "${joined}" >>"${manifest}"
  total=$((total + 1))
done <"${summary}"

echo "BEFORE_CVPIPELINE_DATASET total=${total} complete=${complete} failed=${failed}"
echo "${manifest}"
[[ "${complete}" -gt 0 ]]
