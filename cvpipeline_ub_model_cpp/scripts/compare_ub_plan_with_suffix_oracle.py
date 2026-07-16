#!/usr/bin/env python3
"""Compare model UB plan JSON with suffix-compile PlanMemory dump output."""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path
from typing import Counter


PlanKey = tuple[int, int]
LifetimeKey = tuple[str, int, int, int, int]


def model_multi_and_inplace(payload: dict) -> tuple[Counter[int], int]:
    multi: Counter[int] = collections.Counter()
    inplace = 0
    for function in payload.get("functions", []):
        by_name: dict[str, int] = {}
        for buffer in function.get("buffers", []):
            name = str(buffer.get("name", ""))
            by_name[name] = int(buffer.get("multi_buffer_num", 1))
        multi.update(by_name.values())
        inplace += len(function.get("inplace_pairs", []))
    return multi, inplace


def parse_oracle_contract(
    path: Path, attempt: int, scope: str,
) -> tuple[str, int, Counter[int], int]:
    """Parse status/required/multi/inplace facts omitted by the legacy tuple."""
    current_function = ""
    statuses: list[str] = []
    required = 0
    storage_required = 0
    planned_required = 0
    peak = 0
    ub_buffers: set[tuple[str, str]] = set()
    multi_by_buffer: dict[tuple[str, str], int] = {}
    inplace = 0
    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = raw_line.split("\t")
        if not fields:
            continue
        if fields[0] == "PLANMEM_LIVENESS_ATTEMPT" and len(fields) >= 4:
            current_function = fields[1]
        elif fields[0] == "PLANMEM_PLAN_ATTEMPT" and len(fields) >= 4:
            current_function = fields[1]
            if int(fields[2]) == attempt:
                statuses.append(fields[3])
        elif (fields[0] == "PLANMEM_REQUIRED" and len(fields) >= 4 and
              int(fields[1]) == attempt and fields[2] == scope):
            required = max(required, int(fields[3]))
        elif (fields[0] == "PLANMEM_STORAGE" and len(fields) >= 7 and
              int(fields[1]) == attempt and fields[2] == scope):
            storage_required = max(storage_required,
                                   int(fields[5]) + int(fields[4]))
        elif (fields[0] == "PLANMEM_PEAK" and len(fields) >= 4 and
              int(fields[1]) == attempt and fields[2] == scope):
            peak = max(peak, int(fields[3]))
        elif (fields[0] == "PLANMEM_EXACT_PLANNED_BUFFER" and
              len(fields) >= 6 and int(fields[1]) == attempt and
              fields[2] == scope):
            extent = int(fields[4])
            aligned_extent = ((extent + 255) // 256) * 256
            for offset in fields[5:]:
                planned_required = max(
                    planned_required, int(offset) * 8 + aligned_extent)
        elif (fields[0] == "PLANMEM_EXACT_BUFFER" and len(fields) >= 8 and
              int(fields[1]) == attempt and fields[4] == scope):
            ub_buffers.add((current_function, fields[2]))
        elif (fields[0] == "PLANMEM_EXACT_MULTI" and len(fields) >= 4 and
              int(fields[1]) == attempt):
            multi_by_buffer[(current_function, fields[2])] = int(fields[3])
        elif (fields[0] == "PLANMEM_EXACT_INPLACE" and len(fields) >= 4 and
              int(fields[1]) == attempt):
            inplace += 1
    status = "success" if statuses and all(value == "success" for value in statuses) \
        else "overflow"
    if status == "success" and required == 0:
        required = planned_required or storage_required or peak
    multi: Counter[int] = collections.Counter(
        multi_by_buffer.get(buffer, 1) for buffer in ub_buffers)
    return status, required, multi, inplace


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare modeled UB offsets/peak against suffix-compile oracle TSV.")
    parser.add_argument("--model-json", required=True, type=Path)
    parser.add_argument("--oracle-tsv", required=True, type=Path)
    parser.add_argument("--scope", default="6",
                        help="PlanMemory scope id to compare; 6 is UB in current dumps.")
    parser.add_argument("--attempt", type=int, default=None,
                        help="Oracle attempt to compare. Defaults to model selected_seed.")
    parser.add_argument("--max-diff", type=int, default=12)
    return parser.parse_args()


def plan_multiset_from_model(payload: dict) -> Counter[PlanKey]:
    multiset: Counter[PlanKey] = collections.Counter()
    # Current plan-before-cvpipeline reports are flat and group buffers under
    # functions[].  Keep the historical nested shape readable for old artifacts.
    if isinstance(payload.get("functions"), list):
        for function in payload["functions"]:
            for buffer in function.get("buffers", []):
                multiset[(int(buffer["extent_bits"]),
                          int(buffer["offset_bytes"]))] += 1
        return multiset
    result = payload.get("result", {})
    for buffer in result.get("plan", []):
        multiset[(int(buffer["extent_bits"]), int(buffer["offset_bytes"]))] += 1
    return multiset


def normalized_lifetimes_from_model(payload: dict) -> Counter[LifetimeKey]:
    result: Counter[LifetimeKey] = collections.Counter()
    for function in payload.get("functions", []):
        name = str(function.get("function", ""))
        buffers = function.get("buffers", [])
        event_times = sorted({
            int(buffer[key])
            for buffer in buffers
            for key in ("alloc_time", "free_time")
        })
        ranks = {time: rank for rank, time in enumerate(event_times)}
        for buffer in buffers:
            result[(name, int(buffer["extent_bits"]),
                    int(buffer["offset_bytes"]),
                    ranks[int(buffer["alloc_time"])],
                    ranks[int(buffer["free_time"])])] += 1
    return result


def parse_oracle(
    path: Path, attempt: int | None, scope: str,
) -> tuple[int, int | None, Counter[PlanKey], Counter[LifetimeKey]]:
    success_attempts: list[int] = []
    peaks: dict[int, int] = {}
    buffers_by_attempt: dict[int, Counter[PlanKey]] = collections.defaultdict(collections.Counter)
    # (function, attempt, semantic buffer id) -> (extent, gen, kill)
    buffer_lives: dict[tuple[str, int, str], tuple[int, int, int]] = {}
    # (function, attempt, semantic buffer id) -> planned offsets
    planned_offsets: dict[tuple[str, int, str], list[int]] = collections.defaultdict(list)
    function_for_attempt: dict[int, str] = {}
    current_function = ""

    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = raw_line.split("\t")
        if not fields:
            continue
        tag = fields[0]
        if tag == "PLANMEM_LIVENESS_ATTEMPT" and len(fields) >= 4:
            current_function = fields[1]
            function_for_attempt[int(fields[2])] = current_function
        elif tag == "PLANMEM_PLAN_ATTEMPT" and len(fields) >= 4:
            current_function = fields[1]
            function_for_attempt[int(fields[2])] = current_function
            if fields[3] == "success":
                success_attempts.append(int(fields[2]))
        elif tag == "PLANMEM_PEAK" and len(fields) >= 4:
            line_attempt = int(fields[1])
            if fields[2] == scope:
                peaks[line_attempt] = max(peaks.get(line_attempt, 0),
                                          int(fields[3]))
        elif tag == "PLANMEM_EXACT_BUFFER" and len(fields) >= 8:
            line_attempt = int(fields[1])
            if fields[4] != scope:
                continue
            function = current_function or function_for_attempt.get(line_attempt, "")
            buffer_lives[(function, line_attempt, fields[2])] = (
                int(fields[3]), int(fields[6]), int(fields[7]))
        elif tag == "PLANMEM_EXACT_PLANNED_BUFFER" and len(fields) >= 6:
            line_attempt = int(fields[1])
            if fields[2] != scope:
                continue
            function = current_function or function_for_attempt.get(line_attempt, "")
            extent_bits = int(fields[4])
            for offset in fields[5:]:
                buffers_by_attempt[line_attempt][(extent_bits, int(offset))] += 1
                planned_offsets[(function, line_attempt, fields[3])].append(int(offset))

    selected = attempt
    if selected is None:
        selected = success_attempts[0] if success_attempts else 0
    oracle_lifetimes: Counter[LifetimeKey] = collections.Counter()
    functions = {
        function for function, line_attempt, _ in buffer_lives
        if line_attempt == selected
    }
    for function in functions:
        lives = {
            buffer_id: life
            for (life_function, line_attempt, buffer_id), life in buffer_lives.items()
            if life_function == function and line_attempt == selected
        }
        event_times = sorted({time for _, gen, kill in lives.values()
                              for time in (gen, kill)})
        ranks = {time: rank for rank, time in enumerate(event_times)}
        for buffer_id, (extent, gen, kill) in lives.items():
            for offset in planned_offsets.get(
                    (function, selected, buffer_id), []):
                oracle_lifetimes[(function, extent, offset,
                                  ranks[gen], ranks[kill])] += 1
    selected_peak = peaks.get(selected)
    # PlanMemory emits no PLANMEM_PEAK row for a scope that has no buffers.
    # For UB-only comparison that is the authoritative zero peak, not a
    # missing result (other scopes may still contain planned buffers).
    if selected_peak is None and not buffers_by_attempt[selected]:
        selected_peak = 0
    return (selected, selected_peak, buffers_by_attempt[selected],
            oracle_lifetimes)


def parse_oracle_retry(
    path: Path, scope: str,
) -> tuple[dict[str, int], int, Counter[PlanKey], Counter[LifetimeKey]]:
    current_function = ""
    successful: dict[str, int] = {}
    peaks: dict[tuple[str, int], int] = {}
    lives: dict[tuple[str, int, str], tuple[int, int, int]] = {}
    offsets: dict[tuple[str, int, str], list[int]] = collections.defaultdict(list)
    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = raw_line.split("\t")
        if not fields:
            continue
        if fields[0] == "PLANMEM_LIVENESS_ATTEMPT" and len(fields) >= 4:
            current_function = fields[1]
        elif fields[0] == "PLANMEM_PLAN_ATTEMPT" and len(fields) >= 4:
            current_function = fields[1]
            if fields[3] == "success" and current_function not in successful:
                successful[current_function] = int(fields[2])
        elif fields[0] == "PLANMEM_PEAK" and len(fields) >= 4:
            if fields[2] == scope:
                key = (current_function, int(fields[1]))
                peaks[key] = max(peaks.get(key, 0), int(fields[3]))
        elif fields[0] == "PLANMEM_EXACT_BUFFER" and len(fields) >= 8:
            if fields[4] == scope:
                lives[(current_function, int(fields[1]), fields[2])] = (
                    int(fields[3]), int(fields[6]), int(fields[7]))
        elif (fields[0] == "PLANMEM_EXACT_PLANNED_BUFFER" and
              len(fields) >= 6 and fields[2] == scope):
            key = (current_function, int(fields[1]), fields[3])
            offsets[key].extend(int(value) for value in fields[5:])

    plan: Counter[PlanKey] = collections.Counter()
    normalized: Counter[LifetimeKey] = collections.Counter()
    module_peak = 0
    for function, selected in successful.items():
        function_lives = {
            buffer_id: life
            for (life_function, attempt, buffer_id), life in lives.items()
            if life_function == function and attempt == selected
        }
        event_times = sorted({time for _, gen, kill in function_lives.values()
                              for time in (gen, kill)})
        ranks = {time: rank for rank, time in enumerate(event_times)}
        for buffer_id, (extent, gen, kill) in function_lives.items():
            for offset in offsets.get((function, selected, buffer_id), []):
                plan[(extent, offset)] += 1
                normalized[(function, extent, offset,
                            ranks[gen], ranks[kill])] += 1
        module_peak = max(module_peak, peaks.get((function, selected), 0))
    ub_successful = {
        function: selected
        for function, selected in successful.items()
        if ((function, selected) in peaks or
            any(life_function == function and attempt == selected
                for life_function, attempt, _ in lives))
    }
    return ub_successful, module_peak, plan, normalized


def format_pair(pair: tuple[int, int], count: int) -> str:
    extent_bits, offset_bytes = pair
    return f"extent_bits={extent_bits} offset_bytes={offset_bytes} count={count}"


def print_counter_diff(
    title: str,
    lhs: Counter[tuple[int, int]],
    rhs: Counter[tuple[int, int]],
    max_diff: int,
) -> None:
    delta = lhs - rhs
    if not delta:
        return
    print(title)
    for index, (pair, count) in enumerate(sorted(delta.items())):
        if index >= max_diff:
            print(f"  ... {len(delta) - max_diff} more")
            break
        print(f"  {format_pair(pair, count)}")


def main() -> int:
    args = parse_args()
    payload = json.loads(args.model_json.read_text(encoding="utf-8"))
    if payload.get("precision") == "incomplete" or payload.get("status") == "blocker":
        raise ValueError("model report is not an authoritative exact plan")
    result = payload.get("result", payload)
    model_peak = int(result.get("peak_bits") or result.get("ub_peak_bits") or 0)
    model_attempt = result.get("selected_seed")
    selected_attempt = args.attempt
    if selected_attempt is None and model_attempt is not None:
        selected_attempt = int(model_attempt)

    oracle_attempt, oracle_peak, oracle_plan, oracle_lifetimes = parse_oracle(
        args.oracle_tsv, selected_attempt, args.scope)
    oracle_status, oracle_required, oracle_multi, oracle_inplace = (
        parse_oracle_contract(args.oracle_tsv, oracle_attempt, args.scope))
    model_plan = plan_multiset_from_model(payload)
    model_lifetimes = normalized_lifetimes_from_model(payload)
    model_multi, model_inplace = model_multi_and_inplace(payload)

    peak_match = oracle_peak == model_peak
    plan_match = oracle_plan == model_plan
    lifetime_match = oracle_lifetimes == model_lifetimes
    status_match = oracle_status == payload.get("status")
    model_required = int(payload.get("required_bits") or model_peak)
    required_match = oracle_required == model_required
    multi_match = oracle_multi == model_multi
    inplace_match = oracle_inplace == model_inplace

    print("Suffix-Compile Oracle Comparison")
    print("----------------------------------------------------------------------------")
    print(f"attempt        : {oracle_attempt}")
    print(f"scope          : {args.scope}")
    print(f"model peak     : {model_peak}")
    print(f"oracle peak    : {oracle_peak}")
    print(f"peak match     : {'yes' if peak_match else 'no'}")
    print(f"model buffers  : {sum(model_plan.values())}")
    print(f"oracle buffers : {sum(oracle_plan.values())}")
    print(f"offset match   : {'yes' if plan_match else 'no'}")
    print(f"lifetime match : {'yes' if lifetime_match else 'no'}")
    print(f"status match   : {'yes' if status_match else 'no'}")
    print(f"required match : {'yes' if required_match else 'no'}")
    print(f"multi match    : {'yes' if multi_match else 'no'}")
    print(f"inplace match  : {'yes' if inplace_match else 'no'}")

    if (not peak_match or not plan_match or not lifetime_match or
            not status_match or not required_match or not multi_match or
            not inplace_match):
        print()
        print_counter_diff("Only in model:", model_plan, oracle_plan, args.max_diff)
        print_counter_diff("Only in oracle:", oracle_plan, model_plan, args.max_diff)
        if not lifetime_match:
            print("Lifetime relation differs (function, extent, offset, gen-rank, kill-rank).")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
