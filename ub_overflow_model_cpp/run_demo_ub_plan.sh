#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

# BEFORE_CVPIPELINING_IR="ub_overflow_model_cpp/data/before_cvpipelining/triton.language.flip.ttadapter/before_cvpipelining_func_func_flip_kernel_32.mlir"
# BEFORE_CVPIPELINING_IR="ub_overflow_model_cpp/data/before_cvpipelining/attn_fwd.ttadapter/before_cvpipelining_func_func_attn_fwd_32.mlir"
BEFORE_CVPIPELINING_IR="ub_overflow_model_cpp/data/before_cvpipelining/python_tutorial_09-persistent-matmul.ttadapter/before_cvpipelining_func_func_matmul_kernel_32.mlir"
# BEFORE_CVPIPELINING_IR="ub_overflow_model_cpp/data/before_cvpipelining/triton.language.randn.ttadapter/before_cvpipelining_func_func_kernel_randn_32.mlir"
OUTPUT_ROOT="ub_overflow_model_cpp/output/demo"
DEMO_JSON=""
DEMO_HTML=""
ORACLE_DIR=""
SUFFIX_TOOL="build/bin/bishengir-cvpipeline-suffix-compile"
RUN_ORACLE=true
BUILD_SUFFIX_ORACLE=true

# ---------------------------------------------------------------------------
# CVPipelining pass parameters
# These control loop pipelining and determine the IR entering the suffix.
# ---------------------------------------------------------------------------
CV_DISABLE_PIPELINING=false
CV_PIPELINE_DEPTH=-1
CV_ENABLE_PRELOAD=false
CV_ENABLE_LAZY_LOADING=false

# ---------------------------------------------------------------------------
# MarkMultiBuffer pass parameters (directly affect UB buffer count and peak)
# false keeps only explicit marks already present in IR; true also infers marks.
# Local strategy: no-l0c matches bishengir-compile/suffix default.
# Mix strategy: only-cube enables cube-side auto marks in MIX functions.
# ---------------------------------------------------------------------------
MULTIBUFFER_ENABLE_AUTO=false
MULTIBUFFER_LOCAL_STRATEGY=no-l0c
MULTIBUFFER_MIX_STRATEGY=only-cube

# ---------------------------------------------------------------------------
# Remaining suffix / PlanMemory parameters
# ---------------------------------------------------------------------------
SUFFIX_ENABLE_TRITON_KERNEL_COMPILE=false
SUFFIX_ENABLE_CODE_MOTION=true
SUFFIX_TILE_MIX_CUBE_LOOP=2
SUFFIX_TILE_MIX_VECTOR_LOOP=2
SUFFIX_ENABLE_UBUF_SAVING=false
SUFFIX_DISABLE_ALIGN_ALLOC_SIZE=false
SUFFIX_DISABLE_ENABLE_STRIDE_ALIGN=false
SUFFIX_DISABLE_INFER_HIVM_DATA_LAYOUT=false
RESTRICT_INPLACE_AS_ISA=false
# Keep empty for PlanMemory retry mode; set only to reproduce one attempt.
RANDOM_SEED=""

