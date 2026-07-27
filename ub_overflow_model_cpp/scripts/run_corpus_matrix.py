#!/usr/bin/env python3
"""Legacy cached-cv2pm validation; use run_bisheng_embedded_matrix.py."""

from __future__ import annotations

import argparse
from concurrent.futures import (
    FIRST_COMPLETED,
    ThreadPoolExecutor,
    as_completed,
    wait,
)
import csv
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import threading
import time
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from compare_ub_plan_with_suffix_oracle import (
    model_multi_and_inplace,
    normalized_lifetimes_from_model,
    parse_oracle,
    parse_oracle_contract,
    plan_multiset_from_model,
)
from validation_common import (
    ComparisonOutcome,
    FailureTaxonomy,
    added_difference_fields,
    aggregate_by_seed,
    cluster_by_difference,
    compare_failure,
    counter_evidence,
    cv2pm_failure_signature,
    model_failure_signature,
    pivot_scenario_adapter,
    scalar_evidence,
)


MODULE = Path(__file__).resolve().parents[1]
REPO = MODULE.parent
DEFAULT_MATRIX = MODULE / "config/ub_relevant_parameter_scenarios.tsv"
DEFAULT_PROFILES = REPO / "Output/before_cvpipelining_profiles"
DEFAULT_CACHE = MODULE / "output/cv2pm_oracle_cache"
DEFAULT_MODEL = MODULE / "output/bin/bishengir-ub-overflow-model"
DEFAULT_CV2PM = REPO / "build/bin/cv2pm-bishengir-compile"
DEFAULT_CONVERTER = REPO / "build/bin/bishengir-opt"
DEFAULT_REPORT = MODULE / "output/cv2pm_model_validation.tsv"
DEFAULT_TAXONOMY = MODULE / "config/failure_taxonomy.tsv"
SEEDS = tuple(range(20))
CACHE_SCHEMA = 2
CACHE_EXECUTION_MODE = "full_cv2pm_per_seed"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compare the lightweight model with cv2pm for the curated "
            "27-scenario matrix. cv2pm cache hits never invoke cv2pm."
        )
    )
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument("--profiles-root", type=Path, default=DEFAULT_PROFILES)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--model", type=Path, default=DEFAULT_MODEL)
    parser.add_argument("--cv2pm", type=Path, default=DEFAULT_CV2PM)
    parser.add_argument("--converter", type=Path, default=DEFAULT_CONVERTER)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--config", "--scenario", dest="scenarios",
                        action="append", default=[], metavar="NAME")
    parser.add_argument("--input", action="append", default=[],
                        metavar="ADAPTER.ttadapter")
    parser.add_argument("--max-inputs", type=int, default=0)
    parser.add_argument("--seeds", default="0-19")
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument("--model-timeout", type=int, default=120)
    parser.add_argument("--pipeline-timeout", type=int, default=360)
    parser.add_argument("--plan-timeout", type=int, default=120)
    parser.add_argument(
        "--oracle-variant-runs", type=int, default=0,
        help=(
            "for primary-oracle mismatches, run the same cv2pm seed up to N "
            "more times; writable modes cache every distinct complete "
            "PlanMemory result, while read-only mode audits without changing "
            "the cache; the model must still exactly match one real "
            "observation"
        ),
    )
    parser.add_argument(
        "--cache-mode", choices=("read-write", "read-only", "refresh"),
        default="read-write",
    )
    parser.add_argument("--taxonomy", type=Path, default=DEFAULT_TAXONOMY)
    parser.add_argument(
        "--baseline", type=Path, default=None,
        help="fail only on new mismatches and exact->blocker regressions",
    )
    parser.add_argument(
        "--write-baseline", type=Path, default=None,
        help="record the current outstanding differences as the baseline",
    )
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--no-progress", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    if args.jobs <= 0 or args.model_timeout <= 0:
        parser.error("--jobs and --model-timeout must be positive")
    if args.oracle_variant_runs < 0:
        parser.error("--oracle-variant-runs must be non-negative")
    if args.max_inputs < 0:
        parser.error("--max-inputs must be non-negative")
    return args


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def boolean(value: str) -> bool:
    return value.strip().lower() in {"1", "true", "yes", "on"}


def flag(value: str) -> str:
    return "true" if boolean(value) else "false"


def sync_arguments(row: dict[str, str]) -> list[str]:
    names = (
        "enable_hivm_cross_core_gss",
        "enable_hivm_inject_block_all_sync",
        "disable_auto_inject_block_sync",
    )
    if not any(name in row for name in names):
        return []
    return [
        "--enable-hivm-cross-core-gss="
        f"{flag(row.get('enable_hivm_cross_core_gss', '1'))}",
        "--enable-hivm-inject-block-all-sync="
        f"{flag(row.get('enable_hivm_inject_block_all_sync', '0'))}",
        "--disable-auto-inject-block-sync="
        f"{flag(row.get('disable_auto_inject_block_sync', '0'))}",
    ]


