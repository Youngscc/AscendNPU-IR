#!/usr/bin/env python3
"""Compare modeled and compiler PlanMemory inputs at the suffix boundary."""

from __future__ import annotations

import argparse
import csv
import difflib
import json
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path

from cvpipelining_oracle_common import (
    adapter_and_snapshot,
    canonical_json,
    config_argv,
    discover_inputs,
    safe_name,
    sha256_file,
)


@dataclass(frozen=True)
class Case:
    index: int
    tier: str
    input_path: Path
    cv_config: dict[str, str]
    suffix_config: dict[str, str]


def parse_args() -> argparse.Namespace:
    script = Path(__file__).resolve()
    module = script.parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser(
        description="Validate PlanMemory-input projection byte for byte.")
    parser.add_argument("--input-root", type=Path,
                        default=module / "data/before_cvpipelining")
    parser.add_argument("--cv-config-matrix", type=Path,
                        default=module / "testdata/config_matrix/cvpipelining.tsv")
    parser.add_argument("--suffix-config-matrix", type=Path,
                        default=module / "testdata/config_matrix/suffix_ub_passes.tsv")
    parser.add_argument("--compiler", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--model", type=Path,
                        default=module / "output/bin/cvpipeline_ub_model_dev_validate")
    parser.add_argument("--artifacts", type=Path,
                        default=repo / "Output/experiments/plan_memory_input_matrix")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument("--max-cases", type=int, default=0)
    parser.add_argument("--max-failures", type=int, default=20)
    parser.add_argument("--full-cross-product", action="store_true",
                        help="Run every input with every CV/suffix config pair.")
    parser.add_argument("--axis-product", action="store_true",
                        help="Run every input across each config axis, plus all config pairs.")
    parser.add_argument("--no-resume", action="store_true")
    parser.add_argument(
        "--reuse-existing-oracle", action="store_true",
        help="Reuse existing generic/before-PlanMemory/oracle files and rerun only the model.")
    return parser.parse_args()


