#!/usr/bin/env python3

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODEL = ROOT / "ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model"
FIXTURE = ROOT / "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir"
VECTOR_ADD = (
    ROOT
    / "ub_overflow_model_cpp/examples/inputs/demo_vector_add_before_cvpipelining.mlir"
)
ATTENTION_OVERFLOW = (
    ROOT
    / "ub_overflow_model_cpp/data/before_cvpipelining/attn_fwd.ttadapter/before_cvpipelining_func_func_attn_fwd_32.mlir"
)
SUBSET_HOISTING = (
    ROOT
    / "ub_overflow_model_cpp/data/before_cvpipelining/"
      "triton.language.extra.cann.extension.get_element.ttadapter/"
      "before_cvpipelining_func_func_get_element_test_kernel_32.mlir"
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

timed = subprocess.run(
    [
        str(MODEL),
        f"--before-cvpipelining-ir={VECTOR_ADD}",
        "--format=json",
        "--random-seed=0",
        "--show-runtime-timing",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert timed.returncode == 0, timed.stderr
json.loads(timed.stdout)
timing_rows = [
    line.split("\t") for line in timed.stderr.splitlines()
    if line.startswith("CVPIPELINE_TIMING\t")
]
assert all(len(row) == 7 for row in timing_rows), timing_rows
assert sum(row[3] == "TOTAL" for row in timing_rows) == 1, timing_rows
stage_names = {row[4] for row in timing_rows if row[3] == "STAGE"}
assert {"ParseGenericIR", "CVPipelining", "MarkMultiBuffer", "PlanMemory"} <= \
    stage_names, stage_names
assert all(int(row[6]) >= 0 for row in timing_rows), timing_rows

print("[PASS] opt-in model runtime timing preserves JSON and reports stages")

attention_overflow = subprocess.run(
    [
        str(MODEL),
        f"--before-cvpipelining-ir={ATTENTION_OVERFLOW}",
        "--format=json",
        "--random-seed=0",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert attention_overflow.returncode == 2, attention_overflow
attention_report = json.loads(attention_overflow.stdout)
assert attention_report["precision"] == "exact", attention_report
assert attention_report["status"] == "overflow", attention_report
assert attention_report["ub_peak_bits"] == 1716224, attention_report
assert attention_report["required_bits"] == 1716224, attention_report
assert len(attention_report["functions"]) == 1, attention_report

print("[PASS] dynamic alignment produces an exact overflow plan")

attention_seed_nine = subprocess.run(
    [
        str(MODEL),
        f"--before-cvpipelining-ir={ATTENTION_OVERFLOW}",
        "--format=json",
        "--random-seed=9",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert attention_seed_nine.returncode == 2, attention_seed_nine
attention_seed_nine_report = json.loads(attention_seed_nine.stdout)
assert attention_seed_nine_report["precision"] == "exact", \
    attention_seed_nine_report
assert attention_seed_nine_report["status"] == "overflow", \
    attention_seed_nine_report
assert attention_seed_nine_report["ub_peak_bits"] == 1716224, \
    attention_seed_nine_report
seed_nine_function = attention_seed_nine_report["functions"][0]
assert len(seed_nine_function["buffers"]) == 44, attention_seed_nine_report
seed_nine_buffers = {
    buffer["name"]: buffer for buffer in seed_nine_function["buffers"]
}
assert seed_nine_buffers["%base_0"]["free_time"] == 222, \
    attention_seed_nine_report
assert seed_nine_buffers["%base_29"]["offsets_bytes"] == [0], \
    attention_seed_nine_report
assert ["%base_18", "%base_8"] in seed_nine_function["inplace_pairs"], \
    attention_seed_nine_report

print("[PASS] non-default seed preserves workspace-load liveness and inplace")

triton_attention_overflow = subprocess.run(
    [
        str(MODEL),
        f"--before-cvpipelining-ir={ATTENTION_OVERFLOW}",
        "--format=json",
        "--random-seed=0",
        "--enable-triton-kernel-compile",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert triton_attention_overflow.returncode == 2, triton_attention_overflow
triton_attention_report = json.loads(triton_attention_overflow.stdout)
assert triton_attention_report["precision"] == "exact", triton_attention_report
assert triton_attention_report["status"] == "overflow", triton_attention_report
assert triton_attention_report["ub_peak_bits"] == 1716224, triton_attention_report
assert triton_attention_report["required_bits"] == 1716224, triton_attention_report
assert len(triton_attention_report["functions"]) == 1, triton_attention_report
assert len(triton_attention_report["functions"][0]["buffers"]) == 45, \
    triton_attention_report

print("[PASS] Triton dynamic stride alignment produces an exact overflow plan")

subset_hoisting = subprocess.run(
    [
        str(MODEL),
        f"--before-cvpipelining-ir={SUBSET_HOISTING}",
        "--format=json",
        "--random-seed=0",
    ],
    text=True,
    capture_output=True,
    check=False,
)
assert subset_hoisting.returncode == 0, subset_hoisting
subset_report = json.loads(subset_hoisting.stdout)
assert subset_report["precision"] == "exact", subset_report
assert subset_report["status"] == "success", subset_report
assert subset_report["ub_peak_bits"] == 177408, subset_report
assert subset_report["required_bits"] == 177408, subset_report
assert len(subset_report["functions"]) == 1, subset_report

print("[PASS] subset-hoisting input produces an exact plan")

wrapped = subprocess.run(
    [
        sys.executable,
        str(ROOT / "ub_overflow_model_cpp/scripts/plan_before_cvpipelining_ub.py"),
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
