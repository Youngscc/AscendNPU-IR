#!/usr/bin/env bash

set -u

INPUTS=()
DATA_DIR=""
OUTPUT_ROOT="Output/experiments/ub_peak_pass_ablation"
COMPILER="build/bin/bishengir-compile"
DUMP_SCRIPT="tools/dump_planmemory_ir.sh"
TARGET="Ascend910_9382"
DUMP_FULL_IR_TREE=0
KEEP_FULL_TREE=0
RUN_NAME=""
CASE_TIMEOUT=300

EXPERIMENTS=(
  base

  # Active: coarse and split CV workspace experiments. The split options are
  # meant to identify which part of --disable-auto-cv-work-space-manage reduces
  # local UB pressure.
  cv_workspace_off
  cv_no_mark_multibuffer
  cv_no_pipelining
  cv_no_global_workspace_plan
  cv_balance_off
  legacy_mix_cv_on

  # Active: multi-buffer strategies. Current conservative base did not change
  # UB peak when disabled, but vector/no-limit variants increased UB pressure.
  mb_off
  mb_no_limit
  mb_vector_only

  # Active: mixed or theoretically UB-sensitive transforms.
  inline_fixpipe_off
  preload_off
  autoblockify_off
  code_motion_off
  ops_reorder_off
  size_align_for_cast_off
  storage_align_off
  align_alloc_size_off
  enable_stride_align_off
  infer_hivm_data_layout_off
  nd2nz_on_vector_off
  assume_alive_loops_on
  block_dim2
  block_dim4
  hfusion_max_fusion1
  hfusion_max_fusion2
  hfusion_buffer_tuning1
  hfusion_buffer_tuning2
  tile_cube4_vec1
  tile_cube1_vec4
  tile_cube2_vec2

  # Inactive: no local UB peak change in the current experiments and mainly
  # non-UB/global/sync oriented. Kept here for manual re-enable.
  # symbol_analysis_off
  # deterministic_off
  # global_workspace_reuse_off
  # workspace_mb1
  # workspace_mb3
  # workspace_mb4
  # graph_sync_solver_off
  # cross_core_gss_off
  # auto_inject_sync_off
  # auto_inject_block_sync_off
  # unit_flag_sync_on

  # Inactive: these passes improved or protected UB peak in the current data;
  # turning them off is useful as a sanity check, but not in the default search
  # for pressure-increasing passes.
  # ubuf_saving_off
  # autobind_off
  # split_mix_kernel_off

  # Inactive: no observed UB peak change and weak direct UB-pressure hypothesis
  # for the current local-UB-focused sweep. Kept for manual re-enable.
  # hfusion_count_buffer_dma_off

  # Inactive: invalid or too fragile as standalone experiments. Disabling
  # InsertLoadStoreForMixCV alone leaves later mix-CV passes with a half-formed
  # IR and can crash SplitMixKernel. Disabling the workspace chain while keeping
  # InsertLoadStoreForMixCV can produce invalid hivm.hir.load ops because the
  # workspace rewrite is required to make the inserted load/store legal.
  # cv_no_insert_load_store
  # cv_no_insert_workspace
  # cv_no_bind_workspace_arg
  # cv_no_workspace_chain
)

