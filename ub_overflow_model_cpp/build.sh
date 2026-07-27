#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${MODULE_DIR}/scripts/build_incremental_common.sh"
OUT_DIR="${MODULE_DIR}/output/bin"
CORE_SRC="${MODULE_DIR}/src/main.cpp"
CORE_OUT="${OUT_DIR}/bishengir-ub-overflow-model"
LEGACY_OUT="${OUT_DIR}/cvpipeline_ub_model"
API_SRC="${MODULE_DIR}/src/api.cpp"
API_OBJ="${MODULE_DIR}/output/obj/api.o"
API_LIB="${MODULE_DIR}/output/lib/libub_overflow_model.a"

compile_object_if_needed "${API_SRC}" "${API_OBJ}" "${MODULE_DIR}"
mkdir -p "$(dirname "${API_LIB}")"
if [[ ! -e "${API_LIB}" || "${API_OBJ}" -nt "${API_LIB}" ]]; then
  "${AR:-ar}" rcs "${API_LIB}" "${API_OBJ}"
fi
compile_if_needed "${CORE_SRC}" "${CORE_OUT}" "${MODULE_DIR}"
ln -sfn "$(basename "${CORE_OUT}")" "${LEGACY_OUT}"
echo "${API_LIB}"
