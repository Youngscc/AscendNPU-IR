#!/usr/bin/env bash

set -u

DATA_DIR="data"
OUTPUT_ROOT=""
RUN_ID="compiler_default"
COMPILER="build/bin/bishengir-compile"
STOP_ON_FAIL=0
ALLOW_FAILURES=0
MAX_FILES=0
DUMP_FULL_IR_TREE=0
KEEP_FULL_TREE=1
INCLUDE_HIDDEN=0
PRETTY_FORMAT=1
CASE_TIMEOUT=300
INPUTS=()

usage() {
  cat <<'EOF'
Usage: bash tools/dump_planmemory_ir.sh [options] [-- extra compiler args...]

Batch compile IR files and collect local PlanMemory before/after snapshots.

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
        Always exit 0 after processing available inputs. Useful when the
        pipeline reaches PlanMemory but later fails because hivmc is missing.
  --dump-full-ir-tree
        Enable --mlir-print-ir-after-all and write the full per-pass IR tree
        under each case's ir_tree/ directory. Disabled by default.
  --drop-full-tree
        Delete ir_tree/ after extracting PlanMemory snapshots. This also enables
        --dump-full-ir-tree, because the tree is needed as an extraction source.
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
  before_local_plan_memory.mlir
  after_local_plan_memory.mlir
  after_local_plan_memory_<entry_func>.mlir
  memory_info*.json
  compile.stdout.log
  compile.stderr.log
  output.o
  ir_tree/                         only when --dump-full-ir-tree is used and
                                   --drop-full-tree is not used

The "before" snapshot is produced by BISHENGIR_DUMP_BEFORE_PLAN_MEMORY, which
is wired immediately before the local PlanMemory pass. The "after" snapshot is
selected from the full IR tree when --dump-full-ir-tree or --drop-full-tree is
used. If the before IR has multiple entry functions, such as mix_aic and mix_aiv,
one after_local_plan_memory_<entry_func>.mlir is also written for each entry.
All PlanMemory after dumps are also copied for manual inspection.
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
  if [[ ! -f "$file" ]]; then
    return
  fi
  if [[ "$PRETTY_FORMAT" != "1" ]]; then
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

    # Long operand lists such as ins(%a, %b, %c : ...) become one value per line.
    s/,\s*(%[A-Za-z_][A-Za-z0-9_]*)/,\n        $1/g;
  ' "$file"
}

