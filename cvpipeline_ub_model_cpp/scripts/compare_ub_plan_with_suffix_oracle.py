#!/usr/bin/env python3
"""Compare model UB plan JSON with suffix-compile PlanMemory dump output."""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path
from typing import Counter


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


def plan_multiset_from_model(payload: dict) -> Counter[tuple[int, int]]:
    result = payload.get("result", {})
    plan = result.get("plan", [])
    multiset: Counter[tuple[int, int]] = collections.Counter()
    for buffer in plan:
        multiset[(int(buffer["extent_bits"]), int(buffer["offset_bytes"]))] += 1
    return multiset


def parse_oracle(
    path: Path, attempt: int | None, scope: str
) -> tuple[int, str | None, int | None, int | None, Counter[tuple[int, int]]]:
    success_attempts: list[int] = []
    statuses: dict[int, str] = {}
    peaks: dict[int, int] = {}
    required: dict[int, int] = {}
    buffers_by_attempt: dict[int, Counter[tuple[int, int]]] = collections.defaultdict(collections.Counter)

    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = raw_line.split("\t")
        if not fields:
            continue
        tag = fields[0]
        if tag == "PLANMEM_PLAN_ATTEMPT" and len(fields) >= 4:
            statuses[int(fields[2])] = fields[3]
            if fields[3] == "success":
                success_attempts.append(int(fields[2]))
        elif tag == "PLANMEM_REQUIRED" and len(fields) >= 4:
            line_attempt = int(fields[1])
            if fields[2] == scope:
                required[line_attempt] = int(fields[3])
        elif tag == "PLANMEM_PEAK" and len(fields) >= 4:
            line_attempt = int(fields[1])
            if fields[2] == scope:
                peaks[line_attempt] = max(peaks.get(line_attempt, 0),
                                          int(fields[3]))
        elif tag == "PLANMEM_EXACT_PLANNED_BUFFER" and len(fields) >= 6:
            line_attempt = int(fields[1])
            if fields[2] != scope:
                continue
            extent_bits = int(fields[4])
            for offset in fields[5:]:
                buffers_by_attempt[line_attempt][(extent_bits, int(offset))] += 1

    selected = attempt
    if selected is None:
        selected = success_attempts[0] if success_attempts else max(statuses, default=0)
    return (
        selected,
        statuses.get(selected),
        peaks.get(selected),
        required.get(selected),
        buffers_by_attempt[selected],
    )


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
    result = payload.get("result", {})
    model_overflow = bool(result.get("overflow"))
    model_peak = int(result.get("peak_bits") or result.get("ub_peak_bits") or 0)
    model_required = int(result.get("required_bits") or 0)
    model_attempt = result.get("selected_seed")
    selected_attempt = args.attempt
    if selected_attempt is None and model_attempt is not None:
        selected_attempt = int(model_attempt)

    oracle_attempt, oracle_status, oracle_peak, oracle_required, oracle_plan = parse_oracle(
        args.oracle_tsv, selected_attempt, args.scope)
    model_plan = plan_multiset_from_model(payload)

    status_match = ((oracle_status == "failure") if model_overflow
                    else (oracle_status == "success"))
    size_match = ((oracle_required == model_required) if model_overflow
                  else (oracle_peak == model_peak))
    plan_match = oracle_plan == model_plan

    print("Suffix-Compile Oracle Comparison")
    print("----------------------------------------------------------------------------")
    print(f"attempt        : {oracle_attempt}")
    print(f"scope          : {args.scope}")
    print(f"model status   : {'overflow' if model_overflow else 'success'}")
    print(f"oracle status  : {oracle_status}")
    print(f"status match   : {'yes' if status_match else 'no'}")
    if model_overflow:
        print(f"model required : {model_required}")
        print(f"oracle required: {oracle_required}")
        print(f"required match : {'yes' if size_match else 'no'}")
    else:
        print(f"model peak     : {model_peak}")
        print(f"oracle peak    : {oracle_peak}")
        print(f"peak match     : {'yes' if size_match else 'no'}")
    print(f"model buffers  : {sum(model_plan.values())}")
    print(f"oracle buffers : {sum(oracle_plan.values())}")
    print(f"offset match   : {'yes' if plan_match else 'no'}")

    if not status_match or not size_match or not plan_match:
        print()
        print_counter_diff("Only in model:", model_plan, oracle_plan, args.max_diff)
        print_counter_diff("Only in oracle:", oracle_plan, model_plan, args.max_diff)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
