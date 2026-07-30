#!/usr/bin/env python3
"""Unit checks for same-process model/PlanMemory validation parsing."""

from __future__ import annotations

import importlib.util
from collections import Counter
from pathlib import Path
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/run_bisheng_embedded_matrix.py"
SPEC = importlib.util.spec_from_file_location(
    "run_bisheng_embedded_matrix", SCRIPT
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)

rows = MODULE.load_scenarios(
    ROOT / "config/ub_relevant_parameter_scenarios.tsv", set()
)
assert len(rows) == 35
assert len(MODULE.load_known_timeout_pairs(MODULE.DEFAULT_TIMEOUT_PAIRS)) == 81
assert len(MODULE.load_adapters(MODULE.DEFAULT_ADAPTER_ROOT, set(), 0)) == 160
assert len(MODULE.load_adapters(
    MODULE.DEFAULT_ADAPTER_ROOT,
    {"triton.language.extra.cann.extension.index_put.ttadapter"}, 0,
)) == 1
assert MODULE.selected_seeds("0-19") == list(range(20))
arguments = MODULE.compiler_arguments(rows[0])
assert "--enable-ub-overflow-prediction=true" in arguments
assert "--prune-predicted-ub-overflow=false" in arguments
assert "--enable-triton-kernel-compile" in arguments
assert "--enable-auto-blockify-loop=false" in arguments
assert "--limit-auto-multi-buffer-only-for-local-buffer=false" in arguments
auto_blockify = next(row for row in rows
                     if row["scenario_id"] == "auto_blockify")
assert "--enable-auto-blockify-loop=true" in \
    MODULE.compiler_arguments(auto_blockify)
auto_blockify_auto_mb = next(
    row for row in rows
    if row["scenario_id"] == "auto_blockify_auto_mb"
)
auto_blockify_auto_mb_arguments = MODULE.compiler_arguments(
    auto_blockify_auto_mb
)
assert "--enable-auto-blockify-loop=true" in auto_blockify_auto_mb_arguments
assert "--enable-auto-multi-buffer=true" in auto_blockify_auto_mb_arguments
auto_blockify_local_only = next(
    row for row in rows
    if row["scenario_id"] == "auto_blockify_auto_mb_local_only"
)
assert "--limit-auto-multi-buffer-only-for-local-buffer=true" in \
    MODULE.compiler_arguments(auto_blockify_local_only)
auto_blockify_unrestricted = next(
    row for row in rows
    if row["scenario_id"] == "auto_blockify_auto_mb_unrestricted"
)
auto_blockify_unrestricted_arguments = MODULE.compiler_arguments(
    auto_blockify_unrestricted
)
assert "--limit-auto-multi-buffer-of-local-buffer=no-limit" in \
    auto_blockify_unrestricted_arguments
assert "--limit-auto-multi-buffer-buffer=no-limit" in \
    auto_blockify_unrestricted_arguments
for row in (
    auto_blockify_auto_mb,
    auto_blockify_local_only,
    auto_blockify_unrestricted,
):
    profile = ROOT / "config/pre_cv_profiles" / \
        f"{row['pre_cv_profile']}.args"
    assert profile.is_file(), profile
    profile_arguments = profile.read_text(encoding="utf-8").splitlines()
    assert "--enable-auto-blockify-loop=true" in profile_arguments
    assert "--enable-auto-multi-buffer=true" in profile_arguments
auto_blockify_preload = next(
    row for row in rows if row["scenario_id"] == "auto_blockify_preload"
)
auto_blockify_preload_profile = (
    ROOT / "config/pre_cv_profiles" /
    f"{auto_blockify_preload['pre_cv_profile']}.args"
)
assert auto_blockify_preload_profile.is_file()
auto_blockify_preload_arguments = auto_blockify_preload_profile.read_text(
    encoding="utf-8"
).splitlines()
assert "--enable-auto-blockify-loop=true" in auto_blockify_preload_arguments
assert "--enable-preload=true" in auto_blockify_preload_arguments
for scenario_id in (
    "cv_workspace_manage_off",
    "cv_workspace_manage_off_auto_mb",
    "auto_blockify_workspace_manage_off",
    "auto_blockify_workspace_manage_off_auto_mb",
):
    workspace_manage_off = next(
        row for row in rows if row["scenario_id"] == scenario_id
    )
    workspace_manage_off_arguments = MODULE.compiler_arguments(
        workspace_manage_off
    )
    assert "--enable-ub-overflow-prediction=true" in \
        workspace_manage_off_arguments
    assert "--disable-auto-cv-work-space-manage=true" in \
        workspace_manage_off_arguments