usage() {
  cat <<'EOF'
Usage: bash tools/run_ub_peak_pass_ablation.sh [options]

Run the first-stage UB peak pass-group ablation matrix around local PlanMemory.
Each experiment calls tools/dump_planmemory_ir.sh and keeps PlanMemory before
IR, memory_info*.json, stdout, and stderr by default.

Options:
  -i, --input FILE
        Add one input file. Can be repeated. Defaults to data/attn_fwd.ttadapter.
  -d, --data-dir DIR
        Scan one directory for input files when --input is omitted.
  -o, --output-root DIR
        Root output directory. Defaults to Output/experiments/ub_peak_pass_ablation.
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
  --case-timeout SEC
        Kill one compile case after SEC seconds. Defaults to 300.
        Use 0 to disable the timeout.
  -h, --help
        Show this help.

Outputs:
  <output-root>/<timestamp-or-run-name>/
    experiment_args.tsv
    summary.tsv
    memory_metrics.tsv
    ub_results.tsv
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
  EXPERIMENT_ARGS=(
    "--target=$TARGET"
    "--enable-symbol-analysis=true"
    "--enable-deterministic-computing=true"
    "--enable-hfusion-count-buffer-dma-opt=true"
    "--enable-auto-multi-buffer=true"
    "--limit-auto-multi-buffer-only-for-local-buffer=true"
    "--limit-auto-multi-buffer-of-local-buffer=no-l0c"
    "--limit-auto-multi-buffer-buffer=only-cube"
    "--set-workspace-multibuffer=2"
    "--disable-size-align-for-cast=false"
    "--enable-preload=true"
    "--enable-auto-blockify-loop=true"
    "--enable-ubuf-saving=true"
    "--enable-code-motion=true"
    "--enable-auto-bind-sub-block=true"
    "--enable-hivm-auto-storage-align=true"
    "--enable-hivm-auto-cv-balance=true"
    "--enable-hivm-global-workspace-reuse=true"
    "--enable-hivm-nd2nz-on-vector=true"
    "--enable-hivm-graph-sync-solver=true"
    "--enable-hivm-cross-core-gss=true"
    "--disable-hivm-auto-inject-sync=false"
    "--disable-auto-inject-block-sync=false"
    "--disable-inline-fixpipe=false"
    "--disable-split-mix-kernel=false"
    "--disable-align-alloc-size=false"
    "--disable-enable-stride-align=false"
    "--disable-infer-hivm-data-layout=false"
    "--enable-ops-reorder=true"
    "--disable-auto-cv-work-space-manage=false"
  )

  case "$exp" in
    base)
      ;;
    symbol_analysis_off)
      EXPERIMENT_ARGS+=("--enable-symbol-analysis=false")
      ;;
    deterministic_off)
      EXPERIMENT_ARGS+=("--enable-deterministic-computing=false")
      ;;
    hfusion_count_buffer_dma_off)
      EXPERIMENT_ARGS+=("--enable-hfusion-count-buffer-dma-opt=false")
      ;;
    size_align_for_cast_off)
      EXPERIMENT_ARGS+=("--disable-size-align-for-cast=true")
      ;;
    mb_off)
      EXPERIMENT_ARGS+=("--enable-auto-multi-buffer=false")
      ;;
    mb_global_and_local)
      EXPERIMENT_ARGS+=("--limit-auto-multi-buffer-only-for-local-buffer=false")
      ;;
    mb_no_limit)
      EXPERIMENT_ARGS+=(
        "--limit-auto-multi-buffer-buffer=no-limit"
        "--limit-auto-multi-buffer-of-local-buffer=no-limit"
      )
      ;;
    mb_vector_only)
      EXPERIMENT_ARGS+=(
        "--limit-auto-multi-buffer-buffer=only-vector"
      )
      ;;
    workspace_mb1)
      EXPERIMENT_ARGS+=("--set-workspace-multibuffer=1")
      ;;
    workspace_mb3)
      EXPERIMENT_ARGS+=("--set-workspace-multibuffer=3")
      ;;
    workspace_mb4)
      EXPERIMENT_ARGS+=("--set-workspace-multibuffer=4")
      ;;
    cv_workspace_off)
      EXPERIMENT_ARGS+=("--disable-auto-cv-work-space-manage=true")
      ;;
    cv_no_insert_load_store)
      EXPERIMENT_ARGS+=("--disable-auto-cv-insert-load-store=true")
      ;;
    cv_no_insert_workspace)
      EXPERIMENT_ARGS+=("--disable-auto-cv-insert-workspace=true")
      ;;
    cv_no_bind_workspace_arg)
      EXPERIMENT_ARGS+=("--disable-auto-cv-bind-workspace-arg=true")
      ;;
    cv_no_workspace_chain)
      EXPERIMENT_ARGS+=(
        "--disable-auto-cv-insert-workspace=true"
        "--disable-auto-cv-bind-workspace-arg=true"
      )
      ;;
    cv_no_mark_multibuffer)
      EXPERIMENT_ARGS+=("--disable-auto-cv-mark-multi-buffer=true")
      ;;
    cv_no_pipelining)
      EXPERIMENT_ARGS+=("--disable-cv-pipelining=true")
      ;;
    cv_no_global_workspace_plan)
      EXPERIMENT_ARGS+=("--disable-auto-cv-global-workspace-plan=true")
      ;;
    cv_balance_off)
      EXPERIMENT_ARGS+=("--enable-hivm-auto-cv-balance=false")
      ;;
    global_workspace_reuse_off)
      EXPERIMENT_ARGS+=("--enable-hivm-global-workspace-reuse=false")
      ;;
    legacy_mix_cv_on)
      EXPERIMENT_ARGS+=("--enable-legacy-insert-load-store-for-mix-cv=true")
      ;;
    inline_fixpipe_off)
      EXPERIMENT_ARGS+=("--disable-inline-fixpipe=true")
      ;;
    split_mix_kernel_off)
      EXPERIMENT_ARGS+=("--disable-split-mix-kernel=true")
      ;;
    preload_off)
      EXPERIMENT_ARGS+=("--enable-preload=false")
      ;;
    autoblockify_off)
      EXPERIMENT_ARGS+=("--enable-auto-blockify-loop=false")
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
    align_alloc_size_off)
      EXPERIMENT_ARGS+=("--disable-align-alloc-size=true")
      ;;
    enable_stride_align_off)
      EXPERIMENT_ARGS+=("--disable-enable-stride-align=true")
      ;;
    infer_hivm_data_layout_off)
      EXPERIMENT_ARGS+=("--disable-infer-hivm-data-layout=true")
      ;;
    nd2nz_on_vector_off)
      EXPERIMENT_ARGS+=("--enable-hivm-nd2nz-on-vector=false")
      ;;
    ops_reorder_off)
      EXPERIMENT_ARGS+=("--enable-ops-reorder=false")
      ;;
    graph_sync_solver_off)
      EXPERIMENT_ARGS+=("--enable-hivm-graph-sync-solver=false")
      ;;
    cross_core_gss_off)
      EXPERIMENT_ARGS+=("--enable-hivm-cross-core-gss=false")
      ;;
    auto_inject_sync_off)
      EXPERIMENT_ARGS+=("--disable-hivm-auto-inject-sync=true")
      ;;
    auto_inject_block_sync_off)
      EXPERIMENT_ARGS+=("--disable-auto-inject-block-sync=true")
      ;;
    unit_flag_sync_on)
      EXPERIMENT_ARGS+=("--enable-hivm-unit-flag-sync=true")
      ;;
    assume_alive_loops_on)
      EXPERIMENT_ARGS+=("--enable-hivm-assume-alive-loops=true")
      ;;
    block_dim2)
      EXPERIMENT_ARGS+=("--block-dim=2")
      ;;
    block_dim4)
      EXPERIMENT_ARGS+=("--block-dim=4")
      ;;
    hfusion_max_fusion1)
      EXPERIMENT_ARGS+=("--hfusion-max-horizontal-fusion-size=1")
      ;;
    hfusion_max_fusion2)
      EXPERIMENT_ARGS+=("--hfusion-max-horizontal-fusion-size=2")
      ;;
    hfusion_buffer_tuning1)
      EXPERIMENT_ARGS+=("--hfusion-max-buffer-count-tuning=1")
      ;;
    hfusion_buffer_tuning2)
      EXPERIMENT_ARGS+=("--hfusion-max-buffer-count-tuning=2")
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
target_re = re.compile(r"--target=([^ ]+)")

