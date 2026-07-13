#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_DIR="$(cd "${MODULE_DIR}/.." && pwd)"
TOOL="${REPO_DIR}/build/bin/bishengir-cvpipeline-suffix-compile"
MODEL="${MODULE_DIR}/output/bin/cvpipeline_ub_model_dev_validate"
INPUT="${MODULE_DIR}/testdata/planmemory_b/exact/b_control_cf_diamond_single_arg.mlir"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/c1-focused-cfg.XXXXXX")"
trap 'rm -rf "${TMP_DIR}"' EXIT

"${TOOL}" "${INPUT}" --disable-cv-pipelining --stop-after-cvpipelining \
  --mlir-disable-threading \
  --dump-c1-semantic-oracle="${TMP_DIR}/oracle.tsv" \
  --dump-c1-generic-ir="${TMP_DIR}/generic.mlir" \
  -o "${TMP_DIR}/boundary.mlir"
"${MODEL}" --action=dump-c1-semantic-ir \
  --root="${TMP_DIR}/generic.mlir" --output="${TMP_DIR}/model.tsv"
cmp "${TMP_DIR}/oracle.tsv" "${TMP_DIR}/model.tsv"
echo "C1_FOCUSED_CFG=PASS"
