#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${repo_root}"

mkdir -p ub_overflow_model_cpp/output/tests
compiler="${CXX:-c++}"
runner="ub_overflow_model_cpp/output/tests/auto_blockify_model_runner"

"${compiler}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tools/auto_blockify_model_runner.cpp \
  -o "${runner}"

if [[ $# -gt 0 ]]; then
  exec python3 ub_overflow_model_cpp/scripts/verify_auto_blockify.py \
    --model-runner "${runner}" "$@"
fi

exec python3 ub_overflow_model_cpp/scripts/verify_auto_blockify.py \
  --model-runner "${runner}" \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --repair-known-schema-drift \
  --failure-dir ub_overflow_model_cpp/output/auto_blockify_verification/failures \
  --json-report ub_overflow_model_cpp/output/auto_blockify_verification/report.json
