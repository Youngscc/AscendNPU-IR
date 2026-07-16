#!/usr/bin/env python3
"""Pretty-print a UB plan JSON report for terminal demos."""

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
    parser.add_argument(
        "--html", default="cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html")
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
    result = payload.get("result", {})
    options = payload.get("options", {})
    cvpipelining = options.get("cvpipelining", {})
    suffix = options.get("suffix_plan_memory", {})
    plan = result.get("plan", [])

    status = "OVERFLOW" if result.get("overflow") else (
        "SUCCESS" if result.get("success", False) else "BLOCKED")
    required_bits = int(result.get("required_bits") or 0)
    capacity_bits = int(result.get("capacity_bits") or 0)
    usage = (required_bits / capacity_bits * 100.0) if capacity_bits else 0.0

    print("CVPipelining UB Plan Demo")
    print(divider())
    print(f"Status        : {status}")
    print(f"Precision     : {result.get('precision', '')}")
    peak = bit_size(result.get("peak_bits"))
    print(f"Peak          : {peak}")
    print(f"Required      : {bit_size(required_bits)}")
    print(f"Capacity      : {bit_size(capacity_bits)}")
    print(f"Usage         : {usage:.4f}%")
    print(f"Selected seed : {result.get('selected_seed', '')}")
    print(f"Buffers       : {len(plan)}")
    print()
    print("CVPipelining options")
    print(divider())
    print(f"disable_pipelining : {bool_text(cvpipelining.get('disable_pipelining'))}")
    print(f"pipeline_depth     : {cvpipelining.get('pipeline_depth')}")
    print(f"enable_preload     : {bool_text(cvpipelining.get('enable_preload'))}")
    print(f"enable_lazy_loading: {bool_text(cvpipelining.get('enable_lazy_loading'))}")
    print()
    print("Suffix / PlanMemory options")
    print(divider())
    print(f"auto_multi_buffer  : {bool_text(suffix.get('enable_auto_multi_buffer'))}")
    print(f"local_strategy     : {suffix.get('local_multi_buffer_strategy')}")
    print(f"mix_strategy       : {suffix.get('mix_multi_buffer_strategy')}")
    print(f"random_seed input  : {suffix.get('random_seed')}")
    print(f"restrict_inplace   : {bool_text(suffix.get('restrict_inplace_as_isa'))}")
    print()

    if not plan:
        print("No buffer plan was emitted.")
    else:
        print(f"Buffer plan (first {min(args.max_buffers, len(plan))})")
        print(divider())
        widths = [24, 12, 10, 12, 10]
        print(row(["name", "extent", "offset", "live", "end"], widths))
        print(row(["-" * 24, "-" * 12, "-" * 10, "-" * 12, "-" * 10],
                  widths))
        for buffer in plan[: args.max_buffers]:
            extent_bits = int(buffer.get("extent_bits") or 0)
            offset_bytes = int(buffer.get("offset_bytes") or 0)
            end_bytes = offset_bytes + (extent_bits + 7) // 8
            print(row([
                str(buffer.get("name", ""))[:24],
                bit_size(extent_bits),
                byte_size(offset_bytes),
                f"{buffer.get('alloc_time')}->{buffer.get('free_time')}",
                byte_size(end_bytes),
            ], widths))
        if len(plan) > args.max_buffers:
            print(f"... {len(plan) - args.max_buffers} more buffers")

    input_path = payload.get("input", {}).get("before_cvpipelining_ir", "")
    print()
    print("Artifacts")
    print(divider())
    print(f"Input IR      : {input_path}")
    print(f"JSON report   : {args.json_file}")
    print(f"Visualizer    : {args.html}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