for scenario_id in (
    "auto_blockify_workspace_manage_off",
    "auto_blockify_workspace_manage_off_auto_mb",
    "auto_blockify_inject_block_normal",
):
    interaction = next(
        row for row in rows if row["scenario_id"] == scenario_id
    )
    interaction_arguments = MODULE.compiler_arguments(interaction)
    assert "--enable-auto-blockify-loop=true" in interaction_arguments
    profile = ROOT / "config/pre_cv_profiles" / \
        f"{interaction['pre_cv_profile']}.args"
    assert profile.is_file(), profile
    assert "--enable-auto-blockify-loop=true" in \
        profile.read_text(encoding="utf-8").splitlines()
auto_blockify_workspace_auto_mb = next(
    row for row in rows
    if row["scenario_id"] ==
    "auto_blockify_workspace_manage_off_auto_mb"
)
assert "--enable-auto-multi-buffer=true" in \
    MODULE.compiler_arguments(auto_blockify_workspace_auto_mb)
auto_blockify_inject = next(
    row for row in rows
    if row["scenario_id"] == "auto_blockify_inject_block_normal"
)
assert "--enable-hivm-cross-core-gss=false" in \
    MODULE.compiler_arguments(auto_blockify_inject)

model_plan = Counter({(1024, 100): 2, (1024, 200): 1})
native_plan = Counter({(1024, 100): 1, (1024, 200): 2})
model_lifetimes = Counter({
    ("kernel", 1024, 100, 0, 3): 1,
    ("kernel", 1024, 100, 1, 3): 1,
    ("kernel", 1024, 200, 2, 3): 1,
})
native_lifetimes = Counter({
    ("kernel", 1024, 200, 0, 3): 1,
    ("kernel", 1024, 200, 1, 3): 1,
    ("kernel", 1024, 100, 2, 3): 1,
})
model_inplace = Counter({
    (("kernel", 1024, (100,), 0, 3),
     ("kernel", 1024, (100,), 1, 3)): 1,
})
native_inplace = Counter({
    (("kernel", 1024, (200,), 0, 3),
     ("kernel", 1024, (200,), 1, 3)): 1,
})
assert MODULE.is_equal_extent_identity_permutation(
    model_plan, native_plan, model_lifetimes, native_lifetimes,
    model_inplace, native_inplace,
)
different_lifetime = native_lifetimes.copy()
different_lifetime[("kernel", 1024, 100, 2, 4)] = \
    different_lifetime.pop(("kernel", 1024, 100, 2, 3))
assert not MODULE.is_equal_extent_identity_permutation(
    model_plan, native_plan, model_lifetimes, different_lifetime,
    model_inplace, native_inplace,
)
different_native_target = Counter({
    (("kernel", 1024, (200,), 0, 3),
     ("kernel", 1024, (200,), 2, 3)): 1,
})
assert MODULE.is_ub_decision_equivalent_ordering(
    model_plan, native_plan, model_lifetimes, native_lifetimes,
    model_inplace, different_native_target,
    Counter({("kernel", 1024, 0, 3, 1): 3}),
    Counter({("kernel", 1024, 0, 3, 1): 3}),
)
assert not MODULE.is_ub_decision_equivalent_ordering(
    model_plan, native_plan, model_lifetimes, different_lifetime,
    model_inplace, different_native_target,
    Counter({("kernel", 1024, 0, 3, 1): 3}),
    Counter({("kernel", 1024, 0, 4, 1): 3}),
)

