#!/usr/bin/env bash

set -u

INPUTS=()
DATA_DIR=""
OUTPUT_ROOT="Output/ub_peak_pass_ablation"
COMPILER="build/bin/bishengir-compile"
DUMP_SCRIPT="tools/dump_planmemory_ir.sh"
TARGET="Ascend910_9382"
DUMP_FULL_IR_TREE=0
KEEP_FULL_TREE=0
RUN_NAME=""

EXPERIMENTS=(
  base
  mb_on
  mb_off
  mb_no_limit
  mb_vector_only
  mb_cube_only
  cv_workspace_off
  preload_on
  preload_off
  autoblockify_on
  autoblockify_off
  ubuf_saving_on
  ubuf_saving_off
  code_motion_off
  autobind_off
  storage_align_off
  ops_reorder_off
  tile_cube4_vec1
  tile_cube1_vec4
  tile_cube2_vec2
)

usage() {
  cat <<'EOF'
Usage: bash tools/run_ub_peak_pass_ablation.sh [options]

Run the first-stage UB peak pass-group ablation matrix around local PlanMemory.
Each experiment calls tools/dump_planmemory_ir.sh and keeps PlanMemory before /
after IR, memory_info*.json, stdout, and stderr.

Options:
  -i, --input FILE
        Add one input file. Can be repeated. Defaults to data/attn_fwd.ttadapter.
  -d, --data-dir DIR
        Scan one directory for input files when --input is omitted.
  -o, --output-root DIR
        Root output directory. Defaults to Output/ub_peak_pass_ablation.
        A timestamped run directory is created under this root.
  -c, --compiler FILE
        bishengir-compile binary. Defaults to build/bin/bishengir-compile.
  --dump-script FILE
        Dump script. Defaults to tools/dump_planmemory_ir.sh.
  --target DEVICE
        Target device passed to bishengir-compile. Defaults to Ascend910_9382.
  --run-name NAME
        Use NAME as the run directory instead of a timestamp.
  --keep-full-tree
        Keep the full --mlir-print-ir-tree-dir for every experiment.
        By default the full tree is not created.
  -h, --help
        Show this help.

Outputs:
  <output-root>/<timestamp-or-run-name>/
    experiment_args.tsv
    summary.tsv
    memory_metrics.tsv
    <experiment-name>/
      summary.tsv
      <input-case>/
        before_local_plan_memory.mlir
        after_local_plan_memory*.mlir
        memory_info*.json
        compile.stdout.log
        compile.stderr.log
EOF
}

die() {
  echo "[ERROR] $*" >&2
  exit 2
}

