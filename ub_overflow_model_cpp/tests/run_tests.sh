#!/usr/bin/env bash
set -euo pipefail

mkdir -p ub_overflow_model_cpp/output/tests
compiler="${CXX:-c++}"

bash ub_overflow_model_cpp/build.sh >/dev/null

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror -Iub_overflow_model_cpp/include \
  ub_overflow_model_cpp/tests/test_in_process_api.cpp \
  ub_overflow_model_cpp/output/lib/libub_overflow_model.a \
  -o ub_overflow_model_cpp/output/tests/test_in_process_api
ub_overflow_model_cpp/output/tests/test_in_process_api

stdin_fixture="ub_overflow_model_cpp/data/before_cvpipelining/ascend_tutorial_01-vector-add.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir"
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir="${stdin_fixture}" --format=json \
  > ub_overflow_model_cpp/output/tests/file-input.json
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=- --format=json \
  < "${stdin_fixture}" > ub_overflow_model_cpp/output/tests/stdin-input.json
cmp ub_overflow_model_cpp/output/tests/file-input.json \
  ub_overflow_model_cpp/output/tests/stdin-input.json
echo "[PASS] standalone file and stdin inputs agree"

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
  ub_overflow_model_cpp/tests/test_shadow_overlay.cpp \
  -o ub_overflow_model_cpp/output/tests/test_shadow_overlay
ub_overflow_model_cpp/output/tests/test_shadow_overlay

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

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tests/test_pre_cv_mark_multi_buffer.cpp \
  -o ub_overflow_model_cpp/output/tests/test_pre_cv_mark_multi_buffer
ub_overflow_model_cpp/output/tests/test_pre_cv_mark_multi_buffer

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tests/test_outer_extended_canonicalizer.cpp \
  -o ub_overflow_model_cpp/output/tests/test_outer_extended_canonicalizer
ub_overflow_model_cpp/output/tests/test_outer_extended_canonicalizer

"${compiler}" -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion \
  -Wshadow -Werror \
  ub_overflow_model_cpp/tools/pre_cv_prefix_model_runner.cpp \
  -o ub_overflow_model_cpp/output/tests/pre_cv_prefix_model_runner

python3 ub_overflow_model_cpp/tests/test_validation_common.py
python3 ub_overflow_model_cpp/tests/test_merged_report.py
python3 ub_overflow_model_cpp/tests/test_plan_memory_contract.py
python3 ub_overflow_model_cpp/tests/test_bisheng_embedded_matrix.py
python3 ub_overflow_model_cpp/tests/test_ub_prefix_checkpoints.py
