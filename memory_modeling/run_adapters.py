"""Run S8.5 model and PlanMemory oracle for adapter files under data/."""

from __future__ import annotations

import argparse
import csv
import json
import re
import shutil
import subprocess
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Any

from .core import MemoryScope, to_jsonable
from .s8_local_model import LOCAL_SCOPE_INFO
from .scanner import create_report


REQUIRES_BITS_RE = re.compile(r"requires\s+(?P<bits>\d+)\s+bits")
AVAILABLE_BITS_RE = re.compile(r"while\s+(?P<bits>\d+)\s+bits\s+available")


@dataclass(frozen=True)
class ScopePeak:
    scope: str
    status: str
    peak_bits: int
    peak_bytes: int
    required_bits: int | None = None
    capacity_bits: int | None = None
    error_info: str = ""
    source: str = ""


@dataclass
class AdapterRunResult:
    input_file: str
    work_dir: str
    suffix_input_mlir: str | None = None
    memory_json: str | None = None
    model_memory_info_json: str | None = None
    oracle_returncode: int | None = None
    oracle_status: str = "not_run"
    ub_model_peak_bits: int | None = None
    ub_model_peak_bytes: int | None = None
    ub_ascendir_peak_bits_from_max_addr: int | None = None
    ub_ascendir_peak_bytes_from_max_addr: int | None = None
    ub_overflow_required_bits: int | None = None
    ub_capacity_bits: int | None = None
    ub_delta_model_minus_ascendir_bits: int | None = None
    ub_overflow: bool | None = None
    after_plan_mlir: str | None = None
    oracle_log: str | None = None
    memory_info_files: list[str] | None = None
    errors: list[str] | None = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", type=Path, default=Path("data"))
    parser.add_argument(
        "--pattern",
        action="append",
        default=None,
        help="Glob pattern under data-dir. Can be repeated. Default: *",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        default=Path("Output/memory_modeling_adapters"),
        help="Root directory for run artifacts.",
    )
    parser.add_argument(
        "--run-name",
        default=None,
        help="Run directory name. Defaults to timestamp.",
    )
    parser.add_argument(
        "--bishengir-opt",
        type=Path,
        default=Path("build/bin/bishengir-opt"),
        help="Path to bishengir-opt.",
    )
    parser.add_argument(
        "--no-oracle",
        action="store_true",
        help="Only run the Python S8.5 model, skip C++ PlanMemory oracle.",
    )
    parser.add_argument(
        "--fail-on-oracle-error",
        action="store_true",
        help="Exit non-zero when any oracle invocation returns non-zero.",
    )
    parser.add_argument("--pretty", action="store_true", help="Pretty-print comparison JSON")
    return parser.parse_args()


def sanitize_name(path: Path) -> str:
    text = path.name
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", text)


def discover_inputs(data_dir: Path, patterns: list[str]) -> list[Path]:
    seen: set[Path] = set()
    inputs: list[Path] = []
    for pattern in patterns:
        for path in sorted(data_dir.glob(pattern)):
            if not path.is_file() or path.name.startswith("."):
                continue
            resolved = path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            inputs.append(path)
    return inputs


def bits_to_bytes(bits: int | None) -> int | None:
    if bits is None:
        return None
    return (bits + 7) // 8


def bits_to_kib(bits: int | None) -> float | None:
    byte_count = bits_to_bytes(bits)
    if byte_count is None:
        return None
    return byte_count / 1024