usage() {
  cat <<'EOF'
Usage:
  bash ub_overflow_model_cpp/run_demo_ub_plan.sh [options]

Input/output:
  --before-cvpipelining-ir PATH
  --output-root DIR
  --json PATH
  --html PATH
  --oracle-dir DIR
  --suffix-tool PATH
  --skip-oracle
  --skip-suffix-build

CVPipelining options:
  --cv-disable-pipelining true|false
  --cv-pipeline-depth N
  --cv-enable-preload true|false
  --cv-enable-lazy-loading true|false

MarkMultiBuffer options (affect UB buffer count and peak):
  --suffix-enable-auto-multi-buffer true|false
  --suffix-local-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --suffix-mix-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c

Remaining suffix / PlanMemory options:
  --suffix-enable-code-motion true|false
  --suffix-tile-mix-cube-loop N
  --suffix-tile-mix-vector-loop N
  --suffix-enable-ubuf-saving true|false
  --suffix-enable-triton-kernel-compile true|false
  --suffix-disable-align-alloc-size true|false
  --suffix-disable-enable-stride-align true|false
  --suffix-disable-infer-hivm-data-layout true|false
  --restrict-inplace-as-isa true|false
  --random-seed N        # omit for PlanMemory retry mode
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --before-cvpipelining-ir=*) BEFORE_CVPIPELINING_IR="${1#*=}"; shift ;;
    --before-cvpipelining-ir) BEFORE_CVPIPELINING_IR="$2"; shift 2 ;;
    --output-root=*) OUTPUT_ROOT="${1#*=}"; shift ;;
    --output-root) OUTPUT_ROOT="$2"; shift 2 ;;
    --json=*) DEMO_JSON="${1#*=}"; shift ;;
    --json) DEMO_JSON="$2"; shift 2 ;;
    --html=*) DEMO_HTML="${1#*=}"; shift ;;
    --html) DEMO_HTML="$2"; shift 2 ;;
    --oracle-dir=*) ORACLE_DIR="${1#*=}"; shift ;;
    --oracle-dir) ORACLE_DIR="$2"; shift 2 ;;
    --suffix-tool=*) SUFFIX_TOOL="${1#*=}"; shift ;;
    --suffix-tool) SUFFIX_TOOL="$2"; shift 2 ;;
    --skip-oracle) RUN_ORACLE=false; shift ;;
    --skip-suffix-build) BUILD_SUFFIX_ORACLE=false; shift ;;
    --cv-disable-pipelining=*) CV_DISABLE_PIPELINING="${1#*=}"; shift ;;
    --cv-disable-pipelining) CV_DISABLE_PIPELINING="$2"; shift 2 ;;
    --cv-pipeline-depth=*) CV_PIPELINE_DEPTH="${1#*=}"; shift ;;
    --cv-pipeline-depth) CV_PIPELINE_DEPTH="$2"; shift 2 ;;
    --cv-enable-preload=*) CV_ENABLE_PRELOAD="${1#*=}"; shift ;;
    --cv-enable-preload) CV_ENABLE_PRELOAD="$2"; shift 2 ;;
    --cv-enable-lazy-loading=*) CV_ENABLE_LAZY_LOADING="${1#*=}"; shift ;;
    --cv-enable-lazy-loading) CV_ENABLE_LAZY_LOADING="$2"; shift 2 ;;
    --suffix-enable-auto-multi-buffer=*) MULTIBUFFER_ENABLE_AUTO="${1#*=}"; shift ;;
    --suffix-enable-auto-multi-buffer) MULTIBUFFER_ENABLE_AUTO="$2"; shift 2 ;;
    --suffix-enable-triton-kernel-compile=*) SUFFIX_ENABLE_TRITON_KERNEL_COMPILE="${1#*=}"; shift ;;
    --suffix-enable-triton-kernel-compile) SUFFIX_ENABLE_TRITON_KERNEL_COMPILE="$2"; shift 2 ;;
    --suffix-enable-code-motion=*) SUFFIX_ENABLE_CODE_MOTION="${1#*=}"; shift ;;
    --suffix-enable-code-motion) SUFFIX_ENABLE_CODE_MOTION="$2"; shift 2 ;;
    --suffix-tile-mix-cube-loop=*) SUFFIX_TILE_MIX_CUBE_LOOP="${1#*=}"; shift ;;
    --suffix-tile-mix-cube-loop) SUFFIX_TILE_MIX_CUBE_LOOP="$2"; shift 2 ;;
    --suffix-tile-mix-vector-loop=*) SUFFIX_TILE_MIX_VECTOR_LOOP="${1#*=}"; shift ;;
    --suffix-tile-mix-vector-loop) SUFFIX_TILE_MIX_VECTOR_LOOP="$2"; shift 2 ;;
    --suffix-enable-ubuf-saving=*) SUFFIX_ENABLE_UBUF_SAVING="${1#*=}"; shift ;;
    --suffix-enable-ubuf-saving) SUFFIX_ENABLE_UBUF_SAVING="$2"; shift 2 ;;
    --suffix-disable-align-alloc-size=*) SUFFIX_DISABLE_ALIGN_ALLOC_SIZE="${1#*=}"; shift ;;
    --suffix-disable-align-alloc-size) SUFFIX_DISABLE_ALIGN_ALLOC_SIZE="$2"; shift 2 ;;
    --suffix-disable-enable-stride-align=*) SUFFIX_DISABLE_ENABLE_STRIDE_ALIGN="${1#*=}"; shift ;;
    --suffix-disable-enable-stride-align) SUFFIX_DISABLE_ENABLE_STRIDE_ALIGN="$2"; shift 2 ;;
    --suffix-disable-infer-hivm-data-layout=*) SUFFIX_DISABLE_INFER_HIVM_DATA_LAYOUT="${1#*=}"; shift ;;
    --suffix-disable-infer-hivm-data-layout) SUFFIX_DISABLE_INFER_HIVM_DATA_LAYOUT="$2"; shift 2 ;;
    --suffix-local-multi-buffer-strategy=*) MULTIBUFFER_LOCAL_STRATEGY="${1#*=}"; shift ;;
    --suffix-local-multi-buffer-strategy) MULTIBUFFER_LOCAL_STRATEGY="$2"; shift 2 ;;
    --suffix-mix-multi-buffer-strategy=*) MULTIBUFFER_MIX_STRATEGY="${1#*=}"; shift ;;
    --suffix-mix-multi-buffer-strategy) MULTIBUFFER_MIX_STRATEGY="$2"; shift 2 ;;
    --restrict-inplace-as-isa=*) RESTRICT_INPLACE_AS_ISA="${1#*=}"; shift ;;
    --restrict-inplace-as-isa) RESTRICT_INPLACE_AS_ISA="$2"; shift 2 ;;
    --random-seed=*) RANDOM_SEED="${1#*=}"; shift ;;
    --random-seed) RANDOM_SEED="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "[ERROR] unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

