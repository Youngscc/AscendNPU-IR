#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

PRE_CVPIPELINE_IR="cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir"
DEMO_JSON="cvpipeline_ub_model_cpp/output/demo/ub_plan.json"
DEMO_HTML="cvpipeline_ub_model_cpp/output/demo/ub_plan_visualizer.html"
ORACLE_DIR="cvpipeline_ub_model_cpp/output/demo/suffix_oracle"
SUFFIX_TOOL="build/bin/bishengir-cvpipeline-suffix-compile"
RUN_ORACLE=true
BUILD_SUFFIX_ORACLE=true

# CVPipeline 参数：只影响 createCVPipeliningPass 的建模路径。
CV_DISABLE_PIPELINING=false
CV_PIPELINE_DEPTH=-1
CV_ENABLE_PRELOAD=false
CV_ENABLE_LAZY_LOADING=false

# suffix / PlanMemory 参数：只影响 CVPipeline 之后的 UB buffer 和规划。
SUFFIX_ENABLE_AUTO_MULTI_BUFFER=false
SUFFIX_LOCAL_MULTI_BUFFER_STRATEGY=no-l0c
SUFFIX_MIX_MULTI_BUFFER_STRATEGY=only-cube
RESTRICT_INPLACE_AS_ISA=false
# 默认留空，保持 PlanMemory retry/attempt 行为；只在定位固定 attempt 时设置。
RANDOM_SEED=""

usage() {
  cat <<'EOF'
Usage:
  bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh [options]

Input/output:
  --pre-cvpipeline-ir PATH
  --json PATH
  --html PATH
  --oracle-dir DIR
  --suffix-tool PATH
  --skip-oracle
  --skip-suffix-build

CVPipeline options:
  --cv-disable-pipelining true|false
  --cv-pipeline-depth N
  --cv-enable-preload true|false
  --cv-enable-lazy-loading true|false

Suffix / PlanMemory options:
  --suffix-enable-auto-multi-buffer true|false
  --suffix-local-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --suffix-mix-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --restrict-inplace-as-isa true|false
  --random-seed N        # omit for PlanMemory retry mode
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --pre-cvpipeline-ir) PRE_CVPIPELINE_IR="$2"; shift 2 ;;
    --json) DEMO_JSON="$2"; shift 2 ;;
    --html) DEMO_HTML="$2"; shift 2 ;;
    --oracle-dir) ORACLE_DIR="$2"; shift 2 ;;
    --suffix-tool) SUFFIX_TOOL="$2"; shift 2 ;;
    --skip-oracle) RUN_ORACLE=false; shift ;;
    --skip-suffix-build) BUILD_SUFFIX_ORACLE=false; shift ;;
    --cv-disable-pipelining) CV_DISABLE_PIPELINING="$2"; shift 2 ;;
    --cv-pipeline-depth) CV_PIPELINE_DEPTH="$2"; shift 2 ;;
    --cv-enable-preload) CV_ENABLE_PRELOAD="$2"; shift 2 ;;
    --cv-enable-lazy-loading) CV_ENABLE_LAZY_LOADING="$2"; shift 2 ;;
    --suffix-enable-auto-multi-buffer) SUFFIX_ENABLE_AUTO_MULTI_BUFFER="$2"; shift 2 ;;
    --suffix-local-multi-buffer-strategy) SUFFIX_LOCAL_MULTI_BUFFER_STRATEGY="$2"; shift 2 ;;
    --suffix-mix-multi-buffer-strategy) SUFFIX_MIX_MULTI_BUFFER_STRATEGY="$2"; shift 2 ;;
    --restrict-inplace-as-isa) RESTRICT_INPLACE_AS_ISA="$2"; shift 2 ;;
    --random-seed) RANDOM_SEED="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "[ERROR] unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

echo "[1/5] Building cvpipeline_ub_model incrementally..."
bash cvpipeline_ub_model_cpp/build.sh >/dev/null

