#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

BEFORE_CVPIPELINING_IR="ub_overflow_model_cpp/data/before_cvpipelining/python_tutorial_09-persistent-matmul.ttadapter/before_cvpipelining_func_func_matmul_kernel_32.mlir"
OUTPUT_ROOT="ub_overflow_model_cpp/output/demo"
DEMO_JSON=""
DEMO_HTML=""

CV_DISABLE_PIPELINING=false
CV_PIPELINE_DEPTH=-1
CV_ENABLE_PRELOAD=false
CV_ENABLE_LAZY_LOADING=false
ENABLE_AUTO_MULTI_BUFFER=false
LOCAL_MULTI_BUFFER_STRATEGY=no-l0c
MIX_MULTI_BUFFER_STRATEGY=only-cube
ENABLE_TRITON_KERNEL_COMPILE=false
ENABLE_CODE_MOTION=true
TILE_MIX_CUBE_LOOP=2
TILE_MIX_VECTOR_LOOP=2
ENABLE_UBUF_SAVING=false
ENABLE_HIVM_AUTO_STORAGE_ALIGN=true
RESTRICT_INPLACE_AS_ISA=false
PLAN_MEMORY_SEED=-1

usage() {
  cat <<'EOF'
Usage:
  bash ub_overflow_model_cpp/run_demo_ub_plan.sh [options]

Input/output:
  --before-cvpipelining-ir PATH
  --output-root DIR
  --json PATH
  --html PATH

CVPipelining options:
  --cv-disable-pipelining true|false
  --cv-pipeline-depth N
  --cv-enable-preload true|false
  --cv-enable-lazy-loading true|false

UB-affecting planning options:
  --enable-auto-multi-buffer true|false
  --local-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --mix-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --enable-code-motion true|false
  --tile-mix-cube-loop N
  --tile-mix-vector-loop N
  --enable-ubuf-saving true|false
  --enable-triton-kernel-compile true|false
  --enable-hivm-auto-storage-align true|false
  --restrict-inplace-as-isa true|false
  --plan-memory-seed -1|0..19
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
    --cv-disable-pipelining=*) CV_DISABLE_PIPELINING="${1#*=}"; shift ;;
    --cv-disable-pipelining) CV_DISABLE_PIPELINING="$2"; shift 2 ;;
    --cv-pipeline-depth=*) CV_PIPELINE_DEPTH="${1#*=}"; shift ;;
    --cv-pipeline-depth) CV_PIPELINE_DEPTH="$2"; shift 2 ;;
    --cv-enable-preload=*) CV_ENABLE_PRELOAD="${1#*=}"; shift ;;
    --cv-enable-preload) CV_ENABLE_PRELOAD="$2"; shift 2 ;;
    --cv-enable-lazy-loading=*) CV_ENABLE_LAZY_LOADING="${1#*=}"; shift ;;
    --cv-enable-lazy-loading) CV_ENABLE_LAZY_LOADING="$2"; shift 2 ;;
    --enable-auto-multi-buffer=*) ENABLE_AUTO_MULTI_BUFFER="${1#*=}"; shift ;;
    --enable-auto-multi-buffer) ENABLE_AUTO_MULTI_BUFFER="$2"; shift 2 ;;
    --local-multi-buffer-strategy=*) LOCAL_MULTI_BUFFER_STRATEGY="${1#*=}"; shift ;;
    --local-multi-buffer-strategy) LOCAL_MULTI_BUFFER_STRATEGY="$2"; shift 2 ;;
    --mix-multi-buffer-strategy=*) MIX_MULTI_BUFFER_STRATEGY="${1#*=}"; shift ;;
    --mix-multi-buffer-strategy) MIX_MULTI_BUFFER_STRATEGY="$2"; shift 2 ;;
    --enable-code-motion=*) ENABLE_CODE_MOTION="${1#*=}"; shift ;;
    --enable-code-motion) ENABLE_CODE_MOTION="$2"; shift 2 ;;
    --tile-mix-cube-loop=*) TILE_MIX_CUBE_LOOP="${1#*=}"; shift ;;
    --tile-mix-cube-loop) TILE_MIX_CUBE_LOOP="$2"; shift 2 ;;
    --tile-mix-vector-loop=*) TILE_MIX_VECTOR_LOOP="${1#*=}"; shift ;;
    --tile-mix-vector-loop) TILE_MIX_VECTOR_LOOP="$2"; shift 2 ;;
    --enable-ubuf-saving=*) ENABLE_UBUF_SAVING="${1#*=}"; shift ;;
    --enable-ubuf-saving) ENABLE_UBUF_SAVING="$2"; shift 2 ;;
    --enable-triton-kernel-compile=*) ENABLE_TRITON_KERNEL_COMPILE="${1#*=}"; shift ;;
    --enable-triton-kernel-compile) ENABLE_TRITON_KERNEL_COMPILE="$2"; shift 2 ;;
    --enable-hivm-auto-storage-align=*) ENABLE_HIVM_AUTO_STORAGE_ALIGN="${1#*=}"; shift ;;
    --enable-hivm-auto-storage-align) ENABLE_HIVM_AUTO_STORAGE_ALIGN="$2"; shift 2 ;;
    --restrict-inplace-as-isa=*) RESTRICT_INPLACE_AS_ISA="${1#*=}"; shift ;;
    --restrict-inplace-as-isa) RESTRICT_INPLACE_AS_ISA="$2"; shift 2 ;;
    --plan-memory-seed=*) PLAN_MEMORY_SEED="${1#*=}"; shift ;;
    --plan-memory-seed) PLAN_MEMORY_SEED="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "[ERROR] unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

