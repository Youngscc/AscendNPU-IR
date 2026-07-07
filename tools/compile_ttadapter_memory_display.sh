#!/usr/bin/env bash

set -euo pipefail

INPUT="data/attn_fwd.ttadapter"
OUTPUT="./tmp"
COMPILER="build/bin/bishengir-compile"

usage() {
  cat <<'EOF'
Usage: bash tools/compile_ttadapter_memory_display.sh [options] [input.ttadapter]

Compile one .ttadapter with HFusion + HIVM + Triton-kernel compile enabled and
MemoryDisplay enabled. This is the named version of the manual fake/attn_fwd
smoke commands.

Options:
  -i, --input FILE
        Input .ttadapter. Defaults to data/attn_fwd.ttadapter.
  -o, --output FILE
        Output file. Defaults to ./tmp.
  -c, --compiler FILE
        bishengir-compile binary. Defaults to build/bin/bishengir-compile.
  --fake
        Shortcut for --input data/fake_attn_fwd.ttadapter.
  -h, --help
        Show this help.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--input)
      shift
      [[ $# -gt 0 ]] || { echo "[ERROR] Missing value for --input" >&2; exit 2; }
      INPUT="$1"
      ;;
    -o|--output)
      shift
      [[ $# -gt 0 ]] || { echo "[ERROR] Missing value for --output" >&2; exit 2; }
      OUTPUT="$1"
      ;;
    -c|--compiler)
      shift
      [[ $# -gt 0 ]] || { echo "[ERROR] Missing value for --compiler" >&2; exit 2; }
      COMPILER="$1"
      ;;
    --fake)
      INPUT="data/fake_attn_fwd.ttadapter"
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
      echo "[ERROR] Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
    *)
      INPUT="$1"
      ;;
  esac
  shift
done

[[ -x "$COMPILER" ]] || { echo "[ERROR] Compiler not executable: $COMPILER" >&2; exit 2; }
[[ -f "$INPUT" ]] || { echo "[ERROR] Input not found: $INPUT" >&2; exit 2; }

"$COMPILER" "$INPUT" \
  -o "$OUTPUT" \
  --enable-hfusion-compile \
  --enable-hivm-compile \
  --enable-triton-kernel-compile \
  --enable-memory-display \
  --mlir-disable-threading
