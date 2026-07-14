#!/usr/bin/env bash
set -euo pipefail

mkdir -p cvpipeline_ub_model_cpp/output/tests

c++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  -o cvpipeline_ub_model_cpp/output/tests/test_post_cvpipeline
cvpipeline_ub_model_cpp/output/tests/test_post_cvpipeline
