#!/usr/bin/env python3
"""Contract tests for the plan-before-cvpipeline CLI report (Task 10).

The CVPipeline UB model binary exposes a stable JSON and text report from the
post-CVPipeline per-function planning pipeline.  These tests lock in:

  * the top-level JSON contract (precision, status, ub_peak_bits, required_bits,
    capacity_bits, overflow, assumed_post_cvpipeline_options, functions[],
    diagnostics[], stage_coverage[]);
  * the exit-code mapping (0 success, 2 overflow, 1 blocker) and the orthogonality
    of precision (exact/incomplete) to the status-derived exit code;
  * the per-function shape (source/projected name, status, peak, required, seed,
    buffer plan);
  * the text buffer table gaining a leading ``function`` column;
  * the demo invariants (vector_add ub_peak_bits=65536, randn 23904/24064).

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
    check_eq(rc, 0, "vector_add: success exits 0")
    check(not stderr, f"vector_add: empty stderr (got {stderr!r})")
    payload = json.loads(stdout)
    assert_top_level_contract(payload, label="vector_add")
    # precision is orthogonal to status: whatever precision the demo reports, a
    # success must still exit 0 (incomplete+success is valid).
    check_eq(payload.get("status"), "success", "vector_add: status success")
    check_eq(payload.get("ub_peak_bits"), 65536, "vector_add: ub_peak_bits=65536")
    check_eq(payload.get("required_bits"), 65536, "vector_add: required_bits=65536")
    check_eq(payload.get("overflow"), False, "vector_add: no overflow")
    functions = payload.get("functions", [])
    check(len(functions) >= 1, "vector_add: at least one function")
    for fn in functions:
        assert_function_shape(fn, label="vector_add")
    # The leading function is the projected AIV kernel for vector_add.
    if functions:
        first = functions[0]
        check(bool(first.get("function")), "vector_add: function name is non-empty")
        check(first.get("ub_peak_bits") == 65536,
              f"vector_add: per-function ub_peak_bits=65536 (got {first.get('ub_peak_bits')})")


def test_randn() -> None:
    rc, stdout, _ = run_model(RANDN, fmt="json")
    check_eq(rc, 0, "randn: success exits 0")
    payload = json.loads(stdout)
    assert_top_level_contract(payload, label="randn")
    check_eq(payload.get("status"), "success", "randn: status success")
    # randn plans a single AIV kernel whose raw simultaneous peak (23904) is
    # smaller than its total required footprint (24064).  The report must keep
    # both numbers distinct rather than collapsing them.
    check_eq(payload.get("ub_peak_bits"), 23904, "randn: ub_peak_bits=23904")
    check_eq(payload.get("required_bits"), 24064, "randn: required_bits=24064")
    check(payload.get("ub_peak_bits") != payload.get("required_bits"),
          "randn: peak and required remain distinct (module max, not collapsed)")


def test_text_function_column() -> None:
    rc, stdout, _ = run_model(VECTOR_ADD, fmt="text")
    check_eq(rc, 0, "vector_add text: success exits 0")
    header = "function\tname\textent_bits\toffset_bytes\talloc_time\tfree_time"
    check(header in stdout, "vector_add text: buffer table has leading function column")
    in_table = False
    seen_rows = 0
    for line in stdout.splitlines():
        if line == header:
            in_table = True
            continue
        if in_table and line.strip():
            parts = line.split("\t")
            check_eq(len(parts), 6,
                     "vector_add text: buffer row has 6 tab-separated columns")
            check(bool(parts[0]), "vector_add text: function column is non-empty")
            seen_rows += 1
    check(seen_rows >= 1, "vector_add text: at least one buffer row emitted")
    check("status\tsuccess" in stdout, "vector_add text: has status line")
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
        check_eq(rc, 2, "overflow: exits 2")
        payload = json.loads(stdout)
        assert_top_level_contract(payload, label="overflow")
        check_eq(payload.get("status"), "overflow", "overflow: status overflow")
        check_eq(payload.get("overflow"), True, "overflow: overflow flag true")
        cap = payload.get("capacity_bits")
        req = payload.get("required_bits")
        check(isinstance(req, int) and isinstance(cap, int) and req > cap,
              f"overflow: required ({req}) exceeds capacity ({cap})")
        # precision must not be 'blocked' and must remain exact|incomplete even
        # when the result overflows.
        check(payload.get("precision") in {"exact", "incomplete"},
              f"overflow: precision stays exact|incomplete (got {payload.get('precision')!r})")
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


def main() -> int:
    ensure_binary()
    test_vector_add()
    test_randn()
    test_text_function_column()
    test_overflow()
    test_blocker()
    print()
    if FAILURES:
        print(f"[ERROR] {len(FAILURES)} report-contract assertion(s) failed")
        return 1
    print("[OK] all report-contract assertions passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
