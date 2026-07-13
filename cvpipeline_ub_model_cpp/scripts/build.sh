#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${MODULE_DIR}/output/bin"
CORE_SRC="${MODULE_DIR}/src/cli/cvpipeline_ub_model.cpp"
CORE_OUT="${OUT_DIR}/cvpipeline_ub_model"

mkdir -p "${OUT_DIR}"
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  "${CORE_SRC}" -o "${CORE_OUT}"
echo "${CORE_OUT}"
