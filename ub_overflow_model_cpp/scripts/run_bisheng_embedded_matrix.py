#!/usr/bin/env python3
"""Compare the embedded UB model with real PlanMemory in one BiSheng run."""

from __future__ import annotations

import argparse
from collections import Counter
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait
import csv
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from plan_memory_contract import (
    canonical_function_name,
    model_multi_and_inplace,
    normalized_lifetimes_from_model,
    parse_oracle,
    parse_oracle_contract,
    plan_multiset_from_model,
)
from validation_common import (
    FailureTaxonomy,
    compare_failure,
    counter_evidence,
    bisheng_failure_signature,
    model_failure_signature,
    scalar_evidence,
)


MODULE = SCRIPT_DIR.parent
REPO = MODULE.parent
DEFAULT_MATRIX = MODULE / "config/ub_relevant_parameter_scenarios.tsv"
DEFAULT_TIMEOUT_PAIRS = MODULE / "config/known_timeout_pairs.tsv"
DEFAULT_ADAPTER_ROOT = MODULE / "data/adapter"
DEFAULT_MATRIX_INPUT_ROOT = MODULE / "data/before_cvpipelining"
DEFAULT_COMPILER = REPO / "build/bin/bishengir-compile"
DEFAULT_REPORT = MODULE / "output/bisheng_embedded_validation.tsv"
FAILURE_TAXONOMY = FailureTaxonomy.load()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run the embedded lightweight model and the real BiSheng pipeline "
            "in one process, then compare their local PlanMemory contracts."
        )
    )
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument(
        "--known-timeout-pairs", type=Path, default=DEFAULT_TIMEOUT_PAIRS
    )
    parser.add_argument("--adapter-root", type=Path, default=DEFAULT_ADAPTER_ROOT)
    parser.add_argument("--compiler", type=Path, default=DEFAULT_COMPILER)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--config", "--scenario", dest="scenarios",
                        action="append", default=[], metavar="NAME")
    parser.add_argument("--input", action="append", default=[],
                        metavar="ADAPTER.ttadapter")
    parser.add_argument("--max-inputs", type=int, default=0)
    parser.add_argument("--seeds", default="0-19")
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument("--timeout", type=int, default=360)
    parser.add_argument("--list", action="store_true")
    parser.add_argument(
        "--resume", action="store_true",
        help="resume rows already present in --report",
    )
    parser.add_argument(
        "--include-known-timeouts", action="store_true",
        help="run configuration/input pairs excluded as known timeouts",
    )
    parser.add_argument("--no-progress", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    if args.jobs <= 0 or args.timeout <= 0:
        parser.error("--jobs and --timeout must be positive")
    if args.max_inputs < 0:
        parser.error("--max-inputs must be non-negative")
    return args


def boolean(value: str) -> bool:
    return value.strip().lower() in {"1", "true", "yes", "on"}


def flag(value: str) -> str:
    return "true" if boolean(value) else "false"


def selected_seeds(value: str) -> list[int]:
    result: set[int] = set()
    for original in value.split(","):
        item = original.strip()
        if not item:
            continue
        if "-" in item:
            first, last = (int(part) for part in item.split("-", 1))
            result.update(range(first, last + 1))
        else:
            result.add(int(item))
    if not result or min(result) < 0 or max(result) >= 20:
        raise ValueError("--seeds must select values in [0, 19]")
    return sorted(result)


def load_scenarios(path: Path, selected: set[str]) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    known = {row["scenario_id"] for row in rows}
    missing = selected - known
    if missing:
        raise ValueError("unknown scenario(s): " + ", ".join(sorted(missing)))
    return [row for row in rows if not selected or row["scenario_id"] in selected]


def load_adapters(root: Path, selected: set[str], limit: int) -> list[Path]:
    adapters = sorted(root.glob("*.ttadapter"))
    if selected:
        adapters = [path for path in adapters if path.name in selected]
        missing = selected - {path.name for path in adapters}
        if missing:
            raise ValueError("unknown input(s): " + ", ".join(sorted(missing)))
    else:
        # The adapter directory also carries three directed fixtures.  Keep
        # them available through --input, but preserve the curated 160-input
        # correctness matrix as the default set.
        matrix_names = {
            path.name for path in DEFAULT_MATRIX_INPUT_ROOT.glob("*.ttadapter")
        }
        adapters = [path for path in adapters if path.name in matrix_names]
    return adapters[:limit] if limit else adapters


def load_known_timeout_pairs(path: Path) -> set[tuple[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = csv.DictReader(stream, delimiter="\t")
        return {
            (row["scenario_id"], adapter)
            for row in rows
            for adapter in row["adapters"].split(",")
            if adapter
        }


def compiler_arguments(row: dict[str, str]) -> list[str]:
    values = [
        "--enable-hfusion-compile",
        "--enable-hivm-compile",
        "--enable-triton-kernel-compile",
        "--mlir-disable-threading",
        "--enable-ub-overflow-prediction=true",
        "--prune-predicted-ub-overflow=false",
        "--disable-auto-cv-work-space-manage="
        f"{flag(row['disable_auto_cv_workspace_manage'])}",
        f"--enable-preload={flag(row['enable_preload'])}",
        f"--enable-code-motion={flag(row['enable_code_motion'])}",
        f"--enable-auto-bind-sub-block={flag(row['enable_auto_bind_sub_block'])}",
        "--enable-hivm-auto-storage-align="
        f"{flag(row['enable_hivm_auto_storage_align'])}",
        f"--enable-ubuf-saving={flag(row['enable_ubuf_saving'])}",
        f"--tile-mix-cube-loop={row['tile_mix_cube_loop']}",
        f"--tile-mix-vector-loop={row['tile_mix_vector_loop']}",
        f"--enable-auto-multi-buffer={flag(row['enable_auto_multi_buffer'])}",
        "--limit-auto-multi-buffer-only-for-local-buffer="
        f"{flag(row['limit_auto_multi_buffer_only_for_local_buffer'])}",
        "--limit-auto-multi-buffer-of-local-buffer="
        f"{row['limit_auto_multi_buffer_of_local_buffer']}",
        "--limit-auto-multi-buffer-buffer="
        f"{row['limit_auto_multi_buffer_buffer']}",
        f"--set-workspace-multibuffer={row['set_workspace_multibuffer']}",
        "--enable-hivm-cross-core-gss="
        f"{flag(row['enable_hivm_cross_core_gss'])}",
        "--enable-hivm-inject-block-all-sync="
        f"{flag(row['enable_hivm_inject_block_all_sync'])}",
        "--disable-auto-inject-block-sync="
        f"{flag(row['disable_auto_inject_block_sync'])}",
    ]
    return values


def parse_summary(line: str) -> dict[str, str]:
    return dict(re.findall(r"([a-z_]+)=([^ ]+)", line))


def split_validation_segments(stderr: str) -> list[str]:
    matches = list(re.finditer(
        r"^BISHENGIR_UB_MODEL_VALIDATION_BEGIN\t\d+\s*$",
        stderr, re.MULTILINE,
    ))
    return [
        stderr[match.start():(matches[index + 1].start()
                              if index + 1 < len(matches) else len(stderr))]
        for index, match in enumerate(matches)
    ]


def parse_model_payload(segment: str) -> dict[str, Any]:
    functions: dict[tuple[str, str], dict[str, Any]] = {}
    diagnostics: list[str] = []
    summary: dict[str, str] = {}
    validation_id = ""
    for line in segment.splitlines():
        fields = line.split("\t")
        if fields[0] == "BISHENGIR_UB_MODEL_VALIDATION_BEGIN" and len(fields) == 2:
            validation_id = fields[1]
        elif (fields[0] == "BISHENGIR_UB_MODEL_DIAGNOSTIC" and
              len(fields) >= 4):
            diagnostics.append("\t".join(fields[3:]))
        elif fields[0] == "BISHENGIR_UB_MODEL_FUNCTION" and len(fields) == 7:
            key = (fields[1], fields[2])
            functions[key] = {
                "function": fields[2], "status": fields[3],
                "ub_peak_bits": int(fields[4]),
                "required_bits": int(fields[5]),
                "selected_seed": int(fields[6]),
                "buffers": [], "inplace_pairs": [],
            }
        elif fields[0] == "BISHENGIR_UB_MODEL_BUFFER" and len(fields) >= 9:
            key = (fields[1], fields[2])
            functions[key]["buffers"].append({
                "name": fields[3], "extent_bits": int(fields[4]),
                "multi_buffer_num": int(fields[5]),
                "alloc_time": int(fields[6]), "free_time": int(fields[7]),
                "offsets_bytes": [int(value) for value in fields[8:]],
            })
        elif fields[0] == "BISHENGIR_UB_MODEL_INPLACE" and len(fields) == 5:
            functions[(fields[1], fields[2])]["inplace_pairs"].append(
                [fields[3], fields[4]]
            )
        elif line.startswith("BISHENGIR_UB_MODEL_RESULT "):
            summary = parse_summary(line)
    result: dict[str, Any] = {
        "contract_version": int(summary.get("contract_version", "0")),
        "precision": summary.get("precision", "incomplete"),
        "status": summary.get("status", "internal_error"),
        "ub_peak_bits": None if summary.get("ub_peak_bits") == "unknown"
        else int(summary.get("ub_peak_bits", "0")),
        "required_bits": None if summary.get("required_bits") == "unknown"
        else int(summary.get("required_bits", "0")),
        "selected_seed": None if summary.get("selected_seed") == "unknown"
        else int(summary.get("selected_seed", "0")),
        "decision_path": summary.get("decision_path", ""),
        "non_overflow_upper_bound_proven":
        summary.get("non_overflow_upper_bound_proven", "false") == "true",
        "conservative_upper_bound_bits": None
        if summary.get("conservative_upper_bound_bits") in {None, "unknown"}
        else int(summary["conservative_upper_bound_bits"]),
        "input_digest": summary.get("input_digest", ""),
        "options_digest": summary.get("options_digest", ""),
        "pipeline_fingerprint": summary.get("pipeline_fingerprint", ""),
        "functions": list(functions.values()),
        "diagnostics": diagnostics,
    }
    return {"validation_id": validation_id, "result": result}


def relevant_native_status(
    segment: str, seed: int, model_functions: set[str]
) -> tuple[str, str]:
    """Classify only AIV/model functions; ignore independent CBUF failures."""
    current = ""
    statuses: list[str] = []
    ub_required = False
    non_ub_required = False
    for line in segment.splitlines():
        fields = line.split("\t")
        if fields[0] in {"PLANMEM_LIVENESS_ATTEMPT", "PLANMEM_PLAN_ATTEMPT"} \
                and len(fields) >= 4:
            current = canonical_function_name(fields[1])
            if (fields[0] == "PLANMEM_PLAN_ATTEMPT" and
                    int(fields[2]) == seed and current in model_functions):
                statuses.append(fields[3])
        elif (fields[0] == "PLANMEM_REQUIRED" and len(fields) >= 4 and
              int(fields[1]) == seed and current in model_functions):
            if fields[2] == "6":
                ub_required = True
            else:
                non_ub_required = True
    if not statuses:
        return "unavailable", "no AIV PlanMemory observation"
    if any(status == "failure" for status in statuses):
        if ub_required:
            return "overflow", ""
        if non_ub_required:
            return "unavailable", "AIV failed in a non-UB memory scope"
        return "unavailable", "AIV PlanMemory failed without a UB requirement"
    return "success", ""


def is_equal_extent_identity_permutation(
    model_plan: Counter, native_plan: Counter,
    model_lifetimes: Counter, native_lifetimes: Counter,
    model_inplace: Counter, native_inplace: Counter,
) -> bool:
    """Prove that strict plan differences only rename equal-size buffers.

    Physical offsets are part of the strict PlanMemory contract, so this is
    deliberately a separate result class rather than ``matched``.  The proof
    erases offsets only after status/required/peak/multi have matched, then
    requires the complete extent multiset, lifetime relation, and inplace
    graph to remain identical.
    """
    def plan_without_offsets(values: Counter) -> Counter:
        result: Counter = Counter()
        for (extent, _offset), count in values.items():
            result[extent] += count
        return result

    def lifetimes_without_offsets(values: Counter) -> Counter:
        result: Counter = Counter()
        for (function, extent, _offset, allocate, release), count in values.items():
            result[(function, extent, allocate, release)] += count
        return result

    def identity_without_offset(identity: tuple) -> tuple:
        function, extent, _offsets, allocate, release = identity
        return function, extent, allocate, release

    def inplace_without_offsets(values: Counter) -> Counter:
        result: Counter = Counter()
        for (source, destination), count in values.items():
            result[(identity_without_offset(source),
                    identity_without_offset(destination))] += count
        return result

    return (
        model_plan != native_plan
        and plan_without_offsets(model_plan) ==
        plan_without_offsets(native_plan)
        and lifetimes_without_offsets(model_lifetimes) ==
        lifetimes_without_offsets(native_lifetimes)
        and inplace_without_offsets(model_inplace) ==
        inplace_without_offsets(native_inplace)
    )


def compare_segment(
    segment: str, seed: int, compiler_returncode: int = 0,
    timed_out: bool = False,
) -> tuple[str, list[str], list[str]]:
    payload = parse_model_payload(segment)
    result = payload["result"]
    if result["decision_path"] == "non_overflow_upper_bound":
        return "different", ["validation-pruned-after-non-overflow-proof"], [
            "validation must observe the non-overflow proof and continue "
            "through the full lightweight plan"
        ]
    if (result["non_overflow_upper_bound_proven"] and
            result["decision_path"] !=
            "full_plan_after_non_overflow_upper_bound"):
        return "different", ["non-overflow-proof-signal"], [
            "a proven non-overflow validation result must identify the "
            "full-plan-after-proof path"
        ]
    if result["precision"] != "exact":
        native_failure = bisheng_failure_signature(
            {
                "returncode": compiler_returncode,
                "timeout": timed_out,
                "stderr": segment,
            },
            FAILURE_TAXONOMY,
        )
        if native_failure.failed:
            if not native_failure.primary:
                return "unavailable", [], [
                    "native compiler failed without a stable comparable "
                    f"diagnostic: {native_failure.describe()}"
                ]
            model_failure = model_failure_signature(
                payload, segment, 1, FAILURE_TAXONOMY
            )
            failure_differences = compare_failure(
                model_failure, native_failure
            )
            evidence = [
                "failure: model=%s bisheng=%s"
                % (model_failure.describe(), native_failure.describe())
            ]
            if model_failure.raw:
                evidence.append(f"model_first={model_failure.raw[0]}")
            if native_failure.raw:
                evidence.append(f"bisheng_first={native_failure.raw[0]}")
            return (
                "matched" if not failure_differences else "different",
                failure_differences,
                evidence,
            )
        return "different", ["precision"], [
            f"precision: model={result['precision']} bisheng=exact"
        ]
    if not result["functions"]:
        has_native_ub = any(
            (fields[0] in {"PLANMEM_STORAGE", "PLANMEM_REQUIRED",
                           "PLANMEM_PEAK", "PLANMEM_EXACT_PLANNED_BUFFER"}
             and len(fields) >= 3 and fields[2] == "6")
            or (fields[0] == "PLANMEM_EXACT_BUFFER" and
                len(fields) >= 5 and fields[4] == "6")
            for fields in (line.split("\t") for line in segment.splitlines())
        )
        expected_empty = (
            result["status"] == "success" and
            int(result["ub_peak_bits"] or 0) == 0 and
            int(result["required_bits"] or 0) == 0
        )
        if expected_empty and not has_native_ub:
            return "matched", [], []
        return "different", ["aiv-functions"], [
            f"empty model AIV plan: expected_empty={expected_empty} "
            f"native_has_ub={has_native_ub}"
        ]
    model_functions = {
        canonical_function_name(str(function["function"]))
        for function in result["functions"]
    }
    native_status, reason = relevant_native_status(
        segment, seed, model_functions
    )
    if native_status == "unavailable":
        return "unavailable", [], [reason]
    _, native_peak, native_plan, native_lifetimes = parse_oracle(
        segment, seed, "6"
    )
    _, native_required, native_multi, native_inplace = parse_oracle_contract(
        segment, seed, "6"
    )
    model_peak = int(result["ub_peak_bits"] or 0)
    model_required = int(result["required_bits"] or model_peak)
    model_plan = plan_multiset_from_model(payload)
    model_lifetimes = normalized_lifetimes_from_model(payload)
    model_multi, model_inplace = model_multi_and_inplace(payload)
    differences: list[str] = []
    evidence: list[str] = []
    scalar_values = [
        ("status", result["status"], native_status),
        ("required", model_required, native_required),
    ]
    # A failed native PlanMemory has no applied plan or PLANMEM_PEAK record.
    # Its authoritative UB output is the failure status and required bits.
    if native_status == "success":
        scalar_values.append(("peak", model_peak, int(native_peak or 0)))
    for name, model_value, native_value in scalar_values:
        if model_value != native_value:
            differences.append(name)
            evidence.append(scalar_evidence(name, model_value, native_value))
    if native_status == "success":
        for name, model_value, native_value in (
            ("plan", model_plan, native_plan),
            ("lifetime", model_lifetimes, native_lifetimes),
            ("multi", model_multi, native_multi),
            ("inplace", model_inplace, native_inplace),
        ):
            if model_value != native_value:
                differences.append(name)
                evidence.append(counter_evidence(name, model_value, native_value))
    identity_permutation = (
        set(differences).issubset({"plan", "lifetime", "inplace"})
        and differences
        and model_multi == native_multi
        and is_equal_extent_identity_permutation(
            model_plan, native_plan, model_lifetimes, native_lifetimes,
            model_inplace, native_inplace,
        )
    )
    if identity_permutation:
        evidence.insert(
            0,
            "equal-size identity permutation: strict offsets differ; "
            "extent/lifetime/inplace projections are identical",
        )
        return "identity_permutation", differences, evidence
    return ("matched" if not differences else "different",
            differences, evidence)


def execute_compiler(
    compiler: Path, adapter: Path, scenario: dict[str, str], seed: int,
    timeout: int,
) -> dict[str, Any]:
    env = os.environ.copy()
    for name in (
        "BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY",
        "BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY",
        "BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION",
        "BISHENGIR_DUMP_BEFORE_PLAN_MEMORY",
        "BISHENGIR_DUMP_BEFORE_CVPIPELINING",
        "BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS",
        "BISHENGIR_UB_FLOW_TRACE",
        "BISHENGIR_UB_MODEL_EMIT_RESULT",
    ):
        env.pop(name, None)
    env["BISHENGIR_UB_MODEL_VALIDATION"] = "1"
    env["BISHENGIR_PLAN_MEMORY_FORCE_SEED"] = str(seed)
    # Correctness validation is always live.  Even when the lightweight model
    # proves non-overflow with its conservative fast path, debug evaluation
    # materializes its complete plan and BiSheng continues through the native
    # local PlanMemory pass for the same fixed seed.
    env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
    env["BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY"] = "1"
    command = [
        str(compiler), str(adapter), "-o", os.devnull,
        *compiler_arguments(scenario),
    ]
    started = time.monotonic()
    try:
        completed = subprocess.run(
            command, text=True, capture_output=True, env=env,
            timeout=timeout, check=False, start_new_session=True,
        )
        stderr = completed.stderr
        returncode = completed.returncode
        timed_out = False
    except subprocess.TimeoutExpired as error:
        stderr = error.stderr or ""
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        returncode = 124
        timed_out = True
    return {
        "stderr": stderr,
        "returncode": returncode,
        "timed_out": timed_out,
        "seconds": time.monotonic() - started,
    }


def summarize_observation(
    scenario: dict[str, str], adapter: Path, seed: int, stderr: str,
    compiler_returncode: int, timed_out: bool, seconds: float,
) -> dict[str, Any]:
    segments = split_validation_segments(stderr)
    model_results = [parse_model_payload(segment)["result"]
                     for segment in segments]
    observations = [
        compare_segment(segment, seed, compiler_returncode, timed_out)
        for segment in segments
    ]
    comparable = [value for value in observations if value[0] != "unavailable"]
    differences = sorted({item for _, fields, _ in comparable for item in fields})
    evidence = [item for _, _, values in observations for item in values]
    if timed_out:
        status = "timeout"
    elif any(value[0] == "different" for value in comparable):
        status = "different"
    elif any(value[0] == "identity_permutation" for value in comparable):
        status = "identity_permutation"
    elif comparable:
        status = "matched"
    else:
        status = "unavailable"
    decision_paths = sorted({
        str(result["decision_path"])
        for result in model_results if result["decision_path"]
    })
    native_plan_memory_observed = any(
        line.startswith(("PLANMEM_PLAN_ATTEMPT\t", "PLANMEM_UB_ORACLE_COMPLETE\t"))
        for line in stderr.splitlines()
    )
    proof_verifications: list[bool] = []
    for segment, result in zip(segments, model_results):
        if not result["non_overflow_upper_bound_proven"]:
            continue
        model_functions = {
            canonical_function_name(function["function"])
            for function in result["functions"]
        }
        native_status, _ = relevant_native_status(
            segment, seed, model_functions
        )
        proof_verifications.append(
            result["precision"] == "exact"
            and result["status"] == "success"
            and result["decision_path"] ==
            "full_plan_after_non_overflow_upper_bound"
            and bool(model_functions)
            and native_status == "success"
        )
    non_overflow_proof_verified = (
        "not_applicable" if not proof_verifications
        else str(all(proof_verifications)).lower()
    )
    return {
        "scenario": scenario["scenario_id"], "adapter": adapter.name,
        "seed": seed, "status": status,
        "non_overflow_upper_bound_proven": str(any(
            result["non_overflow_upper_bound_proven"]
            for result in model_results
        )).lower(),
        "decision_paths": ",".join(decision_paths),
        "native_plan_memory_observed": str(native_plan_memory_observed).lower(),
        "non_overflow_proof_verified": non_overflow_proof_verified,
        "compiler_rc": compiler_returncode,
        "attempts": len(segments), "comparable_attempts": len(comparable),
        "differences": ",".join(differences),
        "evidence": " | ".join(evidence[:12]),
        "seconds": f"{seconds:.6f}",
        "diagnostic": stderr[-1200:].replace("\t", " ").replace("\n", "\\n")
        if status not in {"matched", "identity_permutation"} else "",
    }


def run_one(
    compiler: Path, adapter: Path, scenario: dict[str, str], seed: int,
    timeout: int,
) -> dict[str, Any]:
    live = execute_compiler(
        compiler, adapter, scenario, seed, timeout,
    )
    return summarize_observation(
        scenario, adapter, seed, str(live["stderr"]),
        int(live["returncode"]), bool(live["timed_out"]),
        float(live["seconds"]),
    )


def write_report(path: Path, rows: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    columns = report_columns()
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(
            stream, columns, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        writer.writerows(rows)


def read_report(path: Path) -> list[dict[str, Any]]:
    if not path.is_file():
        return []
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def report_columns() -> list[str]:
    return [
        "scenario", "adapter", "seed", "status",
        "non_overflow_upper_bound_proven", "decision_paths",
        "native_plan_memory_observed", "non_overflow_proof_verified",
        "compiler_rc", "attempts", "comparable_attempts", "differences", "evidence",
        "seconds", "diagnostic",
    ]


def main() -> int:
    args = parse_args()
    try:
        scenarios = load_scenarios(args.matrix, set(args.scenarios))
        seeds = selected_seeds(args.seeds)
        adapters = load_adapters(
            args.adapter_root, set(args.input), args.max_inputs
        )
        known_timeouts = (
            set() if args.include_known_timeouts
            else load_known_timeout_pairs(args.known_timeout_pairs)
        )
    except (OSError, ValueError) as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    if args.list:
        for row in scenarios:
            print(f"{row['scenario_id']}\t{row['purpose']}")
        return 0
    if seeds != list(range(20)):
        print(
            "[NOTE] selected seed subset is diagnostic only; correctness "
            "requires --seeds 0-19",
            file=sys.stderr,
        )
    compiler = args.compiler.resolve()
    if not compiler.is_file() or not os.access(compiler, os.X_OK):
        print(f"[ERROR] compiler is not executable: {compiler}", file=sys.stderr)
        return 2
    existing_rows = read_report(args.report) if args.resume else []
    completed_keys = {
        (str(row["scenario"]), str(row["adapter"]), int(row["seed"]))
        for row in existing_rows
    }
    # Interleave adapters instead of launching all 20 seeds of one slow kernel
    # together.  This preserves one-process-per-seed semantics while keeping
    # worker utilization and ETA representative.
    unfiltered_work = [
        (adapter, scenario, seed)
        for scenario in scenarios for seed in seeds for adapter in adapters
    ]
    all_work = [
        value for value in unfiltered_work
        if (value[1]["scenario_id"], value[0].name) not in known_timeouts
    ]
    excluded = len(unfiltered_work) - len(all_work)
    work = [
        value for value in all_work
        if (value[1]["scenario_id"], value[0].name, value[2])
        not in completed_keys
    ]
    total = len(all_work)
    print(
        f"bisheng embedded validation: scenarios={len(scenarios)} "
        f"inputs={len(adapters)} seeds={len(seeds)} total={total} "
        f"jobs={args.jobs} resumed={len(existing_rows)} excluded={excluded} "
        "native_plan_memory=live",
        flush=True,
    )
    rows: list[dict[str, Any]] = list(existing_rows)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    report_mode = "a" if args.resume and args.report.is_file() else "w"
    report_stream = args.report.open(
        report_mode, newline="", encoding="utf-8"
    )
    checkpoint_writer = csv.DictWriter(
        report_stream, report_columns(), delimiter="\t", lineterminator="\n"
    )
    if report_mode == "w":
        checkpoint_writer.writeheader()
        report_stream.flush()
    started = time.monotonic()
    new_completed = 0
    with report_stream, ThreadPoolExecutor(max_workers=args.jobs) as pool:
        pending: dict[Any, tuple[Path, dict[str, str], int]] = {}
        iterator = iter(work)
        for _ in range(min(args.jobs * 2, len(work))):
            adapter, scenario, seed = next(iterator)
            future = pool.submit(
                run_one, compiler, adapter, scenario, seed, args.timeout,
            )
            pending[future] = (adapter, scenario, seed)
        completed_count = len(existing_rows)
        while pending:
            done, _ = wait(pending, return_when=FIRST_COMPLETED)
            for future in done:
                pending.pop(future)
                row = future.result()
                rows.append(row)
                checkpoint_writer.writerow(row)
                report_stream.flush()
                completed_count += 1
                new_completed += 1
                if row["status"] != "matched" and not args.quiet:
                    print(
                        f"[{row['status'].upper()}] {row['scenario']} "
                        f"{row['adapter']} seed={row['seed']}: "
                        f"{row['differences'] or row['evidence']}",
                        file=sys.stderr, flush=True,
                    )
                if not args.no_progress:
                    elapsed = time.monotonic() - started
                    eta = elapsed / new_completed * (total - completed_count)
                    print(
                        f"\r[{completed_count}/{total}] "
                        f"{100 * completed_count / total:6.2f}% "
                        f"ETA {int(eta // 60):02d}:{int(eta % 60):02d}",
                        end="", flush=True,
                    )
                elif new_completed % 100 == 0:
                    elapsed = time.monotonic() - started
                    eta = elapsed / new_completed * (total - completed_count)
                    print(
                        f"progress: {completed_count}/{total} "
                        f"eta_seconds={int(eta)}", flush=True,
                    )
                try:
                    adapter, scenario, seed = next(iterator)
                except StopIteration:
                    continue
                new_future = pool.submit(
                    run_one, compiler, adapter, scenario, seed, args.timeout,
                )
                pending[new_future] = (adapter, scenario, seed)
    if not args.no_progress:
        print()
    rows.sort(key=lambda row: (
        row["scenario"], row["adapter"], int(row["seed"])
    ))
    write_report(args.report, rows)
    counts = Counter(str(row["status"]) for row in rows)
    print(
        "summary: " + " ".join(f"{key}={counts[key]}" for key in
                                ("matched", "identity_permutation", "different",
                                 "unavailable", "timeout"))
    )
    print(args.report)
    return 1 if counts["different"] or counts["timeout"] or counts["unavailable"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