input_name="${BEFORE_CVPIPELINING_IR##*/}"
kernel_name="${input_name%.mlir}"
case "${kernel_name}" in
  before_cvpipelining_func_func_*)
    kernel_name="${kernel_name#before_cvpipelining_func_func_}"
    ;;
  before_cvpipelining_func_*)
    kernel_name="${kernel_name#before_cvpipelining_func_}"
    ;;
esac
if [[ -z "${kernel_name}" ]]; then
  echo "[ERROR] cannot derive kernel name from input: ${BEFORE_CVPIPELINING_IR}" >&2
  exit 2
fi

kernel_output_dir="${OUTPUT_ROOT}/${kernel_name}"
DEMO_JSON="${DEMO_JSON:-${kernel_output_dir}/ub_plan.json}"
DEMO_HTML="${DEMO_HTML:-${kernel_output_dir}/ub_plan_visualizer.html}"
ORACLE_DIR="${ORACLE_DIR:-${kernel_output_dir}/suffix_oracle}"

echo "[INFO] Kernel: ${kernel_name}"
echo "[INFO] Output directory: ${kernel_output_dir}"
echo "[INFO] JSON: ${DEMO_JSON}"
echo "[INFO] HTML: ${DEMO_HTML}"
echo "[INFO] Oracle: ${ORACLE_DIR}"

echo "[1/5] Building bishengir-ub-overflow-model incrementally..."
bash ub_overflow_model_cpp/build.sh >/dev/null

if [[ "${RUN_ORACLE}" == "true" && "${BUILD_SUFFIX_ORACLE}" == "true" ]]; then
  echo "[2/5] Building suffix compiler incrementally..."
  # Keep CMake/Ninja output visible so incremental progress and errors are clear.
  cmake --build build --target bishengir-cvpipeline-suffix-compile -j1
fi

