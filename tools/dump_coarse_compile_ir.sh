#!/usr/bin/env bash

set -u

PRINT_INTERMEDIATE_DUMPS=${PRINT_INTERMEDIATE_DUMPS:-1}
INPUT_IR=${INPUT_IR:-}
OUTPUT_FILE=${OUTPUT_FILE:-}

usage() {
  cat <<'EOF'
Usage: bash tools/dump_coarse_compile_ir.sh [options] [input.mlir]

Options:
  -i, --input FILE
               Input MLIR file. Defaults to ./data/crossCoreDeps_test.mlir.
  -o, --output FILE
               Output file. Defaults to input path with .out suffix.
  --dump       Print and collect intermediate MLIR dumps. This is the default.
  --no-dump    Run compile only, without --mlir-print-ir-after-all or coarse dumps.

Environment:
  PRINT_INTERMEDIATE_DUMPS=0|1  Enable or disable intermediate dumps.
  INPUT_IR=path                 Override input MLIR file.
  OUTPUT_FILE=path              Override output file.
  DUMP_DIR=path                 Override dump directory when dumps are enabled.
EOF
}

normalize_bool() {
  case "$1" in
    1|true|TRUE|yes|YES|on|ON)
      echo 1
      ;;
    0|false|FALSE|no|NO|off|OFF)
      echo 0
      ;;
    *)
      echo "[ERROR] Invalid boolean value: $1" >&2
      return 1
      ;;
  esac
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--input)
      shift
      if [[ $# -eq 0 ]]; then
        echo "[ERROR] Missing value for --input" >&2
        usage >&2
        exit 2
      fi
      INPUT_IR="$1"
      ;;
    -o|--output)
      shift
      if [[ $# -eq 0 ]]; then
        echo "[ERROR] Missing value for --output" >&2
        usage >&2
        exit 2
      fi
      OUTPUT_FILE="$1"
      ;;
    --dump)
      PRINT_INTERMEDIATE_DUMPS=1
      ;;
    --no-dump)
      PRINT_INTERMEDIATE_DUMPS=0
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
      echo "[ERROR] Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
    *)
      if [[ -n "$INPUT_IR" ]]; then
        echo "[ERROR] Multiple input files specified: $INPUT_IR and $1" >&2
        usage >&2
        exit 2
      fi
      INPUT_IR="$1"
      ;;
  esac
  shift
done

