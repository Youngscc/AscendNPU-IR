#!/usr/bin/env bash
set -euo pipefail

mkdir -p cvpipeline_ub_model_cpp/output/tests

c++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  cvpipeline_ub_model_cpp/tests/test_module_plan.cpp \
  -o cvpipeline_ub_model_cpp/output/tests/test_module_plan
cvpipeline_ub_model_cpp/output/tests/test_module_plan

python3 cvpipeline_ub_model_cpp/tests/test_merged_report.py
