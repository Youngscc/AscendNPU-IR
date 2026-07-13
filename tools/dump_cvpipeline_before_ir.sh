#!/usr/bin/env bash

set -u

DATA_DIR="data"
OUTPUT_ROOT=""
RUN_ID="compiler_default"
COMPILER="build/bin/bishengir-compile"
STOP_ON_FAIL=0
ALLOW_FAILURES=0
MAX_FILES=0
KEEP_FULL_TREE=0
INCLUDE_HIDDEN=0
PRETTY_FORMAT=1
CASE_TIMEOUT=300
INPUTS=()
EXTRA_COMPILER_ARGS=()

usage() {
  cat <<'EOF'
Usage: bash tools/dump_cvpipeline_before_ir.sh [options] [-- extra compiler args...]

Batch compile kernels and extract the IR immediately before createCVPipeliningPass.

The script uses the existing MLIR pass IR tree:
  --mlir-print-ir-after-all
  --mlir-print-ir-module-scope
  --mlir-print-ir-tree-dir=<case>/ir_tree

For every *_cv-pipelining.mlir file in the tree, it copies the previous numbered
IR dump in the same function directory as before_cvpipelining_<func>.mlir. This
is the after-IR of the pass that ran immediately before cv-pipelining, i.e. the
best available "before createCVPipeliningPass" snapshot without adding a new C++
dump pass.

Options:
  -d, --data-dir DIR
        Directory to scan. Defaults to ./data.
  -o, --output-root DIR
        Unified output directory. Defaults to ./Output.
  --run-id NAME
        Stage run name under each adapter. Defaults to compiler_default.
  -c, --compiler FILE
        bishengir-compile binary. Defaults to ./build/bin/bishengir-compile.
  -i, --input FILE
        Add one input file. Can be repeated. If omitted, scans --data-dir.
  --max-files N
        Only process the first N inputs after sorting. 0 means no limit.
  --stop-on-fail
        Stop at the first failing compile.
  --allow-failures
        Always exit 0 after processing available inputs. Useful when dumps are
        produced but later compilation fails because hivmc/backend is missing.
  --keep-full-tree
        Keep the full per-pass ir_tree/ under each case. By default it is
        removed after extracting cv-pipelining before/after snapshots.
  --include-hidden
        Include hidden files while scanning --data-dir.
  --raw
        Keep snapshots exactly as printed. By default snapshots are lightly
        reformatted for reading and the original is kept as *.raw.mlir.
  --case-timeout SEC
        Kill one compile case after SEC seconds. Defaults to 300.
        Use 0 to disable the timeout.
  -h, --help
        Show this help.

Extra compiler args:
  Arguments after -- are appended to the default bishengir-compile command.

Outputs per case:
  before_cvpipelining_<func>_<idx>.mlir
  after_cvpipelining_<func>_<idx>.mlir
  cvpipeline_dumps.tsv
  compile.stdout.log
  compile.stderr.log
  output.o
  ir_tree/                         only with --keep-full-tree

Top-level output:
  summary.tsv
EOF
}

die() {
  echo "[ERROR] $*" >&2
  exit 2
}

