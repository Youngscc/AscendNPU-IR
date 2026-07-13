#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${MODULE_DIR}/output/bin"
DEV_SRC="${MODULE_DIR}/src/cli/dev_validate.cpp"
DEV_OUT="${OUT_DIR}/cvpipeline_ub_model_dev_validate"

mkdir -p "${OUT_DIR}"
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  "${DEV_SRC}" -o "${DEV_OUT}"
echo "${DEV_OUT}"
