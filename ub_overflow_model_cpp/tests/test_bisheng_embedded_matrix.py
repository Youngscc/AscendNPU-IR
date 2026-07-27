#!/usr/bin/env python3
"""Unit checks for same-process model/PlanMemory validation parsing."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import sys


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
assert len(rows) == 27
assert len(MODULE.load_known_timeout_pairs(MODULE.DEFAULT_TIMEOUT_PAIRS)) == 66
assert len(MODULE.load_adapters(MODULE.DEFAULT_ADAPTER_ROOT, set(), 0)) == 160
assert len(MODULE.load_adapters(
    MODULE.DEFAULT_ADAPTER_ROOT,
    {"triton.language.extra.cann.extension.index_put.ttadapter"}, 0,
)) == 1
arguments = MODULE.compiler_arguments(rows[0])
assert "--enable-ub-overflow-prediction=true" in arguments
assert "--prune-predicted-ub-overflow=false" in arguments
assert "--enable-triton-kernel-compile" in arguments
assert "--limit-auto-multi-buffer-only-for-local-buffer=false" in arguments

segment = """\
BISHENGIR_UB_MODEL_VALIDATION_BEGIN\t0
BISHENGIR_UB_MODEL_FUNCTION\t0\tkernel\tsuccess\t32768\t32768\t13
BISHENGIR_UB_MODEL_BUFFER\t0\tkernel\t%buffer\t32768\t1\t10\t20\t0
BISHENGIR_UB_MODEL_VALIDATION_END\t0\t1
BISHENGIR_UB_MODEL_RESULT contract_version=1 status=success precision=exact overflow=false ub_peak_bits=32768 required_bits=32768 capacity_bits=1572864 selected_seed=13 serialize_ns=1 model_ns=2 input_digest=x options_digest=y diagnostic_category=none validation_id=0
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
status, differences, evidence = MODULE.compare_segment(segment, 13)
assert status == "matched", (differences, evidence)

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
BISHENGIR_UB_MODEL_RESULT contract_version=1 status=success precision=exact overflow=false ub_peak_bits=0 required_bits=0 capacity_bits=1572864 selected_seed=unknown serialize_ns=1 model_ns=2 input_digest=x options_digest=y diagnostic_category=none validation_id=0
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
BISHENGIR_UB_MODEL_RESULT contract_version=1 status=blocker precision=incomplete overflow=unknown ub_peak_bits=unknown required_bits=unknown capacity_bits=1572864 selected_seed=unknown serialize_ns=1 model_ns=2 input_digest=x options_digest=y diagnostic_category=model_blocker validation_id=0
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

print("[PASS] same-process BiSheng validation parser")
