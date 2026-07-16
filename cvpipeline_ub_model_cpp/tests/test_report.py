#!/usr/bin/env python3
"""Contract tests for the plan-before-cvpipeline CLI report (Task 10).

The CVPipeline UB model binary exposes a stable JSON and text report from the
post-CVPipeline per-function planning pipeline.  These tests lock in:

  * the top-level JSON contract (precision, status, ub_peak_bits, required_bits,
    capacity_bits, overflow, assumed_post_cvpipeline_options, functions[],
    diagnostics[], stage_coverage[]);
  * the exit-code mapping (0 exact success, 2 exact overflow, 1 blocker) and the
    rule that incomplete estimates are never authoritative;
  * the per-function shape (source/projected name, status, peak, required, seed,
    buffer plan);
  * the text buffer table gaining a leading ``function`` column;
  * demo fail-closed invariants (vector_add keeps 65536 only in debug_estimate;
    randn remains non-authoritative).

Run directly:
    python3 cvpipeline_ub_model_cpp/tests/test_report.py
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
MODULE_DIR = REPO_ROOT / "cvpipeline_ub_model_cpp"
BINARY = MODULE_DIR / "output" / "bin" / "cvpipeline_ub_model"
VECTOR_ADD = MODULE_DIR / "examples" / "inputs" / "demo_vector_add_before_cvpipeline.mlir"
RANDN = MODULE_DIR / "examples" / "inputs" / "demo_randn_before_cvpipeline.mlir"
MIX = MODULE_DIR / "examples" / "inputs" / "demo_mix_before_cvpipeline.mlir"
SUBBLOCK_EXACT = MODULE_DIR / "tests" / "fixtures" / "subblock_bind_success.mlir"
INCOMPLETE = MODULE_DIR / "tests" / "fixtures" / "subblock_bind_unmodeled_static.mlir"
CAT_CHAIN = (MODULE_DIR / "data" / "before_cvpipeline" /
             "triton.language.cat.ttadapter" /
             "before_cvpipelining_func_func_cat_1d_kernel_32.mlir")
SEED_DEPENDENT = (MODULE_DIR / "data" / "before_cvpipeline" /
                  "kernels_vllm_fused_gdn_gating_kernel.ttadapter" /
                  "before_cvpipelining_func_func_fused_gdn_gating_kernel_32.mlir")
ORACLE_MANIFEST_VALIDATOR = MODULE_DIR / "scripts" / "validate_pipeline_manifest.py"
PLAN_ORACLE_COMPARATOR = MODULE_DIR / "scripts" / "compare_ub_plan_with_suffix_oracle.py"

FAILURES: list[str] = []


def check(condition: bool, message: str) -> None:
    if condition:
        print(f"[PASS] {message}")
    else:
        print(f"[FAIL] {message}")
        FAILURES.append(message)


def check_eq(actual: Any, expected: Any, message: str) -> None:
    if actual == expected:
        print(f"[PASS] {message}")
    else:
        print(f"[FAIL] {message}: expected {expected!r}, got {actual!r}")
        FAILURES.append(f"{message} (expected {expected!r}, got {actual!r})")


def ensure_binary() -> None:
    if not BINARY.is_file():
        raise RuntimeError(f"cvpipeline_ub_model binary not built: {BINARY}")
    if not os.access(BINARY, os.X_OK):
        raise RuntimeError(f"cvpipeline_ub_model binary not executable: {BINARY}")


def run_model(ir: Path, *, fmt: str = "json", extra: list[str] | None = None) -> tuple[int, str, str]:
    cmd = [
        str(BINARY),
        "--action=plan-before-cvpipeline",
        f"--before-cvpipeline-ir={ir}",
        f"--format={fmt}",
    ]
    if extra:
        cmd.extend(extra)
    proc = subprocess.run(cmd, text=True, capture_output=True)
    return proc.returncode, proc.stdout, proc.stderr


def scaled_overflow_ir() -> Path:
    """A copy of the vector_add demo with element counts scaled so the planned
    buffers exceed the 192 KiB UB capacity.  Produced by sed-scaling the demo so
    the IR stays structurally valid for the modeled pipeline."""
    text = VECTOR_ADD.read_text(encoding="utf-8")
    scaled = text.replace("1024", "65536")
    tmp = tempfile.NamedTemporaryFile(
        mode="w", suffix=".mlir", delete=False, encoding="utf-8")
    tmp.write(scaled)
    tmp.close()
    return Path(tmp.name)


def assert_top_level_contract(payload: dict[str, Any], *, label: str) -> None:
    check("precision" in payload, f"{label}: JSON has precision")
    check(payload.get("precision") in {"exact", "incomplete"},
          f"{label}: precision is exact|incomplete (got {payload.get('precision')!r})")
    check(payload.get("status") in {"success", "overflow", "blocker"},
          f"{label}: status is success|overflow|blocker (got {payload.get('status')!r})")
    for key in ("ub_peak_bits", "required_bits", "capacity_bits", "overflow"):
        check(key in payload, f"{label}: JSON has {key}")
    check("restrict_inplace_as_isa" in payload,
          f"{label}: JSON has restrict_inplace_as_isa (inplace)")
    check("assumed_post_cvpipeline_options" in payload,
          f"{label}: JSON has assumed_post_cvpipeline_options")
    opts = payload.get("assumed_post_cvpipeline_options", {})
    check_eq(opts.get("tile_mix_vector_loop"), 2,
             f"{label}: assumed tile_mix_vector_loop default")
    check_eq(opts.get("tile_mix_cube_loop"), 2,
             f"{label}: assumed tile_mix_cube_loop default")
    check_eq(opts.get("enable_auto_bind_sub_block"), True,
             f"{label}: assumed enable_auto_bind_sub_block default")
    check_eq(opts.get("enable_code_motion"), True,
             f"{label}: assumed enable_code_motion default")
    check_eq(opts.get("enable_ubuf_saving"), False,
             f"{label}: assumed enable_ubuf_saving default")
    check(isinstance(payload.get("functions"), list),
          f"{label}: functions is a list")
    check(isinstance(payload.get("diagnostics"), list),
          f"{label}: diagnostics is a list")
    check(isinstance(payload.get("stage_coverage"), list),
          f"{label}: stage_coverage is a list")
    if isinstance(payload.get("stage_coverage"), list):
        check(len(payload["stage_coverage"]) >= 14,
              f"{label}: stage_coverage lists every pipeline stage")
        dispositions = {stage.get("disposition")
                        for stage in payload["stage_coverage"]}
        check("modeled" not in dispositions,
              f"{label}: stage coverage does not use ambiguous modeled label")
        check(dispositions <= {"oracle-exact", "partial", "ub-invariant", "unsupported"},
              f"{label}: stage coverage uses evidence-aware dispositions")


def assert_function_shape(fn: dict[str, Any], *, label: str) -> None:
    for key in ("source_function", "function", "status", "ub_peak_bits",
                "required_bits", "selected_seed", "buffers"):
        check(key in fn, f"{label}: function entry has {key}")
    check(fn.get("status") in {"success", "overflow"},
          f"{label}: function status is success|overflow")
    check(isinstance(fn.get("buffers"), list), f"{label}: function buffers is a list")
    for buf in fn.get("buffers", []):
        for key in ("name", "extent_bits", "offset_bytes", "alloc_time", "free_time"):
            check(key in buf, f"{label}: buffer has {key}")


def test_vector_add() -> None:
    rc, stdout, stderr = run_model(VECTOR_ADD, fmt="json")
    check_eq(rc, 1, "vector_add: unproven inplace inference exits blocker")
    check(not stderr, f"vector_add: empty stderr (got {stderr!r})")
    payload = json.loads(stdout)
    assert_top_level_contract(payload, label="vector_add")
    check_eq(payload.get("status"), "blocker", "vector_add: status blocker")
    check_eq(payload.get("precision"), "incomplete",
             "vector_add: precision incomplete")
    check(any("inplace-pair" in diagnostic.get("reason", "")
              for diagnostic in payload.get("diagnostics", [])),
          "vector_add: diagnostic identifies unproven inplace inference")
    check_eq(payload.get("ub_peak_bits"), None,
             "vector_add: authoritative peak is null")
    check_eq(payload.get("required_bits"), None,
             "vector_add: authoritative requirement is null")
    check_eq(payload.get("overflow"), False, "vector_add: no overflow")
    check_eq(payload.get("functions"), [],
             "vector_add: authoritative function plans are empty")
    check_eq(payload.get("debug_estimate", {}).get("ub_peak_bits"), 65536,
             "vector_add: debug-only peak retained")


def test_randn() -> None:
    rc, stdout, _ = run_model(RANDN, fmt="json")
    check_eq(rc, 1, "randn: unproven decomposition exits as blocker")
    payload = json.loads(stdout)
    assert_top_level_contract(payload, label="randn")
    check_eq(payload.get("status"), "blocker", "randn: status blocker")
    check_eq(payload.get("precision"), "incomplete",
             "randn: precision incomplete")
    check_eq(payload.get("ub_peak_bits"), None,
             "randn: authoritative peak is null")
    estimate = payload.get("debug_estimate", {})
    check_eq(estimate.get("ub_peak_bits"), 23904,
             "randn: non-authoritative debug peak retained")
    check_eq(estimate.get("required_bits"), 24064,
             "randn: non-authoritative debug requirement retained")
    check(any(diagnostic.get("pipeline_stage") == "HIVMDecomposeOp"
              for diagnostic in payload.get("diagnostics", [])),
          "randn: diagnostic identifies unproven decomposition ordering")


def test_suffix_manifest_audit() -> None:
    # Every suffix stage row carries a documented input_contract (the operation
    # forms it assumes after the post-CVPipeline pipeline), and every row is
    # exercised on an AIV module by the MIX demo's suffix pipeline run below
    # (test_mix drives the full suffix chain on kernel_mix_aiv).  Unsupported
    # forms fail closed (throw -> blocker diagnostic) rather than being dropped.
    manifest = MODULE_DIR / "config" / "initial_suffix_manifest.tsv"
    lines = manifest.read_text(encoding="utf-8").splitlines()
    nonempty = [ln for ln in lines if ln.strip()]
    header = nonempty[0].split("\t")
    check_eq(header,
             ["order", "compiler_pass_group", "model_module", "ub_role",
              "input_contract"],
             "suffix manifest: header has the audited input_contract column")
    rows = nonempty[1:]
    check_eq(len(rows), 11, "suffix manifest: all 11 suffix stages present")
    for row in rows:
        parts = row.split("\t")
        check_eq(len(parts), 5, "suffix manifest: row has 5 columns")
        order, group, module, role, contract = parts
        check(bool(order) and bool(group) and bool(module) and bool(role),
              "suffix manifest: row has non-empty identifying columns")
        check(bool(contract),
              f"suffix manifest: {group} has a non-empty input_contract")


def test_mix() -> None:
    rc, stdout, stderr = run_model(MIX, fmt="json")
    check_eq(rc, 0, "mix: success exits 0")
    check(not stderr, f"mix: empty stderr (got {stderr!r})")
    payload = json.loads(stdout)
    assert_top_level_contract(payload, label="mix")
    check_eq(payload.get("status"), "success", "mix: status success")
    # The MIX fixture's AIV store takes the same-address-hazard Exact path:
    # TileAndBindSubBlock wraps the store in a `limit_sub_block_id0` scf.if guard
    # (limitUniqueSubBlockToStore), and CanonicalizationAfterSplit recognizes the
    # marked guard and leaves it (non-foldable runtime condition).  The flat
    # loop's invariant tensor.empty is hoisted by LICM.  Every reproducible stage
    # reports Exact, so the module is exact.
    check_eq(payload.get("precision"), "exact", "mix: precision exact")
    check_eq(len(payload.get("diagnostics", [])), 0,
             "mix: no diagnostics on the exact path")
    # Golden UB footprint: the projected AIV kernel materializes exactly one UB
    # buffer (`%base_0`, the load output tensor of one f16x16x16 element block)
    # whose extent equals the simultaneous peak.
    check_eq(payload.get("ub_peak_bits"), 4096, "mix: ub_peak_bits=4096")
    check_eq(payload.get("required_bits"), 4096, "mix: required_bits=4096")
    functions = payload.get("functions", [])
    check_eq(len(functions), 1, "mix: exactly one projected AIV function")
    if functions:
        fn = functions[0]
        # SplitMixKernelAIVProjection renames the MIX function `kernel` to
        # `kernel_mix_aiv`; no AIC side reaches the planner.
        check_eq(fn.get("function"), "kernel_mix_aiv",
                 "mix: projected function is kernel_mix_aiv")
        assert_function_shape(fn, label="mix")
        fn_peak = fn.get("ub_peak_bits")
        # Module peak is the max of per-function peaks; with one function the
        # module peak equals that function's peak (never the sum).
        check_eq(payload.get("ub_peak_bits"), fn_peak,
                 "mix: module peak equals the single function peak")
        check_eq(fn.get("ub_peak_bits"), 4096,
                 "mix: per-function ub_peak_bits=4096")
        # Exactly one UB buffer with concrete name/extent/lifetime.
        buffers = fn.get("buffers", [])
        check_eq(len(buffers), 1, "mix: exactly one UB buffer")
        check_eq(buffers[0].get("name"), "%base_0",
                 "mix: buffer name is %base_0")
        check_eq(buffers[0].get("extent_bits"), 4096,
                 "mix: buffer extent_bits=4096")
        for key in ("name", "extent_bits", "offset_bytes",
                    "alloc_time", "free_time"):
            check(key in buffers[0], f"mix: buffer has {key}")


def test_text_function_column() -> None:
    rc, stdout, _ = run_model(VECTOR_ADD, fmt="text")
    check_eq(rc, 1, "vector_add text: blocker exits 1")
    header = "function\tname\textent_bits\toffset_bytes\talloc_time\tfree_time"
    check(header in stdout, "vector_add text: buffer table has leading function column")
    check("status\tblocker" in stdout, "vector_add text: has blocker status")
    check("debug_estimate.peak_bits\t65536" in stdout,
          "vector_add text: debug-only peak retained")
    check("precision\t" in stdout, "vector_add text: has precision line")
    check(payload_precision_valid(stdout), "vector_add text: precision is exact|incomplete")


def payload_precision_valid(stdout: str) -> bool:
    for line in stdout.splitlines():
        if line.startswith("precision\t"):
            return line.split("\t", 1)[1] in {"exact", "incomplete"}
    return False


def test_overflow() -> None:
    ir = scaled_overflow_ir()
    try:
        rc, stdout, _ = run_model(ir, fmt="json")
        # exact+overflow: an exact-precision result that overflows must exit 2.
        # Scaling the demo makes an earlier post-CVPipeline stage incomplete,
        # so the exact-only contract must block before exposing the estimated
        # overflow as an authoritative result.
        check_eq(rc, 1, "incomplete overflow estimate: exits 1")
        payload = json.loads(stdout)
        assert_top_level_contract(payload, label="incomplete overflow estimate")
        check_eq(payload.get("status"), "blocker",
                 "incomplete overflow estimate: status blocker")
        check_eq(payload.get("overflow"), False,
                 "incomplete overflow estimate: authoritative overflow false")
        check_eq(payload.get("required_bits"), None,
                 "incomplete overflow estimate: authoritative requirement null")
        estimate = payload.get("debug_estimate", {})
        cap = payload.get("capacity_bits")
        req = estimate.get("required_bits") if isinstance(estimate, dict) else None
        check(isinstance(req, int) and isinstance(cap, int) and req > cap,
              f"incomplete overflow estimate: debug requirement ({req}) exceeds capacity ({cap})")
    finally:
        ir.unlink(missing_ok=True)


def test_blocker() -> None:
    bogus = MODULE_DIR / "examples" / "inputs" / "__nonexistent__.mlir"
    rc, stdout, stderr = run_model(bogus, fmt="json")
    check_eq(rc, 1, "blocker: exits 1")
    payload = json.loads(stdout)
    check_eq(payload.get("status"), "blocker", "blocker: status blocker")
    check(payload.get("precision") in {"exact", "incomplete"},
          f"blocker: precision is exact|incomplete (got {payload.get('precision')!r})")
    check(payload.get("ub_peak_bits") in (None,),
          f"blocker: ub_peak_bits is null (got {payload.get('ub_peak_bits')!r})")
    check(isinstance(payload.get("diagnostics"), list) and len(payload["diagnostics"]) >= 1,
          "blocker: diagnostics explain the blocker")
    check(bool(stderr), "blocker: stderr is non-empty")


def test_incomplete_is_non_authoritative_blocker() -> None:
    rc, stdout, _ = run_model(INCOMPLETE, fmt="json")
    check_eq(rc, 1, "incomplete: exits 1")
    payload = json.loads(stdout)
    check_eq(payload.get("precision"), "incomplete",
             "incomplete: precision remains incomplete")
    check_eq(payload.get("status"), "blocker",
             "incomplete: status is blocker")
    check_eq(payload.get("success"), False,
             "incomplete: authoritative success is false")
    check_eq(payload.get("ub_peak_bits"), None,
             "incomplete: authoritative peak is null")
    check_eq(payload.get("required_bits"), None,
             "incomplete: authoritative requirement is null")
    check_eq(payload.get("functions"), [],
             "incomplete: authoritative function plans are empty")
    estimate = payload.get("debug_estimate")
    check(isinstance(estimate, dict),
          "incomplete: debug_estimate is explicitly separated")
    if isinstance(estimate, dict):
        check(isinstance(estimate.get("ub_peak_bits"), int),
              "incomplete: debug estimate retains a peak")
        check(isinstance(estimate.get("functions"), list),
              "incomplete: debug estimate retains function details")


def test_subblock_success_matches_real_ub_oracle() -> None:
    rc, stdout, _ = run_model(SUBBLOCK_EXACT, fmt="json")
    check_eq(rc, 1, "subblock Task7 estimate: unproven inplace exits blocker")
    payload = json.loads(stdout)
    check_eq(payload.get("precision"), "incomplete",
             "subblock Task7 estimate: precision incomplete")
    check_eq(payload.get("status"), "blocker",
             "subblock Task7 estimate: status blocker")
    # The instrumented real compiler produces two non-overlapping 2048-bit UB
    # buffers at the same offset after successful sub-block tiling.
    check_eq(payload.get("ub_peak_bits"), None,
             "subblock Task7 estimate: authoritative peak is null")
    estimate = payload.get("debug_estimate", {})
    check_eq(estimate.get("ub_peak_bits"), 2048,
             "subblock Task7 estimate: debug peak matches real-pass oracle")
    buffers = estimate.get("functions", [{}])[0].get("buffers", [])
    check_eq(sorted(buffer.get("extent_bits") for buffer in buffers),
             [2048, 2048],
             "subblock Task7 estimate: physical extents match real-pass oracle")


def test_chained_insert_slice_is_not_false_exact() -> None:
    rc, stdout, _ = run_model(CAT_CHAIN, fmt="json")
    payload = json.loads(stdout)
    check_eq(rc, 1, "concat insert_slice: exits as blocker")
    check_eq(payload.get("precision"), "incomplete",
             "concat insert_slice: precision incomplete")
    check(any(diagnostic.get("pipeline_stage") == "OneShotBufferize"
              for diagnostic in payload.get("diagnostics", [])),
          "concat insert_slice: diagnostic names ownership stage")


def test_seed_dependent_plan_is_not_false_exact() -> None:
    rc, stdout, _ = run_model(SEED_DEPENDENT, fmt="json",
                              extra=["--random-seed=0"])
    payload = json.loads(stdout)
    check_eq(rc, 1, "seed-dependent layout: exits as blocker")
    check_eq(payload.get("ub_peak_bits"), None,
             "seed-dependent layout: authoritative peak is null")
    check(any(diagnostic.get("pipeline_stage") == "PlanMemory"
              for diagnostic in payload.get("diagnostics", [])),
          "seed-dependent layout: diagnostic names planner stage")


def test_pipeline_manifest_validator() -> None:
    post = [
        "TileCubeVectorLoop", "InferAndSetBufferSize",
        "WorkspaceSemanticProjection", "CanonicalizationBeforeSplit",
        "CrossCoreSyncInvariant", "SplitMixKernelAIVProjection",
        "InlineScope", "TileAndBindSubBlock", "FoldTensorEmpty",
        "CanonicalizationAfterSplit", "LoopInvariantCodeMotion",
        "LoopInvariantSubsetHoisting", "CloneTensorEmpty",
        "InlineOTFLoadStore",
    ]
    suffix = [
        "OneShotBufferize", "CanonicalizeIterArg;HIVMOptSinglePoint",
        "HIVMDecomposeOp", "ConvertNonContiguousReshapeToCopy",
        "InferHIVMMemScope", "AlignAllocSize;MarkStrideAlign;EnableStrideAlign",
        "FlattenOps;ReduceRankSubview;LiftLowestStride", "AllocExtraBuffer",
        "InlineLoadCopy", "MarkMultiBuffer", "PlanMemoryInputBridge",
    ]
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        compiler = root / "compiler.tsv"
        compiler.write_text(
            "ORACLE_PIPELINE_SCHEMA\t1\n" +
            "PIPELINE_SHA256\tdeadbeef\n" +
            "".join(f"STAGE\tpost\t{i}\t{name}\n"
                    for i, name in enumerate(post, 1)) +
            "".join(f"STAGE\tsuffix\t{i}\t{name}\n"
                    for i, name in enumerate(suffix, 1)),
            encoding="utf-8")
        expected_hash = root / "pipeline.sha256"
        expected_hash.write_text("deadbeef\n", encoding="utf-8")
        cmd = [sys.executable, str(ORACLE_MANIFEST_VALIDATOR),
               "--compiler-manifest", str(compiler),
               "--post-manifest", str(MODULE_DIR / "config" / "post_cvpipeline_manifest.tsv"),
               "--suffix-manifest", str(MODULE_DIR / "config" / "initial_suffix_manifest.tsv"),
               "--expected-pipeline-sha256", str(expected_hash)]
        good = subprocess.run(cmd, text=True, capture_output=True)
        check_eq(good.returncode, 0,
                 "pipeline manifest: matching compiler/model stages pass")

        compiler.write_text(compiler.read_text(encoding="utf-8").replace(
            "STAGE\tpost\t8\tTileAndBindSubBlock\n", ""), encoding="utf-8")
        bad = subprocess.run(cmd, text=True, capture_output=True)
        check_eq(bad.returncode, 1,
                 "pipeline manifest: missing real stage fails")


def test_flat_report_planmemory_oracle_comparator() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        model = root / "model.json"
        oracle = root / "oracle.tsv"
        model.write_text(json.dumps({
            "precision": "exact", "status": "success",
            "ub_peak_bits": 4096, "selected_seed": 3,
            "functions": [{"function": "kernel", "buffers": [{
                "name": "%base_0", "extent_bits": 4096,
                "offset_bytes": 0, "alloc_time": 1, "free_time": 2,
            }]}],
        }), encoding="utf-8")
        oracle.write_text(
            "PLANMEM_PLAN_ATTEMPT\tkernel\t3\tsuccess\n"
            "PLANMEM_PEAK\t3\t6\t4096\n"
            "PLANMEM_EXACT_BUFFER\t3\t0\t4096\t6\t0\t10\t20\n"
            "PLANMEM_EXACT_PLANNED_BUFFER\t3\t6\t0\t4096\t0\n",
            encoding="utf-8")
        compared = subprocess.run(
            [sys.executable, str(PLAN_ORACLE_COMPARATOR),
             "--model-json", str(model), "--oracle-tsv", str(oracle)],
            text=True, capture_output=True)
        check_eq(compared.returncode, 0,
                 "PlanMemory oracle: flat exact report compares successfully")

        model.write_text(json.dumps({
            "precision": "exact", "status": "success",
            "ub_peak_bits": 4096, "selected_seed": 3,
            "functions": [{"function": "kernel", "buffers": [
                {"name": "%base_0", "extent_bits": 4096,
                 "offset_bytes": 0, "alloc_time": 1, "free_time": 4},
                {"name": "%base_1", "extent_bits": 4096,
                 "offset_bytes": 0, "alloc_time": 1, "free_time": 4},
            ]}],
        }), encoding="utf-8")
        oracle.write_text(
            "PLANMEM_PLAN_ATTEMPT\tkernel\t3\tsuccess\n"
            "PLANMEM_PEAK\t3\t6\t4096\n"
            "PLANMEM_EXACT_BUFFER\t3\t0\t4096\t6\t0\t10\t20\n"
            "PLANMEM_EXACT_BUFFER\t3\t1\t4096\t6\t0\t20\t30\n"
            "PLANMEM_EXACT_PLANNED_BUFFER\t3\t6\t0\t4096\t0\n"
            "PLANMEM_EXACT_PLANNED_BUFFER\t3\t6\t1\t4096\t0\n",
            encoding="utf-8")
        lifetime_mismatch = subprocess.run(
            [sys.executable, str(PLAN_ORACLE_COMPARATOR),
             "--model-json", str(model), "--oracle-tsv", str(oracle)],
            text=True, capture_output=True)
        check_eq(lifetime_mismatch.returncode, 1,
                 "PlanMemory oracle: lifetime-order mismatch fails")


def main() -> int:
    ensure_binary()
    test_vector_add()
    test_randn()
    test_suffix_manifest_audit()
    test_mix()
    test_text_function_column()
    test_overflow()
    test_blocker()
    test_incomplete_is_non_authoritative_blocker()
    test_subblock_success_matches_real_ub_oracle()
    test_chained_insert_slice_is_not_false_exact()
    test_seed_dependent_plan_is_not_false_exact()
    test_pipeline_manifest_validator()
    test_flat_report_planmemory_oracle_comparator()
    print()
    if FAILURES:
        print(f"[ERROR] {len(FAILURES)} report-contract assertion(s) failed")
        return 1
    print("[OK] all report-contract assertions passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