KNOWN_UB_CAPACITY_BITS = {
    "Ascend910_9382": 1572864,
}

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

def target_from_args(args):
    match = target_re.search(args or "")
    return match.group(1) if match else ""

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
    dump_summary = os.path.join(
        exp_dir, "index", "planmemory_compiler_default", "summary.tsv")
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
        ub_alloc_count = 0
        ub_tmp_alloc_count = 0
        ub_total_extent_bits = 0
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
                    ub_alloc_count += safe_int(metric["alloc_count"])
                    ub_tmp_alloc_count += safe_int(metric["tmp_alloc_count"])
                    ub_total_extent_bits += safe_int(metric["total_extent_bits"])

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
            "ub_alloc_count": str(ub_alloc_count) if ub_alloc_count else "",
            "ub_tmp_alloc_count": str(ub_tmp_alloc_count) if ub_tmp_alloc_count else "",
            "ub_total_extent_bits": str(ub_total_extent_bits) if ub_total_extent_bits else "",
            "compiler_args": args_by_exp.get(exp, ""),
        })

known_capacity = max(
    [safe_int(row["ub_capacity_bits"]) for row in summary_rows if row["ub_capacity_bits"]],
    default=0,
)
if not known_capacity:
    for args in args_by_exp.values():
        target = target_from_args(args)
        if target in KNOWN_UB_CAPACITY_BITS:
            known_capacity = KNOWN_UB_CAPACITY_BITS[target]
            break

