#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
MODEL="$(${MODULE_DIR}/build.sh)"

bash "${MODULE_DIR}/scripts/verify_hivm_op_registry.sh"
bash "${MODULE_DIR}/scripts/run_unit_tests.sh"
bash "${MODULE_DIR}/scripts/build_dev_tools.sh" >/dev/null

ORACLE_OBJECTS="${REPO_ROOT}/Output/experiments/one_shot_bufferize_hivm_decompose_op/objects"
if compgen -G "${ORACLE_OBJECTS}/*/pass_tree/builtin_module_no-symbol-name/0_one-shot-bufferize.mlir" >/dev/null; then
  python3 "${MODULE_DIR}/scripts/validate_one_shot_bufferize.py"
  python3 "${MODULE_DIR}/scripts/validate_bufferized_semantic_sensitivity.py"
  python3 "${MODULE_DIR}/scripts/validate_post_bufferization_normalization.py"
  python3 "${MODULE_DIR}/scripts/validate_hivm_decompose_op.py"
  python3 "${MODULE_DIR}/scripts/validate_non_contiguous_reshape_to_copy.py"
  python3 "${MODULE_DIR}/scripts/validate_hivm_decompose_op_semantic_sensitivity.py"
  python3 "${MODULE_DIR}/scripts/validate_alloc_extra_buffer_projection.py"
  python3 "${MODULE_DIR}/scripts/validate_inline_load_copy_buffer_projection.py"
  python3 "${MODULE_DIR}/scripts/validate_mark_multi_buffer.py"
  python3 "${MODULE_DIR}/scripts/validate_plan_memory_input.py"
else
  echo "GENERATED_ORACLE_VALIDATION=SKIP reason=fixtures_unavailable"
fi

FIXED_SEED_SUMMARY="${MODULE_DIR}/output/fixed_seed_oracle/summary.tsv"
if [[ -f "${FIXED_SEED_SUMMARY}" ]]; then
  bash "${MODULE_DIR}/scripts/validate_comparator_sensitivity.sh"
  bash "${MODULE_DIR}/scripts/validate_fixed_seed_exact.sh"
else
  echo "FIXED_SEED_ORACLE_VALIDATION=SKIP reason=summary_unavailable"
fi

SUFFIX_ROOT="${MODULE_DIR}/input/suffix_compile"
if [[ -f "${SUFFIX_ROOT}/ub_peak_summary.tsv" ]]; then
  bash "${MODULE_DIR}/scripts/validate_memory_info_replay.sh"
  bash "${MODULE_DIR}/scripts/validate_local_ub_allocations.sh"
  bash "${MODULE_DIR}/scripts/validate_lifetimes.sh"
  bash "${MODULE_DIR}/scripts/validate_memory_plan.sh"
else
  echo "LEGACY_SUFFIX_ORACLE_VALIDATION=SKIP reason=summary_unavailable"
fi

success_ir="${MODULE_DIR}/testdata/plan_memory/exact/planner_basic_reuse_chain.mlir"
success_json="$(${MODEL} --debug --debug-entry=before-plan-memory --format=json \
  --before-plan-memory-ir="${success_ir}" --random-seed=0)"
grep -q '"status": "success"' <<<"${success_json}"
grep -q '"ub_peak_bits": 65536' <<<"${success_json}"

overflow_ir="${MODULE_DIR}/testdata/plan_memory/exact/boundary_capacity_over_by_one_byte.mlir"
set +e
overflow_json="$(${MODEL} --debug --debug-entry=before-plan-memory --format=json \
  --before-plan-memory-ir="${overflow_ir}" --random-seed=0)"
overflow_status=$?
set -e
if [[ "${overflow_status}" -ne 2 ]]; then
  echo "[ERROR] overflow case returned ${overflow_status}, expected 2" >&2
  exit 1
fi
grep -q '"status": "overflow"' <<<"${overflow_json}"
grep -q '"required_bits": 1573120' <<<"${overflow_json}"
grep -q '"capacity_bits": 1572864' <<<"${overflow_json}"

set +e
blocker_json="$(${MODEL} --debug --debug-entry=before-plan-memory --format=json \
  --before-plan-memory-ir="${REPO_ROOT}/Output/does_not_exist.mlir" \
  --random-seed=0 2>/dev/null)"
blocker_status=$?
set -e
if [[ "${blocker_status}" -ne 1 ]]; then
  echo "[ERROR] blocker case returned ${blocker_status}, expected 1" >&2
  exit 1
fi
grep -q '"precision": "blocked"' <<<"${blocker_json}"
grep -q '"status": "blocker"' <<<"${blocker_json}"

echo "PLAN_MEMORY_MODEL_VALIDATION=PASS"
