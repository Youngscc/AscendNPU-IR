#!/usr/bin/env python3
"""Measure the real embedded UB model path with the non-overflow fast path."""

from __future__ import annotations

import argparse
from collections import Counter
import csv
import json
import math
import os
from pathlib import Path
import re
import statistics
import subprocess
import tempfile
import time
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_COMPILER = ROOT / "build/bin/bishengir-compile"
DEFAULT_ADAPTER_ROOT = ROOT / "ub_overflow_model_cpp/data/adapter"
DEFAULT_BEFORE_CV_ROOT = ROOT / "ub_overflow_model_cpp/data/before_cvpipelining"
MACHINE_PREFIX = "BISHENGIR_UB_MODEL_RESULT "


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Measure production evaluate() in the real before-AutoBlockify "
            "pass. "
            "The real prefix executes, but the compiler stops immediately "
            "after prediction and does not run native PlanMemory."
        )
    )
    parser.add_argument(
        "--variant", action="append", default=[], metavar="NAME=COMPILER",
        help="compiler variant; repeat to perform interleaved A/B measurement",
    )
    parser.add_argument("--adapter-root", type=Path,
                        default=DEFAULT_ADAPTER_ROOT)
    parser.add_argument("--input", action="append", default=[])
    parser.add_argument("--max-inputs", type=int, default=0)
    parser.add_argument(
        "--include-unpaired-adapters", action="store_true",
        help="also measure adapters without a matching 160-input before-CV corpus entry",
    )
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--warmup-inputs", type=int, default=8)
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--summary", type=Path, default=None)
    return parser.parse_args()


def variants(values: list[str]) -> list[tuple[str, Path]]:
    if not values:
        return [("current", DEFAULT_COMPILER.resolve())]
    result: list[tuple[str, Path]] = []
    names: set[str] = set()
    for value in values:
        name, separator, path = value.partition("=")
        if not separator or not name or not path:
            raise ValueError(f"invalid --variant {value!r}; expected NAME=PATH")
        if name in names:
            raise ValueError(f"duplicate variant name: {name}")
        compiler = Path(path).resolve()
        if not compiler.is_file() or not os.access(compiler, os.X_OK):
            raise ValueError(f"compiler is not executable: {compiler}")
        names.add(name)
        result.append((name, compiler))
    return result


def adapters(root: Path, selected: set[str], limit: int,
             include_unpaired: bool) -> list[Path]:
    values = sorted(root.resolve().glob("*.ttadapter"))
    if not include_unpaired:
        paired = {path.name for path in DEFAULT_BEFORE_CV_ROOT.iterdir()
                  if path.is_dir()}
        values = [value for value in values if value.name in paired]
    if selected:
        values = [value for value in values if value.name in selected]
        missing = selected - {value.name for value in values}
        if missing:
            raise ValueError("unknown adapters: " + ", ".join(sorted(missing)))
    if limit > 0:
        values = values[:limit]
    if not values:
        raise ValueError("no adapter inputs selected")
    return values


def parse_machine_result(stderr: str) -> dict[str, str] | None:
    lines = [line for line in stderr.splitlines()
             if line.startswith(MACHINE_PREFIX)]
    if len(lines) != 1:
        return None
    result: dict[str, str] = {}
    for field in lines[0][len(MACHINE_PREFIX):].split():
        key, separator, value = field.partition("=")
        if separator:
            result[key] = value
    return result


def parse_max_rss(stderr: str) -> int:
    match = re.search(r"^\s*(\d+)\s+maximum resident set size\s*$",
                      stderr, flags=re.MULTILINE)
    return int(match.group(1)) if match else 0


def compiler_environment() -> dict[str, str]:
    env = dict(os.environ)
    for key in (
        "BISHENGIR_UB_MODEL_VALIDATION",
        "BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS",
        "BISHENGIR_PLAN_MEMORY_FORCE_SEED",
        "BISHENGIR_UB_MODEL_FORCE_FULL_PLAN",
        "BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY",
        "BISHENGIR_UB_FLOW_TRACE",
    ):
        env.pop(key, None)
    env["BISHENGIR_UB_MODEL_EMIT_RESULT"] = "1"
    env["BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION"] = "1"
    return env


def run_one(compiler: Path, adapter: Path, timeout: int,
            output: Path) -> dict[str, Any]:
    command = [
        "/usr/bin/time", "-l", str(compiler), str(adapter),
        "--enable-hfusion-compile=true",
        "--enable-triton-kernel-compile=true",
        "--prune-predicted-ub-overflow=false",
        "-o", str(output),
    ]
    started = time.monotonic_ns()
    completed = subprocess.run(
        command, text=True, capture_output=True,
        env=compiler_environment(), timeout=timeout, check=False,
    )
    wall_ns = time.monotonic_ns() - started
    machine = parse_machine_result(completed.stderr)
    if machine is None:
        return {
            "adapter": adapter.name,
            "measurement_status": "unavailable",
            "status": "unavailable", "precision": "incomplete",
            "overflow": "unknown", "decision_path": "unknown",
            "model_ns": 0, "serialize_ns": 0, "prediction_ns": 0,
            "process_wall_ns": wall_ns,
            "max_rss_bytes": parse_max_rss(completed.stderr),
            "compiler_rc": completed.returncode,
            "diagnostic": completed.stderr[-1200:].replace("\n", "\\n"),
        }
    model_ns = int(machine["model_ns"])
    serialize_ns = int(machine.get("serialize_ns", "0"))
    return {
        "adapter": adapter.name,
        "measurement_status": "observed",
        "status": machine["status"],
        "precision": machine["precision"],
        "overflow": machine["overflow"],
        "decision_path": machine.get("decision_path", "unknown"),
        "model_ns": model_ns,
        "serialize_ns": serialize_ns,
        "prediction_ns": model_ns + serialize_ns,
        "process_wall_ns": wall_ns,
        "max_rss_bytes": parse_max_rss(completed.stderr),
        "compiler_rc": completed.returncode,
        "diagnostic": "",
    }


