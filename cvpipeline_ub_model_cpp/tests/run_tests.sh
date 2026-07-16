#!/usr/bin/env bash
set -euo pipefail

mkdir -p cvpipeline_ub_model_cpp/output/tests
compiler="${CXX:-c++}"

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  cvpipeline_ub_model_cpp/tests/test_module_plan.cpp \
  -o cvpipeline_ub_model_cpp/output/tests/test_module_plan
cvpipeline_ub_model_cpp/output/tests/test_module_plan

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  cvpipeline_ub_model_cpp/tests/test_capability_parity.cpp \
  -o cvpipeline_ub_model_cpp/output/tests/test_capability_parity
cvpipeline_ub_model_cpp/output/tests/test_capability_parity

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  cvpipeline_ub_model_cpp/tests/test_checked_math.cpp \
  -o cvpipeline_ub_model_cpp/output/tests/test_checked_math
cvpipeline_ub_model_cpp/output/tests/test_checked_math

python3 cvpipeline_ub_model_cpp/tests/test_capability_manifest.py
python3 cvpipeline_ub_model_cpp/tests/test_merged_report.py
python3 cvpipeline_ub_model_cpp/tests/test_oracle_comparison.py
python3 cvpipeline_ub_model_cpp/tests/test_compile_default_sync.py