input_name="${BEFORE_CVPIPELINING_IR##*/}"
kernel_name="${input_name%.mlir}"
case "${kernel_name}" in
  before_cvpipelining_func_func_*) kernel_name="${kernel_name#before_cvpipelining_func_func_}" ;;
  before_cvpipelining_func_*) kernel_name="${kernel_name#before_cvpipelining_func_}" ;;
esac
if [[ -z "${kernel_name}" ]]; then
  echo "[ERROR] cannot derive kernel name from input: ${BEFORE_CVPIPELINING_IR}" >&2
  exit 2
fi

kernel_output_dir="${OUTPUT_ROOT}/${kernel_name}"
DEMO_JSON="${DEMO_JSON:-${kernel_output_dir}/ub_plan.json}"
DEMO_HTML="${DEMO_HTML:-${kernel_output_dir}/ub_plan_visualizer.html}"

echo "[INFO] Kernel: ${kernel_name}"
echo "[INFO] JSON: ${DEMO_JSON}"
echo "[INFO] HTML: ${DEMO_HTML}"

echo "[1/3] Building bishengir-ub-overflow-model incrementally..."
bash ub_overflow_model_cpp/build.sh >/dev/null

echo "[2/3] Running lightweight model and writing JSON..."
mkdir -p "$(dirname "${DEMO_JSON}")"
model_args=(
  python3 ub_overflow_model_cpp/scripts/plan_before_cvpipelining_ub.py
  --before-cvpipelining-ir="${BEFORE_CVPIPELINING_IR}"
  --cv-disable-pipelining="${CV_DISABLE_PIPELINING}"
  --cv-pipeline-depth="${CV_PIPELINE_DEPTH}"
  --cv-enable-preload="${CV_ENABLE_PRELOAD}"
  --cv-enable-lazy-loading="${CV_ENABLE_LAZY_LOADING}"
  --enable-auto-multi-buffer="${ENABLE_AUTO_MULTI_BUFFER}"
  --enable-code-motion="${ENABLE_CODE_MOTION}"
  --tile-mix-cube-loop="${TILE_MIX_CUBE_LOOP}"
  --tile-mix-vector-loop="${TILE_MIX_VECTOR_LOOP}"
  --enable-ubuf-saving="${ENABLE_UBUF_SAVING}"
  --enable-triton-kernel-compile="${ENABLE_TRITON_KERNEL_COMPILE}"
  --enable-hivm-auto-storage-align="${ENABLE_HIVM_AUTO_STORAGE_ALIGN}"
  --local-multi-buffer-strategy="${LOCAL_MULTI_BUFFER_STRATEGY}"
  --mix-multi-buffer-strategy="${MIX_MULTI_BUFFER_STRATEGY}"
  --plan-memory-seed="${PLAN_MEMORY_SEED}"
  --format=json
  --output="${DEMO_JSON}"
)
if [[ "${RESTRICT_INPLACE_AS_ISA}" == "true" || "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
  model_args+=(--restrict-inplace-as-isa)
fi

if "${model_args[@]}" >/dev/null; then
  :
else
  model_status=$?
  if [[ "${model_status}" -eq 2 ]] && python3 -c \
      'import json,sys; raise SystemExit(0 if json.load(open(sys.argv[1]))["result"]["overflow"] is True else 1)' \
      "${DEMO_JSON}"; then
    echo "[INFO] Lightweight model reports exact UB overflow; rendering the complete plan."
  else
    echo "[ERROR] Lightweight model failed (exit ${model_status})." >&2
    exit "${model_status}"
  fi
fi

echo "[3/3] Rendering summary and HTML..."
python3 ub_overflow_model_cpp/scripts/render_ub_demo_html.py \
  --template ub_overflow_model_cpp/demo/ub_plan_visualizer.html \
  --json "${DEMO_JSON}" \
  --output "${DEMO_HTML}" >/dev/null
python3 ub_overflow_model_cpp/scripts/print_ub_plan_summary.py \
  "${DEMO_JSON}" --html "${DEMO_HTML}"