def build_model_memory_info(report: Any) -> dict[str, Any]:
    buffers_by_id = {buffer.stable_id: buffer for buffer in report.state.buffers}
    functions = report.state.metadata.get("functions", {})
    records: list[dict[str, Any]] = []

    for function_name, function_info in functions.items():
        allocations_by_scope = function_info.get("allocations", {})
        for scope, allocations in allocations_by_scope.items():
            memory_info_array: list[dict[str, Any]] = []
            for allocation in allocations:
                buffer = buffers_by_id.get(allocation["buffer"])
                if buffer is None:
                    continue
                size_bits = buffer.size_bits
                aligned_size_bits = buffer.aligned_size_bits
                offset_bits = int(allocation["offset_bits"])
                extent_bits = int(allocation["size_bits"])
                memory_info_array.append(
                    {
                        "name": buffer.stable_id,
                        "raw_name": buffer.attrs.get("raw_name"),
                        "function": function_name,
                        "type": buffer.type_text,
                        "scope": scope,
                        "copy_index": allocation["copy_index"],
                        "start": allocation["start"],
                        "end": allocation["end"],
                        "offset_bits": offset_bits,
                        "offset_bytes": bits_to_bytes(offset_bits),
                        "extent_bits": extent_bits,
                        "extent_bytes": bits_to_bytes(extent_bits),
                        "end_addr_bits": offset_bits + extent_bits,
                        "end_addr_bytes": bits_to_bytes(offset_bits + extent_bits),
                        "size_bits": size_bits,
                        "size_bytes": bits_to_bytes(size_bits),
                        "aligned_size_bits": aligned_size_bits,
                        "aligned_size_bytes": bits_to_bytes(aligned_size_bits),
                        "multi_buffer_factor": buffer.multi_buffer_factor,
                        "use_count": buffer.attrs.get("use_count"),
                        "element_type": buffer.attrs.get("element_type"),
                        "element_bits": buffer.attrs.get("element_bits"),
                        "shape": buffer.attrs.get("shape"),
                    }
                )
            peak_bits = function_info.get("scope_peaks_bits", {}).get(scope, 0)
            records.append(
                {
                    "function": function_name,
                    "scope": scope,
                    "status": "model",
                    "peak_bits": peak_bits,
                    "peak_bytes": bits_to_bytes(peak_bits),
                    "peak_kib": bits_to_kib(peak_bits),
                    "memory_info_array": memory_info_array,
                }
            )

    return {
        "source": "s8.5_text_model",
        "input_file": report.input_file,
        "checkpoint": report.checkpoint.checkpoint_id,
        "precision": report.state.precision.value,
        "Record": records,
        "blockers": report.state.blockers,
    }


