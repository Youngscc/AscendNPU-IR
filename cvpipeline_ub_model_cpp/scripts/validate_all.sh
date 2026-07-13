#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
MODEL="$(${MODULE_DIR}/scripts/build.sh)"

bash "${MODULE_DIR}/scripts/verify_hivm_op_registry.sh"
bash "${MODULE_DIR}/scripts/run_unit_tests.sh"
bash "${MODULE_DIR}/scripts/build_dev_tools.sh" >/dev/null
python3 "${MODULE_DIR}/scripts/validate_one_shot_bufferize.py"
python3 "${MODULE_DIR}/scripts/validate_bufferized_semantic_sensitivity.py"
python3 "${MODULE_DIR}/scripts/validate_post_bufferization_normalization.py"
python3 "${MODULE_DIR}/scripts/validate_hivm_decompose_op.py"
python3 "${MODULE_DIR}/scripts/validate_non_contiguous_reshape_to_copy.py"
python3 "${MODULE_DIR}/scripts/validate_c3_semantic_sensitivity.py"
python3 "${MODULE_DIR}/scripts/validate_c4_buffer_projection.py"
python3 "${MODULE_DIR}/scripts/validate_c5_buffer_projection.py"
python3 "${MODULE_DIR}/scripts/validate_c6_multi_buffer.py"
python3 "${MODULE_DIR}/scripts/validate_c7_planmemory_input.py"
bash "${MODULE_DIR}/scripts/validate_comparator_sensitivity.sh"
bash "${MODULE_DIR}/scripts/validate_memory_info_replay.sh"
bash "${MODULE_DIR}/scripts/validate_local_ub_allocations.sh"
bash "${MODULE_DIR}/scripts/validate_lifetimes.sh"
bash "${MODULE_DIR}/scripts/validate_memory_plan.sh"
bash "${MODULE_DIR}/scripts/validate_fixed_seed_exact.sh"

success_ir="${REPO_ROOT}/Output/adapters/kernels_vllm_rejection_random_sample_kernel.ttadapter/03_planmemory/all/before_local_plan_memory.mlir"
success_json="$(${MODEL} --action=plan-local-memory --format=json \
  --before-planmemory-ir="${success_ir}" --random-seed=0)"
grep -q '"status": "success"' <<<"${success_json}"
grep -q '"ub_peak_bits": 288' <<<"${success_json}"

overflow_ir="${REPO_ROOT}/Output/adapters/attn_fwd.ttadapter/02_suffix/compiler_default/before_cvpipelining_func_func_attn_fwd_32.before_local_plan_memory.mlir"
set +e
overflow_json="$(${MODEL} --action=plan-local-memory --format=json \
  --before-planmemory-ir="${overflow_ir}" --random-seed=0)"
overflow_status=$?
set -e
if [[ "${overflow_status}" -ne 2 ]]; then
  echo "[ERROR] overflow case returned ${overflow_status}, expected 2" >&2
  exit 1
fi
grep -q '"status": "overflow"' <<<"${overflow_json}"
grep -q '"required_bits": 1716224' <<<"${overflow_json}"
grep -q '"capacity_bits": 1572864' <<<"${overflow_json}"

set +e
blocker_json="$(${MODEL} --action=plan-local-memory --format=json \
  --before-planmemory-ir="${REPO_ROOT}/Output/does_not_exist.mlir" \
  --random-seed=0 2>/dev/null)"
blocker_status=$?
set -e
if [[ "${blocker_status}" -ne 1 ]]; then
  echo "[ERROR] blocker case returned ${blocker_status}, expected 1" >&2
  exit 1
fi
grep -q '"precision": "blocked"' <<<"${blocker_json}"
grep -q '"status": "blocker"' <<<"${blocker_json}"

echo "PLANMEMORY_MODEL_VALIDATION=PASS"
