#!/usr/bin/env python3
"""Measure model and native before-AutoBlockify-to-PlanMemory costs."""

from __future__ import annotations

import argparse
from collections import Counter
import csv
import json
import math
import os
from pathlib import Path
import re
import signal
import statistics
import subprocess
import sys
import tempfile
import time
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from run_bisheng_embedded_matrix import (
    DEFAULT_ADAPTER_ROOT,
    DEFAULT_MATRIX,
    DEFAULT_TIMEOUT_PAIRS,
    compiler_arguments,
    load_adapters,
    load_known_timeout_pairs,
    load_scenarios,
)


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_COMPILER = ROOT / "build/bin/bishengir-compile"
MACHINE_PREFIX = "BISHENGIR_UB_MODEL_RESULT "
NATIVE_PREFIX = "BISHENGIR_UB_NATIVE_RANGE_TIME "
STAGE_PREFIX = "BISHENGIR_UB_MODEL_STAGE_TIME "
def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, default=DEFAULT_COMPILER)
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument(
        "--known-timeout-pairs", type=Path, default=DEFAULT_TIMEOUT_PAIRS
    )
    parser.add_argument("--adapter-root", type=Path,
                        default=DEFAULT_ADAPTER_ROOT)
    parser.add_argument("--config", action="append", default=[])
    parser.add_argument("--input", action="append", default=[])
    parser.add_argument("--max-inputs", type=int, default=0)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--warmup-inputs", type=int, default=8)
    parser.add_argument("--timeout", type=int, default=360)
    # Kept as a no-op for compatibility with commands recorded before the
    # prediction hook learned the workspace-manager-off branch.  Every matrix
    # scenario is active now.
    parser.add_argument(
        "--include-inactive", action="store_true", help=argparse.SUPPRESS
    )
    parser.add_argument(
        "--include-known-timeouts", action="store_true",
        help="run native BiSheng even for audited long-timeout pairs",
    )
    parser.add_argument(
        "--collect-stage-timings", action="store_true",
        help="diagnostic mode; adds detailed timer overhead",
    )
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--summary", type=Path, required=True)
    return parser.parse_args()


def key_values(line: str, prefix: str) -> dict[str, str]:
    return dict(
        field.split("=", 1)
        for field in line[len(prefix):].split()
        if "=" in field
    )


def first_record(stderr: str, prefix: str) -> dict[str, str] | None:
    values = records(stderr, prefix)
    return values[0] if values else None


def records(stderr: str, prefix: str) -> list[dict[str, str]]:
    return [
        key_values(line, prefix)
        for line in stderr.splitlines()
        if line.startswith(prefix)
    ]


def max_rss(stderr: str) -> int:
    match = re.search(
        r"^\s*(\d+)\s+maximum resident set size\s*$",
        stderr,
        flags=re.MULTILINE,
    )
    return int(match.group(1)) if match else 0


def measurement_environment(
    collect_stage_timings: bool = False,
    skip_native: bool = False,
) -> dict[str, str]:
    environment = dict(os.environ)
    for name in (
        "BISHENGIR_UB_MODEL_VALIDATION",
        "BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS",
        "BISHENGIR_PLAN_MEMORY_FORCE_SEED",
        "BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION",
        "BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY",
        "BISHENGIR_UB_FLOW_TRACE",
        "BISHENGIR_UB_MODEL_COMPARE_TEXT_ENTRY",
        "BISHENGIR_UB_NATIVE_RANGE_TIMING",
        "BISHENGIR_UB_MODEL_STAGE_TIMING",
    ):
        environment.pop(name, None)
    environment["BISHENGIR_UB_MODEL_EMIT_RESULT"] = "1"
    environment["BISHENGIR_UB_MODEL_FORCE_FULL_PLAN"] = "1"
    if skip_native:
        environment["BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION"] = "1"
    else:
        environment["BISHENGIR_UB_NATIVE_RANGE_TIMING"] = "1"
        environment["BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY"] = "1"
    if collect_stage_timings:
        environment["BISHENGIR_UB_MODEL_STAGE_TIMING"] = "1"
    return environment


