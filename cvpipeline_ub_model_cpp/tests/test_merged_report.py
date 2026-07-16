#!/usr/bin/env python3

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODEL = ROOT / "cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model"
FIXTURE = ROOT / "cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir"
VECTOR_ADD = (
    ROOT
    / "cvpipeline_ub_model_cpp/examples/inputs/demo_vector_add_before_cvpipelining.mlir"
)
DYNAMIC_ALIGN = (
    ROOT
    / "cvpipeline_ub_model_cpp/data/before_cvpipelining/attn_fwd.ttadapter/before_cvpipelining_func_func_attn_fwd_32.mlir"
)


completed = subprocess.run(
    [
        str(MODEL),
        "--debug",
        "--debug-entry=after-cvpipelining",
        f"--after-cvpipelining-ir={FIXTURE}",
        "--format=json",
        "--random-seed=0",
    ],
    text=True,
    capture_output=True,
    check=False,
)

assert completed.returncode == 0, completed.stderr
report = json.loads(completed.stdout)
assert report["precision"] == "exact", report
assert report["status"] == "success", report
assert report["ub_peak_bits"] == 98304, report
assert report["required_bits"] == 98304, report
assert [item["function"] for item in report["functions"]] == [
    "small_aiv",
    "large_aiv",
], report
assert [item["ub_peak_bits"] for item in report["functions"]] == [
    65536,
    98304,
], report

print("[PASS] merged CLI reports independent AIV plans")

inplace = subprocess.run(
    [
        str(MODEL),
        f"--before-cvpipelining-ir={VECTOR_ADD}",
        "--format=json",
        "--random-seed=0",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert inplace.returncode == 0, inplace
inplace_report = json.loads(inplace.stdout)
assert inplace_report["precision"] == "exact", inplace_report
assert inplace_report["status"] == "success", inplace_report
assert inplace_report["ub_peak_bits"] == 65536, inplace_report
assert inplace_report["required_bits"] == 65536, inplace_report
assert inplace_report["functions"][0]["inplace_pairs"], inplace_report
assert [
    (buffer["alloc_time"], buffer["free_time"])
    for buffer in inplace_report["functions"][0]["buffers"]
] == [(25, 39), (34, 39), (39, 43)], inplace_report

print("[PASS] default PlanMemory inplace pairs remain exact")

exception_blocked = subprocess.run(
    [
        str(MODEL),
        f"--before-cvpipelining-ir={DYNAMIC_ALIGN}",
        "--format=json",
        "--random-seed=0",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert exception_blocked.returncode == 1, exception_blocked
exception_report = json.loads(exception_blocked.stdout)
assert exception_report["precision"] == "incomplete", exception_report
assert exception_report["status"] == "blocker", exception_report
assert exception_report["ub_peak_bits"] is None, exception_report
assert exception_report["required_bits"] is None, exception_report
assert exception_report["functions"] == [], exception_report

print("[PASS] pass exceptions obey the blocker contract")

wrapped = subprocess.run(
    [
        sys.executable,
        str(ROOT / "cvpipeline_ub_model_cpp/scripts/plan_before_cvpipelining_ub.py"),
        f"--before-cvpipelining-ir={VECTOR_ADD}",
        "--format=json",
        "--random-seed=0",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert wrapped.returncode == 0, wrapped
wrapped_report = json.loads(wrapped.stdout)
assert wrapped_report["result"]["precision"] == "exact", wrapped_report
assert wrapped_report["result"]["functions"][0]["inplace_pairs"], wrapped_report
assert wrapped_report["result"]["plan"], wrapped_report
assert wrapped_report["result"]["peak_bits"] == 65536, wrapped_report

print("[PASS] Python wrapper preserves exact inplace reports")
