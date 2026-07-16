#!/usr/bin/env bash
# Demo driver for the CVPipeline UB model.
#
# Runs the lightweight C++ model (cvpipeline_ub_model --action=plan-before-cvpipeline)
# on the before-CVPipeline example inputs and prints a per-demo UB plan summary.
# The model owns the whole path: CVPipelining model -> post-CVPipeline AIV
# projection (14 stages) -> per-function suffix planning -> UB peak/buffers.
#
# Input boundary: compiler-emitted generic MLIR immediately before
# createCVPipeliningPass.  The model assumes the compiler's post-CVPipeline
# defaults: tile_mix_vector_loop=2, tile_mix_cube_loop=2,
# enable_auto_bind_sub_block=true, enable_code_motion=true,
# enable_ubuf_saving=false.  UB-saving and non-default post-pipeline configs are
# unsupported (reported incomplete/blocker, never silently forced exact).
#
# This script does NOT invoke the historical suffix compiler/oracle; the
# post-CVPipeline pipeline is modeled directly by the lightweight tool.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

# Default demo set: the two pure-AIV demos plus the end-to-end MIX demo.
DEMO_VECTOR_ADD="cvpipeline_ub_model_cpp/examples/inputs/demo_vector_add_before_cvpipeline.mlir"
DEMO_RANDN="cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir"
DEMO_MIX="cvpipeline_ub_model_cpp/examples/inputs/demo_mix_before_cvpipeline.mlir"

# Visualizer outputs for the primary (MIX) demo.
DEMO_JSON="cvpipeline_ub_model_cpp/output/demo/ub_plan.json"
DEMO_HTML="cvpipeline_ub_model_cpp/output/demo/ub_plan_visualizer.html"

# CVPipeline knobs (only affect the createCVPipeliningPass modeling path).
CV_DISABLE_PIPELINING=false
CV_PIPELINE_DEPTH=-1
CV_ENABLE_PRELOAD=false
CV_ENABLE_LAZY_LOADING=false

# Suffix / PlanMemory knobs (only affect UB buffers after CVPipeline).
SUFFIX_ENABLE_AUTO_MULTI_BUFFER=false
SUFFIX_LOCAL_MULTI_BUFFER_STRATEGY=no-l0c
SUFFIX_MIX_MULTI_BUFFER_STRATEGY=only-cube
RESTRICT_INPLACE_AS_ISA=false
RANDOM_SEED=""

usage() {
  cat <<'EOF'
Usage:
  bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh [options]

Runs the lightweight CVPipeline UB model on the vector_add, randn, and MIX
demos and prints each demo's precision, status, and UB peak.

Primary demo / visualization:
  --json PATH            JSON output for the primary demo (default: MIX)
  --html PATH            HTML visualizer output for the primary demo

CVPipeline options (affect createCVPipeliningPass modeling only):
  --cv-disable-pipelining true|false
  --cv-pipeline-depth N
  --cv-enable-preload true|false
  --cv-enable-lazy-loading true|false

Suffix / PlanMemory options (affect UB buffers after CVPipeline):
  --suffix-enable-auto-multi-buffer true|false
  --suffix-local-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --suffix-mix-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --restrict-inplace-as-isa true|false
  --random-seed N        # omit for PlanMemory retry mode (default)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --json) DEMO_JSON="$2"; shift 2 ;;
    --html) DEMO_HTML="$2"; shift 2 ;;
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

echo "[1/3] Building the lightweight CVPipeline UB model..."
bash cvpipeline_ub_model_cpp/build.sh >/dev/null
MODEL="cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model"

