#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${repo_root}"

mkdir -p cvpipeline_ub_model_cpp/output/tests
compiler="${CXX:-c++}"
runner="cvpipeline_ub_model_cpp/output/tests/auto_blockify_model_runner"

"${compiler}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  cvpipeline_ub_model_cpp/tools/auto_blockify_model_runner.cpp \
  -o "${runner}"

if [[ $# -gt 0 ]]; then
  exec python3 cvpipeline_ub_model_cpp/scripts/verify_auto_blockify.py \
    --model-runner "${runner}" "$@"
fi

exec python3 cvpipeline_ub_model_cpp/scripts/verify_auto_blockify.py \
  --model-runner "${runner}" \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipelining \
  --repair-known-schema-drift \
  --failure-dir cvpipeline_ub_model_cpp/output/auto_blockify_verification/failures \
  --json-report cvpipeline_ub_model_cpp/output/auto_blockify_verification/report.json
