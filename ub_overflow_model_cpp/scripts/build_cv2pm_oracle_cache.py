#!/usr/bin/env python3
"""Build a compact 20-seed cv2pm oracle cache for curated scenarios."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import csv
from dataclasses import dataclass
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import threading
import time
from typing import Any


MODULE_DIR = Path(__file__).resolve().parents[1]
REPO_ROOT = MODULE_DIR.parent
DEFAULT_MATRIX = MODULE_DIR / "config/ub_relevant_parameter_scenarios.tsv"
DEFAULT_PROFILES = REPO_ROOT / "Output/before_cvpipelining_profiles"
DEFAULT_COMPILER = REPO_ROOT / "build/bin/cv2pm-bishengir-compile"
DEFAULT_CACHE = MODULE_DIR / "output/cv2pm_oracle_cache"
CACHE_SCHEMA = 2
EXECUTION_MODE = "full_cv2pm_per_seed"
SEEDS = tuple(range(20))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument("--profiles-root", type=Path, default=DEFAULT_PROFILES)
    parser.add_argument("--compiler", type=Path, default=DEFAULT_COMPILER)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument(
        "--seed-jobs",
        type=int,
        default=1,
        help="parallel full cv2pm fixed-seed runs inside each case",
    )
    parser.add_argument(
        "--pipeline-timeout", type=int, default=360,
        help="base timeout for one full cv2pm fixed-seed run",
    )
    parser.add_argument(
        "--plan-timeout", type=int, default=120,
        help="additional PlanMemory dump allowance for each full run",
    )
    parser.add_argument("--scenario", action="append", default=[])
    parser.add_argument("--input", action="append", default=[])
    parser.add_argument("--max-inputs", type=int, default=0)
    parser.add_argument("--refresh", action="store_true")
    parser.add_argument(
        "--fail-on-oracle-failure",
        action="store_true",
        help="return a non-zero status when a cached oracle run failed",
    )
    args = parser.parse_args()
    if args.jobs <= 0 or args.seed_jobs <= 0:
        parser.error("--jobs and --seed-jobs must be positive")
    if args.pipeline_timeout <= 0 or args.plan_timeout <= 0:
        parser.error("timeouts must be positive")
    if args.max_inputs < 0:
        parser.error("--max-inputs must be non-negative")
    return args


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
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


def scenario_arguments(row: dict[str, str]) -> list[str]:
    arguments = [
        f"--disable-auto-cv-work-space-manage={flag(row['disable_auto_cv_workspace_manage'])}",
        f"--enable-preload={flag(row['enable_preload'])}",
        f"--enable-code-motion={flag(row['enable_code_motion'])}",
        f"--enable-auto-bind-sub-block={flag(row['enable_auto_bind_sub_block'])}",
        f"--enable-hivm-auto-storage-align={flag(row['enable_hivm_auto_storage_align'])}",
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
    arguments.append("--enable-triton-kernel-compile=true")
    return arguments


def run_command(
    command: list[str], timeout: int, env: dict[str, str] | None = None
) -> dict[str, Any]:
    def output_text(value: str | bytes | None) -> str:
        if value is None:
            return ""
        if isinstance(value, bytes):
            return value.decode("utf-8", errors="replace")
        return value

    started = time.monotonic()
    try:
        completed = subprocess.run(
            command,
            text=True,
            capture_output=True,
            env=env,
            timeout=timeout,
            check=False,
            start_new_session=True,
        )
        return {
            "returncode": completed.returncode,
            "stdout": completed.stdout,
            "stderr": completed.stderr,
            "timeout": False,
            "seconds": round(time.monotonic() - started, 6),
        }
    except subprocess.TimeoutExpired as error:
        return {
            "returncode": 124,
            "stdout": output_text(error.stdout),
            "stderr": output_text(error.stderr)
            + f"\n[TIMEOUT] exceeded {timeout}s\n",
            "timeout": True,
            "seconds": round(time.monotonic() - started, 6),
        }


def write_gzip_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(
        f".{path.name}.{os.getpid()}.{threading.get_ident()}.tmp"
    )
    try:
        with gzip.open(temporary, "wt", encoding="utf-8", compresslevel=6) as stream:
            json.dump(payload, stream, sort_keys=True, separators=(",", ":"))
            stream.write("\n")
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def read_gzip_json(path: Path) -> dict[str, Any]:
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        value = json.load(stream)
    if not isinstance(value, dict):
        raise ValueError("cache record is not an object")
    return value


def safe_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9._-]+", "_", value)


@dataclass(frozen=True)
class Task:
    scenario: dict[str, str]
    adapter: str
    input_path: Path


def identity_for(
    task: Task, compiler_digest: str, input_digest: str
) -> dict[str, Any]:
    return {
        "schema": CACHE_SCHEMA,
        "execution_mode": EXECUTION_MODE,
        "compiler_sha256": compiler_digest,
        "input_sha256": input_digest,
        "scenario_id": task.scenario["scenario_id"],
        "pre_cv_profile": task.scenario["pre_cv_profile"],
        "arguments": scenario_arguments(task.scenario),
        "seeds": list(SEEDS),
    }


def record_path(cache_dir: Path, task: Task) -> Path:
    return (
        cache_dir
        / safe_name(task.scenario["scenario_id"])
        / f"{safe_name(task.adapter)}.json.gz"
    )


def seed_result_failed(result: dict[str, Any]) -> bool:
    return bool(result.get("timeout")) or (
        result.get("returncode") != 0
        and re.search(
            r"\berror:\s+[A-Za-z][A-Za-z0-9_-]* overflow, requires\s+",
            str(result.get("stderr", "")),
            re.IGNORECASE,
        )
        is None
    )


def reached_ub_oracle(result: dict[str, Any]) -> bool:
    """Whether a full cv2pm run reached a comparable UB oracle boundary."""
    return re.search(
        r"^PLANMEM_(?:(?:LIVENESS|PLAN)_ATTEMPT\t|UB_ORACLE_COMPLETE\t)",
        str(result.get("stderr", "")),
        re.MULTILINE,
    ) is not None


def cached_summary(
    path: Path, identity: dict[str, Any]
) -> dict[str, Any] | None:
    try:
        record = read_gzip_json(path)
        if record.get("identity") != identity:
            return None
        if record.get("pipeline_failed"):
            return {"hit": True, "pipeline_failed": True, "seed_failures": 20}
        seed_results = record.get("seed_results", {})
        if sorted(int(seed) for seed in seed_results) != list(SEEDS):
            return None
        seed_variants = record.get("seed_variants", {})
        if not isinstance(seed_variants, dict) or any(
            not str(seed).isdigit() or int(seed) not in SEEDS
            or not isinstance(values, list)
            or not all(isinstance(value, dict) for value in values)
            for seed, values in seed_variants.items()
        ):
            return None
        return {
            "hit": True,
            "pipeline_failed": False,
            "seed_failures": sum(
                seed_result_failed(result) for result in seed_results.values()
            ),
        }
    except (OSError, ValueError, TypeError, json.JSONDecodeError, gzip.BadGzipFile):
        return None


def build_one(
    task: Task,
    compiler: Path,
    compiler_digest: str,
    cache_dir: Path,
    pipeline_timeout: int,
    plan_timeout: int,
    seed_jobs: int,
    refresh: bool,
    on_seed_complete: Any = None,
) -> dict[str, Any]:
    input_digest = sha256_file(task.input_path)
    identity = identity_for(task, compiler_digest, input_digest)
    output = record_path(cache_dir, task)
    if not refresh:
        summary = cached_summary(output, identity)
        if summary is not None:
            return summary

    arguments = scenario_arguments(task.scenario)
    oracle_env = os.environ.copy()
    oracle_env.pop("BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY", None)
    oracle_env.pop("BISHENGIR_DUMP_BEFORE_PLAN_MEMORY", None)
    oracle_env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
    full_run_timeout = pipeline_timeout + plan_timeout

    with tempfile.TemporaryDirectory(prefix="cv2pm-full-seed-") as temporary:
        seed0_boundary = Path(temporary) / "seed0-before-plan-memory.mlirbc"

        def run_seed(seed: int) -> tuple[str, dict[str, Any]]:
            command = [
                str(compiler),
                str(task.input_path),
                "--mlir-disable-threading",
                *arguments,
                f"--plan-memory-seed={seed}",
                "--ub-oracle-only",
                "-o",
                os.devnull,
            ]
            run_env = oracle_env
            if seed == 0:
                run_env = oracle_env.copy()
                run_env["BISHENGIR_DUMP_BEFORE_PLAN_MEMORY"] = str(
                    seed0_boundary
                )
            result = run_command(command, full_run_timeout, env=run_env)
            if on_seed_complete is not None:
                on_seed_complete()
            return str(seed), result

        # Seed 0 is also the probe for seed-independent failures before local
        # PlanMemory.  The boundary file distinguishes a slow/crashing
        # PlanMemory attempt from a prefix failure that should skip the other
        # 19 equally expensive full pipelines.
        seed0_key, seed0_result = run_seed(0)
        if not reached_ub_oracle(seed0_result) and not seed0_boundary.is_file():
            payload = {
                "identity": identity,
                "pipeline_failed": True,
                "pipeline": seed0_result,
                "seed_results": {},
            }
            write_gzip_json(output, payload)
            return {
                "hit": False,
                "pipeline_failed": True,
                "seed_failures": 20,
            }

        seed_results: dict[str, dict[str, Any]] = {seed0_key: seed0_result}
        remaining_seeds = SEEDS[1:]
        if seed_jobs == 1:
            for seed in remaining_seeds:
                key, result = run_seed(seed)
                seed_results[key] = result
        else:
            with ThreadPoolExecutor(
                max_workers=min(seed_jobs, len(remaining_seeds))
            ) as seed_pool:
                futures = [
                    seed_pool.submit(run_seed, seed)
                    for seed in remaining_seeds
                ]
                for future in as_completed(futures):
                    key, result = future.result()
                    seed_results[key] = result

    seed_failures = sum(
        seed_result_failed(result) for result in seed_results.values()
    )
    payload = {
        "identity": identity,
        "pipeline_failed": False,
        "pipeline": seed0_result,
        "seed_results": seed_results,
        # Optional alternatives are independently observed complete cv2pm
        # runs for the same fixed seed.  They are never synthetic plans.
        "seed_variants": {},
    }
    write_gzip_json(output, payload)
    return {
        "hit": False,
        "pipeline_failed": False,
        "seed_failures": seed_failures,
    }


def load_scenarios(path: Path, selected: set[str]) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = [
            row
            for row in csv.DictReader(stream, delimiter="\t")
            if not selected or row["scenario_id"] in selected
        ]
    missing = selected - {row["scenario_id"] for row in rows}
    if missing:
        raise ValueError("unknown scenario(s): " + ", ".join(sorted(missing)))
    return rows


def profile_inputs(
    profiles_root: Path, profile: str, selected: set[str], max_inputs: int
) -> list[tuple[str, Path]]:
    root = profiles_root / profile
    values = sorted(
        (path.parent.name, path)
        for path in root.glob("*.ttadapter/before_cvpipelining.mlirbc")
        if not selected or path.parent.name in selected
    )
    missing = selected - {name for name, _ in values}
    if missing:
        raise ValueError(
            f"{profile}: unknown input(s): " + ", ".join(sorted(missing))
        )
    return values[:max_inputs] if max_inputs else values


def main() -> int:
    args = parse_args()
    compiler = args.compiler.resolve()
    if not compiler.is_file() or not os.access(compiler, os.X_OK):
        print(f"[ERROR] cv2pm compiler is not executable: {compiler}", file=sys.stderr)
        return 2
    try:
        scenarios = load_scenarios(args.matrix.resolve(), set(args.scenario))
        tasks = [
            Task(scenario, adapter, path)
            for scenario in scenarios
            for adapter, path in profile_inputs(
                args.profiles_root.resolve(),
                scenario["pre_cv_profile"],
                set(args.input),
                args.max_inputs,
            )
        ]
    except (OSError, ValueError) as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    compiler_digest = sha256_file(compiler)
    cache_dir = args.cache_dir.resolve()
    cache_dir.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    completed = hits = pipeline_failures = seed_failures = 0
    last_report = started
    seed_progress_lock = threading.Lock()
    seed_runs_completed = 0
    last_seed_report = started

    def on_seed_complete() -> None:
        nonlocal seed_runs_completed, last_seed_report
        with seed_progress_lock:
            seed_runs_completed += 1
            now = time.monotonic()
            if now - last_seed_report < 10:
                return
            elapsed = now - started
            rate = seed_runs_completed / elapsed if elapsed else 0.0
            remaining = len(tasks) * len(SEEDS) - seed_runs_completed
            eta_upper = remaining / rate if rate else 0.0
            print(
                f"oracle_runs={seed_runs_completed}/"
                f"{len(tasks) * len(SEEDS)} elapsed={elapsed:.1f}s "
                f"upper_eta={eta_upper:.1f}s",
                flush=True,
            )
            last_seed_report = now
    print(
        f"cv2pm cache: scenarios={len(scenarios)} inputs_per_scenario="
        f"{len(tasks) // len(scenarios) if scenarios else 0} cases={len(tasks)} "
        f"seed_results={len(tasks) * len(SEEDS)} workers={args.jobs}",
        flush=True,
    )
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [
            pool.submit(
                build_one,
                task,
                compiler,
                compiler_digest,
                cache_dir,
                args.pipeline_timeout,
                args.plan_timeout,
                args.seed_jobs,
                args.refresh,
                on_seed_complete,
            )
            for task in tasks
        ]
        for future in as_completed(futures):
            result = future.result()
            completed += 1
            hits += int(result["hit"])
            pipeline_failures += int(result["pipeline_failed"])
            seed_failures += int(result["seed_failures"])
            now = time.monotonic()
            if now - last_report >= 10 or completed == len(tasks):
                elapsed = now - started
                rate = completed / elapsed if elapsed else 0.0
                eta = (len(tasks) - completed) / rate if rate else 0.0
                print(
                    f"progress={completed}/{len(tasks)} "
                    f"seed_results={completed * 20}/{len(tasks) * 20} "
                    f"hits={hits} pipeline_failures={pipeline_failures} "
                    f"seed_failures={seed_failures} elapsed={elapsed:.1f}s "
                    f"eta={eta:.1f}s",
                    flush=True,
                )
                last_report = now
    manifest = {
        "schema": CACHE_SCHEMA,
        "execution_mode": EXECUTION_MODE,
        "compiler": str(compiler),
        "compiler_sha256": compiler_digest,
        "matrix": str(args.matrix.resolve()),
        "matrix_sha256": sha256_file(args.matrix.resolve()),
        "profiles_root": str(args.profiles_root.resolve()),
        "scenarios": len(scenarios),
        "cases": len(tasks),
        "seed_results": len(tasks) * len(SEEDS),
        "pipeline_timeout_seconds": args.pipeline_timeout,
        "plan_timeout_seconds": args.plan_timeout,
        "seed_jobs": args.seed_jobs,
        "oracle_processes_run": seed_runs_completed,
        "hits": hits,
        "pipeline_failures": pipeline_failures,
        "seed_failures": seed_failures,
        "elapsed_seconds": round(time.monotonic() - started, 3),
    }
    (cache_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(manifest, sort_keys=True), flush=True)
    if args.fail_on_oracle_failure and (pipeline_failures or seed_failures):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