if [[ "${RUN_ORACLE}" == "true" && "${BUILD_SUFFIX_ORACLE}" == "true" ]]; then
  echo "[2/5] Building suffix compiler incrementally..."
  # 保留 CMake/Ninja 输出，便于观察增量构建进度和编译错误。
  cmake --build build --target bishengir-cvpipeline-suffix-compile -j8
fi

echo "[3/5] Running lightweight model and writing JSON..."
mkdir -p "$(dirname "${DEMO_JSON}")"
model_args=(
  python3 cvpipeline_ub_model_cpp/scripts/plan_precvpipeline_ub.py
  --pre-cvpipeline-ir="${PRE_CVPIPELINE_IR}" \
  --cv-disable-pipelining="${CV_DISABLE_PIPELINING}" \
  --cv-pipeline-depth="${CV_PIPELINE_DEPTH}" \
  --cv-enable-preload="${CV_ENABLE_PRELOAD}" \
  --cv-enable-lazy-loading="${CV_ENABLE_LAZY_LOADING}" \
  --suffix-enable-auto-multi-buffer="${SUFFIX_ENABLE_AUTO_MULTI_BUFFER}" \
  --suffix-local-multi-buffer-strategy="${SUFFIX_LOCAL_MULTI_BUFFER_STRATEGY}" \
  --suffix-mix-multi-buffer-strategy="${SUFFIX_MIX_MULTI_BUFFER_STRATEGY}" \
  --format=json \
  --output="${DEMO_JSON}"
)
if [[ "${RESTRICT_INPLACE_AS_ISA}" == "true" || "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
  model_args+=(--restrict-inplace-as-isa)
fi
if [[ -n "${RANDOM_SEED}" ]]; then
  model_args+=(--random-seed="${RANDOM_SEED}")
fi
"${model_args[@]}" >"$(dirname "${DEMO_JSON}")/model_stdout.log"

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
    "${SUFFIX_TOOL}" "${PRE_CVPIPELINE_IR}"
    --mlir-disable-threading
    --cv-pipeline-depth="${CV_PIPELINE_DEPTH}"
    --disable-cv-pipelining="${CV_DISABLE_PIPELINING}"
    --enable-preload="${CV_ENABLE_PRELOAD}"
    --enable-cv-lazy-loading="${CV_ENABLE_LAZY_LOADING}"
    --enable-auto-multi-buffer="${SUFFIX_ENABLE_AUTO_MULTI_BUFFER}"
    --limit-auto-multi-buffer-of-local-buffer "${SUFFIX_LOCAL_MULTI_BUFFER_STRATEGY}"
    --limit-auto-multi-buffer-buffer "${SUFFIX_MIX_MULTI_BUFFER_STRATEGY}"
    -o "${oracle_ir}"
  )
  if [[ "${RESTRICT_INPLACE_AS_ISA}" == "true" || "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
    suffix_args+=(--restrict-inplace-as-isa)
  fi
  if [[ -n "${RANDOM_SEED}" ]]; then
    suffix_args+=(--plan-memory-seed "${RANDOM_SEED}")
  fi
  BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS=1 "${suffix_args[@]}" \
    >"${ORACLE_DIR}/stdout.log" 2>"${oracle_tsv}"
else
  echo "[3/4] Skipping suffix-compile oracle."
fi

echo
echo "[5/5] Summary"
python3 cvpipeline_ub_model_cpp/scripts/render_ub_demo_html.py \
  --template cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html \
  --json "${DEMO_JSON}" \
  --output "${DEMO_HTML}" >/dev/null
python3 cvpipeline_ub_model_cpp/scripts/print_ub_plan_summary.py \
  "${DEMO_JSON}" --html "${DEMO_HTML}"
if [[ "${RUN_ORACLE}" == "true" ]]; then
  echo
  python3 cvpipeline_ub_model_cpp/scripts/compare_ub_plan_with_suffix_oracle.py \
    --model-json "${DEMO_JSON}" \
    --oracle-tsv "${ORACLE_DIR}/oracle.tsv"
fi