slugify() {
  local name="$1"
  name=${name#./}
  printf '%s' "$name" | sed -E 's#[^A-Za-z0-9._-]+#_#g; s#_+#_#g'
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

format_ir_in_place() {
  local file="$1"
  if [[ ! -f "$file" || "$PRETTY_FORMAT" != "1" ]]; then
    return
  fi

  local raw_file="${file%.mlir}.raw.mlir"
  cp "$file" "$raw_file"

  perl -0pi -e '
    s/\r\n/\n/g;

    # Make top-level module attributes scannable. This is whitespace-only MLIR.
    s/^(module attributes) \{/$1 {\n  /mg;
    s/#hacc\.target_device_spec</#hacc.target_device_spec<\n    /g;
    s/(#dlti\.dl_entry<[^>]+>),\s*/$1,\n    /g;
    s/>>>,\s*(hacc\.|hivm\.)/>>>,\n  $1/g;
    s/,\s*(hacc\.hivmc_|hivm\.module_)/,\n  $1/g;

    # Function signatures and attributes are often the longest single lines.
    s/^(\s*func\.func @[^(]+\()/$1\n      /mg;
    s/,\s*(%[A-Za-z_][A-Za-z0-9_]*)/,\n      $1/g;
    s/\) attributes \{/\)\n    attributes \{/g;
    s/(\) -> [^{\n]+) attributes \{/$1\n    attributes \{/g;
    s/,\s*(hacc\.|hivm\.|func_)/,\n      $1/g;
    s/\(\n\s+\)/()/g;
    s/\n\s{6}(hacc\.hivmc_|hivm\.module_)/\n  $1/g;

    # HIVM ops are easier to compare when operands and results sit apart.
    s/^(\s*)(hivm\.hir\.[^\s]+)\s+ins\(/$1$2\n$1  ins(/mg;
    s/^(\s*)(%[A-Za-z0-9_]+ = hivm\.hir\.[^\s]+\(.*?\))\s+:/$1$2\n$1  :/mg;
    s/^(\s*)ins\((.*?)\)\s+outs\(/$1ins($2)\n$1outs(/mg;
    s/^(\s*)outs\((.*?)\)\s+(core_type|loc)\s*=/$1outs($2)\n$1$3 =/mg;
    s/\)\s+loc\(/\)\n      loc\(/g;
    s/,\s*(%[A-Za-z_][A-Za-z0-9_]*)/,\n        $1/g;
  ' "$file"
}

pass_index_from_file() {
  local file="$1"
  local base
  base=$(basename "$file")
  if [[ "$base" =~ ^([0-9]+)_ ]]; then
    printf '%s' "${BASH_REMATCH[1]}"
  else
    printf '%s' "-1"
  fi
}

find_previous_pass_dump() {
  local cv_file="$1"
  local dir
  local cv_idx
  local best_idx=-1
  local best_file=""
  local f
  local idx
  dir=$(dirname "$cv_file")
  cv_idx=$(pass_index_from_file "$cv_file")
  if [[ "$cv_idx" -lt 0 ]]; then
    return
  fi
  for f in "$dir"/*.mlir; do
    [[ -f "$f" ]] || continue
    idx=$(pass_index_from_file "$f")
    if [[ "$idx" -ge 0 && "$idx" -lt "$cv_idx" && "$idx" -gt "$best_idx" ]]; then
      best_idx="$idx"
      best_file="$f"
    fi
  done
  if [[ -n "$best_file" ]]; then
    printf '%s' "$best_file"
  fi
}

extract_cvpipeline_dumps() {
  local tree_dir="$1"
  local case_dir="$2"
  local tsv="$case_dir/cvpipeline_dumps.tsv"
  local cv_file=""
  local before_src=""
  local func_label=""
  local safe_label=""
  local cv_idx=""
  local before_dst=""
  local after_dst=""
  local count=0

  printf 'func\tpass_index\tbefore_snapshot\tafter_snapshot\tbefore_source\tafter_source\n' >"$tsv"
  if [[ ! -d "$tree_dir" ]]; then
    printf '%s' 0
    return
  fi

  while IFS= read -r cv_file; do
    [[ -n "$cv_file" ]] || continue
    before_src=$(find_previous_pass_dump "$cv_file")
    if [[ -z "$before_src" || ! -f "$before_src" ]]; then
      continue
    fi

    func_label=$(basename "$(dirname "$cv_file")")
    safe_label=$(slugify "$func_label")
    cv_idx=$(pass_index_from_file "$cv_file")
    before_dst="$case_dir/before_cvpipelining_${safe_label}_${cv_idx}.mlir"
    after_dst="$case_dir/after_cvpipelining_${safe_label}_${cv_idx}.mlir"

    cp "$before_src" "$before_dst"
    cp "$cv_file" "$after_dst"
    format_ir_in_place "$before_dst"
    format_ir_in_place "$after_dst"

    printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$func_label" "$cv_idx" "$before_dst" "$after_dst" "$before_src" "$cv_file" >>"$tsv"
    count=$((count + 1))
  done < <(find "$tree_dir" -type f -name '*_cv-pipelining.mlir' | sort)

  printf '%s' "$count"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
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
    --run-id)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --run-id"
      RUN_ID="$1"
      ;;
    -c|--compiler)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --compiler"
      COMPILER="$1"
      ;;
    -i|--input)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --input"
      INPUTS+=("$1")
      ;;
    --max-files)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --max-files"
      MAX_FILES="$1"
      ;;
    --stop-on-fail)
      STOP_ON_FAIL=1
      ;;
    --allow-failures)
      ALLOW_FAILURES=1
      ;;
    --keep-full-tree)
      KEEP_FULL_TREE=1
      ;;
    --include-hidden)
      INCLUDE_HIDDEN=1
      ;;
    --raw)
      PRETTY_FORMAT=0
      ;;
    --case-timeout)
      shift
      [[ $# -gt 0 ]] || die "Missing value for --case-timeout"
      CASE_TIMEOUT="$1"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      EXTRA_COMPILER_ARGS=("$@")
      break
      ;;
    -*)
      die "Unknown argument: $1"
      ;;
    *)
      INPUTS+=("$1")
      ;;
  esac
  shift
done

[[ "$MAX_FILES" =~ ^[0-9]+$ ]] || die "--max-files must be a non-negative integer"
[[ "$CASE_TIMEOUT" =~ ^[0-9]+$ ]] || die "--case-timeout must be a non-negative integer"

if [[ ! -x "$COMPILER" ]]; then
  die "Compiler not found or not executable: $COMPILER"
fi
COMPILER=$(abs_path "$COMPILER")

if [[ ${#INPUTS[@]} -eq 0 ]]; then
  [[ -d "$DATA_DIR" ]] || die "Data directory not found: $DATA_DIR"
  find_args=("$DATA_DIR" -maxdepth 1 -type f -name '*.ttadapter')
  if [[ "$INCLUDE_HIDDEN" != "1" ]]; then
    find_args+=(! -name '.*')
  fi
  while IFS= read -r input; do
    INPUTS+=("$input")
  done < <(find "${find_args[@]}" | sort)
fi

if [[ "$MAX_FILES" -gt 0 && ${#INPUTS[@]} -gt "$MAX_FILES" ]]; then
  INPUTS=("${INPUTS[@]:0:$MAX_FILES}")
fi

if [[ ${#INPUTS[@]} -eq 0 ]]; then
  die "No input files found"
fi

if [[ -z "$OUTPUT_ROOT" ]]; then
  OUTPUT_ROOT="Output"
fi
mkdir -p "$OUTPUT_ROOT"
OUTPUT_ROOT=$(abs_path "$OUTPUT_ROOT")
ADAPTER_ROOT="$OUTPUT_ROOT/adapters"
INDEX_ROOT="$OUTPUT_ROOT/index/cvpipeline_${RUN_ID}"
mkdir -p "$ADAPTER_ROOT" "$INDEX_ROOT"

SUMMARY="$INDEX_ROOT/summary.tsv"
printf 'input\tstatus\tcvpipeline_count\tcase_dir\tbefore_snapshots\tafter_snapshots\tstdout\tstderr\n' >"$SUMMARY"

success_count=0
fail_count=0

for input in "${INPUTS[@]}"; do
  if [[ ! -f "$input" ]]; then
    echo "[WARN] Skip missing input: $input" >&2
    continue
  fi

  input_abs=$(abs_path "$input")
  adapter=$(basename "$input")
  case_dir="$ADAPTER_ROOT/$adapter/01_cvpipeline/$RUN_ID"
  tree_dir="$case_dir/ir_tree"
  stdout_log="$case_dir/compile.stdout.log"
  stderr_log="$case_dir/compile.stderr.log"
  output_file="$case_dir/output.o"
  mkdir -p "$tree_dir"

  echo "[INFO] Running: $input"

  compile_cmd=(
    "$COMPILER" "$input_abs"
    -o "$output_file"
    --enable-hfusion-compile
    --enable-hivm-compile
    --enable-triton-kernel-compile
    --enable-memory-display
    --mlir-disable-threading
    --mlir-print-ir-after-all
    --mlir-print-ir-module-scope
    "--mlir-print-ir-tree-dir=$tree_dir"
    "${EXTRA_COMPILER_ARGS[@]}"
  )

  set +e
  timeout_marker="$case_dir/.compile_timeout"
  rm -f "$timeout_marker"
  if [[ "$CASE_TIMEOUT" -gt 0 ]]; then
    (
      cd "$case_dir" &&
        "${compile_cmd[@]}"
    ) >"$stdout_log" 2>"$stderr_log" &
    compile_pid=$!
    (
      sleep "$CASE_TIMEOUT"
      if kill -0 "$compile_pid" 2>/dev/null; then
        {
          echo "[TIMEOUT] Compile exceeded ${CASE_TIMEOUT}s."
          echo "[TIMEOUT] Killing process tree rooted at pid ${compile_pid}."
        } >>"$stderr_log"
        : >"$timeout_marker"
        pkill -TERM -P "$compile_pid" 2>/dev/null
        kill -TERM "$compile_pid" 2>/dev/null
        sleep 5
        pkill -KILL -P "$compile_pid" 2>/dev/null
        kill -KILL "$compile_pid" 2>/dev/null
      fi
    ) &
    watchdog_pid=$!
    wait "$compile_pid"
    status=$?
    kill "$watchdog_pid" 2>/dev/null
    wait "$watchdog_pid" 2>/dev/null
    if [[ -f "$timeout_marker" ]]; then
      status=124
    fi
  else
    (
      cd "$case_dir" &&
        "${compile_cmd[@]}"
    ) >"$stdout_log" 2>"$stderr_log"
    status=$?
  fi
  rm -f "$timeout_marker"
  set -e

  cv_count=$(extract_cvpipeline_dumps "$tree_dir" "$case_dir")

  before_snapshots=""
  after_snapshots=""
  if [[ -f "$case_dir/cvpipeline_dumps.tsv" ]]; then
    before_snapshots=$(awk -F '\t' 'NR > 1 { if (out != "") out = out ","; out = out $3 } END { print out }' "$case_dir/cvpipeline_dumps.tsv")
    after_snapshots=$(awk -F '\t' 'NR > 1 { if (out != "") out = out ","; out = out $4 } END { print out }' "$case_dir/cvpipeline_dumps.tsv")
  fi

  if [[ "$KEEP_FULL_TREE" != "1" ]]; then
    rm -rf "$tree_dir"
  fi

  if [[ "$status" -eq 0 ]]; then
    run_status="ok"
    success_count=$((success_count + 1))
  else
    run_status="fail:$status"
    fail_count=$((fail_count + 1))
  fi

  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$input_abs" "$run_status" "$cv_count" "$case_dir" \
    "$before_snapshots" "$after_snapshots" "$stdout_log" "$stderr_log" >>"$SUMMARY"

  if [[ "$cv_count" -eq 0 ]]; then
    echo "[WARN] No cv-pipelining dump found for $input" >&2
  else
    echo "[INFO] Extracted $cv_count cv-pipelining before snapshot(s)"
  fi

  if [[ "$status" -ne 0 && "$STOP_ON_FAIL" == "1" ]]; then
    echo "[ERROR] Stop on failing case: $input (status=$status)" >&2
    break
  fi
done

echo "[INFO] Summary: $SUMMARY"
echo "[INFO] Success: $success_count, failed: $fail_count"

if [[ "$fail_count" -gt 0 && "$ALLOW_FAILURES" != "1" ]]; then
  exit 1
fi
exit 0