# A single logical buffer inplaced onto a two-stage target has two physical
# offsets, but it remains one buffer with the same lifetime and multi count.
multi_target_model_plan = Counter({(1024, 100): 2, (1024, 200): 1})
multi_target_native_plan = Counter({(1024, 100): 3})
multi_target_model_lifetimes = Counter({
    ("kernel", 1024, 100, 0, 1): 2,
    ("kernel", 1024, 200, 1, 2): 1,
})
multi_target_native_lifetimes = Counter({
    ("kernel", 1024, 100, 0, 1): 2,
    ("kernel", 1024, 100, 1, 2): 2,
})
multi_target_model_inplace = Counter({
    (("kernel", 1024, (200,), 1, 2),
     ("kernel", 1024, (300,), 0, 1)): 1,
})
multi_target_native_inplace = Counter({
    (("kernel", 1024, (100, 200), 1, 2),
     ("kernel", 1024, (100, 200), 0, 1)): 1,
})
logical_buffers = Counter({
    ("kernel", 1024, 0, 1, 2): 1,
    ("kernel", 1024, 1, 2, 1): 1,
})
assert MODULE.is_ub_decision_equivalent_ordering(
    multi_target_model_plan, multi_target_native_plan,
    multi_target_model_lifetimes, multi_target_native_lifetimes,
    multi_target_model_inplace, multi_target_native_inplace,
    logical_buffers, logical_buffers,
)
different_logical_buffers = logical_buffers.copy()
different_logical_buffers[("kernel", 2048, 1, 2, 1)] = 1
assert not MODULE.is_ub_decision_equivalent_ordering(
    multi_target_model_plan, multi_target_native_plan,
    multi_target_model_lifetimes, multi_target_native_lifetimes,
    multi_target_model_inplace, multi_target_native_inplace,
    logical_buffers, different_logical_buffers,
)

segment = """\
BISHENGIR_UB_MODEL_VALIDATION_BEGIN\t0
BISHENGIR_UB_MODEL_FUNCTION\t0\tkernel\tsuccess\t32768\t32768\t13
BISHENGIR_UB_MODEL_BUFFER\t0\tkernel\t%buffer\t32768\t1\t10\t20\t0
BISHENGIR_UB_MODEL_VALIDATION_END\t0\t1
BISHENGIR_UB_MODEL_RESULT contract_version=1 status=success precision=exact overflow=false ub_peak_bits=32768 required_bits=32768 capacity_bits=1572864 selected_seed=13 decision_path=full_plan_after_non_overflow_upper_bound non_overflow_upper_bound_proven=true conservative_upper_bound_bits=65536 serialize_ns=1 model_ns=2 input_digest=x options_digest=y pipeline_fingerprint=pipeline-v1 diagnostic_category=none validation_id=0
PLANMEM_LIVENESS_ATTEMPT\tkernel\t0\t13
PLANMEM_EXACT_BUFFER\t13\t0\t32768\t6\t0\t10\t20
PLANMEM_EXACT_MULTI\t13\t0\t1
PLANMEM_LIVENESS_ATTEMPT_END\t13
PLANMEM_PLAN_ATTEMPT\tkernel\t13\tsuccess
PLANMEM_APPLIED_INPLACE_COUNT\t13\t0
PLANMEM_STORAGE\t13\t6\t32768\t32768\t0\t0\t1
PLANMEM_EXACT_PLANNED_BUFFER\t13\t6\t0\t32768\t0
PLANMEM_PEAK\t13\t6\t32768
PLANMEM_PLAN_ATTEMPT_END\t13
"""
assert MODULE.split_validation_segments(segment) == [segment]
payload = MODULE.parse_model_payload(segment)
assert payload["validation_id"] == "0"
assert payload["result"]["functions"][0]["buffers"][0]["offsets_bytes"] == [0]
assert payload["result"]["non_overflow_upper_bound_proven"] is True
assert payload["result"]["decision_path"] == \
    "full_plan_after_non_overflow_upper_bound"
assert payload["result"]["conservative_upper_bound_bits"] == 65536
assert payload["result"]["functions"], \
    "fast non-overflow validation must still materialize a full model plan"
status, differences, evidence = MODULE.compare_segment(segment, 13)
assert status == "matched", (differences, evidence)
summary = MODULE.summarize_observation(
    rows[0], Path("sample.ttadapter"), 13, segment, 0, False, 0.01
)
assert summary["non_overflow_upper_bound_proven"] == "true"
assert summary["decision_paths"] == \
    "full_plan_after_non_overflow_upper_bound"