def model_scope_peaks(
    input_file: Path, model_memory_info_path: Path
) -> dict[str, ScopePeak]:
    report = create_report(input_file, checkpoint_id="S8.5")
    model_memory_info_path.write_text(
        json.dumps(build_model_memory_info(report), indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    peaks: dict[str, ScopePeak] = {}
    for peak in report.state.peaks:
        scope = peak.scope.value
        peaks[scope] = ScopePeak(
            scope=scope,
            status="predicted",
            peak_bits=peak.peak_bits or 0,
            peak_bytes=peak.peak_bytes or 0,
            required_bits=peak.required_bits,
            capacity_bits=peak.capacity_bits,
            source=peak.source,
        )
    return peaks


def parse_memory_display_file(path: Path) -> dict[str, ScopePeak]:
    data = json.loads(path.read_text(encoding="utf-8"))
    peaks: dict[str, ScopePeak] = {}
    for record in data.get("Record", []):
        raw_scope = str(record.get("scope", "unknown"))
        scope = raw_scope.upper()
        values: list[int] = []
        for info in record.get("memory_info_array", []):
            extent_bits = int(info.get("extent") or 0)
            offsets = info.get("offset") or [0]
            values.extend(int(offset_bytes) * 8 + extent_bits for offset_bytes in offsets)
        peak_bits = max(values) if values else 0
        error_info = record.get("error_info", "") or ""
        required_match = REQUIRES_BITS_RE.search(error_info)
        available_match = AVAILABLE_BITS_RE.search(error_info)
        required_bits = int(required_match.group("bits")) if required_match else None
        capacity_bits = (
            int(available_match.group("bits"))
            if available_match
            else LOCAL_SCOPE_INFO.get(MemoryScope(scope), (None, None))[1]
            if scope in MemoryScope._value2member_map_
            else None
        )
        if required_bits is not None:
            peak_bits = max(peak_bits, required_bits)
        peaks[scope] = ScopePeak(
            scope=scope,
            status=str(record.get("status", "unknown")),
            peak_bits=peak_bits,
            peak_bytes=(peak_bits + 7) // 8,
            required_bits=required_bits,
            capacity_bits=capacity_bits,
            error_info=error_info,
            source=str(path),
        )
    return peaks


def collect_oracle_peaks(work_dir: Path) -> tuple[dict[str, ScopePeak], list[str]]:
    peaks: dict[str, ScopePeak] = {}
    files = sorted(work_dir.glob("memory_info*.json"))
    for path in files:
        peaks.update(parse_memory_display_file(path))
    return peaks, [str(path) for path in files]


def run_oracle(
    input_file: Path, work_dir: Path, bishengir_opt: Path
) -> tuple[int, Path, Path | None]:
    log_path = work_dir / "oracle.log"
    after_plan = work_dir.resolve() / "after_plan.mlir"
    command = [
        str(bishengir_opt.resolve()),
        str(input_file.resolve()),
        "--hivm-plan-memory=mem-plan-mode=local-mem-plan enable-memory-display=true",
        "-o",
        str(after_plan),
    ]
    proc = subprocess.run(
        command,
        cwd=work_dir,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    log_text = ""
    if proc.stdout:
        log_text += "[stdout]\n" + proc.stdout
    if proc.stderr:
        if log_text and not log_text.endswith("\n"):
            log_text += "\n"
        log_text += "[stderr]\n" + proc.stderr
    if log_text:
        log_path.write_text(log_text, encoding="utf-8")
        return proc.returncode, after_plan, log_path
    return proc.returncode, after_plan, None


def fill_ub_comparison(
    result: AdapterRunResult,
    model_peaks: dict[str, ScopePeak],
    oracle_peaks: dict[str, ScopePeak],
) -> None:
    model_ub = model_peaks.get("UB")
    oracle_ub = oracle_peaks.get("UB")
    if model_ub is not None:
        result.ub_model_peak_bits = model_ub.peak_bits
        result.ub_model_peak_bytes = model_ub.peak_bytes
    if oracle_ub is not None:
        result.ub_ascendir_peak_bits_from_max_addr = oracle_ub.peak_bits
        result.ub_ascendir_peak_bytes_from_max_addr = oracle_ub.peak_bytes
        result.ub_overflow_required_bits = oracle_ub.required_bits
        result.ub_capacity_bits = oracle_ub.capacity_bits
        result.ub_overflow = oracle_ub.status == "fail"
    if (
        result.ub_model_peak_bits is not None
        and result.ub_ascendir_peak_bits_from_max_addr is not None
    ):
        result.ub_delta_model_minus_ascendir_bits = (
            result.ub_model_peak_bits - result.ub_ascendir_peak_bits_from_max_addr
        )


def write_case_memory_json(result: AdapterRunResult, path: Path) -> None:
    result.memory_json = str(path)
    payload = {
        "input_file": result.input_file,
        "suffix_input_mlir": result.suffix_input_mlir,
        "oracle_status": result.oracle_status,
        "oracle_returncode": result.oracle_returncode,
        "ub": {
            "model_peak_bits": result.ub_model_peak_bits,
            "model_peak_bytes": result.ub_model_peak_bytes,
            "ascendir_peak_bits_from_max_addr": result.ub_ascendir_peak_bits_from_max_addr,
            "ascendir_peak_bytes_from_max_addr": result.ub_ascendir_peak_bytes_from_max_addr,
            "delta_model_minus_ascendir_bits": result.ub_delta_model_minus_ascendir_bits,
            "overflow": result.ub_overflow,
            "overflow_required_bits": result.ub_overflow_required_bits,
            "capacity_bits": result.ub_capacity_bits,
        },
        "artifacts": {
            "model_memory_info_json": result.model_memory_info_json,
            "after_plan_mlir": result.after_plan_mlir,
            "memory_info_files": result.memory_info_files or [],
            "oracle_log": result.oracle_log,
        },
        "errors": result.errors or [],
    }
    path.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def run_one(
    input_file: Path,
    run_dir: Path,
    bishengir_opt: Path,
    no_oracle: bool,
) -> AdapterRunResult:
    work_dir = run_dir / sanitize_name(input_file)
    work_dir.mkdir(parents=True, exist_ok=True)
    suffix_input = work_dir / "suffix_input.mlir"
    memory_json = work_dir / "memory.json"
    model_memory_info = work_dir / "model_memory_info.json"
    shutil.copyfile(input_file, suffix_input)
    result = AdapterRunResult(
        input_file=str(input_file),
        work_dir=str(work_dir),
        suffix_input_mlir=str(suffix_input),
        model_memory_info_json=str(model_memory_info),
        memory_info_files=[],
        errors=[],
    )

    model_peaks: dict[str, ScopePeak] = {}
    oracle_peaks: dict[str, ScopePeak] = {}
    try:
        model_peaks = model_scope_peaks(input_file, model_memory_info)
    except Exception as exc:  # noqa: BLE001 - keep batch runner moving.
        result.errors = (result.errors or []) + [f"model failed: {exc}"]

    if no_oracle:
        result.oracle_status = "skipped"
        fill_ub_comparison(result, model_peaks, oracle_peaks)
        write_case_memory_json(result, memory_json)
        return result

    if not bishengir_opt.is_file():
        result.oracle_status = "missing_binary"
        result.errors = (result.errors or []) + [
            f"bishengir-opt not found: {bishengir_opt}"
        ]
        fill_ub_comparison(result, model_peaks, oracle_peaks)
        write_case_memory_json(result, memory_json)
        return result

    returncode, after_plan, log_path = run_oracle(input_file, work_dir, bishengir_opt)
    result.oracle_returncode = returncode
    result.oracle_status = "success" if returncode == 0 else "failed"
    result.after_plan_mlir = str(after_plan) if after_plan.exists() else None
    result.oracle_log = str(log_path) if log_path is not None else None
    oracle_peaks, memory_info_files = collect_oracle_peaks(work_dir)
    result.memory_info_files = memory_info_files
    fill_ub_comparison(result, model_peaks, oracle_peaks)
    write_case_memory_json(result, memory_json)
    return result


def write_csv(results: list[AdapterRunResult], path: Path) -> None:
    fieldnames = [
        "input_file",
        "oracle_status",
        "oracle_returncode",
        "ub_model_peak_bits",
        "ub_ascendir_peak_bits_from_max_addr",
        "ub_delta_model_minus_ascendir_bits",
        "ub_overflow",
        "ub_overflow_required_bits",
        "ub_capacity_bits",
        "memory_json",
        "model_memory_info_json",
        "suffix_input_mlir",
        "after_plan_mlir",
        "oracle_log",
        "work_dir",
        "errors",
    ]

    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for result in results:
            row = {
                field_name: getattr(result, field_name)
                for field_name in fieldnames
                if field_name != "errors"
            }
            row["errors"] = " | ".join(result.errors or [])
            writer.writerow(row)


def print_summary(results: list[AdapterRunResult]) -> None:
    for result in results:
        print(f"[{result.oracle_status}] {result.input_file}")
        print(
            "  UB: "
            f"model={result.ub_model_peak_bits or '-'} bits, "
            f"ascendir_max_addr={result.ub_ascendir_peak_bits_from_max_addr or '-'} bits, "
            f"delta={result.ub_delta_model_minus_ascendir_bits if result.ub_delta_model_minus_ascendir_bits is not None else '-'} bits, "
            f"overflow_required={result.ub_overflow_required_bits or '-'} bits, "
            f"capacity={result.ub_capacity_bits or '-'} bits"
        )
        print(f"  memory_json: {result.memory_json}")
        for error in result.errors or []:
            print(f"  error: {error}")


def main() -> int:
    args = parse_args()
    patterns = args.pattern or ["*"]
    data_dir = args.data_dir
    if not data_dir.is_dir():
        raise SystemExit(f"data directory not found: {data_dir}")

    run_name = args.run_name or datetime.now().strftime("run_%Y%m%d_%H%M%S")
    run_dir = args.output_root / run_name
    run_dir.mkdir(parents=True, exist_ok=True)

    inputs = discover_inputs(data_dir, patterns)
    results = [
        run_one(input_file, run_dir, args.bishengir_opt, args.no_oracle)
        for input_file in inputs
    ]

    comparison_json = run_dir / "comparison.json"
    comparison_csv = run_dir / "comparison.csv"
    comparison_json.write_text(
        json.dumps(
            to_jsonable([asdict(result) for result in results]),
            indent=2 if args.pretty else None,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    write_csv(results, comparison_csv)
    print_summary(results)
    print(f"[INFO] comparison JSON: {comparison_json}")
    print(f"[INFO] comparison CSV:  {comparison_csv}")
    return 1 if args.fail_on_oracle_error and any(r.oracle_returncode for r in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
