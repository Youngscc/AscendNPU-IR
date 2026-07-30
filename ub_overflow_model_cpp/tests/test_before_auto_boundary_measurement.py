#!/usr/bin/env python3
"""Unit checks for before-AutoBlockify boundary timing reports."""

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/measure_before_auto_boundary.py"
SPEC = importlib.util.spec_from_file_location(
    "measure_before_auto_boundary", SCRIPT
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)

stderr = """\
BISHENGIR_UB_MODEL_RESULT status=success precision=exact overflow=false decision_path=full_plan model_ns=100
BISHENGIR_UB_NATIVE_RANGE_TIME start=before_autoblockify end=after_local_plan_memory ns=400
      123456  maximum resident set size
"""
assert MODULE.first_record(stderr, MODULE.MACHINE_PREFIX)["model_ns"] == "100"
assert MODULE.first_record(stderr, MODULE.NATIVE_PREFIX)["ns"] == "400"
assert len(MODULE.records(stderr + stderr, MODULE.MACHINE_PREFIX)) == 2
assert MODULE.max_rss(stderr) == 123456
assert not hasattr(MODULE, "INACTIVE_SCENARIOS")

old_environment = dict(os.environ)
try:
    os.environ["BISHENGIR_UB_MODEL_VALIDATION"] = "1"
    os.environ["BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION"] = "1"
    environment = MODULE.measurement_environment(collect_stage_timings=True)
finally:
    os.environ.clear()
    os.environ.update(old_environment)
assert "BISHENGIR_UB_MODEL_VALIDATION" not in environment
assert "BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION" not in environment
assert environment["BISHENGIR_UB_MODEL_FORCE_FULL_PLAN"] == "1"
assert environment["BISHENGIR_UB_NATIVE_RANGE_TIMING"] == "1"
assert environment["BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY"] == "1"
assert environment["BISHENGIR_UB_MODEL_STAGE_TIMING"] == "1"
skip_environment = MODULE.measurement_environment(skip_native=True)
assert skip_environment["BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION"] == "1"
assert "BISHENGIR_UB_NATIVE_RANGE_TIMING" not in skip_environment
assert "BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY" not in skip_environment

rows = [
    {
        "round": 0,
        "scenario": "default",
        "adapter": "a.ttadapter",
        "model_observed": "true",
        "model_attempts": 1,
        "native_observed": "true",
        "native_attempts": 1,
        "native_complete": "true",
        "native_skipped_known_timeout": "false",
        "full_plan_verified": "true",
        "status": "success",
        "decision_path": "full_plan",
        "model_ns": 100,
        "native_range_ns": 400,
        "stage_timings": (
            "ImportMLIRModule#0=5;PreCV.AutoBlockify#0=10;"
            "CVToPlanMemoryPipeline#0=20;PlanMemory#0=15"
        ),
        "process_wall_ns": 1000,
        "max_rss_bytes": 2000,
        "compiler_rc": 0,
        "timed_out": "false",
    },
    {
        "round": 0,
        "scenario": "default",
        "adapter": "b.ttadapter",
        "model_observed": "true",
        "model_attempts": 2,
        "native_observed": "false",
        "native_attempts": 0,
        "native_complete": "false",
        "native_skipped_known_timeout": "true",
        "full_plan_verified": "true",
        "status": "overflow,success",
        "decision_path": "full_plan,full_plan",
        "model_ns": 200,
        "native_range_ns": 0,
        "stage_timings": (
            "ImportMLIRModule#0=6;PreCV.AutoBlockify#0=15;"
            "CVToPlanMemoryPipeline#0=25;PlanMemory#0=18"
        ),
        "process_wall_ns": 2000,
        "max_rss_bytes": 3000,
        "compiler_rc": 0,
        "timed_out": "false",
    },
]
summary = MODULE.summarize(rows, 1)
assert summary["model_full_plan_samples"] == 2
assert summary["paired_native_samples"] == 1
assert summary["model_unavailable"] == 0
assert summary["known_native_timeout_skipped"] == 1
assert summary["native_unavailable"] == 0
assert summary["bisheng_over_model_aggregate"] == 4.0
assert summary["peak_rss_bytes"] == 3000
assert summary["pre_cv_prefix_stage_total_ns"] == 25
assert summary["input_bridge_stage_total_ns"] == 11
assert summary["cv_to_plan_memory_stage_total_ns"] == 45
assert summary["model_attempt_statuses"] == {"overflow": 1, "success": 2}
assert summary["decision_paths"] == {"full_plan": 3}

with tempfile.TemporaryDirectory() as temporary:
    report = Path(temporary) / "report.tsv"
    report.write_text("\t".join(MODULE.COLUMNS) + "\n", encoding="utf-8")
    assert MODULE.load_completed_rows(report) == []

print("[PASS] before-AutoBlockify boundary measurement helpers")
