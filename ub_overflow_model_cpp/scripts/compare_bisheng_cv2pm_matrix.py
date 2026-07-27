#!/usr/bin/env python3
"""Compare production bishengir-compile with cv2pm before local PlanMemory."""

from __future__ import annotations

import argparse
import csv
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import threading
import time


MODULE = Path(__file__).resolve().parents[1]
REPO = MODULE.parent


@dataclass(frozen=True)
class EffectiveConfig:
    disable_auto_cv_workspace_manage: bool
    preload: bool
    lazy_loading: bool
    code_motion: bool
    tile_cube: int
    tile_vector: int
    ubuf_saving: bool
    auto_multi_buffer: bool
    local_strategy: str
    mix_strategy: str


def boolean(value: str) -> bool:
    return value.strip().lower() in {"1", "true", "yes", "on"}


def effective_config(row: dict[str, str]) -> EffectiveConfig:
    local_strategy = row["local_multi_buffer_strategy"]
    # The production A3 CLI only exposes no-l0c and no-limit for local
    # buffers. ONLY_VECTOR was an experimental lightweight/suffix value, so
    # keep the production default instead of inventing a closest mapping.
    if local_strategy == "only-vector":
        local_strategy = "no-l0c"
    return EffectiveConfig(
        # The matrix column is an experiment label inherited from the
        # lightweight model.  The production compiler has no independent
        # "disable CVPipelining only" option; its real control is
        # DisableAutoCVWorkSpaceManage.  Validation must follow production
        # semantics instead of adding a new compiler option.
        disable_auto_cv_workspace_manage=boolean(
            row["disable_cv_pipelining"]
        ),
        preload=boolean(row["enable_preload"]),
        lazy_loading=boolean(row["enable_cv_lazy_loading"]),
        code_motion=boolean(row["enable_code_motion"]),
        tile_cube=int(row["tile_mix_cube_loop"]),
        tile_vector=int(row["tile_mix_vector_loop"]),
        ubuf_saving=boolean(row["enable_ubuf_saving"]),
        auto_multi_buffer=boolean(row["enable_auto_multi_buffer"]),
        local_strategy=local_strategy,
        mix_strategy=row["mix_multi_buffer_strategy"],
    )


def config_args(config: EffectiveConfig) -> list[str]:
    flag = lambda value: "true" if value else "false"
    return [
        "--disable-auto-cv-work-space-manage="
        f"{flag(config.disable_auto_cv_workspace_manage)}",
        f"--enable-preload={flag(config.preload)}",
        f"--enable-lazy-loading={flag(config.lazy_loading)}",
        f"--enable-code-motion={flag(config.code_motion)}",
        f"--tile-mix-cube-loop={config.tile_cube}",
        f"--tile-mix-vector-loop={config.tile_vector}",
        f"--enable-ubuf-saving={flag(config.ubuf_saving)}",
        f"--enable-auto-multi-buffer={flag(config.auto_multi_buffer)}",
        f"--limit-auto-multi-buffer-of-local-buffer={config.local_strategy}",
        f"--limit-auto-multi-buffer-buffer={config.mix_strategy}",
    ]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def safe_name(value: str) -> str:
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in value)


def run_command(
    command: list[str], env: dict[str, str], stderr: Path, timeout: int
) -> tuple[int, bool, float]:
    started = time.monotonic()
    with stderr.open("wb") as stream:
        try:
            result = subprocess.run(
                command,
                stdout=subprocess.DEVNULL,
                stderr=stream,
                env=env,
                timeout=timeout,
                check=False,
            )
            return result.returncode, False, time.monotonic() - started
        except subprocess.TimeoutExpired:
            return 124, True, time.monotonic() - started


def diagnostic_tail(path: Path, limit: int = 600) -> str:
    if not path.is_file():
        return ""
    return path.read_text(errors="replace")[-limit:].replace("\t", " ").replace(
        "\n", "\\n"
    )


