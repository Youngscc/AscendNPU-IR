#!/usr/bin/env python3
"""Pretty-print a UB plan JSON report (plan-before-cvpipeline) for terminal demos.

The CLI emits a flat top-level contract: status, precision, ub_peak_bits,
required_bits, capacity_bits, assumed_post_cvpipeline_options, functions[]
(each with buffers[]), diagnostics[], stage_coverage[]. There is no "result"
or "options" wrapper.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Print a compact terminal summary for a UB plan JSON file.")
    parser.add_argument("json_file", type=Path)
    parser.add_argument("--max-buffers", type=int, default=12)
    parser.add_argument("--html", default="cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html")
    return parser.parse_args()


def bit_size(bits: int | float | None) -> str:
    value = int(bits or 0)
    if value >= 8 * 1024 * 1024:
        return f"{value / 8 / 1024 / 1024:.2f} MiB"
    if value >= 8 * 1024:
        return f"{value / 8 / 1024:.2f} KiB"
    return f"{value} bits"


def byte_size(value: int | float | None) -> str:
    size = int(value or 0)
    if size >= 1024 * 1024:
        return f"{size / 1024 / 1024:.2f} MiB"
    if size >= 1024:
        return f"{size / 1024:.2f} KiB"
    return f"{size} B"


def bool_text(value: Any) -> str:
    if isinstance(value, bool):
        return "yes" if value else "no"
    return str(value)


def row(columns: list[str], widths: list[int]) -> str:
    return "  ".join(text.ljust(width) for text, width in zip(columns, widths))


def divider(width: int = 76) -> str:
    return "-" * width


def main() -> int:
    args = parse_args()
    payload = json.loads(args.json_file.read_text(encoding="utf-8"))

    # Top-level contract (flat — no "result" wrapper).
    status = payload.get("status")
    if not status:
        status = "overflow" if payload.get("overflow") else (
            "success" if payload.get("success", False) else "blocker")
    status = str(status).upper()
    peak_bits = payload.get("ub_peak_bits")
    required_bits = int(payload.get("required_bits") or 0)
    capacity_bits = int(payload.get("capacity_bits") or 0)
    usage = (required_bits / capacity_bits * 100.0) if capacity_bits else 0.0
    functions = payload.get("functions", [])
    assumed = payload.get("assumed_post_cvpipeline_options", {})

    # Buffers live under functions[].buffers[]; flatten with a function tag.
    plan: list[dict[str, Any]] = []
    for function in functions:
        for buffer in function.get("buffers", []):
            entry = dict(buffer)
            entry["function"] = function.get("function", "")
            plan.append(entry)

    print("CVPipeline UB Plan Demo")
    print(divider())
    print(f"Status        : {status}")
    print(f"Precision     : {payload.get('precision', '')}")
    print(f"Peak (module) : {bit_size(peak_bits)}")
    print(f"Required      : {bit_size(required_bits)}")
    print(f"Capacity      : {bit_size(capacity_bits)}")
    print(f"Usage         : {usage:.4f}%")
    print(f"Selected seed : {payload.get('selected_seed', '')}")
    print(f"Functions     : {len(functions) if functions else '(not reported)'}")
    print(f"Buffers       : {len(plan)}")
    print()
    print("Assumed post-CVPipeline options")
    print(divider())
    print(f"tile_mix_vector_loop      : {assumed.get('tile_mix_vector_loop')}")
    print(f"tile_mix_cube_loop        : {assumed.get('tile_mix_cube_loop')}")
    print(f"enable_auto_bind_sub_block: {bool_text(assumed.get('enable_auto_bind_sub_block'))}")
    print(f"enable_code_motion        : {bool_text(assumed.get('enable_code_motion'))}")
    print(f"enable_ubuf_saving        : {bool_text(assumed.get('enable_ubuf_saving'))}")
    print(f"restrict_inplace_as_isa   : {bool_text(payload.get('restrict_inplace_as_isa'))}")
    print()

    if functions:
        print("Per-function UB (module reports the max, never the sum)")
        print(divider())
        widths = [24, 10, 12, 12, 8]
        print(row(["function", "status", "peak", "required", "seed"], widths))
        print(row(["-" * 24, "-" * 10, "-" * 12, "-" * 12, "-" * 8], widths))
        for function in functions:
            print(row([
                str(function.get("function", ""))[:24],
                str(function.get("status", "")),
                bit_size(function.get("ub_peak_bits")),
                bit_size(function.get("required_bits")),
                str(function.get("selected_seed", "")),
            ], widths))
        print()

    if not plan:
        print("No buffer plan was emitted.")
    else:
        shown = min(args.max_buffers, len(plan))
        print(f"Buffer plan (first {shown}, grouped by function)")
        print(divider())
        widths = [20, 20, 11, 9, 11, 9]
        print(row(["function", "name", "extent", "offset", "live", "end"], widths))
        print(row(["-" * 20, "-" * 20, "-" * 11, "-" * 9, "-" * 11, "-" * 9],
                  widths))
        current = None
        for buffer in plan[: args.max_buffers]:
            function = str(buffer.get("function", ""))
            if current is None or function != current:
                current = function
                label = function if function else "(unknown function)"
                print(row([f"[{label}]", "", "", "", "", ""], widths))
            extent_bits = int(buffer.get("extent_bits") or 0)
            offset_bytes = int(buffer.get("offset_bytes") or 0)
            end_bytes = offset_bytes + (extent_bits + 7) // 8
            print(row([
                "",
                str(buffer.get("name", ""))[:20],
                bit_size(extent_bits),
                byte_size(offset_bytes),
                f"{buffer.get('alloc_time')}->{buffer.get('free_time')}",
                byte_size(end_bytes),
            ], widths))
        if len(plan) > args.max_buffers:
            print(f"... {len(plan) - args.max_buffers} more buffers")

    diagnostics = payload.get("diagnostics", [])
    if diagnostics:
        print()
        print(f"Diagnostics ({len(diagnostics)})")
        print(divider())
        for diag in diagnostics:
            stage = diag.get("pipeline_stage", "")
            func = diag.get("function", "")
            reason = diag.get("reason", "")
            op = diag.get("operation", "")
            op_part = f" [{op}]" if op else ""
            print(f"  [{stage}] {func}{op_part}: {reason}")

    print()
    print("Artifacts")
    print(divider())
    print(f"JSON report   : {args.json_file}")
    print(f"Visualizer    : {args.html}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