def read_matrix(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    if not rows or "config_id" not in rows[0]:
        raise SystemExit(f"[ERROR] invalid or empty config matrix: {path}")
    return rows


def unique_suffix_configs(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    fields = (
        "enable_auto_multi_buffer",
        "local_multi_buffer_strategy",
        "mix_multi_buffer_strategy",
    )
    unique: dict[tuple[str, ...], dict[str, str]] = {}
    for row in rows:
        key = tuple(row[field] for field in fields)
        if key not in unique:
            unique[key] = {"config_id": row["config_id"],
                           **{field: row[field] for field in fields}}
    return list(unique.values())


def build_cases(inputs: list[Path], cv_configs: list[dict[str, str]],
                suffix_configs: list[dict[str, str]],
                full_cross_product: bool, axis_product: bool) -> list[Case]:
    cases: list[Case] = []
    default_cv = next(
        (row for row in cv_configs
         if row["disable_cv_pipelining"] == "0"
         and row["enable_preload"] == "0"
         and row["cv_pipeline_depth"] == "-1"
         and row["enable_cv_lazy_loading"] == "0"),
        cv_configs[0],
    )
    default_suffix = next(
        (row for row in suffix_configs
         if row["enable_auto_multi_buffer"] == "0"),
        suffix_configs[0],
    )
    if full_cross_product:
        for input_path in inputs:
            for cv_config in cv_configs:
                for suffix_config in suffix_configs:
                    cases.append(Case(len(cases), "full", input_path,
                                      cv_config, suffix_config))
        return cases

    if axis_product:
        for input_path in inputs:
            for cv_config in cv_configs:
                cases.append(Case(len(cases), "all_inputs_cv_axis", input_path,
                                  cv_config, default_suffix))
            for suffix_config in suffix_configs:
                if suffix_config == default_suffix:
                    continue
                cases.append(Case(len(cases), "all_inputs_suffix_axis",
                                  input_path, default_cv, suffix_config))
        for pair_index, (cv_config, suffix_config) in enumerate(
                (pair for cv in cv_configs for pair in
                 ((cv, suffix) for suffix in suffix_configs))):
            input_path = inputs[pair_index % len(inputs)]
            cases.append(Case(len(cases), "all_config_pairs", input_path,
                              cv_config, suffix_config))
        return cases

    for input_path in inputs:
        cases.append(Case(len(cases), "all_inputs_default", input_path,
                          default_cv, default_suffix))
    for pair_index, (cv_config, suffix_config) in enumerate(
            (pair for cv in cv_configs for pair in
             ((cv, suffix) for suffix in suffix_configs))):
        input_path = inputs[pair_index % len(inputs)]
        cases.append(Case(len(cases), "all_config_pairs", input_path,
                          cv_config, suffix_config))
    return cases


def first_difference(expected: bytes, actual: bytes) -> str:
    expected_lines = expected.decode(errors="replace").splitlines()
    actual_lines = actual.decode(errors="replace").splitlines()
    difference = list(difflib.unified_diff(
        expected_lines, actual_lines, fromfile="oracle", tofile="model",
        n=1))
    excerpt = " | ".join(difference[2:8]) if len(difference) > 2 else "unknown byte mismatch"
    return (excerpt[:1000].replace("\t", "\\t")
            .replace("\r", "\\r").replace("\n", "\\n"))


def mismatch_kind(expected: bytes, actual: bytes) -> str:
    expected_lines = expected.splitlines()
    actual_lines = actual.splitlines()
    if len(actual_lines) == 1 and actual_lines[0].startswith(
            b"PLANMEMORY_CANONICAL_INPUT"):
        return "empty_model_input"
    expected_allocs = [line for line in expected_lines if line.startswith(b"ALLOC\t")]
    actual_allocs = [line for line in actual_lines if line.startswith(b"ALLOC\t")]
    if expected_allocs != actual_allocs:
        return "allocation_mismatch"
    return "operation_sequence_mismatch"


def log_excerpt(path: Path) -> str:
    if not path.is_file():
        return ""
    text = path.read_text(errors="replace").strip()
    return (text[:1000].replace("\t", "\\t")
            .replace("\r", "\\r").replace("\n", "\\n"))


def run_case(case: Case, args: argparse.Namespace, input_root: Path,
             compiler_hash: str, model_hash: str) -> dict[str, str]:
    adapter, snapshot = adapter_and_snapshot(case.input_path, input_root)
    cv_id = safe_name(case.cv_config["config_id"])
    suffix_id = safe_name(case.suffix_config["config_id"])
    case_id = f"{case.index:05d}_{case.tier}_{cv_id}_{suffix_id}"
    run_dir = args.artifacts / "cases" / adapter / snapshot / case_id
    run_dir.mkdir(parents=True, exist_ok=True)
    generic = run_dir / "after_cvpipelining.generic.mlir"
    before_plan_memory = run_dir / "before_plan_memory.mlir"
    after_plan_memory = run_dir / "after_plan_memory.mlir"
    oracle = run_dir / "oracle.canonical.tsv"
    model = run_dir / "model.canonical.tsv"
    compiler_log = run_dir / "compiler.stderr.log"
    model_log = run_dir / "model.stderr.log"
    oracle_log = run_dir / "oracle.stderr.log"
    marker = run_dir / "complete.json"
    oracle_marker = run_dir / "oracle.complete.json"

    combined_config = dict(case.cv_config)
    combined_config.update({
        field: case.suffix_config[field]
        for field in ("enable_auto_multi_buffer",
                      "local_multi_buffer_strategy",
                      "mix_multi_buffer_strategy")
    })
    fingerprint = {
        "input_sha256": sha256_file(case.input_path),
        "compiler_sha256": compiler_hash,
        "model_sha256": model_hash,
        "config": combined_config,
    }
    oracle_fingerprint = {
        "input_sha256": fingerprint["input_sha256"],
        "compiler_sha256": compiler_hash,
        "config": combined_config,
    }
    oracle_is_current = False
    if oracle_marker.is_file():
        try:
            oracle_is_current = (
                json.loads(oracle_marker.read_text()) == oracle_fingerprint)
        except (OSError, json.JSONDecodeError):
            pass
    if (not args.no_resume and oracle_is_current and marker.is_file()
            and oracle.is_file() and model.is_file()):
        try:
            if json.loads(marker.read_text()) == fingerprint and oracle.read_bytes() == model.read_bytes():
                return make_result(case, adapter, snapshot, case_id, "pass", run_dir)
        except (OSError, json.JSONDecodeError):
            pass

    reuse_oracle = (args.reuse_existing_oracle and oracle_is_current
                    and generic.is_file() and before_plan_memory.is_file()
                    and oracle.is_file())
    compiler_rc = 0
    compiler_outcome = "reused_or_success"
    compiler_diagnostic = ""
    if not reuse_oracle:
        for stale_output in (generic, before_plan_memory, after_plan_memory):
            stale_output.unlink(missing_ok=True)
        compiler_command = [
            str(args.compiler), str(case.input_path), "--mlir-disable-threading",
            f"--dump-after-cvpipelining-generic-ir={generic}", "--plan-memory-seed=0",
            "--enable-memory-display=false", *config_argv(combined_config),
            "-o", str(after_plan_memory),
        ]
        env = os.environ.copy()
        env["BISHENGIR_DUMP_BEFORE_PLAN_MEMORY"] = str(before_plan_memory)
        with compiler_log.open("wb") as stderr:
            compiler_rc = subprocess.run(
                compiler_command, stdout=subprocess.DEVNULL, stderr=stderr,
                env=env).returncode
        compiler_diagnostic = log_excerpt(compiler_log)
        if not generic.is_file() or not before_plan_memory.is_file():
            return make_result(case, adapter, snapshot, case_id,
                               "compiler_failed", run_dir,
                               compiler_diagnostic or str(compiler_rc),
                               "compiler_boundary_failure",
                               compiler_rc, "boundary_missing",
                               compiler_diagnostic)
        if compiler_rc != 0:
            compiler_outcome = "failed_after_plan_memory_boundary"

    model_command = [
        str(args.model), "--action=model-plan-memory-canonical-input",
        f"--root={generic}", f"--output={model}",
        "--enable-code-motion=" + combined_config["enable_code_motion"],
        "--limit-auto-multi-buffer-of-local-buffer="
        + case.suffix_config["local_multi_buffer_strategy"],
        "--limit-auto-multi-buffer-buffer="
        + case.suffix_config["mix_multi_buffer_strategy"],
    ]
    if case.suffix_config["enable_auto_multi_buffer"] == "1":
        model_command.append("--enable-auto-multi-buffer")
    with model_log.open("wb") as stderr:
        model_rc = subprocess.run(
            model_command, stdout=subprocess.DEVNULL, stderr=stderr).returncode
    if model_rc != 0 or not model.is_file():
        return make_result(case, adapter, snapshot, case_id,
                           "model_failed", run_dir,
                           log_excerpt(model_log) or str(model_rc),
                           "model_blocker", compiler_rc, compiler_outcome,
                           compiler_diagnostic)

    if not reuse_oracle:
        oracle_command = [
            str(args.model), "--action=dump-plan-memory-canonical-input-oracle",
            f"--root={before_plan_memory}", f"--output={oracle}",
        ]
        with oracle_log.open("wb") as stderr:
            oracle_rc = subprocess.run(
                oracle_command, stdout=subprocess.DEVNULL, stderr=stderr).returncode
        if oracle_rc != 0 or not oracle.is_file():
            return make_result(case, adapter, snapshot, case_id,
                               "oracle_failed", run_dir,
                               log_excerpt(oracle_log) or str(oracle_rc),
                               "oracle_failure", compiler_rc,
                               compiler_outcome, compiler_diagnostic)
        oracle_marker.write_text(canonical_json(oracle_fingerprint) + "\n")

    oracle_bytes = oracle.read_bytes()
    model_bytes = model.read_bytes()
    if oracle_bytes != model_bytes:
        return make_result(case, adapter, snapshot, case_id, "mismatch",
                           run_dir, first_difference(oracle_bytes, model_bytes),
                           mismatch_kind(oracle_bytes, model_bytes),
                           compiler_rc, compiler_outcome,
                           compiler_diagnostic)
    marker.write_text(canonical_json(fingerprint) + "\n")
    return make_result(case, adapter, snapshot, case_id, "pass", run_dir,
                       compiler_returncode=compiler_rc,
                       compiler_outcome=compiler_outcome,
                       compiler_diagnostic=compiler_diagnostic)


def make_result(case: Case, adapter: str, snapshot: str, case_id: str,
                status: str, run_dir: Path, diagnostic: str = "",
                category: str = "exact", compiler_returncode: int = 0,
                compiler_outcome: str = "reused_or_success",
                compiler_diagnostic: str = "") -> dict[str, str]:
    return {
        "case_index": str(case.index), "case_id": case_id,
        "tier": case.tier, "adapter": adapter, "snapshot": snapshot,
        "cv_config_id": case.cv_config["config_id"],
        "suffix_config_id": case.suffix_config["config_id"],
        "enable_auto_multi_buffer": case.suffix_config["enable_auto_multi_buffer"],
        "local_strategy": case.suffix_config["local_multi_buffer_strategy"],
        "mix_strategy": case.suffix_config["mix_multi_buffer_strategy"],
        "status": status, "category": category, "diagnostic": diagnostic,
        "compiler_returncode": str(compiler_returncode),
        "compiler_outcome": compiler_outcome,
        "compiler_diagnostic": compiler_diagnostic,
        "artifacts": str(run_dir),
    }


def main() -> int:
    args = parse_args()
    args.input_root = args.input_root.resolve()
    args.artifacts = args.artifacts.resolve()
    args.compiler = args.compiler.resolve()
    args.model = args.model.resolve()
    required = (args.input_root, args.cv_config_matrix,
                args.suffix_config_matrix, args.compiler, args.model)
    if any(not path.exists() for path in required):
        missing = [str(path) for path in required if not path.exists()]
        raise SystemExit("[ERROR] missing required path(s): " + ", ".join(missing))
    if args.jobs < 1:
        raise SystemExit("[ERROR] --jobs must be positive")

    inputs = discover_inputs(args.input_root)
    cv_configs = read_matrix(args.cv_config_matrix)
    suffix_configs = unique_suffix_configs(read_matrix(args.suffix_config_matrix))
    cases = build_cases(inputs, cv_configs, suffix_configs,
                        args.full_cross_product, args.axis_product)
    if args.max_cases:
        cases = cases[:args.max_cases]
    args.artifacts.mkdir(parents=True, exist_ok=True)
    print(f"PlanMemory-input matrix: inputs={len(inputs)} cv_configs={len(cv_configs)} "
          f"suffix_configs={len(suffix_configs)} cases={len(cases)} jobs={args.jobs}",
          flush=True)

    compiler_hash = sha256_file(args.compiler)
    model_hash = sha256_file(args.model)
    results: list[dict[str, str]] = []
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(run_case, case, args, args.input_root,
                                   compiler_hash, model_hash)
                   for case in cases]
        for completed, future in enumerate(as_completed(futures), 1):
            result = future.result()
            results.append(result)
            if completed % 25 == 0 or completed == len(cases):
                failures = sum(row["status"] != "pass" for row in results)
                print(f"progress={completed}/{len(cases)} failures={failures}",
                      flush=True)

    results.sort(key=lambda row: int(row["case_index"]))
    fields = list(results[0]) if results else []
    summary = args.artifacts / "summary.tsv"
    with summary.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(results)
    counts: dict[str, int] = {}
    for result in results:
        counts[result["status"]] = counts.get(result["status"], 0) + 1
    (args.artifacts / "summary.json").write_text(canonical_json({
        "inputs": len(inputs), "cv_configs": len(cv_configs),
        "suffix_configs": len(suffix_configs), "cases": len(cases),
        "full_cross_product": args.full_cross_product, "status_counts": counts,
        "axis_product": args.axis_product,
        "compiler_sha256": compiler_hash, "model_sha256": model_hash,
    }) + "\n")
    print("PlanMemory-input result: " + " ".join(f"{key}={value}" for key, value in sorted(counts.items())))
    print(summary)
    failures = [row for row in results if row["status"] != "pass"]
    for failure in failures[:args.max_failures]:
        print(f"FAIL {failure['case_id']}: {failure['status']} "
              f"{failure['diagnostic']} ({failure['artifacts']})")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