abs_path() {
  local path="$1"
  if [[ "$path" == /* ]]; then
    printf '%s' "$path"
    return
  fi
  local dir
  local base
  dir=$(dirname "$path")
  base=$(basename "$path")
  printf '%s/%s' "$(cd "$dir" && pwd -P)" "$base"
}

join_args() {
  local out=""
  local arg=""
  for arg in "$@"; do
    if [[ -n "$out" ]]; then
      out="$out "
    fi
    out="$out$arg"
  done
  printf '%s' "$out"
}

set_experiment_args() {
  local exp="$1"
  EXPERIMENT_ARGS=("--target=$TARGET")

  case "$exp" in
    base)
      ;;
    mb_on)
      EXPERIMENT_ARGS+=("--enable-auto-multi-buffer=true")
      ;;
    mb_off)
      EXPERIMENT_ARGS+=("--enable-auto-multi-buffer=false")
      ;;
    mb_no_limit)
      EXPERIMENT_ARGS+=(
        "--enable-auto-multi-buffer=true"
        "--limit-auto-multi-buffer-buffer=no-limit"
        "--limit-auto-multi-buffer-of-local-buffer=no-limit"
      )
      ;;
    mb_vector_only)
      EXPERIMENT_ARGS+=(
        "--enable-auto-multi-buffer=true"
        "--limit-auto-multi-buffer-buffer=only-vector"
      )
      ;;
    mb_cube_only)
      EXPERIMENT_ARGS+=(
        "--enable-auto-multi-buffer=true"
        "--limit-auto-multi-buffer-buffer=only-cube"
      )
      ;;
    cv_workspace_off)
      EXPERIMENT_ARGS+=("--disable-auto-cv-work-space-manage=true")
      ;;
    preload_on)
      EXPERIMENT_ARGS+=("--enable-preload=true")
      ;;
    preload_off)
      EXPERIMENT_ARGS+=("--enable-preload=false")
      ;;
    autoblockify_on)
      EXPERIMENT_ARGS+=("--enable-auto-blockify-loop=true")
      ;;
    autoblockify_off)
      EXPERIMENT_ARGS+=("--enable-auto-blockify-loop=false")
      ;;
    ubuf_saving_on)
      EXPERIMENT_ARGS+=("--enable-ubuf-saving=true")
      ;;
    ubuf_saving_off)
      EXPERIMENT_ARGS+=("--enable-ubuf-saving=false")
      ;;
    code_motion_off)
      EXPERIMENT_ARGS+=("--enable-code-motion=false")
      ;;
    autobind_off)
      EXPERIMENT_ARGS+=("--enable-auto-bind-sub-block=false")
      ;;
    storage_align_off)
      EXPERIMENT_ARGS+=("--enable-hivm-auto-storage-align=false")
      ;;
    ops_reorder_off)
      EXPERIMENT_ARGS+=("--enable-ops-reorder=false")
      ;;
    tile_cube4_vec1)
      EXPERIMENT_ARGS+=("--tile-mix-cube-loop=4" "--tile-mix-vector-loop=1")
      ;;
    tile_cube1_vec4)
      EXPERIMENT_ARGS+=("--tile-mix-cube-loop=1" "--tile-mix-vector-loop=4")
      ;;
    tile_cube2_vec2)
      EXPERIMENT_ARGS+=("--tile-mix-cube-loop=2" "--tile-mix-vector-loop=2")
      ;;
    *)
      die "Unknown experiment: $exp"
      ;;
  esac
}

write_python_summarizer() {
  local file="$1"
  cat >"$file" <<'PY'
import csv
import glob
import json
import os
import re
import sys

run_root = sys.argv[1]

args_by_exp = {}
args_path = os.path.join(run_root, "experiment_args.tsv")
with open(args_path, newline="") as f:
    reader = csv.DictReader(f, delimiter="\t")
    for row in reader:
        args_by_exp[row["experiment_name"]] = row["compiler_args"]

overflow_re = re.compile(r"requires\s+([0-9]+)\s+bits\s+while\s+([0-9]+)\s+bits\s+available")

def read_tsv(path):
    if not os.path.exists(path):
        return []
    with open(path, newline="") as f:
        return list(csv.DictReader(f, delimiter="\t"))

def parse_required_capacity(text):
    match = overflow_re.search(text or "")
    if not match:
        return "", ""
    return match.group(1), match.group(2)

def safe_int(value):
    try:
        return int(value)
    except Exception:
        return 0

def memory_file_metrics(path):
    rows = []
    try:
        with open(path) as f:
            data = json.load(f)
    except Exception as exc:
        rows.append({
            "memory_info_file": path,
            "kernel": "",
            "scope": "",
            "status": "json_parse_failed",
            "error_info": str(exc),
            "peak_bits": "",
            "required_bits": "",
            "capacity_bits": "",
            "alloc_count": "",
            "tmp_alloc_count": "",
            "total_extent_bits": "",
        })
        return rows

    kernel = data.get("Header", {}).get("KernelName", "")
    for record in data.get("Record", []):
        array = record.get("memory_info_array", []) or []
        peak_bits = 0
        total_extent_bits = 0
        tmp_alloc_count = 0
        for item in array:
            extent = safe_int(item.get("extent", 0))
            total_extent_bits += extent
            if item.get("is_tmpbuf") is True:
                tmp_alloc_count += 1
            offsets = item.get("offset", [])
            if not isinstance(offsets, list):
                offsets = [offsets]
            for offset in offsets:
                peak_bits = max(peak_bits, safe_int(offset) * 8 + extent)
        error_info = record.get("error_info", "") or ""
        required_bits, capacity_bits = parse_required_capacity(error_info)
        rows.append({
            "memory_info_file": path,
            "kernel": kernel,
            "scope": record.get("scope", ""),
            "status": record.get("status", ""),
            "error_info": error_info.replace("\n", " "),
            "peak_bits": str(peak_bits) if array else "",
            "required_bits": required_bits,
            "capacity_bits": capacity_bits,
            "alloc_count": str(len(array)),
            "tmp_alloc_count": str(tmp_alloc_count),
            "total_extent_bits": str(total_extent_bits) if array else "",
        })
    return rows

summary_rows = []
metric_rows = []

for exp in sorted(args_by_exp.keys()):
    exp_dir = os.path.join(run_root, exp)
    dump_summary = os.path.join(exp_dir, "summary.tsv")
    for dump_row in read_tsv(dump_summary):
        case_dir = dump_row.get("case_dir", "")
        stderr_log = os.path.join(case_dir, "compile.stderr.log") if case_dir else ""
        stderr_text = ""
        if os.path.exists(stderr_log):
            with open(stderr_log, errors="replace") as f:
                stderr_text = f.read()

        memory_files = []
        listed = dump_row.get("memory_info_files", "")
        if listed:
            memory_files = [p for p in listed.split(",") if p]
        if not memory_files and case_dir:
            memory_files = sorted(glob.glob(os.path.join(case_dir, "memory_info*.json")))

        ub_statuses = []
        ub_peak_bits = []
        ub_required_bits = []
        ub_capacity_bits = []
        for memory_file in memory_files:
            for metric in memory_file_metrics(memory_file):
                metric_row = {
                    "experiment_name": exp,
                    "input": dump_row.get("input", ""),
                    **metric,
                }
                metric_rows.append(metric_row)
                if metric["scope"] == "ub":
                    if metric["status"]:
                        ub_statuses.append(metric["status"])
                    if metric["peak_bits"]:
                        ub_peak_bits.append(safe_int(metric["peak_bits"]))
                    if metric["required_bits"]:
                        ub_required_bits.append(safe_int(metric["required_bits"]))
                    if metric["capacity_bits"]:
                        ub_capacity_bits.append(safe_int(metric["capacity_bits"]))

        stderr_required, stderr_capacity = parse_required_capacity(stderr_text)
        if stderr_required:
            ub_required_bits.append(safe_int(stderr_required))
        if stderr_capacity:
            ub_capacity_bits.append(safe_int(stderr_capacity))

        ub_peak = max(ub_peak_bits) if ub_peak_bits else ""
        ub_required = max(ub_required_bits) if ub_required_bits else ""
        ub_capacity = max(ub_capacity_bits) if ub_capacity_bits else ""
        ub_measure = ub_required or ub_peak

        summary_rows.append({
            "experiment_name": exp,
            "input": dump_row.get("input", ""),
            "status": dump_row.get("status", ""),
            "case_dir": case_dir,
            "before_ir": dump_row.get("before_snapshot", ""),
            "after_ir": dump_row.get("after_snapshot", ""),
            "memory_info_files": ",".join(memory_files),
            "stderr_log": stderr_log,
            "ub_status": ";".join(sorted(set(ub_statuses))),
            "ub_measure_bits": str(ub_measure) if ub_measure != "" else "",
            "ub_peak_bits": str(ub_peak) if ub_peak != "" else "",
            "ub_required_bits": str(ub_required) if ub_required != "" else "",
            "ub_capacity_bits": str(ub_capacity) if ub_capacity != "" else "",
            "delta_vs_base_bits": "",
            "delta_vs_base_percent": "",
            "compiler_args": args_by_exp.get(exp, ""),
        })

base_by_input = {}
for row in summary_rows:
    if row["experiment_name"] == "base" and row["ub_measure_bits"]:
        base_by_input[row["input"]] = safe_int(row["ub_measure_bits"])

for row in summary_rows:
    base = base_by_input.get(row["input"], 0)
    measure = safe_int(row["ub_measure_bits"])
    if base and measure:
        delta = measure - base
        row["delta_vs_base_bits"] = str(delta)
        row["delta_vs_base_percent"] = f"{delta / base * 100:.2f}"

summary_fields = [
    "experiment_name",
    "input",
    "status",
    "case_dir",
    "before_ir",
    "after_ir",
    "memory_info_files",
    "stderr_log",
    "ub_status",
    "ub_measure_bits",
    "ub_peak_bits",
    "ub_required_bits",
    "ub_capacity_bits",
    "delta_vs_base_bits",
    "delta_vs_base_percent",
    "compiler_args",
]

metric_fields = [
    "experiment_name",
    "input",
    "memory_info_file",
    "kernel",
    "scope",
    "status",
    "peak_bits",
    "required_bits",
    "capacity_bits",
    "alloc_count",
    "tmp_alloc_count",
    "total_extent_bits",
    "error_info",
]

with open(os.path.join(run_root, "summary.tsv"), "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=summary_fields, delimiter="\t")
    writer.writeheader()
    writer.writerows(summary_rows)

with open(os.path.join(run_root, "memory_metrics.tsv"), "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=metric_fields, delimiter="\t")
    writer.writeheader()
    writer.writerows(metric_rows)
PY
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--input)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --input"
      INPUTS+=("$1")
      ;;
    -d|--data-dir)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --data-dir"
      DATA_DIR="$1"
      ;;
    -o|--output-root)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --output-root"
      OUTPUT_ROOT="$1"
      ;;
    -c|--compiler)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --compiler"
      COMPILER="$1"
      ;;
    --dump-script)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --dump-script"
      DUMP_SCRIPT="$1"
      ;;
    --target)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --target"
      TARGET="$1"
      ;;
    --run-name)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --run-name"
      RUN_NAME="$1"
      ;;
    --keep-full-tree)
      DUMP_FULL_IR_TREE=1
      KEEP_FULL_TREE=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Unknown option: $1"
      ;;
  esac
  shift
done

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd -P)
cd "$REPO_ROOT" || die "Failed to enter repo root: $REPO_ROOT"

if [[ ${#INPUTS[@]} -eq 0 && -n "$DATA_DIR" ]]; then
  [[ -d "$DATA_DIR" ]] || die "Data directory not found: $DATA_DIR"
  while IFS= read -r file; do
    INPUTS+=("$file")
  done < <(find "$DATA_DIR" -maxdepth 1 -type f | sort)
fi

if [[ ${#INPUTS[@]} -eq 0 ]]; then
  INPUTS=("data/attn_fwd.ttadapter")
fi

[[ -f "$DUMP_SCRIPT" ]] || die "Dump script not found: $DUMP_SCRIPT"
[[ -x "$COMPILER" ]] || die "Compiler not executable: $COMPILER"

if [[ -z "$RUN_NAME" ]]; then
  RUN_NAME=$(date +%Y%m%d_%H%M%S)
fi

RUN_ROOT="$OUTPUT_ROOT/$RUN_NAME"
mkdir -p "$RUN_ROOT" || die "Failed to create output root: $RUN_ROOT"
RUN_ROOT=$(abs_path "$RUN_ROOT")

ARGS_TSV="$RUN_ROOT/experiment_args.tsv"
printf 'experiment_name\tcompiler_args\n' > "$ARGS_TSV"

echo "[INFO] Run root: $RUN_ROOT"
echo "[INFO] Inputs: ${INPUTS[*]}"
echo "[INFO] Target: $TARGET"

for exp in "${EXPERIMENTS[@]}"; do
  set_experiment_args "$exp"
  exp_output="$RUN_ROOT/$exp"
  args_text=$(join_args "${EXPERIMENT_ARGS[@]}")
  printf '%s\t%s\n' "$exp" "$args_text" >> "$ARGS_TSV"

  dump_cmd=(
    bash "$DUMP_SCRIPT"
    --output-root "$exp_output"
    --compiler "$COMPILER"
    --allow-failures
  )

  if [[ "$DUMP_FULL_IR_TREE" == "1" ]]; then
    dump_cmd+=(--dump-full-ir-tree)
  fi

  if [[ "$DUMP_FULL_IR_TREE" == "1" && "$KEEP_FULL_TREE" != "1" ]]; then
    dump_cmd+=(--drop-full-tree)
  fi

  for input in "${INPUTS[@]}"; do
    dump_cmd+=(--input "$input")
  done

  dump_cmd+=(-- "${EXPERIMENT_ARGS[@]}")

  echo "[INFO] Experiment: $exp"
  echo "[INFO] Args: $args_text"
  "${dump_cmd[@]}"
done

summarizer="$RUN_ROOT/.summarize_ub_peak_ablation.py"
write_python_summarizer "$summarizer"
python3 "$summarizer" "$RUN_ROOT"

echo "[INFO] Summary: $RUN_ROOT/summary.tsv"
echo "[INFO] Metrics: $RUN_ROOT/memory_metrics.tsv"