echo "[3/5] Running lightweight model and writing JSON..."
mkdir -p "$(dirname "${DEMO_JSON}")"
model_args=(
  python3 ub_overflow_model_cpp/scripts/plan_before_cvpipelining_ub.py
  --before-cvpipelining-ir="${BEFORE_CVPIPELINING_IR}"
  --cv-disable-pipelining="${CV_DISABLE_PIPELINING}"
  --cv-pipeline-depth="${CV_PIPELINE_DEPTH}"
  --cv-enable-preload="${CV_ENABLE_PRELOAD}"
  --cv-enable-lazy-loading="${CV_ENABLE_LAZY_LOADING}"
  --suffix-enable-auto-multi-buffer="${MULTIBUFFER_ENABLE_AUTO}"
  --suffix-enable-code-motion="${SUFFIX_ENABLE_CODE_MOTION}"
  --suffix-tile-mix-cube-loop="${SUFFIX_TILE_MIX_CUBE_LOOP}"
  --suffix-tile-mix-vector-loop="${SUFFIX_TILE_MIX_VECTOR_LOOP}"
  --suffix-enable-ubuf-saving="${SUFFIX_ENABLE_UBUF_SAVING}"
  --suffix-enable-triton-kernel-compile="${SUFFIX_ENABLE_TRITON_KERNEL_COMPILE}"
  --suffix-disable-align-alloc-size="${SUFFIX_DISABLE_ALIGN_ALLOC_SIZE}"
  --suffix-disable-enable-stride-align="${SUFFIX_DISABLE_ENABLE_STRIDE_ALIGN}"
  --suffix-disable-infer-hivm-data-layout="${SUFFIX_DISABLE_INFER_HIVM_DATA_LAYOUT}"
  --suffix-local-multi-buffer-strategy="${MULTIBUFFER_LOCAL_STRATEGY}"
  --suffix-mix-multi-buffer-strategy="${MULTIBUFFER_MIX_STRATEGY}"
  --format=json
  --output="${DEMO_JSON}"
)
if [[ "${RESTRICT_INPLACE_AS_ISA}" == "true" || "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
  model_args+=(--restrict-inplace-as-isa)
fi
if [[ -n "${RANDOM_SEED}" ]]; then
  model_args+=(--random-seed="${RANDOM_SEED}")
fi
model_log="$(dirname "${DEMO_JSON}")/model_stdout.log"
MODEL_OVERFLOW=false
if "${model_args[@]}" >"${model_log}"; then
  :
else
  model_status=$?
  if [[ "${model_status}" -eq 2 ]] && python3 -c \
      'import json,sys; raise SystemExit(0 if json.load(open(sys.argv[1]))["result"]["overflow"] is True else 1)' \
      "${DEMO_JSON}"; then
    MODEL_OVERFLOW=true
    echo "[INFO] Lightweight model reports exact UB overflow; continuing demo generation."
  else
    echo "[ERROR] Lightweight model failed (exit ${model_status})." >&2
    echo "        Input: ${BEFORE_CVPIPELINING_IR}" >&2
    echo "        JSON report: ${DEMO_JSON}" >&2
    echo "        Wrapper stdout: ${model_log}" >&2
    exit "${model_status}"
  fi
fi

if [[ "${RUN_ORACLE}" == "true" ]]; then
  if [[ ! -x "${SUFFIX_TOOL}" ]]; then
    echo "[ERROR] suffix compiler not found or not executable: ${SUFFIX_TOOL}" >&2
    echo "        build it with: cmake --build build --target bishengir-cvpipeline-suffix-compile -j8" >&2
    exit 1
  fi

  echo "[4/5] Running suffix-compile oracle..."
  mkdir -p "${ORACLE_DIR}"
  oracle_tsv="${ORACLE_DIR}/oracle.tsv"
  oracle_ir="${ORACLE_DIR}/after_local_plan_memory.mlir"
  suffix_args=(
    "${SUFFIX_TOOL}" "${BEFORE_CVPIPELINING_IR}"
    --mlir-disable-threading
    --cv-pipeline-depth="${CV_PIPELINE_DEPTH}"
    --disable-cv-pipelining="${CV_DISABLE_PIPELINING}"
    --enable-preload="${CV_ENABLE_PRELOAD}"
    --enable-cv-lazy-loading="${CV_ENABLE_LAZY_LOADING}"
    --enable-auto-multi-buffer="${MULTIBUFFER_ENABLE_AUTO}"
    --enable-code-motion="${SUFFIX_ENABLE_CODE_MOTION}"
    --tile-mix-cube-loop="${SUFFIX_TILE_MIX_CUBE_LOOP}"
    --tile-mix-vector-loop="${SUFFIX_TILE_MIX_VECTOR_LOOP}"
    --enable-ubuf-saving="${SUFFIX_ENABLE_UBUF_SAVING}"
    --enable-triton-kernel-compile="${SUFFIX_ENABLE_TRITON_KERNEL_COMPILE}"
    --disable-align-alloc-size="${SUFFIX_DISABLE_ALIGN_ALLOC_SIZE}"
    --disable-enable-stride-align="${SUFFIX_DISABLE_ENABLE_STRIDE_ALIGN}"
    --disable-infer-hivm-data-layout="${SUFFIX_DISABLE_INFER_HIVM_DATA_LAYOUT}"
    --limit-auto-multi-buffer-of-local-buffer "${MULTIBUFFER_LOCAL_STRATEGY}"
    --limit-auto-multi-buffer-buffer "${MULTIBUFFER_MIX_STRATEGY}"
    -o "${oracle_ir}"
  )
  if [[ "${RESTRICT_INPLACE_AS_ISA}" == "true" || "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
    suffix_args+=(--restrict-inplace-as-isa)
  fi
  if [[ -n "${RANDOM_SEED}" ]]; then
    suffix_args+=(--plan-memory-seed "${RANDOM_SEED}")
  fi
  oracle_stdout="${ORACLE_DIR}/stdout.log"
  if BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS=1 "${suffix_args[@]}" \
      >"${oracle_stdout}" 2>"${oracle_tsv}"; then
    :
  else
    oracle_status=$?
    if [[ "${MODEL_OVERFLOW}" == "true" ]] && \
        python3 ub_overflow_model_cpp/scripts/compare_ub_plan_with_suffix_oracle.py \
          --model-json "${DEMO_JSON}" \
          --oracle-tsv "${oracle_tsv}" >/dev/null; then
      echo "[INFO] Suffix compiler confirms the complete overflow plan."
    else
      echo "[ERROR] Suffix compiler failed (exit ${oracle_status})." >&2
      echo "        Input: ${BEFORE_CVPIPELINING_IR}" >&2
      if [[ -s "${oracle_tsv}" ]]; then
        echo "        Last diagnostic lines:" >&2
        tail -n 80 "${oracle_tsv}" >&2
        echo "        Full diagnostics: ${oracle_tsv}" >&2
      elif [[ -s "${oracle_stdout}" ]]; then
        echo "        Last stdout lines:" >&2
        tail -n 80 "${oracle_stdout}" >&2
        echo "        Full stdout: ${oracle_stdout}" >&2
      else
        echo "        Compiler produced no diagnostic output." >&2
      fi
      exit "${oracle_status}"
    fi
  fi
else
  echo "[4/5] Skipping suffix-compile oracle."
fi

echo
echo "[5/5] Summary"
python3 ub_overflow_model_cpp/scripts/render_ub_demo_html.py \
  --template ub_overflow_model_cpp/demo/ub_plan_visualizer.html \
  --json "${DEMO_JSON}" \
  --output "${DEMO_HTML}" >/dev/null
python3 ub_overflow_model_cpp/scripts/print_ub_plan_summary.py \
  "${DEMO_JSON}" --html "${DEMO_HTML}"
if [[ "${RUN_ORACLE}" == "true" ]]; then
  echo
  python3 ub_overflow_model_cpp/scripts/compare_ub_plan_with_suffix_oracle.py \
    --model-json "${DEMO_JSON}" \
    --oracle-tsv "${ORACLE_DIR}/oracle.tsv"
fi