assert summary["native_plan_memory_observed"] == "true"
assert summary["non_overflow_proof_verified"] == "true"

identity = MODULE.cache_identity(
    rows[0], ROOT / "data/adapter/ascend_tutorial_01-vector-add.ttadapter",
    13, "native-fingerprint"
)
assert len(identity["adapter_sha256"]) == 64
assert identity["native_oracle_fingerprint"] == "native-fingerprint"
assert "pipeline_fingerprint" not in identity
assert "BISHENGIR_UB_MODEL_RESULT" in MODULE.model_projection(segment)
assert "PLANMEM_PEAK" not in MODULE.model_projection(segment)
assert "BISHENGIR_UB_MODEL_RESULT" not in MODULE.native_projection(segment)
assert "PLANMEM_PEAK" in MODULE.native_projection(segment)
with tempfile.TemporaryDirectory(prefix="embedded-cache-test-") as temporary:
    path = Path(temporary) / "record.json.gz"
    MODULE.write_oracle_cache_record(path, identity, [segment], 0)
    record = MODULE.matching_cache_record(path, identity)
    assert record is not None
    synthetic = MODULE.model_projection(segment) + record["native_segments"][0]
    status, differences, evidence = MODULE.compare_segment(synthetic, 13)
    assert status == "matched", (differences, evidence)
    stale = dict(identity)
    stale["adapter_sha256"] = "changed"
    assert MODULE.matching_cache_record(path, stale) is None
    MODULE.write_oracle_cache_record(path, identity, [segment, segment], 0)
    assert MODULE.replayable_cache_record(path) is None

incorrectly_pruned = segment.replace(
    "decision_path=full_plan_after_non_overflow_upper_bound",
    "decision_path=non_overflow_upper_bound",
)
status, differences, _ = MODULE.compare_segment(incorrectly_pruned, 13)
assert status == "different"
assert differences == ["validation-pruned-after-non-overflow-proof"]

different = segment.replace(
    "PLANMEM_PEAK\t13\t6\t32768", "PLANMEM_PEAK\t13\t6\t65536"
)
status, differences, _ = MODULE.compare_segment(different, 13)
assert status == "different"
assert differences == ["peak"]

non_ub_failure = segment.replace(
    "PLANMEM_PLAN_ATTEMPT\tkernel\t13\tsuccess",
    "PLANMEM_PLAN_ATTEMPT\tkernel\t13\tfailure\n"
    "PLANMEM_REQUIRED\t13\t5\t999999",
)
status, _, evidence = MODULE.compare_segment(non_ub_failure, 13)
assert status == "unavailable"

# A retry attempt that fails outside the modelled UB scope provides no native
# evidence about the non-overflow proof.  It must be ignored rather than
# counted as a false proof when a later comparable attempt verifies it.
retry_summary = MODULE.summarize_observation(
    rows[0], Path("sample.ttadapter"), 13,
    non_ub_failure + segment, 0, False, 0.02,
)
assert retry_summary["status"] == "matched"
assert retry_summary["attempts"] == 2
assert retry_summary["comparable_attempts"] == 1
assert retry_summary["non_overflow_proof_verified"] == "true"
assert "non-UB" in evidence[0]

overflow = segment.replace(
    "\tkernel\tsuccess\t32768\t32768\t13",
    "\tkernel\toverflow\t2000000\t2000000\t13",
).replace(
    "status=success precision=exact overflow=false ub_peak_bits=32768 "
    "required_bits=32768",
    "status=overflow precision=exact overflow=true ub_peak_bits=2000000 "
    "required_bits=2000000",
).replace(
    "PLANMEM_PLAN_ATTEMPT\tkernel\t13\tsuccess",
    "PLANMEM_PLAN_ATTEMPT\tkernel\t13\tfailure\n"
    "PLANMEM_REQUIRED\t13\t6\t2000000",
).replace(
    "PLANMEM_EXACT_PLANNED_BUFFER\t13\t6\t0\t32768\t0\n", "",
).replace("PLANMEM_PEAK\t13\t6\t32768\n", "")
status, differences, evidence = MODULE.compare_segment(overflow, 13)
assert status == "matched", (differences, evidence)

