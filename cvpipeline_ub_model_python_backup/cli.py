"""Command line helpers for CVPipeline UB modeling."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
import sys

from .ir_parser import iter_before_planmemory_files, parse_planmemory_before_ir
from .lifetime import analyze_lifetimes
from .plan_memory import (
    first_fit_plan,
    iter_memory_info_files,
    load_memory_info,
    replay_planmemory_offsets,
)


def _result_dict(case: str, memory_info: Path, oracle_result, first_fit_result=None):
    row = {
        "case": case,
        "memory_info": str(memory_info),
        "peak_bytes": oracle_result.peak_bytes,
        "peak_bits": oracle_result.peak_bits,
        "capacity_bits": oracle_result.capacity_bits,
        "overflow": str(oracle_result.overflow).lower(),
        "alloc_count": oracle_result.alloc_count,
        "precision": "exact_plan",
        "mode": oracle_result.mode,
    }
    if first_fit_result is not None:
        row.update(
            {
                "first_fit_peak_bytes": first_fit_result.peak_bytes,
                "first_fit_peak_bits": first_fit_result.peak_bits,
                "first_fit_delta_bits": first_fit_result.peak_bits
                - oracle_result.peak_bits,
            }
        )
    return row


def cmd_summarize(args: argparse.Namespace) -> int:
    info = load_memory_info(args.memory_info, memory_space=args.memory_space)
    result = replay_planmemory_offsets(info.records)
    first_fit = first_fit_plan(info.records) if args.include_first_fit else None
    row = _result_dict(Path(args.memory_info).parent.name, Path(args.memory_info), result, first_fit)

    if args.format == "json":
        print(json.dumps(row, indent=2, sort_keys=True))
    else:
        for key, value in row.items():
            print(f"{key}\t{value}")
    return 0


def _read_expected_summary(path: Path) -> dict[str, dict[str, str]]:
    with path.open() as f:
        return {row["case"]: row for row in csv.DictReader(f, delimiter="\t")}


def _buffer_name(record) -> str:
    return record.buffer.split("=", 1)[0].strip()


def _overlaps(left: tuple[int, int], right: tuple[int, int]) -> bool:
    return not (left[1] < right[0] or right[1] < left[0])


def _ir_parse_row(
    before_ir: Path,
    expected_row: dict[str, str] | None,
    memory_space: str,
    core_type: str,
) -> dict[str, object]:
    parsed = parse_planmemory_before_ir(before_ir)
    ir_records = parsed.filter(core_type=core_type, memory_space=memory_space)
    memory_info_path = before_ir.with_name("memory_info_aiv.json")
    memory_records = ()
    if memory_info_path.exists():
        memory_records = load_memory_info(memory_info_path, memory_space=memory_space).records

    ir_names = {record.ssa_name for record in ir_records}
    memory_names = {_buffer_name(record) for record in memory_records}
    ir_extent_multiset = sorted(record.extent_bits for record in ir_records)
    memory_extent_multiset = sorted(record.extent_bits for record in memory_records)
    ir_tmpbuf_names = {record.ssa_name for record in ir_records if record.is_tmpbuf_hint}
    memory_tmpbuf_names = {
        _buffer_name(record) for record in memory_records if record.is_tmpbuf
    }
    extent_mismatch_names = []
    if memory_records:
        ir_by_name = {record.ssa_name: record for record in ir_records}
        memory_by_name = {_buffer_name(record): record for record in memory_records}
        for name in sorted(ir_names & memory_names):
            if ir_by_name[name].extent_bits != memory_by_name[name].extent_bits:
                extent_mismatch_names.append(name)
    expected_alloc_count = (
        int(expected_row["aiv_ub_alloc_count"]) if expected_row is not None else -1
    )
    alloc_count_match = (
        expected_alloc_count == -1 or len(ir_records) == expected_alloc_count
    )
    memory_count_match = not memory_records or len(ir_records) == len(memory_records)
    name_set_match = not memory_records or ir_names == memory_names
    extent_multiset_match = (
        not memory_records or ir_extent_multiset == memory_extent_multiset
    )
    tmpbuf_set_match = not memory_records or ir_tmpbuf_names == memory_tmpbuf_names
    # B1 validates the parser's allocation identity.  PlanMemory size
    # normalization is deliberately left to B3, so extent mismatches are
    # diagnostic here rather than a B1 failure.
    match = (
        alloc_count_match
        and memory_count_match
        and name_set_match
        and tmpbuf_set_match
    )

    return {
        "case": before_ir.parent.name,
        "match": str(match).lower(),
        "ir_alloc_count": len(ir_records),
        "expected_alloc_count": expected_alloc_count,
        "memory_info_alloc_count": len(memory_records),
        "alloc_count_match": str(alloc_count_match).lower(),
        "name_set_match": str(name_set_match).lower(),
        "extent_multiset_match": str(extent_multiset_match).lower(),
        "extent_mismatch_names": ",".join(extent_mismatch_names),
        "tmpbuf_set_match": str(tmpbuf_set_match).lower(),
        "ir_tmpbuf_count": len(ir_tmpbuf_names),
        "memory_info_tmpbuf_count": len(memory_tmpbuf_names),
        "missing_in_ir": ",".join(sorted(memory_names - ir_names)),
        "extra_in_ir": ",".join(sorted(ir_names - memory_names)),
        "before_ir": str(before_ir),
        "memory_info": str(memory_info_path) if memory_info_path.exists() else "",
    }


def cmd_parse_ir(args: argparse.Namespace) -> int:
    parsed = parse_planmemory_before_ir(args.before_ir)
    records = parsed.filter(core_type=args.core_type, memory_space=args.memory_space)
    if args.format == "json":
        print(
            json.dumps(
                [
                    {
                        "ssa_name": record.ssa_name,
                        "memref_type": record.memref_type,
                        "shape": record.shape,
                        "element_type": record.element_type,
                        "element_bits": record.element_bits,
                        "extent_bits": record.extent_bits,
                        "memory_space": record.memory_space,
                        "function_name": record.function_name,
                        "core_type": record.core_type,
                        "line": record.line,
                        "alignment_bytes": record.alignment_bytes,
                        "is_tmpbuf_hint": record.is_tmpbuf_hint,
                    }
                    for record in records
                ],
                indent=2,
                sort_keys=True,
            )
        )
        return 0

    print(f"before_ir\t{parsed.path}")
    print(f"core_type\t{args.core_type}")
    print(f"memory_space\t{args.memory_space}")
    print(f"alloc_count\t{len(records)}")
    print(f"tmpbuf_count\t{sum(record.is_tmpbuf_hint for record in records)}")
    print(f"extent_bits_total\t{sum(record.extent_bits for record in records)}")
    return 0


def cmd_batch(args: argparse.Namespace) -> int:
    rows = []
    for memory_info in iter_memory_info_files(args.root):
        case = memory_info.parent.name
        info = load_memory_info(memory_info, memory_space=args.memory_space)
        result = replay_planmemory_offsets(info.records)
        first_fit = first_fit_plan(info.records) if args.include_first_fit else None
        rows.append(_result_dict(case, memory_info, result, first_fit))

    fields = list(rows[0].keys()) if rows else [
        "case",
        "memory_info",
        "peak_bytes",
        "peak_bits",
        "capacity_bits",
        "overflow",
        "alloc_count",
        "precision",
        "mode",
    ]
    output = Path(args.output) if args.output else None
    out_f = output.open("w", newline="") if output else sys.stdout
    try:
        writer = csv.DictWriter(out_f, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if output:
            out_f.close()
    return 0


def cmd_validate(args: argparse.Namespace) -> int:
    expected = _read_expected_summary(Path(args.expected_summary))
    rows = []
    failures = 0
    for memory_info in iter_memory_info_files(args.root):
        case = memory_info.parent.name
        if case not in expected:
            continue
        info = load_memory_info(memory_info, memory_space=args.memory_space)
        result = replay_planmemory_offsets(info.records)
        expected_row = expected[case]
        expected_bits = int(expected_row["aiv_ub_peak_bits"])
        expected_bytes = int(expected_row["aiv_ub_peak_bytes"])
        expected_alloc_count = int(expected_row["aiv_ub_alloc_count"])
        match = (
            result.peak_bits == expected_bits
            and result.peak_bytes == expected_bytes
            and result.alloc_count == expected_alloc_count
        )
        failures += 0 if match else 1
        rows.append(
            {
                "case": case,
                "match": str(match).lower(),
                "expected_peak_bits": expected_bits,
                "actual_peak_bits": result.peak_bits,
                "delta_bits": result.peak_bits - expected_bits,
                "expected_peak_bytes": expected_bytes,
                "actual_peak_bytes": result.peak_bytes,
                "expected_alloc_count": expected_alloc_count,
                "actual_alloc_count": result.alloc_count,
                "memory_info": str(memory_info),
            }
        )

    fields = [
        "case",
        "match",
        "expected_peak_bits",
        "actual_peak_bits",
        "delta_bits",
        "expected_peak_bytes",
        "actual_peak_bytes",
        "expected_alloc_count",
        "actual_alloc_count",
        "memory_info",
    ]
    output = Path(args.output) if args.output else None
    out_f = output.open("w", newline="") if output else sys.stdout
    try:
        writer = csv.DictWriter(out_f, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if output:
            out_f.close()

    if args.quiet:
        return 1 if failures else 0
    print(f"validated_cases={len(rows)}")
    print(f"failures={failures}")
    return 1 if failures else 0


def cmd_validate_ir(args: argparse.Namespace) -> int:
    expected = (
        _read_expected_summary(Path(args.expected_summary))
        if args.expected_summary
        else {}
    )
    rows = []
    failures = 0
    for before_ir in iter_before_planmemory_files(args.root):
        expected_row = expected.get(before_ir.parent.name)
        row = _ir_parse_row(before_ir, expected_row, args.memory_space, args.core_type)
        failures += 0 if row["match"] == "true" else 1
        rows.append(row)

    fields = [
        "case",
        "match",
        "ir_alloc_count",
        "expected_alloc_count",
        "memory_info_alloc_count",
        "alloc_count_match",
        "name_set_match",
        "extent_multiset_match",
        "extent_mismatch_names",
        "tmpbuf_set_match",
        "ir_tmpbuf_count",
        "memory_info_tmpbuf_count",
        "missing_in_ir",
        "extra_in_ir",
        "before_ir",
        "memory_info",
    ]
    output = Path(args.output) if args.output else None
    out_f = output.open("w", newline="") if output else sys.stdout
    try:
        writer = csv.DictWriter(out_f, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if output:
            out_f.close()

    if args.quiet:
        return 1 if failures else 0
    print(f"validated_ir_cases={len(rows)}")
    print(f"failures={failures}")
    return 1 if failures else 0


def _lifetime_rows(before_ir: Path, memory_space: str, core_type: str):
    analysis = analyze_lifetimes(
        before_ir, memory_space=memory_space, core_type=core_type
    )
    memory_info_path = before_ir.with_name("memory_info_aiv.json")
    memory_records = ()
    if memory_info_path.exists():
        memory_records = load_memory_info(memory_info_path, memory_space=memory_space).records
    memory_by_name = {_buffer_name(record): record for record in memory_records}

    rows = []
    for record in analysis.records:
        memory_record = memory_by_name.get(record.ssa_name)
        expected_lifetime = memory_record.lifetime if memory_record else (-1, -1)
        actual_lifetime = record.lifetime
        direct_lifetime = record.direct_lifetime
        strict_match = actual_lifetime == expected_lifetime
        overlap_match = (
            expected_lifetime == (-1, -1)
            or _overlaps(actual_lifetime, expected_lifetime)
            == _overlaps(expected_lifetime, actual_lifetime)
        )
        rows.append(
            {
                "case": before_ir.parent.name,
                "ssa_name": record.ssa_name,
                "strict_match": str(strict_match).lower(),
                "overlap_match": str(overlap_match).lower(),
                "actual_start": actual_lifetime[0],
                "actual_end": actual_lifetime[1],
                "expected_start": expected_lifetime[0],
                "expected_end": expected_lifetime[1],
                "delta_start": actual_lifetime[0] - expected_lifetime[0],
                "delta_end": actual_lifetime[1] - expected_lifetime[1],
                "direct_start": direct_lifetime[0],
                "direct_end": direct_lifetime[1],
                "group": ",".join(record.group),
                "memory_info": str(memory_info_path) if memory_info_path.exists() else "",
                "before_ir": str(before_ir),
            }
        )
    return rows


def cmd_lifetime(args: argparse.Namespace) -> int:
    rows = _lifetime_rows(Path(args.before_ir), args.memory_space, args.core_type)
    if args.format == "json":
        print(json.dumps(rows, indent=2, sort_keys=True))
        return 0

    fields = [
        "ssa_name",
        "actual_start",
        "actual_end",
        "expected_start",
        "expected_end",
        "strict_match",
        "overlap_match",
        "direct_start",
        "direct_end",
        "group",
    ]
    writer = csv.DictWriter(sys.stdout, fieldnames=fields, delimiter="\t")
    writer.writeheader()
    for row in rows:
        writer.writerow({field: row[field] for field in fields})
    return 0


def cmd_validate_lifetime(args: argparse.Namespace) -> int:
    rows = []
    for before_ir in iter_before_planmemory_files(args.root):
        rows.extend(_lifetime_rows(before_ir, args.memory_space, args.core_type))

    fields = [
        "case",
        "ssa_name",
        "strict_match",
        "overlap_match",
        "actual_start",
        "actual_end",
        "expected_start",
        "expected_end",
        "delta_start",
        "delta_end",
        "direct_start",
        "direct_end",
        "group",
        "memory_info",
        "before_ir",
    ]
    output = Path(args.output) if args.output else None
    out_f = output.open("w", newline="") if output else sys.stdout
    try:
        writer = csv.DictWriter(out_f, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if output:
            out_f.close()

    strict_failures = sum(row["strict_match"] != "true" for row in rows)
    overlap_failures = sum(row["overlap_match"] != "true" for row in rows)
    pairwise_failures = 0
    pairwise_total = 0
    rows_by_case: dict[str, list[dict[str, object]]] = {}
    for row in rows:
        rows_by_case.setdefault(str(row["case"]), []).append(row)
    for case_rows in rows_by_case.values():
        for index, left in enumerate(case_rows):
            for right in case_rows[index + 1 :]:
                actual_left = (int(left["actual_start"]), int(left["actual_end"]))
                actual_right = (int(right["actual_start"]), int(right["actual_end"]))
                expected_left = (
                    int(left["expected_start"]),
                    int(left["expected_end"]),
                )
                expected_right = (
                    int(right["expected_start"]),
                    int(right["expected_end"]),
                )
                actual_overlap = _overlaps(actual_left, actual_right)
                expected_overlap = _overlaps(expected_left, expected_right)
                pairwise_total += 1
                pairwise_failures += 0 if actual_overlap == expected_overlap else 1
    cases = len({row["case"] for row in rows})
    if args.quiet:
        return 1 if strict_failures else 0
    print(f"validated_lifetime_cases={cases}")
    print(f"validated_lifetime_records={len(rows)}")
    print(f"strict_failures={strict_failures}")
    print(f"overlap_failures={overlap_failures}")
    print(f"pairwise_overlap_failures={pairwise_failures}")
    print(f"pairwise_overlap_total={pairwise_total}")
    return 1 if strict_failures else 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Stage-B PlanMemory helpers for the CVPipeline UB model."
    )
    sub = parser.add_subparsers(dest="command", required=True)

    summarize = sub.add_parser("summarize", help="Summarize one memory_info JSON")
    summarize.add_argument("memory_info")
    summarize.add_argument("--memory-space", default="ub")
    summarize.add_argument("--format", choices=["text", "json"], default="text")
    summarize.add_argument("--include-first-fit", action="store_true")
    summarize.set_defaults(func=cmd_summarize)

    batch = sub.add_parser("batch", help="Summarize every memory_info_aiv.json under a root")
    batch.add_argument("--root", required=True)
    batch.add_argument("--output")
    batch.add_argument("--memory-space", default="ub")
    batch.add_argument("--include-first-fit", action="store_true")
    batch.set_defaults(func=cmd_batch)

    validate = sub.add_parser("validate", help="Validate replayed peaks against a TSV summary")
    validate.add_argument("--root", required=True)
    validate.add_argument("--expected-summary", required=True)
    validate.add_argument("--output")
    validate.add_argument("--memory-space", default="ub")
    validate.add_argument("--quiet", action="store_true")
    validate.set_defaults(func=cmd_validate)

    parse_ir = sub.add_parser("parse-ir", help="Parse one PlanMemory-before IR")
    parse_ir.add_argument("before_ir")
    parse_ir.add_argument("--memory-space", default="ub")
    parse_ir.add_argument("--core-type", default="AIV")
    parse_ir.add_argument("--format", choices=["text", "json"], default="text")
    parse_ir.set_defaults(func=cmd_parse_ir)

    validate_ir = sub.add_parser(
        "validate-ir",
        help="Validate PlanMemory-before IR parsing against memory_info and summary TSV",
    )
    validate_ir.add_argument("--root", required=True)
    validate_ir.add_argument("--expected-summary")
    validate_ir.add_argument("--output")
    validate_ir.add_argument("--memory-space", default="ub")
    validate_ir.add_argument("--core-type", default="AIV")
    validate_ir.add_argument("--quiet", action="store_true")
    validate_ir.set_defaults(func=cmd_validate_ir)

    lifetime = sub.add_parser("lifetime", help="Analyze one PlanMemory-before IR lifetime")
    lifetime.add_argument("before_ir")
    lifetime.add_argument("--memory-space", default="ub")
    lifetime.add_argument("--core-type", default="AIV")
    lifetime.add_argument("--format", choices=["text", "json"], default="text")
    lifetime.set_defaults(func=cmd_lifetime)

    validate_lifetime = sub.add_parser(
        "validate-lifetime",
        help="Validate B2 lifetime analysis against memory_info life_time_in_ir",
    )
    validate_lifetime.add_argument("--root", required=True)
    validate_lifetime.add_argument("--output")
    validate_lifetime.add_argument("--memory-space", default="ub")
    validate_lifetime.add_argument("--core-type", default="AIV")
    validate_lifetime.add_argument("--quiet", action="store_true")
    validate_lifetime.set_defaults(func=cmd_validate_lifetime)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