def compare_one(
    adapter: Path,
    config: EffectiveConfig,
    group_id: int,
    output_root: Path,
    compiler: Path,
    cv2pm: Path,
    timeout: int,
) -> dict[str, object]:
    case = output_root / "cases" / f"g{group_id:02d}" / adapter.name
    case.mkdir(parents=True, exist_ok=True)
    # MLIR text does not encode SSA use-list order, but CVPipelining observes
    # that order.  Bytecode does encode and restore it, so this validation-only
    # boundary preserves the in-memory production state without changing any
    # production pass implementation.
    before_cv = case / "before_cvpipelining.mlirbc"
    real_before_pm = case / "bisheng_before_local_plan_memory.mlir"
    cv2pm_before_pm = case / "cv2pm_before_local_plan_memory.mlir"
    real_stderr = case / "bisheng.stderr.log"
    cv2pm_stderr = case / "cv2pm.stderr.log"
    common_env = os.environ.copy()
    common_env["BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY"] = "1"
    real_env = common_env.copy()
    real_env["BISHENGIR_DUMP_BEFORE_CVPIPELINING"] = str(before_cv)
    real_env["BISHENGIR_DUMP_BEFORE_PLAN_MEMORY"] = str(real_before_pm)
    arguments = config_args(config)
    real_command = [
        str(compiler),
        str(adapter),
        "-o",
        str(case / "unused.o"),
        "--enable-hfusion-compile",
        "--enable-hivm-compile",
        "--enable-triton-kernel-compile",
        "--mlir-disable-threading",
        *arguments,
    ]
    real_exit, real_timeout, real_seconds = run_command(
        real_command, real_env, real_stderr, timeout
    )
    cv2pm_exit = -1
    cv2pm_timeout = False
    cv2pm_seconds = 0.0
    # Run the extracted suffix whenever the production compiler reached the
    # CVPipelining boundary.  A production failure after that boundary is
    # itself behavior that cv2pm must reproduce; requiring a before-PM output
    # would hide exactly those cases.
    if before_cv.is_file():
        cv2pm_env = common_env.copy()
        cv2pm_env["BISHENGIR_DUMP_BEFORE_PLAN_MEMORY"] = str(cv2pm_before_pm)
        cv2pm_command = [
            str(cv2pm),
            str(before_cv),
            "-o",
            str(case / "unused.mlir"),
            "--enable-triton-kernel-compile",
            "--mlir-disable-threading",
            *arguments,
        ]
        cv2pm_exit, cv2pm_timeout, cv2pm_seconds = run_command(
            cv2pm_command, cv2pm_env, cv2pm_stderr, timeout
        )
    comparable = real_before_pm.is_file() and cv2pm_before_pm.is_file()
    real_hash = sha256(real_before_pm) if real_before_pm.is_file() else ""
    cv2pm_hash = sha256(cv2pm_before_pm) if cv2pm_before_pm.is_file() else ""
    exact = comparable and real_hash == cv2pm_hash
    matched_failure = (
        not comparable
        and not real_before_pm.is_file()
        and not cv2pm_before_pm.is_file()
        and real_exit != 0
        and real_exit == cv2pm_exit
        and not real_timeout
        and not cv2pm_timeout
    )
    if exact:
        # Exact cases only need hashes/results; retaining 2240 triplets of IR
        # unnecessarily increases disk traffic and slows the matrix run.
        before_cv.unlink(missing_ok=True)
        real_before_pm.unlink(missing_ok=True)
        cv2pm_before_pm.unlink(missing_ok=True)
        real_stderr.unlink(missing_ok=True)
        cv2pm_stderr.unlink(missing_ok=True)
        try:
            case.rmdir()
        except OSError:
            pass
    return {
        "group": group_id,
        "adapter": adapter.name,
        "status": (
            "exact"
            if exact
            else (
                "matched_failure"
                if matched_failure
                else ("different" if comparable else "error")
            )
        ),
        "real_exit": real_exit,
        "cv2pm_exit": cv2pm_exit,
        "real_timeout": int(real_timeout),
        "cv2pm_timeout": int(cv2pm_timeout),
        "real_sha256": real_hash,
        "cv2pm_sha256": cv2pm_hash,
        "real_seconds": f"{real_seconds:.6f}",
        "cv2pm_seconds": f"{cv2pm_seconds:.6f}",
        "diagnostic": (
            diagnostic_tail(real_stderr) + " | " + diagnostic_tail(cv2pm_stderr)
        ).strip(" |"),
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--matrix", type=Path, default=MODULE / "config/corpus_test_matrix.tsv"
    )
    parser.add_argument("--adapter-root", type=Path, default=MODULE / "data/adapter")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=MODULE / "data/before_cvpipelining/manifest.tsv",
    )
    parser.add_argument("--compiler", type=Path, default=REPO / "build/bin/bishengir-compile")
    parser.add_argument("--cv2pm", type=Path, default=REPO / "build/bin/cv2pm-bishengir-compile")
    parser.add_argument(
        "--output-root", type=Path, default=REPO / "Output/bisheng_cv2pm_matrix"
    )
    parser.add_argument("--jobs", type=int, default=12)
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("--max-inputs", type=int, default=0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.output_root.exists():
        print(f"[ERROR] output already exists: {args.output_root}", file=sys.stderr)
        return 2
    args.output_root.mkdir(parents=True)
    rows = list(csv.DictReader(args.matrix.open(), delimiter="\t"))
    manifest = list(csv.DictReader(args.manifest.open(), delimiter="\t"))
    adapters = [
        args.adapter_root / row["adapter"]
        for row in manifest
        if row["dump_status"] == "complete"
    ]
    if args.max_inputs:
        adapters = adapters[: args.max_inputs]
    groups: dict[EffectiveConfig, list[str]] = {}
    required: dict[str, str] = {}
    for row in rows:
        groups.setdefault(effective_config(row), []).append(row["name"])
        required[row["name"]] = row["required"]
    numbered = {config: index for index, config in enumerate(groups, 1)}
    (args.output_root / "effective_groups.json").write_text(
        json.dumps(
            [
                {
                    "group": numbered[config],
                    "logical_configs": names,
                    "effective": config.__dict__,
                }
                for config, names in groups.items()
            ],
            indent=2,
        )
        + "\n"
    )
    jobs = [
        (adapter, config, numbered[config])
        for config in groups
        for adapter in adapters
    ]
    started = time.monotonic()
    completed = 0
    results: list[dict[str, object]] = []
    print(
        f"logical={len(rows) * len(adapters)} effective={len(jobs)} "
        f"inputs={len(adapters)} configs={len(rows)} groups={len(groups)} "
        f"workers={args.jobs}",
        flush=True,
    )
    lock = threading.Lock()
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [
            pool.submit(
                compare_one,
                adapter,
                config,
                group_id,
                args.output_root,
                args.compiler.resolve(),
                args.cv2pm.resolve(),
                args.timeout,
            )
            for adapter, config, group_id in jobs
        ]
        for future in as_completed(futures):
            result = future.result()
            with lock:
                results.append(result)
                completed += 1
                if completed % 20 == 0 or completed == len(jobs):
                    elapsed = time.monotonic() - started
                    rate = completed / elapsed if elapsed else 0.0
                    eta = (len(jobs) - completed) / rate if rate else 0.0
                    bad = sum(
                        r["status"] not in {"exact", "matched_failure"}
                        for r in results
                    )
                    print(
                        f"progress={completed}/{len(jobs)} "
                        f"logical={min(len(rows)*len(adapters), completed*len(rows)//len(groups))}/"
                        f"{len(rows)*len(adapters)} bad={bad} "
                        f"elapsed={elapsed:.1f}s eta={eta:.1f}s",
                        flush=True,
                    )
    results.sort(key=lambda row: (int(row["group"]), str(row["adapter"])))
    columns = list(results[0]) if results else []
    with (args.output_root / "effective_results.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(results)
    logical_results = []
    group_names = {numbered[config]: names for config, names in groups.items()}
    for result in results:
        for name in group_names[int(result["group"])]:
            logical_results.append({"config": name, "required": required[name], **result})
    logical_columns = list(logical_results[0]) if logical_results else []
    with (args.output_root / "results.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=logical_columns, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(logical_results)
    exact = sum(row["status"] == "exact" for row in logical_results)
    matched_failures = sum(
        row["status"] == "matched_failure" for row in logical_results
    )
    total = len(logical_results)
    elapsed = time.monotonic() - started
    summary = {
        "logical_total": total,
        "logical_exact": exact,
        "logical_matched_failures": matched_failures,
        "logical_unmatched": total - exact - matched_failures,
        "effective_total": len(results),
        "effective_exact": sum(row["status"] == "exact" for row in results),
        "effective_matched_failures": sum(
            row["status"] == "matched_failure" for row in results
        ),
        "elapsed_seconds": elapsed,
        "workers": args.jobs,
    }
    (args.output_root / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print("SUMMARY " + json.dumps(summary), flush=True)
    return 0 if exact + matched_failures == total else 1


if __name__ == "__main__":
    raise SystemExit(main())