def selected_seeds(value: str) -> list[int]:
    result: set[int] = set()
    for item in value.split(","):
        item = item.strip()
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


def profile_inputs(
    profiles_root: Path, profile: str, selected: set[str], max_inputs: int
) -> list[tuple[str, Path]]:
    values = sorted(
        (path.parent.name, path)
        for path in (profiles_root / profile).glob(
            "*.ttadapter/before_cvpipelining.mlirbc"
        )
        if not selected or path.parent.name in selected
    )
    missing = selected - {name for name, _ in values}
    if missing:
        raise ValueError(
            f"{profile}: unknown input(s): " + ", ".join(sorted(missing))
        )
    return values[:max_inputs] if max_inputs else values


def model_arguments(row: dict[str, str], seed: int) -> list[str]:
    arguments = [
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
        "--limit-auto-multi-buffer-of-local-buffer="
        f"{row['limit_auto_multi_buffer_of_local_buffer']}",
        "--limit-auto-multi-buffer-buffer="
        f"{row['limit_auto_multi_buffer_buffer']}",
    ]
    arguments.extend(sync_arguments(row))
    arguments.extend((
        "--enable-triton-kernel-compile=true",
        f"--plan-memory-seed={seed}",
        "--format=json",
    ))
    return arguments


def oracle_arguments(row: dict[str, str]) -> list[str]:
    """Semantic cv2pm arguments used in the cache identity."""
    return model_arguments(row, 0)[:-2]


def cache_path(cache: Path, scenario: str, adapter: str) -> Path:
    return cache / scenario / f"{adapter}.json.gz"


def read_cache(path: Path) -> dict[str, Any]:
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        value = json.load(stream)
    if not isinstance(value, dict):
        raise ValueError(f"cache record is not an object: {path}")
    return value


def read_cache_manifest(cache: Path, matrix: Path) -> dict[str, Any]:
    """Load and validate the immutable identity of an oracle snapshot."""
    path = cache / "manifest.json"
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read oracle cache manifest {path}: {error}")
    if not isinstance(value, dict):
        raise ValueError(f"oracle cache manifest is not an object: {path}")
    if value.get("schema") != CACHE_SCHEMA:
        raise ValueError(
            f"unsupported oracle cache manifest schema: {value.get('schema')!r}"
        )
    if value.get("execution_mode") != CACHE_EXECUTION_MODE:
        raise ValueError(
            "unsupported oracle cache execution mode: "
            f"{value.get('execution_mode')!r}"
        )
    expected_matrix = sha256_file(matrix)
    if value.get("matrix_sha256") != expected_matrix:
        raise ValueError(
            "oracle cache matrix hash differs: "
            f"cache={value.get('matrix_sha256')} current={expected_matrix}"
        )
    compiler_digest = value.get("compiler_sha256")
    if not isinstance(compiler_digest, str) or not re.fullmatch(
        r"[0-9a-f]{64}", compiler_digest
    ):
        raise ValueError("oracle cache manifest has no valid compiler SHA-256")
    return value


def fill_oracle_cache(args: argparse.Namespace) -> None:
    if args.cache_mode == "read-only":
        return
    command = [
        sys.executable,
        str(MODULE / "scripts/build_cv2pm_oracle_cache.py"),
        "--matrix", str(args.matrix),
        "--profiles-root", str(args.profiles_root),
        "--compiler", str(args.cv2pm),
        "--cache-dir", str(args.cache_dir),
        "--jobs", str(args.jobs),
        "--pipeline-timeout", str(args.pipeline_timeout),
        "--plan-timeout", str(args.plan_timeout),
    ]
    for scenario in args.scenarios:
        command.extend(("--scenario", scenario))
    for adapter in args.input:
        command.extend(("--input", adapter))
    if args.max_inputs:
        command.extend(("--max-inputs", str(args.max_inputs)))
    if args.cache_mode == "refresh":
        command.append("--refresh")
    completed = subprocess.run(command, check=False)
    if completed.returncode:
        raise RuntimeError("cv2pm oracle cache generation failed")