def percentile(values: list[int], fraction: float) -> float:
    ordered = sorted(values)
    if not ordered:
        return 0.0
    position = fraction * (len(ordered) - 1)
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return float(ordered[lower])
    return (ordered[lower] * (upper - position) +
            ordered[upper] * (position - lower))


def summarize(rows: list[dict[str, Any]], rounds: int) -> dict[str, Any]:
    result: dict[str, Any] = {
        "measurement_mode": "production_real_fast_path",
        "rounds": rounds,
        "variants": {},
    }
    for variant in sorted({str(row["variant"]) for row in rows}):
        all_selected = [row for row in rows if row["variant"] == variant]
        selected = [row for row in all_selected
                    if row["measurement_status"] == "observed"]
        prediction = [int(row["prediction_ns"]) for row in selected]
        per_round = []
        for round_index in range(rounds):
            current = [row for row in selected
                       if int(row["round"]) == round_index]
            per_round.append({
                "round": round_index,
                "prediction_total_ns": sum(
                    int(row["prediction_ns"]) for row in current
                ),
                "model_total_ns": sum(int(row["model_ns"]) for row in current),
                "serialize_total_ns": sum(
                    int(row["serialize_ns"]) for row in current
                ),
                "process_wall_total_ns": sum(
                    int(row["process_wall_ns"]) for row in current
                ),
                "peak_rss_bytes": max(
                    (int(row["max_rss_bytes"]) for row in current), default=0
                ),
            })
        result["variants"][variant] = {
            "samples": len(selected),
            "requested_samples": len(all_selected),
            "inputs": len({str(row["adapter"]) for row in all_selected}),
            "unavailable": len(all_selected) - len(selected),
            "prediction_ns": {
                "median": statistics.median(prediction),
                "mean": statistics.fmean(prediction),
                "p95": percentile(prediction, 0.95),
                "maximum": max(prediction, default=0),
            },
            "rounds": per_round,
            "fast_path_hits": sum(
                row["decision_path"] == "non_overflow_upper_bound"
                for row in selected
            ),
            "full_plan_fallthrough": sum(
                row["decision_path"] != "non_overflow_upper_bound"
                for row in selected
            ),
            "decision_paths": dict(sorted(Counter(
                str(row["decision_path"]) for row in selected
            ).items())),
            "statuses": dict(sorted(Counter(
                str(row["status"]) for row in selected
            ).items())),
        }
    return result


def main() -> int:
    args = parse_args()
    try:
        measured_variants = variants(args.variant)
        inputs = adapters(
            args.adapter_root, set(args.input), args.max_inputs,
            args.include_unpaired_adapters,
        )
        if args.rounds <= 0 or args.warmup_inputs < 0:
            raise ValueError("rounds must be positive and warmup-inputs nonnegative")
    except ValueError as error:
        print(f"[ERROR] {error}", file=os.sys.stderr)
        return 2

    args.report.parent.mkdir(parents=True, exist_ok=True)
    rows: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="cvub-perf-") as temporary:
        output = Path(temporary) / "unused.o"
        warmups = inputs[:args.warmup_inputs]
        for _, compiler in measured_variants:
            for adapter in warmups:
                run_one(compiler, adapter, args.timeout, output)
        total = args.rounds * len(inputs) * len(measured_variants)
        completed = 0
        for round_index in range(args.rounds):
            for input_index, adapter in enumerate(inputs):
                order = list(measured_variants)
                if (round_index + input_index) % 2:
                    order.reverse()
                for name, compiler in order:
                    row = run_one(compiler, adapter, args.timeout, output)
                    row.update({"variant": name, "round": round_index})
                    rows.append(row)
                    completed += 1
                    print(f"\r[{completed}/{total}] {adapter.name} {name}",
                          end="", flush=True)
    print()

    columns = [
        "variant", "round", "adapter", "measurement_status", "status",
        "precision", "overflow",
        "decision_path", "model_ns", "serialize_ns", "prediction_ns",
        "process_wall_ns", "max_rss_bytes", "compiler_rc", "diagnostic",
    ]
    with args.report.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, columns, delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    summary = summarize(rows, args.rounds)
    if args.summary is not None:
        args.summary.parent.mkdir(parents=True, exist_ok=True)
        args.summary.write_text(json.dumps(summary, indent=2) + "\n",
                                encoding="utf-8")
    print(json.dumps(summary, indent=2))
    print(args.report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