if known_capacity:
    for row in summary_rows:
        if row["ub_measure_bits"] and not row["ub_capacity_bits"]:
            row["ub_capacity_bits"] = str(known_capacity)

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
    "ub_alloc_count",
    "ub_tmp_alloc_count",
    "ub_total_extent_bits",
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

ub_result_fields = [
    "experiment_name",
    "input",
    "result",
    "measure_bits",
    "capacity_bits",
    "delta_vs_base_bits",
    "delta_vs_base_percent",
    "alloc_count",
    "tmp_alloc_count",
    "total_extent_bits",
]

with open(os.path.join(run_root, "summary.tsv"), "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=summary_fields, delimiter="\t")
    writer.writeheader()
    writer.writerows(summary_rows)

with open(os.path.join(run_root, "memory_metrics.tsv"), "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=metric_fields, delimiter="\t")
    writer.writeheader()
    writer.writerows(metric_rows)

with open(os.path.join(run_root, "ub_results.tsv"), "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=ub_result_fields, delimiter="\t")
    writer.writeheader()
    for row in summary_rows:
        writer.writerow({
            "experiment_name": row.get("experiment_name", ""),
            "input": row.get("input", ""),
            "result": row.get("ub_status", ""),
            "measure_bits": row.get("ub_measure_bits", ""),
            "capacity_bits": row.get("ub_capacity_bits", ""),
            "delta_vs_base_bits": row.get("delta_vs_base_bits", ""),
            "delta_vs_base_percent": row.get("delta_vs_base_percent", ""),
            "alloc_count": row.get("ub_alloc_count", ""),
            "tmp_alloc_count": row.get("ub_tmp_alloc_count", ""),
            "total_extent_bits": row.get("ub_total_extent_bits", ""),
        })
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
    --case-timeout)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --case-timeout"
      [[ "$1" =~ ^[0-9]+$ ]] || die "--case-timeout must be a non-negative integer"
      CASE_TIMEOUT="$1"
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
    base=$(basename "$file")
    if [[ "$base" == .* ]]; then
      continue
    fi
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
if [[ "$CASE_TIMEOUT" -gt 0 ]]; then
  echo "[INFO] Per-case timeout: ${CASE_TIMEOUT}s"
else
  echo "[INFO] Per-case timeout: disabled"
fi

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
    --case-timeout "$CASE_TIMEOUT"
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
echo "[INFO] UB results: $RUN_ROOT/ub_results.tsv"