def run_one(
    compiler: Path,
    adapter: Path,
    scenario: dict[str, str],
    timeout: int,
    output: Path,
    collect_stage_timings: bool = False,
    skip_native: bool = False,
) -> dict[str, Any]:
    command = [
        "/usr/bin/time", "-l", str(compiler), str(adapter),
        "-o", str(output), *compiler_arguments(scenario),
    ]
    started = time.monotonic_ns()
    process = subprocess.Popen(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=measurement_environment(collect_stage_timings, skip_native),
        start_new_session=True,
    )
    try:
        _, stderr = process.communicate(timeout=timeout)
        returncode = process.returncode
        timed_out = False
    except subprocess.TimeoutExpired as error:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        _, final_stderr = process.communicate()
        stderr = final_stderr or error.stderr or ""
        returncode = 124
        timed_out = True
    wall_ns = time.monotonic_ns() - started
    models = records(stderr, MACHINE_PREFIX)
    natives = records(stderr, NATIVE_PREFIX)
    stages = records(stderr, STAGE_PREFIX)
    decision_paths = [model.get("decision_path", "") for model in models]
    model_ns = sum(int(model.get("model_ns", "0")) for model in models)
    native_ns = sum(int(native.get("ns", "0")) for native in natives)
    valid_full_plan = bool(models) and all(
        path == "full_plan" for path in decision_paths
    )
    native_complete = bool(models) and len(natives) == len(models)
    return {
        "scenario": scenario["scenario_id"],
        "adapter": adapter.name,
        "model_observed": str(bool(models)).lower(),
        "model_attempts": len(models),
        "native_observed": str(bool(natives)).lower(),
        "native_attempts": len(natives),
        "native_complete": str(native_complete).lower(),
        "native_skipped_known_timeout": str(skip_native).lower(),
        "full_plan_verified": str(valid_full_plan).lower(),
        "status": ",".join(
            model.get("status", "unavailable") for model in models
        ) or "unavailable",
        "precision": ",".join(
            model.get("precision", "incomplete") for model in models
        ) or "incomplete",
        "overflow": ",".join(
            model.get("overflow", "unknown") for model in models
        ) or "unknown",
        "decision_path": ",".join(decision_paths),
        "model_ns": model_ns,
        "native_range_ns": native_ns,
        "stage_timings": ";".join(
            f"{stage.get('name', 'unknown')}#{stage.get('occurrence', '0')}="
            f"{stage.get('ns', '0')}" for stage in stages
        ),
        "bisheng_over_model": native_ns / model_ns
        if model_ns and native_ns and native_complete else 0.0,
        "process_wall_ns": wall_ns,
        "max_rss_bytes": max_rss(stderr),
        "compiler_rc": returncode,
        "timed_out": str(timed_out).lower(),
        "diagnostic": "" if models and valid_full_plan else
        stderr[-1200:].replace("\t", " ").replace("\n", "\\n"),
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


def distribution(values: list[int]) -> dict[str, float]:
    return {
        "total_ns": float(sum(values)),
        "median_ns": float(statistics.median(values)) if values else 0.0,
        "mean_ns": statistics.fmean(values) if values else 0.0,
        "p95_ns": percentile(values, 0.95),
        "max_ns": float(max(values, default=0)),
    }


def summarize(
    rows: list[dict[str, Any]], rounds: int,
    collect_stage_timings: bool = False,
) -> dict[str, Any]:
    model_rows = [row for row in rows
                  if row["model_observed"] == "true" and
                  row["full_plan_verified"] == "true"]
    native_expected = [
        row for row in model_rows
        if row["native_skipped_known_timeout"] != "true"
    ]
    paired = [row for row in native_expected
              if row["native_complete"] == "true"]
    model_values = [int(row["model_ns"]) for row in model_rows]
    paired_model = [int(row["model_ns"]) for row in paired]
    native_values = [int(row["native_range_ns"]) for row in paired]
    round_totals = []
    for round_index in range(rounds):
        current = [row for row in paired if int(row["round"]) == round_index]
        model_total = sum(int(row["model_ns"]) for row in current)
        native_total = sum(int(row["native_range_ns"]) for row in current)
        round_totals.append({
            "round": round_index,
            "paired": len(current),
            "model_total_ns": model_total,
            "native_total_ns": native_total,
            "bisheng_over_model": native_total / model_total
            if model_total else 0.0,
        })
    paired_model_total = sum(paired_model)
    native_total = sum(native_values)
    stage_totals: dict[str, int] = {}
    for row in model_rows:
        for field in str(row.get("stage_timings", "")).split(";"):
            name_occurrence, separator, nanoseconds = field.partition("=")
            if not separator:
                continue
            name = name_occurrence.rsplit("#", 1)[0]
            stage_totals[name] = stage_totals.get(name, 0) + int(nanoseconds)
    prefix_stage_total = sum(
        value for name, value in stage_totals.items()
        if name.startswith("PreCV.")
    )
    input_stage_total = sum(
        stage_totals.get(name, 0)
        for name in ("ImportMLIRModule", "ParseGenericIR")
    )
    suffix_stage_total = stage_totals.get("CVToPlanMemoryPipeline", 0)
    statuses = Counter(
        status
        for row in model_rows
        for status in str(row["status"]).split(",")
        if status
    )
    decision_paths = Counter(
        path
        for row in model_rows
        for path in str(row["decision_path"]).split(",")
        if path
    )
    return {
        "measurement_mode": (
            "diagnostic_stage_timing" if collect_stage_timings
            else "production_full_plan"
        ),
        "rounds": rounds,
        "requested_samples": len(rows),
        "inputs": len({str(row["adapter"]) for row in rows}),
        "scenarios": len({str(row["scenario"]) for row in rows}),
        "model_full_plan_samples": len(model_rows),
        "paired_native_samples": len(paired),
        "model_unavailable": len(rows) - len(model_rows),
        "known_native_timeout_skipped": len(model_rows) - len(native_expected),
        "native_expected_samples": len(native_expected),
        "native_unavailable": len(native_expected) - len(paired),
        "native_partial": sum(
            row["native_observed"] == "true" and
            row["native_complete"] != "true"
            for row in native_expected
        ),
        "model_attempt_statuses": dict(sorted(statuses.items())),
        "decision_paths": dict(sorted(decision_paths.items())),
        "timed_out_samples": sum(
            row["timed_out"] == "true" for row in rows
        ),
        "compiler_returncodes": dict(sorted(Counter(
            str(row["compiler_rc"]) for row in rows
        ).items())),
        "model": distribution(model_values),
        "paired_model": distribution(paired_model),
        "native": distribution(native_values),
        "bisheng_over_model_aggregate":
        native_total / paired_model_total if paired_model_total else 0.0,
        "stage_totals_ns": dict(sorted(stage_totals.items())),
        "input_bridge_stage_total_ns": input_stage_total,
        "pre_cv_prefix_stage_total_ns": prefix_stage_total,
        "cv_to_plan_memory_stage_total_ns": suffix_stage_total,
        "unclassified_model_time_ns": max(
            0,
            sum(model_values) - input_stage_total - prefix_stage_total -
            suffix_stage_total,
        ),
        "process_wall_total_ns": sum(
            int(row["process_wall_ns"]) for row in rows
        ),
        "peak_rss_bytes": max(
            (int(row["max_rss_bytes"]) for row in rows), default=0
        ),
        "round_totals": round_totals,
    }


COLUMNS = [
    "round", "scenario", "adapter", "model_observed", "model_attempts",
    "native_observed", "native_attempts", "native_complete",
    "native_skipped_known_timeout",
    "full_plan_verified", "status", "precision",
    "overflow", "decision_path", "model_ns", "native_range_ns",
    "stage_timings", "bisheng_over_model", "process_wall_ns", "max_rss_bytes",
    "compiler_rc", "timed_out", "diagnostic",
]


def load_completed_rows(path: Path) -> list[dict[str, Any]]:
    if not path.is_file():
        return []
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    if rows and set(rows[0]) != set(COLUMNS):
        raise ValueError(f"incompatible report schema: {path}")
    return rows


def main() -> int:
    arguments = parse_args()
    if arguments.rounds < 1 or arguments.warmup_inputs < 0:
        raise ValueError("rounds must be positive and warmup-inputs nonnegative")
    compiler = arguments.compiler.resolve()
    if not compiler.is_file() or not os.access(compiler, os.X_OK):
        raise ValueError(f"compiler is not executable: {compiler}")
    scenarios = load_scenarios(arguments.matrix, set(arguments.config))
    inputs = load_adapters(
        arguments.adapter_root, set(arguments.input), arguments.max_inputs
    )
    if not scenarios or not inputs:
        raise ValueError("no active scenarios or inputs selected")
    known_timeout_pairs = load_known_timeout_pairs(
        arguments.known_timeout_pairs
    )

    arguments.report.parent.mkdir(parents=True, exist_ok=True)
    arguments.summary.parent.mkdir(parents=True, exist_ok=True)
    rows = load_completed_rows(arguments.report) if arguments.resume else []
    with tempfile.TemporaryDirectory(prefix="before-auto-perf-") as temporary:
        output = Path(temporary) / "unused.o"
        for adapter in inputs[:arguments.warmup_inputs]:
            skip_native = (
                not arguments.include_known_timeouts and
                (scenarios[0]["scenario_id"], adapter.name)
                in known_timeout_pairs
            )
            run_one(
                compiler, adapter, scenarios[0], arguments.timeout, output,
                arguments.collect_stage_timings, skip_native,
            )
        total = arguments.rounds * len(scenarios) * len(inputs)
        selected_keys = {
            (round_index, scenario["scenario_id"], adapter.name)
            for round_index in range(arguments.rounds)
            for scenario in scenarios
            for adapter in inputs
        }
        rows = [
            row for row in rows
            if (int(row["round"]), str(row["scenario"]),
                str(row["adapter"])) in selected_keys
        ]
        completed_keys = {
            (int(row["round"]), str(row["scenario"]), str(row["adapter"]))
            for row in rows
        }
        completed = len(rows)
        started = time.monotonic()
        with arguments.report.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(
                stream, COLUMNS, delimiter="\t", lineterminator="\n"
            )
            writer.writeheader()
            writer.writerows(rows)
            stream.flush()
            for round_index in range(arguments.rounds):
                for scenario in scenarios:
                    for adapter in inputs:
                        key = (round_index, scenario["scenario_id"], adapter.name)
                        if key in completed_keys:
                            continue
                        skip_native = (
                            not arguments.include_known_timeouts and
                            (scenario["scenario_id"], adapter.name)
                            in known_timeout_pairs
                        )
                        row = run_one(
                            compiler, adapter, scenario, arguments.timeout,
                            output, arguments.collect_stage_timings,
                            skip_native,
                        )
                        row["round"] = round_index
                        rows.append(row)
                        writer.writerow(row)
                        stream.flush()
                        completed += 1
                        measured = completed - len(completed_keys)
                        elapsed = time.monotonic() - started
                        remaining = total - completed
                        eta = elapsed / measured * remaining if measured else 0
                        print(
                            f"\r[{completed}/{total}] "
                            f"{scenario['scenario_id']} {adapter.name} "
                            f"ETA {int(eta // 60):02d}:{int(eta % 60):02d}",
                            end="",
                            flush=True,
                        )
    print()
    summary = summarize(
        rows, arguments.rounds, arguments.collect_stage_timings
    )
    arguments.summary.write_text(
        json.dumps(summary, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(summary, indent=2))
    print(arguments.report)
    return 0 if (
        summary["model_unavailable"] == 0 and
        summary["paired_native_samples"] > 0
    ) else 1


if __name__ == "__main__":
    raise SystemExit(main())