copy_plan_memory_after_dumps() {
  local tree_dir="$1"
  local before_file="$2"
  local dst="$3"
  local all_dir="$4"
  local src=""
  local entry_func=""
  local entry_dir=""
  local entry_src=""
  local entry_funcs=()
  local selected_sources=()
  local selected_labels=()

  if [[ ! -d "$tree_dir" ]]; then
    return
  fi

  mkdir -p "$all_dir"
  while IFS= read -r src; do
    local rel
    rel=${src#"$tree_dir"/}
    rel=$(slugify "$rel")
    cp "$src" "$all_dir/$rel"
    format_ir_in_place "$all_dir/$rel"
  done < <(find "$tree_dir" -type f -name '*_hivm-plan-memory.mlir' | sort)

  src=""
  if [[ -f "$before_file" ]]; then
    while IFS= read -r entry_func; do
      [[ -n "$entry_func" ]] || continue
      entry_funcs+=("$entry_func")
    done < <(
      awk '
        match($0, /func\.func @[^ (]+/) {
          currentFunc = substr($0, RSTART + length("func.func @"), RLENGTH - length("func.func @"))
        }
        /hacc\.entry/ && currentFunc != "" {
          print currentFunc
          currentFunc = ""
        }
      ' "$before_file"
    )
  fi

  if [[ ${#entry_funcs[@]} -gt 0 ]]; then
    for entry_func in "${entry_funcs[@]}"; do
      entry_dir="func_func_${entry_func}"
      entry_src=$(find "$tree_dir" -type f -path "*/$entry_dir/*_hivm-plan-memory.mlir" | sort | tail -n 1)
      if [[ -n "$entry_src" && -f "$entry_src" ]]; then
        selected_sources+=("$entry_src")
        selected_labels+=("$entry_func")
        cp "$entry_src" "${dst%.mlir}_${entry_func}.mlir"
        format_ir_in_place "${dst%.mlir}_${entry_func}.mlir"
      fi
    done
  fi

  if [[ ${#selected_sources[@]} -gt 0 ]]; then
    src="${selected_sources[$((${#selected_sources[@]} - 1))]}"
  fi
  if [[ -z "$src" ]]; then
    src=$(find "$tree_dir" -type f -name '*_hivm-plan-memory.mlir' ! -path '*infer_task_type_function*' | sort | tail -n 1)
  fi
  if [[ -z "$src" ]]; then
    src=$(find "$tree_dir" -type f -name '*_hivm-plan-memory.mlir' | sort | tail -n 1)
  fi
  if [[ -n "$src" && -f "$src" ]]; then
    cp "$src" "$dst"
    format_ir_in_place "$dst"
    if [[ ${#selected_sources[@]} -gt 0 ]]; then
      local summary=""
      local i=0
      for ((i = 0; i < ${#selected_sources[@]}; i++)); do
        if [[ -n "$summary" ]]; then
          summary="$summary,"
        fi
        summary="$summary${selected_labels[$i]}:${selected_sources[$i]}"
      done
      printf '%s' "$summary"
    else
      printf '%s' "$src"
    fi
  fi
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
      [[ "$1" =~ ^[0-9]+$ ]] || die "--max-files must be a non-negative integer"
      MAX_FILES="$1"
      ;;
    --stop-on-fail)
      STOP_ON_FAIL=1
      ;;
    --allow-failures)
      ALLOW_FAILURES=1
      ;;
    --dump-full-ir-tree)
      DUMP_FULL_IR_TREE=1
      ;;
    --drop-full-tree)
      DUMP_FULL_IR_TREE=1
      KEEP_FULL_TREE=0
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
      [[ "$1" =~ ^[0-9]+$ ]] || die "--case-timeout must be a non-negative integer"
      CASE_TIMEOUT="$1"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      break
      ;;
    -*)
      die "Unknown option: $1"
      ;;
    *)
      INPUTS+=("$1")
      ;;
  esac
  shift
done

EXTRA_COMPILER_ARGS=("$@")

[[ -x "$COMPILER" ]] || die "Compiler not executable: $COMPILER"
COMPILER=$(abs_path "$COMPILER")

if [[ ${#INPUTS[@]} -eq 0 ]]; then
  [[ -d "$DATA_DIR" ]] || die "Data directory not found: $DATA_DIR"
  while IFS= read -r file; do
    base=$(basename "$file")
    if [[ "$INCLUDE_HIDDEN" != "1" && "$base" == .* ]]; then
      continue
    fi
    INPUTS+=("$file")
  done < <(find "$DATA_DIR" -maxdepth 1 -type f -name '*.ttadapter' | sort)
fi

[[ ${#INPUTS[@]} -gt 0 ]] || die "No input files found"

if [[ "$MAX_FILES" -gt 0 && ${#INPUTS[@]} -gt "$MAX_FILES" ]]; then
  INPUTS=("${INPUTS[@]:0:$MAX_FILES}")
fi

if [[ -z "$OUTPUT_ROOT" ]]; then
  OUTPUT_ROOT="./Output"
fi

mkdir -p "$OUTPUT_ROOT" || die "Failed to create output root: $OUTPUT_ROOT"
OUTPUT_ROOT=$(abs_path "$OUTPUT_ROOT")
ADAPTER_ROOT="$OUTPUT_ROOT/adapters"
INDEX_ROOT="$OUTPUT_ROOT/index/planmemory_${RUN_ID}"
mkdir -p "$ADAPTER_ROOT" "$INDEX_ROOT"
SUMMARY="$INDEX_ROOT/summary.tsv"
printf 'input\tstatus\tcase_dir\tbefore_snapshot\tafter_snapshot\tafter_source\tmemory_info_files\n' > "$SUMMARY"

echo "[INFO] Inputs: ${#INPUTS[@]}"
echo "[INFO] Output root: $OUTPUT_ROOT"
if [[ "$CASE_TIMEOUT" -gt 0 ]]; then
  echo "[INFO] Per-case timeout: ${CASE_TIMEOUT}s"
else
  echo "[INFO] Per-case timeout: disabled"
fi

ENABLE_DUMP_BEFORE_OPTION=0
if "$COMPILER" --help 2>&1 | grep -q -- "--enable-dump-ir-before-plan-memory"; then
  ENABLE_DUMP_BEFORE_OPTION=1
fi

success_count=0
fail_count=0

for input in "${INPUTS[@]}"; do
  if [[ ! -f "$input" ]]; then
    echo "[WARN] Skip missing input: $input" >&2
    continue
  fi

  input_abs=$(abs_path "$input")
  adapter=$(basename "$input")
  case_dir="$ADAPTER_ROOT/$adapter/03_planmemory/$RUN_ID"
  tree_dir="$case_dir/ir_tree"
  all_after_dir="$case_dir/plan_memory_after_dumps"
  before_snapshot="$case_dir/before_local_plan_memory.mlir"
  after_snapshot="$case_dir/after_local_plan_memory.mlir"
  stdout_log="$case_dir/compile.stdout.log"
  stderr_log="$case_dir/compile.stderr.log"
  output_file="$case_dir/output.o"

  mkdir -p "$case_dir"
  if [[ "$DUMP_FULL_IR_TREE" == "1" ]]; then
    mkdir -p "$tree_dir"
  fi
  echo "[INFO] Running: $input"

  compile_cmd=(
    "$COMPILER" "$input_abs"
    -o "$output_file"
    --enable-hfusion-compile
    --enable-hivm-compile
    --enable-triton-kernel-compile
    --enable-memory-display
    --mlir-disable-threading
    "${EXTRA_COMPILER_ARGS[@]}"
  )

  if [[ "$ENABLE_DUMP_BEFORE_OPTION" == "1" ]]; then
    compile_cmd+=(--enable-dump-ir-before-plan-memory)
  fi

  if [[ "$DUMP_FULL_IR_TREE" == "1" ]]; then
    compile_cmd+=(
      --mlir-print-ir-after-all
      --mlir-print-ir-module-scope
      "--mlir-print-ir-tree-dir=$tree_dir"
    )
  fi

  set +e
  timeout_marker="$case_dir/.compile_timeout"
  rm -f "$timeout_marker"
  if [[ "$CASE_TIMEOUT" -gt 0 ]]; then
    (
      cd "$case_dir" &&
        BISHENGIR_DUMP_BEFORE_PLAN_MEMORY="$before_snapshot" \
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
        BISHENGIR_DUMP_BEFORE_PLAN_MEMORY="$before_snapshot" \
          "${compile_cmd[@]}"
    ) >"$stdout_log" 2>"$stderr_log"
    status=$?
  fi
  rm -f "$timeout_marker"
  set -e

  if [[ -f "$before_snapshot" ]]; then
    format_ir_in_place "$before_snapshot"
  fi

  after_source=""
  if [[ "$DUMP_FULL_IR_TREE" == "1" ]]; then
    after_source=$(copy_plan_memory_after_dumps "$tree_dir" "$before_snapshot" "$after_snapshot" "$all_after_dir")
  fi

  if [[ "$DUMP_FULL_IR_TREE" == "1" && "$KEEP_FULL_TREE" != "1" ]]; then
    rm -rf "$tree_dir"
    if [[ -n "$after_source" ]]; then
      after_source="ir_tree dropped; matching source was copied under $all_after_dir"
    fi
  fi

  if [[ "$status" -eq 0 ]]; then
    run_status="ok"
    success_count=$((success_count + 1))
  else
    run_status="fail:$status"
    fail_count=$((fail_count + 1))
  fi

  [[ -f "$before_snapshot" ]] || before_snapshot=""
  [[ -f "$after_snapshot" ]] || after_snapshot=""
  memory_info_files=""
  for memory_info_file in "$case_dir"/memory_info*.json; do
    if [[ -f "$memory_info_file" ]]; then
      if [[ -n "$memory_info_files" ]]; then
        memory_info_files="$memory_info_files,$memory_info_file"
      else
        memory_info_files="$memory_info_file"
      fi
    fi
  done
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$input" "$run_status" "$case_dir" "$before_snapshot" "$after_snapshot" "$after_source" "$memory_info_files" \
    >> "$SUMMARY"

  echo "[INFO] Done: $input ($run_status)"
  if [[ "$STOP_ON_FAIL" == "1" && "$status" -ne 0 ]]; then
    break
  fi
done

echo "[INFO] Summary: $SUMMARY"
echo "[INFO] Success: $success_count, failed: $fail_count"

if [[ "$fail_count" -gt 0 && "$ALLOW_FAILURES" != "1" ]]; then
  exit 1
fi