run_demo() {
  local label="$1"
  local ir="$2"
  echo
  echo "=== ${label} (${ir}) ==="
  local args=(
    "${MODEL}"
    --action=plan-before-cvpipeline
    --before-cvpipeline-ir="${ir}"
    --cv-pipeline-depth="${CV_PIPELINE_DEPTH}"
    --disable-cv-pipelining="${CV_DISABLE_PIPELINING}"
    --enable-preload="${CV_ENABLE_PRELOAD}"
    --enable-cv-lazy-loading="${CV_ENABLE_LAZY_LOADING}"
    --enable-auto-multi-buffer="${SUFFIX_ENABLE_AUTO_MULTI_BUFFER}"
    --limit-auto-multi-buffer-of-local-buffer="${SUFFIX_LOCAL_MULTI_BUFFER_STRATEGY}"
    --limit-auto-multi-buffer-buffer="${SUFFIX_MIX_MULTI_BUFFER_STRATEGY}"
    --format=json
  )
  if [[ "${RESTRICT_INPLACE_AS_ISA}" == "true" || "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
    args+=(--restrict-inplace-as-isa)
  fi
  if [[ -n "${RANDOM_SEED}" ]]; then
    args+=(--random-seed="${RANDOM_SEED}")
  fi
  python3 - "${args[@]}" <<'PY'
import json, subprocess, sys
cmd = sys.argv[1:]
proc = subprocess.run(cmd, text=True, capture_output=True)
if proc.returncode not in (0, 1, 2) or not proc.stdout.strip():
    print(f"  [ERROR] exit {proc.returncode}: {proc.stderr.strip()}", file=sys.stderr)
    sys.exit(proc.returncode)
payload = json.loads(proc.stdout)
print(f"  precision = {payload.get('precision')}")
print(f"  status    = {payload.get('status')}")
print(f"  ub_peak_bits   = {payload.get('ub_peak_bits')}")
print(f"  required_bits  = {payload.get('required_bits')}")
print(f"  capacity_bits  = {payload.get('capacity_bits')}")
print(f"  functions      = {len(payload.get('functions', []))}")
estimate = payload.get("debug_estimate")
if estimate:
    print(f"  debug_estimate = peak={estimate.get('ub_peak_bits')} "
          f"required={estimate.get('required_bits')} (non-authoritative)")
for fn in payload.get("functions", []):
    print(f"    - {fn.get('function')}: peak={fn.get('ub_peak_bits')} "
          f"required={fn.get('required_bits')} buffers={len(fn.get('buffers', []))}")
for diag in payload.get("diagnostics", []):
    print(f"  [diagnostic] {diag.get('pipeline_stage')}: {diag.get('reason')}")
PY
}

echo "[2/3] Running demos..."
run_demo "vector_add (pure-AIV)" "${DEMO_VECTOR_ADD}"
run_demo "randn (pure-AIV)" "${DEMO_RANDN}"
run_demo "MIX (end-to-end)" "${DEMO_MIX}"

echo
echo "[3/3] Writing primary demo JSON + visualizer..."
mkdir -p "$(dirname "${DEMO_JSON}")"
primary_args=(
  "${MODEL}"
  --action=plan-before-cvpipeline
  --before-cvpipeline-ir="${DEMO_MIX}"
  --cv-pipeline-depth="${CV_PIPELINE_DEPTH}"
  --disable-cv-pipelining="${CV_DISABLE_PIPELINING}"
  --enable-preload="${CV_ENABLE_PRELOAD}"
  --enable-cv-lazy-loading="${CV_ENABLE_LAZY_LOADING}"
  --enable-auto-multi-buffer="${SUFFIX_ENABLE_AUTO_MULTI_BUFFER}"
  --limit-auto-multi-buffer-of-local-buffer="${SUFFIX_LOCAL_MULTI_BUFFER_STRATEGY}"
  --limit-auto-multi-buffer-buffer="${SUFFIX_MIX_MULTI_BUFFER_STRATEGY}"
  --format=json
)
if [[ "${RESTRICT_INPLACE_AS_ISA}" == "true" || "${RESTRICT_INPLACE_AS_ISA}" == "1" ]]; then
  primary_args+=(--restrict-inplace-as-isa)
fi
if [[ -n "${RANDOM_SEED}" ]]; then
  primary_args+=(--random-seed="${RANDOM_SEED}")
fi
"${primary_args[@]}" >"${DEMO_JSON}"
python3 cvpipeline_ub_model_cpp/scripts/render_ub_demo_html.py \
  --template cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html \
  --json "${DEMO_JSON}" \
  --output "${DEMO_HTML}" >/dev/null
python3 cvpipeline_ub_model_cpp/scripts/print_ub_plan_summary.py \
  "${DEMO_JSON}" --html "${DEMO_HTML}"
