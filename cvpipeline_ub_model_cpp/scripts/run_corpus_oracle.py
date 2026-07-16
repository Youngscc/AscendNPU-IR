#!/usr/bin/env python3
"""Run corpus-wide real-compiler/final-PlanMemory differential validation."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from compare_ub_plan_with_suffix_oracle import (
    canonical_function_name,
    model_multi_and_inplace,
    normalized_lifetimes_from_model,
    parse_oracle,
    parse_oracle_contract,
    parse_oracle_retry,
    plan_multiset_from_model,
)
from cvpipeline_oracle_common import discover_inputs, safe_name


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus-root", required=True, type=Path)
    parser.add_argument("--model", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--seeds", default="0-19")
    parser.add_argument("--restrict-inplace-as-isa", action="store_true")
    parser.add_argument("--enable-auto-multi-buffer", action="store_true")
    parser.add_argument("--enable-triton-kernel-compile", action="store_true")
    parser.add_argument("--local-multi-buffer-strategy", default="no-l0c",
                        choices=("no-limit", "only-cube", "only-vector", "no-l0c"))
    parser.add_argument("--mix-multi-buffer-strategy", default="only-cube",
                        choices=("no-limit", "only-cube", "only-vector", "no-l0c"))
    parser.add_argument("--stage-snapshots-dir", type=Path)
    parser.add_argument("--pipeline-manifest", type=Path)
    parser.add_argument("--max-cases", type=int)
    parser.add_argument("--require-all-exact", action="store_true")
    parser.add_argument("--check-retry", action="store_true",
                        help="Also compare the default retry-selected plan")
    parser.add_argument("--quiet", action="store_true",
                        help="Only print the summary and failures")
    return parser.parse_args()


def seeds(text: str) -> list[int]:
    result: set[int] = set()
    for token in text.split(","):
        token = token.strip()
        if "-" in token:
            first, last = (int(value) for value in token.split("-", 1))
            result.update(range(first, last + 1))
        elif token:
            result.add(int(token))
    if not result or min(result) < 0 or max(result) > 19:
        raise ValueError("--seeds must select values in [0, 19]")
    return sorted(result)


def run(command: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, text=True, capture_output=True, env=env)


def main() -> int:
    args = parse_args()
    selected_seeds = seeds(args.seeds)
    inputs = discover_inputs(args.corpus_root)
    if args.max_cases is not None:
        inputs = inputs[:args.max_cases]
    if not inputs:
        print("no before_cvpipelining_*.mlir inputs found", file=sys.stderr)
        return 1
    for executable in (args.model, args.compiler):
        if not executable.is_file() or not os.access(executable, os.X_OK):
            print(f"not an executable: {executable}", file=sys.stderr)
            return 1

    failures: list[str] = []
    blockers = 0
    exact = 0
    manifest_written = False
    with tempfile.TemporaryDirectory(prefix="cvub-corpus-oracle-") as temp:
        temp_root = Path(temp)
        for case_index, input_path in enumerate(inputs):
            relative = input_path.relative_to(args.corpus_root.resolve())
            case_name = safe_name(str(relative))
            case_was_blocker = False
            for seed in selected_seeds:
                model_command = [
                    str(args.model),
                    f"--before-cvpipelining-ir={input_path}",
                    f"--random-seed={seed}",
                    "--format=json",
                    f"--enable-auto-multi-buffer={'true' if args.enable_auto_multi_buffer else 'false'}",
                    f"--enable-triton-kernel-compile={'true' if args.enable_triton_kernel_compile else 'false'}",
                    "--limit-auto-multi-buffer-of-local-buffer",
                    args.local_multi_buffer_strategy,
                    "--limit-auto-multi-buffer-buffer",
                    args.mix_multi_buffer_strategy,
                ]
                if args.restrict_inplace_as_isa:
                    model_command.append("--restrict-inplace-as-isa")
                modeled = run(model_command)
                try:
                    payload = json.loads(modeled.stdout)
                except json.JSONDecodeError:
                    failures.append(
                        f"{relative} seed {seed}: model did not emit JSON: {modeled.stderr.strip()}")
                    break

                if payload.get("precision") != "exact":
                    if modeled.returncode != 1 or \
                            payload.get("status") != "blocker" or \
                            payload.get("ub_peak_bits") is not None or \
                            payload.get("required_bits") is not None or \
                            payload.get("functions") != []:
                        failures.append(
                            f"{relative} seed {seed}: incomplete report violates blocker contract")
                    # A blocker is safe, but the corpus gate must still prove
                    # the real pipeline accepts the input and emit its stage
                    # snapshots for coverage auditing.
                    if seed == selected_seeds[0]:
                        blocker_output = temp_root / f"{case_name}.blocker.mlir"
                        blocker_command = [
                            str(args.compiler), str(input_path), "-o",
                            str(blocker_output), f"--plan-memory-seed={seed}",
                            "--mlir-disable-threading",
                            f"--enable-auto-multi-buffer={'true' if args.enable_auto_multi_buffer else 'false'}",
                            f"--enable-triton-kernel-compile={'true' if args.enable_triton_kernel_compile else 'false'}",
                            f"--limit-auto-multi-buffer-of-local-buffer={args.local_multi_buffer_strategy}",
                            f"--limit-auto-multi-buffer-buffer={args.mix_multi_buffer_strategy}",
                        ]
                        if args.restrict_inplace_as_isa:
                            blocker_command.append("--restrict-inplace-as-isa")
                        if args.stage_snapshots_dir:
                            blocker_command.append(
                                f"--dump-stage-oracle-dir={args.stage_snapshots_dir / case_name}")
                        if not manifest_written and args.pipeline_manifest:
                            blocker_command.append(
                                f"--dump-pipeline-stage-manifest={args.pipeline_manifest}")
                            manifest_written = True
                        compiled_blocker = run(blocker_command)
                        expected_capacity_failure = re.search(
                            r"error: [^\n]* overflow, requires ",
                            compiled_blocker.stderr) is not None
                        if (compiled_blocker.returncode != 0 and
                                not expected_capacity_failure):
                            failures.append(
                                f"{relative}: real compiler failed on model blocker: "
                                f"{compiled_blocker.stderr[-2000:]}")
                    if seed != selected_seeds[0] and not case_was_blocker:
                        failures.append(
                            f"{relative} seed {seed}: precision changed from "
                            "exact to blocker across seeds")
                    case_was_blocker = True
                    # Blocker safety is a per-seed contract.  Continue running
                    # the model for every requested seed; only the expensive
                    # real compiler acceptance/snapshot check is seed 0.
                    continue

                if case_was_blocker:
                    failures.append(
                        f"{relative} seed {seed}: precision changed from "
                        "blocker to exact across seeds")
                    continue
                expected_model_rc = 2 if payload.get("status") == "overflow" else 0
                if modeled.returncode != expected_model_rc:
                    failures.append(
                        f"{relative} seed {seed}: exact {payload.get('status')} "
                        f"returned {modeled.returncode}, expected {expected_model_rc}")
                    break

                oracle_output = temp_root / f"{case_name}.{seed}.mlir"
                compiler_command = [
                    str(args.compiler), str(input_path), "-o", str(oracle_output),
                    f"--plan-memory-seed={seed}", "--mlir-disable-threading",
                    f"--enable-auto-multi-buffer={'true' if args.enable_auto_multi_buffer else 'false'}",
                    f"--enable-triton-kernel-compile={'true' if args.enable_triton_kernel_compile else 'false'}",
                    f"--limit-auto-multi-buffer-of-local-buffer={args.local_multi_buffer_strategy}",
                    f"--limit-auto-multi-buffer-buffer={args.mix_multi_buffer_strategy}",
                ]
                if args.restrict_inplace_as_isa:
                    compiler_command.append("--restrict-inplace-as-isa")
                if seed == selected_seeds[0] and args.stage_snapshots_dir:
                    stage_dir = args.stage_snapshots_dir / case_name
                    compiler_command.append(f"--dump-stage-oracle-dir={stage_dir}")
                if not manifest_written and args.pipeline_manifest:
                    compiler_command.append(
                        f"--dump-pipeline-stage-manifest={args.pipeline_manifest}")
                    manifest_written = True
                oracle_env = dict(os.environ)
                oracle_env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
                compiled = run(compiler_command, env=oracle_env)
                expected_capacity_failure = re.search(
                    r"error: [^\n]* overflow, requires ",
                    compiled.stderr) is not None
                if (compiled.returncode != 0 and
                        not (payload.get("status") == "overflow" and
                             expected_capacity_failure)):
                    failures.append(
                        f"{relative} seed {seed}: real compiler failed: {compiled.stderr[-2000:]}")
                    break
                oracle_tsv = temp_root / f"{case_name}.{seed}.oracle.tsv"
                oracle_tsv.write_text(compiled.stderr, encoding="utf-8")
                _, oracle_peak, oracle_plan, oracle_lifetimes = parse_oracle(
                    oracle_tsv, seed, "6")
                oracle_status, oracle_required, oracle_multi, oracle_inplace = (
                    parse_oracle_contract(oracle_tsv, seed, "6"))
                model_peak = int(payload["ub_peak_bits"])
                model_plan = plan_multiset_from_model(payload)
                model_lifetimes = normalized_lifetimes_from_model(payload)
                model_multi, model_inplace = model_multi_and_inplace(payload)
                differences = []
                if oracle_status != payload.get("status"):
                    differences.append("status")
                if oracle_required != int(payload["required_bits"]):
                    differences.append("required")
                if oracle_peak != model_peak:
                    differences.append("peak")
                if oracle_plan != model_plan:
                    differences.append("plan")
                if oracle_lifetimes != model_lifetimes:
                    differences.append("lifetime")
                if oracle_multi != model_multi:
                    differences.append("multi")
                if oracle_inplace != model_inplace:
                    differences.append("inplace")
                if differences:
                    failures.append(
                        f"{relative} seed {seed}: {','.join(differences)} differ "
                        f"(model peak={model_peak}, oracle peak={oracle_peak}, "
                        f"model buffers={sum(model_plan.values())}, "
                        f"oracle buffers={sum(oracle_plan.values())})")
                    break
            if not case_was_blocker and args.check_retry:
                retry_model_command = [
                    str(args.model),
                    f"--before-cvpipelining-ir={input_path}", "--format=json",
                    f"--enable-auto-multi-buffer={'true' if args.enable_auto_multi_buffer else 'false'}",
                    f"--enable-triton-kernel-compile={'true' if args.enable_triton_kernel_compile else 'false'}",
                    "--limit-auto-multi-buffer-of-local-buffer",
                    args.local_multi_buffer_strategy,
                    "--limit-auto-multi-buffer-buffer",
                    args.mix_multi_buffer_strategy,
                ]
                if args.restrict_inplace_as_isa:
                    retry_model_command.append("--restrict-inplace-as-isa")
                modeled_retry = run(retry_model_command)
                try:
                    retry_payload = json.loads(modeled_retry.stdout)
                except json.JSONDecodeError:
                    failures.append(
                        f"{relative} retry: model did not emit JSON: "
                        f"{modeled_retry.stderr.strip()}")
                else:
                    if retry_payload.get("precision") != "exact":
                        failures.append(
                            f"{relative} retry: fixed-seed Exact became blocker")
                    else:
                        retry_output = temp_root / f"{case_name}.retry.mlir"
                        retry_compiler_command = [
                            str(args.compiler), str(input_path), "-o",
                            str(retry_output), "--plan-memory-seed=-1",
                            "--mlir-disable-threading",
                            f"--enable-auto-multi-buffer={'true' if args.enable_auto_multi_buffer else 'false'}",
                            f"--enable-triton-kernel-compile={'true' if args.enable_triton_kernel_compile else 'false'}",
                            f"--limit-auto-multi-buffer-of-local-buffer={args.local_multi_buffer_strategy}",
                            f"--limit-auto-multi-buffer-buffer={args.mix_multi_buffer_strategy}",
                        ]
                        if args.restrict_inplace_as_isa:
                            retry_compiler_command.append("--restrict-inplace-as-isa")
                        oracle_env = dict(os.environ)
                        oracle_env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
                        compiled_retry = run(retry_compiler_command,
                                             env=oracle_env)
                        if compiled_retry.returncode != 0:
                            failures.append(
                                f"{relative} retry: real compiler failed: "
                                f"{compiled_retry.stderr[-2000:]}")
                        else:
                            retry_oracle = temp_root / f"{case_name}.retry.tsv"
                            retry_oracle.write_text(compiled_retry.stderr,
                                                    encoding="utf-8")
                            selected_by_function, retry_peak, retry_plan, retry_lifetimes = (
                                parse_oracle_retry(retry_oracle, "6"))
                            model_selected = {
                                canonical_function_name(str(function["function"])):
                                int(function["selected_seed"])
                                for function in retry_payload.get("functions", [])
                                if (function.get("buffers") or
                                    int(function.get("ub_peak_bits", 0)) != 0)
                            }
                            if (retry_peak != int(retry_payload["ub_peak_bits"]) or
                                    retry_plan != plan_multiset_from_model(retry_payload) or
                                    retry_lifetimes != normalized_lifetimes_from_model(
                                        retry_payload) or
                                    selected_by_function != model_selected):
                                failures.append(
                                    f"{relative} retry: selected-seed/peak/plan/"
                                    "lifetime differ")
            if case_was_blocker:
                blockers += 1
            else:
                exact += 1
            status = "BLOCKER" if case_was_blocker else "EXACT"
            if not args.quiet:
                print(f"[{case_index + 1}/{len(inputs)}] {status} {relative}")

    if args.require_all_exact and blockers:
        failures.append(f"{blockers} corpus inputs returned blocker")
    print(f"summary: inputs={len(inputs)} exact={exact} blockers={blockers} failures={len(failures)}")
    for failure in failures:
        print(f"[FAIL] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