if [[ $# -gt 0 ]]; then
  if [[ -n "$INPUT_IR" ]]; then
    echo "[ERROR] Multiple input files specified: $INPUT_IR and $1" >&2
    usage >&2
    exit 2
  fi
  INPUT_IR="$1"
  shift
fi

if [[ $# -gt 0 ]]; then
  echo "[ERROR] Unexpected extra arguments: $*" >&2
  usage >&2
  exit 2
fi

if ! PRINT_INTERMEDIATE_DUMPS=$(normalize_bool "$PRINT_INTERMEDIATE_DUMPS"); then
  usage >&2
  exit 2
fi

INPUT_IR=${INPUT_IR:-./data/crossCoreDeps_test.mlir}
if [[ -z "$OUTPUT_FILE" ]]; then
  input_dir=$(dirname "$INPUT_IR")
  input_base=$(basename "$INPUT_IR")
  if [[ "$input_base" == *.* ]]; then
    OUTPUT_FILE="$input_dir/${input_base%.*}.out"
  else
    OUTPUT_FILE="$input_dir/$input_base.out"
  fi
fi

if [[ ! -f "$INPUT_IR" ]]; then
  echo "[ERROR] Input MLIR file not found: $INPUT_IR" >&2
  exit 2
fi

echo "[INFO] Input MLIR: $INPUT_IR"
echo "[INFO] Output file: $OUTPUT_FILE"

compile_cmd=(
  build/bin/bishengir-compile "$INPUT_IR"
  -o "$OUTPUT_FILE"
  --enable-hfusion-compile
  --enable-hivm-compile
    --enable-triton-kernel-compile
    --enable-memory-display
    --enable-dump-ir-before-plan-memory
    --mlir-disable-threading
  )

if [[ "$PRINT_INTERMEDIATE_DUMPS" == "1" ]]; then
  DUMP_DIR=${DUMP_DIR:-./Output/mlir_ir_dump_$(date +%Y%m%d_%H%M%S)}
  mkdir -p "$DUMP_DIR"
  echo "[INFO] MLIR IR dump dir: $DUMP_DIR"
  compile_cmd+=(
    --mlir-print-ir-after-all
    --mlir-print-ir-module-scope
    "--mlir-print-ir-tree-dir=$DUMP_DIR"
  )
else
  echo "[INFO] Intermediate MLIR dumps disabled"
fi

set +e
if [[ "$PRINT_INTERMEDIATE_DUMPS" == "1" ]]; then
  BISHENGIR_DUMP_BEFORE_PLAN_MEMORY="$DUMP_DIR/before_plan_memory.mlir" \
    "${compile_cmd[@]}"
else
  "${compile_cmd[@]}"
fi
status=$?
set -e

if [[ "$PRINT_INTERMEDIATE_DUMPS" != "1" ]]; then
  exit "$status"
fi

COARSE_DIR="$DUMP_DIR/coarse"
mkdir -p "$COARSE_DIR"

format_ir_in_place() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    return
  fi

  perl -0pi -e '
    s/^(\s*)(hivm\.hir\.[^\s]+)\s+ins\(/$1$2\n$1  ins(/mg;
    s/^(\s*)ins\((.*?)\)\s+outs\(/$1ins($2)\n$1outs(/mg;
  ' "$file"
}

copy_exact() {
  local src="$1"
  local dst="$2"
  if [[ -f "$src" ]]; then
    cp "$src" "$COARSE_DIR/$dst"
    format_ir_in_place "$COARSE_DIR/$dst"
    echo "[INFO] coarse dump: $dst <= $src"
  fi
}

copy_last_match() {
  local pattern="$1"
  local dst="$2"
  local src
  src=$(find "$DUMP_DIR" -type f -path "$pattern" | sort | tail -n 1)
  if [[ -n "$src" && -f "$src" ]]; then
    cp "$src" "$COARSE_DIR/$dst"
    format_ir_in_place "$COARSE_DIR/$dst"
    echo "[INFO] coarse dump: $dst <= $src"
  fi
}

copy_first_match() {
  local pattern="$1"
  local dst="$2"
  local src
  src=$(find "$DUMP_DIR" -type f -path "$pattern" | sort | head -n 1)
  if [[ -n "$src" && -f "$src" ]]; then
    cp "$src" "$COARSE_DIR/$dst"
    format_ir_in_place "$COARSE_DIR/$dst"
    echo "[INFO] coarse dump: $dst <= $src"
  fi
}

cp "$INPUT_IR" "$COARSE_DIR/00_input_ir.mlir"
format_ir_in_place "$COARSE_DIR/00_input_ir.mlir"
copy_exact "$DUMP_DIR/builtin_module_no-symbol-name/7_adapt-triton-kernel.mlir" \
  "01_after_adapt_triton_kernel.mlir"
copy_exact "$DUMP_DIR/builtin_module_no-symbol-name/18_hfusion-hoist-tensor-empty.mlir" \
  "02_after_hfusion.mlir"
copy_exact "$DUMP_DIR/builtin_module_no-symbol-name/21_convert-to-hivm-op.mlir" \
  "03_after_convert_to_hivm.mlir"
copy_last_match "$DUMP_DIR/builtin_module_no-symbol-name/*_one-shot-bufferize.mlir" \
  "04_after_bufferization.mlir"
copy_first_match "$DUMP_DIR/builtin_module_no-symbol-name/func_func__attn_fwd/*_hivm-plan-memory.mlir" \
  "05_after_workspace_plan_memory.mlir"
copy_last_match "$DUMP_DIR/builtin_module_no-symbol-name/*_hivm-infer-mem-scope.mlir" \
  "06_after_hivm_postbuffer_infer_mem_scope.mlir"
copy_last_match "$DUMP_DIR/builtin_module_no-symbol-name/func_func__attn_fwd/*_hivm-mark-multi-buffer.mlir" \
  "07_after_mark_multi_buffer.mlir"
copy_exact "$DUMP_DIR/before_plan_memory.mlir" \
  "08_before_local_plan_memory.mlir"
copy_last_match "$DUMP_DIR/builtin_module_no-symbol-name/func_func__attn_fwd/*_hivm-plan-memory.mlir" \
  "09_after_local_plan_memory_attempt.mlir"

cat > "$COARSE_DIR/README.md" <<'EOF'
# Coarse IR Dumps

这些文件是从完整 `--mlir-print-ir-after-all` dump 中挑出来的粗粒度快照，用来观察
输入 IR 如何逐步走向内存规划。

| File | Meaning |
| --- | --- |
| `00_input_ir.mlir` | 原始输入 IR。 |
| `01_after_adapt_triton_kernel.mlir` | Triton kernel 入口适配之后，参数/target 等开始规范化。 |
| `02_after_hfusion.mlir` | HFusion 阶段之后，tensor/HFusion 相关规范化基本结束。 |
| `03_after_convert_to_hivm.mlir` | ConvertToHIVM 之后，进入 HIVM op/entry kernel 形态。 |
| `04_after_bufferization.mlir` | bufferization 之后，tensor 结果转为 memref/alloc/load/store 形态。 |
| `05_after_workspace_plan_memory.mlir` | 较早的 workspace/global PlanMemory 之后。 |
| `06_after_hivm_postbuffer_infer_mem_scope.mlir` | post-bufferization 中 mem scope 推断之后。 |
| `07_after_mark_multi_buffer.mlir` | local PlanMemory 之前的 multi-buffer 标记之后。 |
| `08_before_local_plan_memory.mlir` | 自定义 dump，精确位于 local PlanMemory 前。 |
| `09_after_local_plan_memory_attempt.mlir` | local PlanMemory attempt 的 IR 快照；overflow 时通常还保留 alloc，未替换成最终 pointer_cast。 |

完整细粒度 dump 仍保留在上一级目录。
EOF

echo "[INFO] Coarse IR dump dir: $COARSE_DIR"
exit "$status"
