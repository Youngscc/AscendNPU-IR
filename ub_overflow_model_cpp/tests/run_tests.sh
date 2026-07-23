#!/usr/bin/env bash
set -euo pipefail

mkdir -p ub_overflow_model_cpp/output/tests
compiler="${CXX:-c++}"

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tests/test_module_plan.cpp \
  -o ub_overflow_model_cpp/output/tests/test_module_plan
ub_overflow_model_cpp/output/tests/test_module_plan

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tests/test_capability_parity.cpp \
  -o ub_overflow_model_cpp/output/tests/test_capability_parity
ub_overflow_model_cpp/output/tests/test_capability_parity

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tests/test_checked_math.cpp \
  -o ub_overflow_model_cpp/output/tests/test_checked_math
ub_overflow_model_cpp/output/tests/test_checked_math

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tests/test_auto_blockify_parallel_loop.cpp \
  -o ub_overflow_model_cpp/output/tests/test_auto_blockify_parallel_loop
ub_overflow_model_cpp/output/tests/test_auto_blockify_parallel_loop

python3 ub_overflow_model_cpp/tests/test_capability_manifest.py
python3 ub_overflow_model_cpp/tests/test_merged_report.py
python3 ub_overflow_model_cpp/tests/test_oracle_comparison.py
python3 ub_overflow_model_cpp/tests/test_compile_default_sync.py
python3 ub_overflow_model_cpp/tests/test_corpus_matrix.py
