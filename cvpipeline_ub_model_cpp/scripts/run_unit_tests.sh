#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${MODULE_DIR}/output/bin"
MODEL_SOURCE="${MODULE_DIR}/tests/model_core_tests.cpp"
MODEL_BINARY="${OUT_DIR}/cvpipeline_ub_model_tests"
SEMANTIC_SOURCE="${MODULE_DIR}/tests/semantic_ir_tests.cpp"
SEMANTIC_BINARY="${OUT_DIR}/semantic_ir_tests"
INLINE_SOURCE="${MODULE_DIR}/tests/inline_load_copy_tests.cpp"
INLINE_BINARY="${OUT_DIR}/inline_load_copy_tests"
MULTI_BUFFER_SOURCE="${MODULE_DIR}/tests/mark_multi_buffer_tests.cpp"
MULTI_BUFFER_BINARY="${OUT_DIR}/mark_multi_buffer_tests"

mkdir -p "${OUT_DIR}"
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  "${MODEL_SOURCE}" -o "${MODEL_BINARY}"
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  "${SEMANTIC_SOURCE}" -o "${SEMANTIC_BINARY}"
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  "${INLINE_SOURCE}" -o "${INLINE_BINARY}"
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  "${MULTI_BUFFER_SOURCE}" -o "${MULTI_BUFFER_BINARY}"
"${MODEL_BINARY}"
"${SEMANTIC_BINARY}"
"${INLINE_BINARY}"
"${MULTI_BUFFER_BINARY}"