def materialize_model_input(
    source: Path, converter: Path, directory: Path
) -> Path:
    digest = sha256_file(source)
    destination = directory / digest[:2] / f"{digest}.mlir"
    if destination.is_file():
        return destination
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(
        f".{destination.name}.{os.getpid()}.{threading.get_ident()}.tmp"
    )
    completed = subprocess.run(
        [str(converter), str(source), "--mlir-print-op-generic", "-o", str(temporary)],
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode or not temporary.is_file():
        temporary.unlink(missing_ok=True)
        raise RuntimeError(
            f"cannot convert {source}: {completed.stderr[-1000:]}"
        )
    os.replace(temporary, destination)
    return destination


def compare_result(
    payload: dict[str, Any], oracle: str, seed: int
) -> ComparisonOutcome:
    outcome = ComparisonOutcome()
    result = payload.get("result") if isinstance(payload.get("result"), dict) else payload
    outcome.model_exact = result.get("precision") == "exact"
    if not outcome.model_exact:
        outcome.differences.append("precision")
        diagnostics = result.get("diagnostics") or []
        outcome.evidence.append(
            "precision: model=%s cv2pm=exact%s" % (
                result.get("precision"),
                f" ({diagnostics[0]})" if diagnostics else "",
            )
        )
        return outcome
    _, oracle_peak, oracle_plan, oracle_lifetimes = parse_oracle(
        oracle, seed, "6"
    )
    oracle_status, oracle_required, oracle_multi, oracle_inplace = (
        parse_oracle_contract(oracle, seed, "6")
    )
    model_peak = int(result.get("ub_peak_bits") or result.get("peak_bits") or 0)
    model_required = int(result.get("required_bits") or model_peak)
    model_plan = plan_multiset_from_model(payload)
    model_lifetimes = normalized_lifetimes_from_model(payload)
    model_multi, model_inplace = model_multi_and_inplace(payload)
    scalars = (
        ("status", result.get("status"), oracle_status),
        ("required", model_required, oracle_required),
        ("peak", model_peak, oracle_peak),
    )
    for name, model_value, oracle_value in scalars:
        if model_value != oracle_value:
            outcome.differences.append(name)
            outcome.evidence.append(
                scalar_evidence(name, model_value, oracle_value)
            )
    counters = (
        ("plan", model_plan, oracle_plan),
        ("lifetime", model_lifetimes, oracle_lifetimes),
        ("multi", model_multi, oracle_multi),
        ("inplace", model_inplace, oracle_inplace),
    )
    for name, model_counter, oracle_counter in counters:
        if model_counter != oracle_counter:
            outcome.differences.append(name)
            outcome.evidence.append(
                counter_evidence(name, model_counter, oracle_counter)
            )
    return outcome


def compare_plan_observations(
    payload: dict[str, Any], returncode: int,
    observations: list[dict[str, Any]], seed: int,
) -> tuple[ComparisonOutcome, int]:
    """Compare against complete cv2pm observations without weakening fields.

    Some production passes use pointer-keyed DenseMap/DenseSet iteration.  A
    fixed seed can consequently have more than one process-level PlanMemory
    result.  Every candidate here is an independently executed full cv2pm
    suffix; no synthetic canonicalization or field omission is allowed.
    """
    best: ComparisonOutcome | None = None
    for index, observation in enumerate(observations):
        outcome = compare_result(
            payload, str(observation.get("stderr", "")), seed
        )
        result = (
            payload.get("result")
            if isinstance(payload.get("result"), dict)
            else payload
        )
        expected_rc = 2 if result.get("status") == "overflow" else 0
        if returncode != expected_rc:
            outcome.differences.insert(0, "returncode")
            outcome.evidence.insert(
                0, scalar_evidence("returncode", returncode, expected_rc)
            )
        if outcome.matched:
            return outcome, index
        if best is None or len(outcome.differences) < len(best.differences):
            best = outcome
    assert best is not None
    return best, -1


def oracle_observations(
    record: dict[str, Any], seed: int
) -> list[dict[str, Any]]:
    primary = record["seed_results"][str(seed)]
    variants = record.get("seed_variants", {}).get(str(seed), [])
    if not isinstance(variants, list) or not all(
        isinstance(value, dict) for value in variants
    ):
        raise ValueError(f"invalid seed_variants entry for seed {seed}")
    return [primary, *variants]


def compare_pipeline_failure(
    payload: dict[str, Any],
    stderr: str,
    returncode: int,
    pipeline: dict[str, Any],
    taxonomy: FailureTaxonomy,
) -> ComparisonOutcome:
    """Compare a case cv2pm could not compile as far as PlanMemory.

    This is real, deterministic oracle data.  Skipping it hides the worst
    error class the model can make: happily reporting exact/success for an
    input the production compiler cannot even compile.
    """
    outcome = ComparisonOutcome()
    oracle_signature = cv2pm_failure_signature(pipeline, taxonomy)
    model_signature = model_failure_signature(
        payload, stderr, returncode, taxonomy
    )
    outcome.model_exact = not model_signature.failed
    outcome.differences = compare_failure(model_signature, oracle_signature)
    if outcome.differences:
        outcome.evidence.append(
            "failure: model=%s cv2pm=%s"
            % (model_signature.describe(), oracle_signature.describe())
        )
        if oracle_signature.raw:
            outcome.evidence.append(f"cv2pm_first={oracle_signature.raw[0]}")
        if model_signature.raw:
            outcome.evidence.append(f"model_first={model_signature.raw[0]}")
    return outcome


def run_one(
    model: Path,
    model_input: Path,
    scenario: dict[str, str],
    adapter: str,
    seed: int,
    oracle_data: dict[str, Any] | list[dict[str, Any]],
    timeout: int,
    kind: str,
    taxonomy: FailureTaxonomy,
) -> dict[str, Any]:
    """Run the model once and compare it against the cached cv2pm result.

    `kind` is "plan" when cv2pm reached PlanMemory and produced a full dump,
    or "pipeline_failure" when cv2pm failed deterministically before it.  Both
    are comparable oracle data; only timeouts are not.
    """
    command = [str(model), str(model_input), *model_arguments(scenario, seed)]
    started = time.monotonic()
    base = {
        "scenario": scenario["scenario_id"], "adapter": adapter,
        "seed": seed, "kind": kind, "oracle_variant": "",
    }
    try:
        completed = subprocess.run(
            command, text=True, capture_output=True, timeout=timeout, check=False
        )
    except subprocess.TimeoutExpired:
        return {
            **base, "status": "model_timeout", "differences": "timeout",
            "evidence": "", "model_exact": "0",
            "model_seconds": f"{time.monotonic() - started:.6f}",
            "diagnostic": "",
        }
    try:
        payload = json.loads(completed.stdout)
    except json.JSONDecodeError:
        return {
            **base, "status": "model_error", "differences": "json",
            "evidence": "", "model_exact": "0",
            "model_seconds": f"{time.monotonic() - started:.6f}",
            "diagnostic": completed.stderr[-1000:].replace("\n", "\\n"),
        }
    if kind == "pipeline_failure":
        outcome = compare_pipeline_failure(
            payload, completed.stderr, completed.returncode,
            oracle_data, taxonomy,
        )
    else:
        assert isinstance(oracle_data, list)
        outcome, matched_observation = compare_plan_observations(
            payload, completed.returncode, oracle_data, seed
        )
    row = {
        **base,
        "status": "matched" if outcome.matched else "different",
        "differences": ",".join(outcome.differences),
        "evidence": outcome.evidence_text(),
        "model_exact": "1" if outcome.model_exact else "0",
        "model_seconds": f"{time.monotonic() - started:.6f}",
        "diagnostic": completed.stderr[-1000:].replace("\n", "\\n")
        if outcome.differences else "",
    }
    if kind == "plan":
        row["oracle_variant"] = (
            str(matched_observation) if matched_observation >= 0 else ""
        )
        # Private fields are retained only until optional live cv2pm
        # observation is complete; they are removed before writing the TSV.
        row["_payload"] = payload
        row["_returncode"] = completed.returncode
    return row


def oracle_contract_signature(stderr: str, seed: int) -> tuple[Any, ...]:
    """Return exactly the cv2pm facts used by the matrix comparison."""
    _, peak, plan, lifetimes = parse_oracle(stderr, seed, "6")
    status, required, multi, inplace = parse_oracle_contract(
        stderr, seed, "6"
    )
    return (
        status, required, peak,
        tuple(sorted(plan.items())),
        tuple(sorted(lifetimes.items())),
        tuple(sorted(multi.items())),
        tuple(sorted(inplace.items())),
    )


def run_cv2pm_observation(
    cv2pm: Path, source: Path, scenario: dict[str, str], seed: int,
    timeout: int,
) -> dict[str, Any]:
    """Run one complete, fixed-seed cv2pm process for a real observation."""
    environment = os.environ.copy()
    environment.pop("BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY", None)
    environment.pop("BISHENGIR_DUMP_BEFORE_PLAN_MEMORY", None)
    environment["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
    command = [
        str(cv2pm), str(source), "--mlir-disable-threading",
        *oracle_arguments(scenario), f"--plan-memory-seed={seed}",
        "--ub-oracle-only", "-o", os.devnull,
    ]
    started = time.monotonic()
    try:
        completed = subprocess.run(
            command, text=True, capture_output=True, env=environment,
            timeout=timeout, check=False, start_new_session=True,
        )
        return {
            "returncode": completed.returncode,
            "stdout": completed.stdout,
            "stderr": completed.stderr,
            "timeout": False,
            "seconds": round(time.monotonic() - started, 6),
        }
    except subprocess.TimeoutExpired as error:
        stderr = error.stderr or ""
        if isinstance(stderr, bytes):
            stderr = stderr.decode("utf-8", errors="replace")
        return {
            "returncode": 124,
            "stdout": "",
            "stderr": stderr + f"\n[TIMEOUT] exceeded {timeout}s\n",
            "timeout": True,
            "seconds": round(time.monotonic() - started, 6),
        }


def observe_cv2pm_variants(
    cv2pm: Path, source: Path, scenario: dict[str, str], seed: int,
    payload: dict[str, Any], model_returncode: int,
    existing: list[dict[str, Any]], runs: int, timeout: int,
) -> tuple[list[dict[str, Any]], tuple[Any, ...] | None]:
    """Collect distinct full contracts, stopping once the model matches one."""
    known = {
        oracle_contract_signature(str(value.get("stderr", "")), seed)
        for value in existing
    }
    added: list[dict[str, Any]] = []
    matched: tuple[Any, ...] | None = None
    for _ in range(runs):
        observation = run_cv2pm_observation(
            cv2pm, source, scenario, seed, timeout
        )
        if observation.get("timeout") or re.search(
            r"^PLANMEM_(?:(?:LIVENESS|PLAN)_ATTEMPT\t|UB_ORACLE_COMPLETE\t)",
            str(observation.get("stderr", "")), re.MULTILINE,
        ) is None:
            continue
        try:
            signature = oracle_contract_signature(
                str(observation.get("stderr", "")), seed
            )
        except (ValueError, RuntimeError):
            continue
        if signature not in known:
            known.add(signature)
            added.append(observation)
        outcome, _ = compare_plan_observations(
            payload, model_returncode, [observation], seed
        )
        if outcome.matched:
            matched = signature
            break
    return added, matched


def augment_oracle_variants(
    args: argparse.Namespace, results: list[dict[str, Any]],
    contexts: dict[
        tuple[str, str, int],
        tuple[dict[str, str], Path, Path, dict[str, Any]],
    ],
) -> tuple[int, int]:
    """Observe only primary mismatches, then atomically extend cache records."""
    targets = [
        row for row in results
        if row.get("kind") == "plan" and row.get("status") == "different"
        and isinstance(row.get("_payload"), dict)
    ]
    if not targets or args.oracle_variant_runs <= 0:
        return 0, 0
    timeout = args.pipeline_timeout + args.plan_timeout
    print(
        f"oracle variant audit: mismatches={len(targets)} "
        f"max_runs_each={args.oracle_variant_runs} workers={args.jobs}",
        flush=True,
    )
    added_total = matched_total = 0
    persist_variants = (
        getattr(args, "cache_mode", "read-write") != "read-only"
    )
    dirty_records: dict[Path, dict[str, Any]] = {}
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures: dict[Any, tuple[dict[str, Any], Path, dict[str, Any]]] = {}
        for row in targets:
            key = (str(row["scenario"]), str(row["adapter"]), int(row["seed"]))
            scenario, source, path, record = contexts[key]
            observations = oracle_observations(record, int(row["seed"]))
            future = pool.submit(
                observe_cv2pm_variants,
                args.cv2pm.resolve(), source, scenario, int(row["seed"]),
                row["_payload"], int(row["_returncode"]), observations,
                args.oracle_variant_runs, timeout,
            )
            futures[future] = (row, path, record)
        completed = 0
        for future in as_completed(futures):
            row, path, record = futures[future]
            added, matched_signature = future.result()
            seed_key = str(row["seed"])
            if added:
                variants = record.setdefault("seed_variants", {}).setdefault(
                    seed_key, []
                )
                variants.extend(added)
                if persist_variants:
                    dirty_records[path] = record
                added_total += len(added)
            if matched_signature is not None:
                observations = oracle_observations(record, int(row["seed"]))
                index = next(
                    index for index, observation in enumerate(observations)
                    if oracle_contract_signature(
                        str(observation.get("stderr", "")), int(row["seed"])
                    ) == matched_signature
                )
                row.update({
                    "status": "matched", "differences": "", "evidence": "",
                    "diagnostic": "", "oracle_variant": str(index),
                })
                matched_total += 1
            completed += 1
            if not args.no_progress and (
                completed == len(targets) or completed % max(args.jobs, 1) == 0
            ):
                print(
                    f"oracle_variant_progress={completed}/{len(targets)} "
                    f"new_contracts={added_total} resolved={matched_total}",
                    flush=True,
                )
    for path, record in dirty_records.items():
        # This process is the sole writer after cache generation finishes.
        # Reuse the builder's atomic gzip shape without importing it and
        # creating a circular module dependency.
        temporary = path.with_name(f".{path.name}.{os.getpid()}.tmp")
        try:
            with gzip.open(
                temporary, "wt", encoding="utf-8", compresslevel=6
            ) as stream:
                json.dump(record, stream, sort_keys=True, separators=(",", ":"))
                stream.write("\n")
            os.replace(temporary, path)
        finally:
            temporary.unlink(missing_ok=True)
    return added_total, matched_total


def main() -> int:
    args = parse_args()
    try:
        scenarios = load_scenarios(args.matrix.resolve(), set(args.scenarios))
        seeds = selected_seeds(args.seeds)
    except (OSError, ValueError) as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    if args.list:
        for row in scenarios:
            print(f"{row['scenario_id']}\t{row['pre_cv_profile']}\t{row['purpose']}")
        return 0
    executables = [("model", args.model), ("converter", args.converter)]
    if args.cache_mode != "read-only":
        executables.append(("cv2pm", args.cv2pm))
    for name, executable in executables:
        if not executable.is_file() or not os.access(executable, os.X_OK):
            print(f"[ERROR] {name} is not executable: {executable}", file=sys.stderr)
            return 2
    try:
        fill_oracle_cache(args)
    except RuntimeError as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2

    try:
        cache_manifest = read_cache_manifest(
            args.cache_dir.resolve(), args.matrix.resolve()
        )
    except ValueError as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    oracle_snapshot_digest = str(cache_manifest["compiler_sha256"])

    selected_inputs = set(args.input)
    # A read-only cache is the oracle artifact and never invokes the current
    # cv2pm binary.  Keep validating its input, arguments, seed set, and that
    # all selected records came from one compiler build, but do not invalidate
    # it merely because an unrelated local relink changed the current binary's
    # digest.  Mutating cache modes still require an exact compiler digest.
    cv2pm_digest = (
        None if args.cache_mode == "read-only"
        else sha256_file(args.cv2pm.resolve())
    )
    cached_compiler_digests: set[str] = set()
    cases: list[tuple[dict[str, str], str, Path, dict[str, Any]]] = []
    failure_cases: list[tuple[dict[str, str], str, Path, dict[str, Any]]] = []
    timeout_cases: list[tuple[str, str]] = []
    skipped_timeouts = cache_misses = 0
    for scenario in scenarios:
        try:
            inputs = profile_inputs(
                args.profiles_root.resolve(), scenario["pre_cv_profile"],
                selected_inputs, args.max_inputs,
            )
        except ValueError as error:
            print(f"[ERROR] {error}", file=sys.stderr)
            return 2
        for adapter, source in inputs:
            path = cache_path(
                args.cache_dir.resolve(), scenario["scenario_id"], adapter
            )
            if not path.is_file():
                cache_misses += 1
                continue
            record = read_cache(path)
            identity = record.get("identity", {})
            cached_compiler_digest = identity.get("compiler_sha256")
            if (identity.get("scenario_id") != scenario["scenario_id"] or
                    identity.get("schema") != CACHE_SCHEMA or
                    identity.get("execution_mode") != CACHE_EXECUTION_MODE or
                    identity.get("input_sha256") != sha256_file(source) or
                    not isinstance(cached_compiler_digest, str) or
                    (cv2pm_digest is not None and
                     cached_compiler_digest != cv2pm_digest) or
                    identity.get("arguments") != oracle_arguments(scenario) or
                    identity.get("seeds") != list(SEEDS)):
                cache_misses += 1
                continue
            cached_compiler_digests.add(cached_compiler_digest)
            if record.get("pipeline_failed"):
                # A cv2pm timeout is not a deterministic output and cannot be
                # used as an oracle.  A deterministic pre-PlanMemory failure
                # is, and gets compared like any other case.
                if record.get("pipeline", {}).get("timeout"):
                    skipped_timeouts += 1
                    timeout_cases.append(
                        (scenario["scenario_id"], adapter)
                    )
                    continue
                failure_cases.append((scenario, adapter, source, record))
                continue
            cases.append((scenario, adapter, source, record))
    if cache_misses:
        print(f"[ERROR] oracle cache misses or stale records: {cache_misses}",
              file=sys.stderr)
        return 2
    if len(cached_compiler_digests) > 1:
        print("[ERROR] selected oracle records use multiple cv2pm builds: "
              f"{len(cached_compiler_digests)}", file=sys.stderr)
        return 2
    if cached_compiler_digests and cached_compiler_digests != {
        oracle_snapshot_digest
    }:
        print(
            "[ERROR] selected oracle records do not belong to the cache "
            f"manifest snapshot: manifest={oracle_snapshot_digest} "
            f"records={','.join(sorted(cached_compiler_digests))}",
            file=sys.stderr,
        )
        return 2

    input_cache = args.cache_dir.resolve() / "_model_inputs"
    unique_sources = sorted(
        {source for _, _, source, _ in cases}
        | {source for _, _, source, _ in failure_cases}
    )
    converted: dict[Path, Path] = {}
    try:
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = {
                pool.submit(materialize_model_input, source,
                            args.converter.resolve(), input_cache): source
                for source in unique_sources
            }
            for future in as_completed(futures):
                converted[futures[future]] = future.result()
    except RuntimeError as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2

    try:
        taxonomy = FailureTaxonomy.load(args.taxonomy.resolve())
    except (OSError, ValueError, re.error) as error:
        print(f"[ERROR] failure taxonomy: {error}", file=sys.stderr)
        return 2

    tasks = [
        (scenario, adapter, converted[source], seed,
         oracle_observations(record, seed), "plan")
        for scenario, adapter, source, record in cases
        for seed in seeds
    ]
    task_contexts = {
        (scenario["scenario_id"], adapter, seed): (
            scenario, source,
            cache_path(args.cache_dir.resolve(), scenario["scenario_id"], adapter),
            record,
        )
        for scenario, adapter, source, record in cases
        for seed in seeds
    }
    # A pre-PlanMemory failure is seed-independent: cv2pm never reached the
    # seeded retry loop.  Comparing it once per case is enough, and running it
    # 20 times would only inflate both the runtime and the failure counts.
    tasks.extend(
        (scenario, adapter, converted[source], seeds[0],
         record.get("pipeline", {}), "pipeline_failure")
        for scenario, adapter, source, record in failure_cases
    )
    started = time.monotonic()
    results: list[dict[str, Any]] = []
    print(
        f"cv2pm-model validation: scenarios={len(scenarios)} "
        f"comparable_cases={len(cases)} seeds={len(seeds)} "
        f"failure_parity_cases={len(failure_cases)} tasks={len(tasks)} "
        f"skipped_timeouts={skipped_timeouts} workers={args.jobs} "
        f"oracle_snapshot={oracle_snapshot_digest[:12]}",
        flush=True,
    )
    model = args.model.resolve()
    # Keep only a small number of submitted tasks alive. Submitting the
    # complete matrix up front retains tens of thousands of Future objects,
    # delays the first progress update until submission finishes, and adds a
    # large memory spike without increasing model parallelism.
    max_in_flight = max(args.jobs, args.jobs * 2)
    task_iterator = iter(tasks)
    pending = set()
    last = started
    failures = 0

    def submit_one(pool: ThreadPoolExecutor) -> bool:
        try:
            scenario, adapter, model_input, seed, oracle, kind = next(
                task_iterator
            )
        except StopIteration:
            return False
        pending.add(pool.submit(
            run_one, model, model_input, scenario, adapter, seed, oracle,
            args.model_timeout, kind, taxonomy,
        ))
        return True

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        while len(pending) < max_in_flight and submit_one(pool):
            pass
        while pending:
            completed, pending = wait(
                pending, return_when=FIRST_COMPLETED
            )
            for future in completed:
                row = future.result()
                results.append(row)
                failures += row["status"] != "matched"
            while len(pending) < max_in_flight and submit_one(pool):
                pass
            now = time.monotonic()
            if (not args.no_progress and
                    (now - last >= 5 or len(results) == len(tasks))):
                rate = len(results) / (now - started) if now > started else 0
                eta = (len(tasks) - len(results)) / rate if rate else 0
                print(
                    f"progress={len(results)}/{len(tasks)} failures={failures} "
                    f"elapsed={now-started:.1f}s eta={eta:.1f}s",
                    flush=True,
                )
                last = now

    added_variants, matched_variants = augment_oracle_variants(
        args, results, task_contexts
    )

    results.sort(key=lambda row: (
        str(row["scenario"]), str(row["adapter"]), int(row["seed"])
    ))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    columns = ["scenario", "adapter", "seed", "kind", "status",
               "oracle_variant", "differences", "evidence", "model_exact",
               "model_seconds", "diagnostic"]
    with args.report.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns, delimiter="\t",
                                lineterminator="\n", extrasaction="ignore")
        writer.writeheader()
        writer.writerows(results)

    # Freeze the coverage blind spot instead of only counting it: these
    # combinations are the largest kernels, exactly the ones most likely to
    # overflow, so it must be explicit which inputs have no evidence at all.
    if timeout_cases:
        timeout_report = args.report.with_name(
            args.report.stem + ".timeouts.tsv"
        )
        with timeout_report.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
            writer.writerow(["scenario", "adapter"])
            writer.writerows(sorted(timeout_cases))
    else:
        timeout_report = None

    failed = [row for row in results if row["status"] != "matched"]
    exact_rows = [row for row in results if row.get("model_exact") == "1"]
    baseline_verdict = apply_baseline(args, results)
    if failed and not args.quiet:
        for row in failed[:50]:
            # Full evidence goes to the TSV; the console keeps one readable
            # line per failure.
            evidence = str(row.get("evidence", ""))
            if len(evidence) > 160:
                evidence = evidence[:157] + "..."
            print(
                f"[FAIL] {row['scenario']} {row['adapter']} seed={row['seed']}"
                f" ({row['kind']}): {row['differences']}"
                + (f"  {evidence}" if evidence else ""),
                file=sys.stderr,
            )
        if len(failed) > 50:
            print(f"[FAIL] ... {len(failed)-50} more; see {args.report}",
                  file=sys.stderr)
    if not args.quiet:
        print_triage_summary(results)
    elapsed = time.monotonic() - started
    # Track exact coverage separately from the mismatch count.  Degrading a
    # hard input to blocker lowers the mismatch count while making the model
    # less useful; that must never look like progress.
    print(
        f"summary: matched={len(results)-len(failed)} failures={len(failed)} "
        f"exact={len(exact_rows)}/{len(results)} "
        f"failure_parity_cases={len(failure_cases)} "
        f"skipped_timeouts={skipped_timeouts} "
        f"variant_matches={sum(row.get('oracle_variant') not in ('', '0') for row in results)} "
        f"new_variant_contracts={added_variants} "
        f"new_variant_matches={matched_variants} "
        + (f"timeout_list={timeout_report} " if timeout_report else "")
        + f"elapsed={elapsed:.3f}s report={args.report}",
        flush=True,
    )
    if baseline_verdict is not None:
        return baseline_verdict
    return 1 if failed else 0


def print_triage_summary(results: list[dict[str, Any]]) -> None:
    """Print the three cuts that turn a flat report into a work list."""
    plan_rows = [row for row in results if row.get("kind") == "plan"]
    seed_split = aggregate_by_seed(plan_rows, expected_seeds=SEEDS)
    pivot = pivot_scenario_adapter(results)
    clusters = cluster_by_difference(results)
    if not clusters:
        return
    print("\n--- triage ---")
    print(
        "all-20-seed failures (strong pre-PlanMemory clue, not proof): "
        f"{len(seed_split['all_seed_failures'])}"
    )
    print(
        "seed-varying cases (both matched and failed seeds observed): "
        f"{len(seed_split['seed_varying'])}"
    )
    if seed_split["inconclusive"]:
        print(
            "inconclusive seed coverage (all observed seeds failed, but not "
            f"all 20 were run): {len(seed_split['inconclusive'])}"
        )
    if pivot["input_wide_adapters"]:
        print(
            "adapters failing in every scenario they appear in "
            f"(input-dependent pass modeling): "
            f"{', '.join(pivot['input_wide_adapters'][:10])}"
            + (" ..." if len(pivot["input_wide_adapters"]) > 10 else "")
        )
    per_scenario = pivot["per_scenario_failing_adapters"]
    if per_scenario:
        worst = sorted(per_scenario.items(), key=lambda item: -item[1])[:5]
        print(
            "scenarios with the most failing adapters (option handling): "
            + ", ".join(f"{name}={count}" for name, count in worst)
        )
    print("difference clusters (largest first):")
    for signature, info in list(clusters.items())[:10]:
        representative = info["representative"]
        print(
            f"  {signature or '(none)':38s} n={info['count']:<6d} "
            f"adapters={info['adapters']:<4d} scenarios={info['scenarios']:<3d} "
            f"repro: --config {representative['scenario']} "
            f"--input {representative['adapter']} "
            f"--seeds {representative['seed']}"
        )


def apply_baseline(
    args: argparse.Namespace, results: list[dict[str, Any]]
) -> int | None:
    """Ratchet against a recorded baseline.

    With ~69k comparisons, fixing one pass routinely perturbs another.  The
    ratchet fails only on *new* mismatches and on exact -> blocker regressions,
    so known-outstanding differences do not drown out the signal.
    """
    if args.write_baseline:
        args.write_baseline.parent.mkdir(parents=True, exist_ok=True)
        with args.write_baseline.open(
            "w", newline="", encoding="utf-8"
        ) as stream:
            writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
            writer.writerow(
                ["scenario", "adapter", "seed", "differences", "model_exact"]
            )
            writer.writerows(
                [row["scenario"], row["adapter"], row["seed"],
                 row["differences"], row["model_exact"]]
                for row in results
                if row["status"] != "matched" or row.get("model_exact") != "1"
            )
        print(f"baseline written: {args.write_baseline}", flush=True)
        return None
    if not args.baseline:
        return None
    known: dict[tuple[str, str, str], tuple[str, str]] = {}
    with args.baseline.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            known[(row["scenario"], row["adapter"], row["seed"])] = (
                row["differences"], row["model_exact"],
            )
    regressions: list[str] = []
    for row in results:
        key = (str(row["scenario"]), str(row["adapter"]), str(row["seed"]))
        recorded = known.get(key)
        if row["status"] != "matched" and recorded is None:
            regressions.append(
                f"new mismatch: {key[0]} {key[1]} seed={key[2]}: "
                f"{row['differences']}"
            )
        elif recorded is not None:
            if recorded[1] == "1" and row.get("model_exact") != "1":
                regressions.append(
                    f"exact->blocker regression: {key[0]} {key[1]} "
                    f"seed={key[2]}"
                )
            added = added_difference_fields(
                recorded[0], str(row.get("differences", ""))
            )
            if added:
                regressions.append(
                    f"expanded mismatch: {key[0]} {key[1]} seed={key[2]} "
                    f"added={','.join(added)}"
                )
    # Only rows actually run can be judged resolved; a subset run must not
    # report every unvisited baseline entry as fixed.
    ran = {
        (str(row["scenario"]), str(row["adapter"]), str(row["seed"])): row
        for row in results
    }
    covered = [key for key in known if key in ran]
    resolved = sum(
        1 for key in covered
        if ran[key]["status"] == "matched"
        and ran[key].get("model_exact") == "1"
    )
    print(
        f"baseline: known={len(known)} covered={len(covered)} "
        f"regressions={len(regressions)} resolved={resolved}",
        flush=True,
    )
    for entry in regressions[:50]:
        print(f"[REGRESSION] {entry}", file=sys.stderr)
    return 1 if regressions else 0


if __name__ == "__main__":
    raise SystemExit(main())