cube_only = """\
BISHENGIR_UB_MODEL_VALIDATION_BEGIN\t0
BISHENGIR_UB_MODEL_VALIDATION_END\t0\t0
BISHENGIR_UB_MODEL_RESULT contract_version=1 status=success precision=exact overflow=false ub_peak_bits=0 required_bits=0 capacity_bits=1572864 selected_seed=unknown serialize_ns=1 model_ns=2 input_digest=x options_digest=y pipeline_fingerprint=pipeline-v1 diagnostic_category=none validation_id=0
PLANMEM_LIVENESS_ATTEMPT\tcube_kernel\t0\t13
PLANMEM_EXACT_BUFFER\t13\t0\t32768\t5\t0\t1\t2
PLANMEM_LIVENESS_ATTEMPT_END\t13
PLANMEM_PLAN_ATTEMPT\tcube_kernel\t13\tsuccess
PLANMEM_STORAGE\t13\t5\t32768\t32768\t0\t0\t1
PLANMEM_PEAK\t13\t5\t32768
PLANMEM_PLAN_ATTEMPT_END\t13
"""
status, differences, evidence = MODULE.compare_segment(cube_only, 13)
assert status == "matched", (differences, evidence)

aligned_failure = """\
BISHENGIR_UB_MODEL_VALIDATION_BEGIN\t0
BISHENGIR_UB_MODEL_DIAGNOSTIC\t0\tmodel_blocker\tTileCubeVectorLoop: scope.scope: Failed to collect vector loop tiling info
BISHENGIR_UB_MODEL_VALIDATION_END\t0\t0
BISHENGIR_UB_MODEL_RESULT contract_version=1 status=blocker precision=incomplete overflow=unknown ub_peak_bits=unknown required_bits=unknown capacity_bits=1572864 selected_seed=unknown serialize_ns=1 model_ns=2 input_digest=x options_digest=y pipeline_fingerprint=pipeline-v1 diagnostic_category=model_blocker validation_id=0
loc("test.mlir":1:1): error: 'scope.scope' op Failed to collect vector loop tiling info
"""
status, differences, evidence = MODULE.compare_segment(
    aligned_failure, 13, compiler_returncode=1
)
assert status == "matched", (differences, evidence)
assert "tiling.collect_vector_loop_info" in evidence[0]

unstable_abort = aligned_failure.split("loc(", 1)[0] + "Stack dump:\n"
status, differences, evidence = MODULE.compare_segment(
    unstable_abort, 13, compiler_returncode=-6
)
assert status == "unavailable", (differences, evidence)
assert "without a stable comparable diagnostic" in evidence[0]

status, differences, evidence = MODULE.compare_segment(aligned_failure, 13)
assert status == "different", (differences, evidence)
assert differences == ["precision"]

captured: dict[str, object] = {}


class Completed:
    stderr = segment
    returncode = 0


def fake_run(command: list[str], **kwargs: object) -> Completed:
    captured["command"] = command
    captured["env"] = kwargs["env"]
    return Completed()


original_run = MODULE.subprocess.run
try:
    MODULE.subprocess.run = fake_run
    observation = MODULE.execute_compiler(
        Path("/compiler"), Path("/input.ttadapter"), rows[0], 7, 10
    )
finally:
    MODULE.subprocess.run = original_run
env = captured["env"]
assert isinstance(env, dict)
assert env["BISHENGIR_UB_MODEL_VALIDATION"] == "1"
assert env["BISHENGIR_PLAN_MEMORY_FORCE_SEED"] == "7"
assert env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] == "1"
assert env["BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY"] == "1"
assert "BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION" not in env
assert observation["returncode"] == 0

captured.clear()
try:
    MODULE.subprocess.run = fake_run
    MODULE.execute_compiler(
        Path("/compiler"), Path("/input.ttadapter"), rows[0], 7, 10,
        stop_after_prediction=True,
    )
finally:
    MODULE.subprocess.run = original_run
env = captured["env"]
assert isinstance(env, dict)
assert env["BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION"] == "1"
assert "BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS" not in env
assert "BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY" not in env

print("[PASS] same-process BiSheng validation parser")
