#!/usr/bin/env python3
"""Validate OneShotBufferize output across IR/config combinations."""

from __future__ import annotations

import argparse
import collections
import csv
import json
import os
import shutil
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
    input_path: Path
    config: dict[str, str]


def parse_args() -> argparse.Namespace:
    script = Path(__file__).resolve()
    module = script.parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser(
        description="Validate OneShotBufferize output delivered to "
                    "HIVMDecomposeOp.")
    parser.add_argument(
        "--input-root", type=Path,
        default=module / "data/before_cvpipelining")
    parser.add_argument(
        "--config-matrix", type=Path,
        default=module / "testdata/config_matrix/cvpipelining.tsv")
    parser.add_argument(
        "--compiler", type=Path,
        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument(
        "--model", type=Path,
        default=module / "output/bin/cvpipeline_ub_model_dev_validate")
    parser.add_argument(
        "--artifacts", type=Path,
        default=repo / "Output/experiments/one_shot_bufferize")
    parser.add_argument("--jobs", type=int,
                        default=min(8, os.cpu_count() or 1))
    parser.add_argument(
        "--max-inputs", type=int, default=12,
        help="Deterministically sample this many inputs; 0 selects all.")
    parser.add_argument(
        "--max-configs", type=int, default=0,
        help="Deterministically sample this many configs; 0 selects all.")
    parser.add_argument("--max-cases", type=int, default=0)
    parser.add_argument("--max-failures", type=int, default=30)
    parser.add_argument("--no-resume", action="store_true")
    parser.add_argument(
        "--reuse-existing-oracle", action="store_true",
        help="Reuse existing CVPipelining and OneShotBufferize dumps.")
    return parser.parse_args()


def read_matrix(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    if not rows or "config_id" not in rows[0]:
        raise SystemExit(f"[ERROR] invalid config matrix: {path}")
    return rows


def sample_evenly(values: list, limit: int) -> list:
    if limit <= 0 or limit >= len(values):
        return values
    if limit == 1:
        return [values[0]]
    indices = {
        round(index * (len(values) - 1) / (limit - 1))
        for index in range(limit)
    }
    return [values[index] for index in sorted(indices)]


def report_fields(path: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    if not path.is_file():
        return fields
    for line in path.read_text(errors="replace").splitlines():
        columns = line.split("\t")
        if columns[0] in {
                "PREBUFFER_TEXT_EXACT", "ALLOCATIONS_EXACT",
                "OPERATION_COUNTS_EXACT", "MODEL_ALLOCATIONS",
                "ORACLE_ALLOCATIONS"}:
            fields[columns[0].lower()] = columns[1]
        elif columns[0] == "SUMMARY" and len(columns) == 4:
            fields["checked_values"] = columns[1]
            fields["checked_accesses"] = columns[2]
            fields["semantic_errors"] = columns[3]
    return fields


def log_excerpt(path: Path) -> str:
    if not path.is_file():
        return ""
    return path.read_text(errors="replace").strip()[:1000].replace("\n", "\\n")


def one_shot_pair(tree: Path) -> tuple[Path, Path] | None:
    before = list(tree.rglob("0_one-shot-bufferize.mlir"))
    after = list(tree.rglob("1_one-shot-bufferize.mlir"))
    if len(before) != 1 or len(after) != 1:
        return None
    return before[0], after[0]


def result_row(case: Case, adapter: str, snapshot: str, status: str,
               category: str, directory: Path, diagnostic: str = "",
               fields: dict[str, str] | None = None) -> dict[str, str]:
    values = fields or {}
    return {
        "case_index": str(case.index),
        "adapter": adapter,
        "snapshot": snapshot,
        "config_id": case.config["config_id"],
        "status": status,
        "category": category,
        "prebuffer_text_exact": values.get("prebuffer_text_exact", ""),
        "operation_counts_exact": values.get("operation_counts_exact", ""),
        "allocations_exact": values.get("allocations_exact", ""),
        "model_allocations": values.get("model_allocations", "0"),
        "oracle_allocations": values.get("oracle_allocations", "0"),
        "checked_values": values.get("checked_values", "0"),
        "checked_accesses": values.get("checked_accesses", "0"),
        "semantic_errors": values.get("semantic_errors", "0"),
        "diagnostic": diagnostic,
        "artifacts": str(directory),
    }


def run_case(case: Case, args: argparse.Namespace,
             input_root: Path) -> dict[str, str]:
    adapter, snapshot = adapter_and_snapshot(case.input_path, input_root)
    config_id = safe_name(case.config["config_id"])
    directory = args.artifacts / adapter / snapshot / config_id
    after_cvpipelining = directory / "after_cvpipelining.generic.mlir"
    pass_tree = directory / "pass_tree"
    suffix_output = directory / "suffix.mlir"
    compiler_log = directory / "compiler.stderr.log"
    model_log = directory / "model.stderr.log"
    report = directory / "one_shot_bufferize_output.tsv"
    complete = directory / "complete.json"
    fingerprint = {
        "input_sha256": sha256_file(case.input_path),
        "compiler_sha256": sha256_file(args.compiler),
        "model_sha256": sha256_file(args.model),
        "config": case.config,
    }
    if not args.no_resume and complete.is_file() and report.is_file():
        try:
            if json.loads(complete.read_text()) == fingerprint:
                fields = report_fields(report)
                if fields.get("semantic_errors") == "0" and \
                        fields.get("allocations_exact") == "1":
                    return result_row(case, adapter, snapshot, "pass", "exact",
                                      directory, fields=fields)
        except (OSError, json.JSONDecodeError):
            pass

    directory.mkdir(parents=True, exist_ok=True)
    pair = one_shot_pair(pass_tree)
    compiler_status = 0
    reuse_oracle = args.reuse_existing_oracle and \
        after_cvpipelining.is_file() and pair is not None
    if not reuse_oracle:
        if pass_tree.exists():
            shutil.rmtree(pass_tree)
        command = [
            str(args.compiler), str(case.input_path), "--mlir-disable-threading",
            "--enable-memory-display=false", "--plan-memory-seed=0",
            "--mlir-print-op-generic", "--mlir-print-ir-module-scope",
            f"--mlir-print-ir-tree-dir={pass_tree}",
            "--mlir-print-ir-before=one-shot-bufferize",
            "--mlir-print-ir-after=one-shot-bufferize",
            f"--dump-after-cvpipelining-generic-ir={after_cvpipelining}",
            *config_argv(case.config), "-o", str(suffix_output),
        ]
        with compiler_log.open("wb") as stderr:
            compiler_status = subprocess.run(
                command, stdout=subprocess.DEVNULL, stderr=stderr).returncode
        pair = one_shot_pair(pass_tree)
    if not after_cvpipelining.is_file() or pair is None:
        return result_row(
            case, adapter, snapshot, "compiler_failed", "oracle_failure",
            directory, log_excerpt(compiler_log) or str(compiler_status))

    before_one_shot, after_one_shot = pair
    report.unlink(missing_ok=True)
    model_command = [
        str(args.model),
        "--action=validate-one-shot-bufferize-semantic-ir",
        f"--root={after_cvpipelining}",
        f"--attempt-root={before_one_shot}",
        f"--after-ir={after_one_shot}",
        f"--output={report}",
        "--enable-code-motion=" + case.config["enable_code_motion"],
    ]
    with model_log.open("wb") as stderr:
        model_status = subprocess.run(
            model_command, stdout=subprocess.DEVNULL, stderr=stderr).returncode
    fields = report_fields(report)
    if not report.is_file():
        return result_row(
            case, adapter, snapshot, "model_failed", "model_failure",
            directory, log_excerpt(model_log) or str(model_status), fields)
    if model_status == 0:
        complete.write_text(canonical_json(fingerprint) + "\n")
        return result_row(case, adapter, snapshot, "pass", "exact",
                          directory, fields=fields)
    if fields.get("operation_counts_exact") == "0":
        category = "one_shot_bufferize_operation_mismatch"
    elif fields.get("allocations_exact") == "0":
        category = "one_shot_bufferize_allocation_mismatch"
    elif int(fields.get("semantic_errors", "0")) != 0:
        category = "one_shot_bufferize_buffer_semantic_mismatch"
    else:
        category = "model_failure"
    return result_row(case, adapter, snapshot, "mismatch", category,
                      directory, log_excerpt(model_log), fields)


def main() -> int:
    args = parse_args()
    args.input_root = args.input_root.resolve()
    args.config_matrix = args.config_matrix.resolve()
    args.compiler = args.compiler.resolve()
    args.model = args.model.resolve()
    args.artifacts = args.artifacts.resolve()
    required = (args.input_root, args.config_matrix, args.compiler, args.model)
    missing = [str(path) for path in required if not path.exists()]
    if missing:
        raise SystemExit("[ERROR] missing path(s): " + ", ".join(missing))
    if args.jobs < 1:
        raise SystemExit("[ERROR] --jobs must be positive")

    inputs = sample_evenly(discover_inputs(args.input_root), args.max_inputs)
    configs = sample_evenly(read_matrix(args.config_matrix), args.max_configs)
    input_configs = [(input_path, config)
                     for input_path in inputs for config in configs]
    cases = [Case(index, input_path, config)
             for index, (input_path, config) in enumerate(input_configs)]
    if args.max_cases:
        cases = cases[:args.max_cases]
    args.artifacts.mkdir(parents=True, exist_ok=True)
    print(f"OneShotBufferize matrix: inputs={len(inputs)} configs={len(configs)} "
          f"cases={len(cases)} jobs={args.jobs}", flush=True)

    results: list[dict[str, str]] = []
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = [executor.submit(run_case, case, args, args.input_root)
                   for case in cases]
        for completed, future in enumerate(as_completed(futures), 1):
            results.append(future.result())
            if completed % 20 == 0 or completed == len(cases):
                failures = sum(row["status"] != "pass" for row in results)
                print(f"progress={completed}/{len(cases)} failures={failures}",
                      flush=True)

    results.sort(key=lambda row: int(row["case_index"]))
    summary = args.artifacts / "summary.tsv"
    fields = list(results[0]) if results else []
    with summary.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(results)
    counts: dict[str, int] = {}
    categories: dict[str, int] = {}
    for row in results:
        counts[row["status"]] = counts.get(row["status"], 0) + 1
        categories[row["category"]] = categories.get(row["category"], 0) + 1
    categories_by_input: dict[tuple[str, str], set[str]] = collections.defaultdict(set)
    for row in results:
        categories_by_input[(row["adapter"], row["snapshot"])].add(
            row["category"])
    input_categories: dict[str, int] = {}
    for values in categories_by_input.values():
        category = next(iter(values)) if len(values) == 1 else "mixed"
        input_categories[category] = input_categories.get(category, 0) + 1
    (args.artifacts / "summary.json").write_text(canonical_json({
        "inputs": len(inputs), "configs": len(configs),
        "cases": len(cases), "status_counts": counts,
        "category_counts": categories,
        "input_category_counts": input_categories,
        "compiler_sha256": sha256_file(args.compiler),
        "model_sha256": sha256_file(args.model),
    }) + "\n")
    print("OneShotBufferize result: " +
          " ".join(f"{key}={value}" for key, value in sorted(counts.items())))
    print("categories: " +
          " ".join(f"{key}={value}" for key, value in sorted(categories.items())))
    print("input categories: " + " ".join(
        f"{key}={value}" for key, value in sorted(input_categories.items())))
    print(summary)
    for failure in (row for row in results if row["status"] != "pass"):
        print(f"FAIL {failure['adapter']} {failure['config_id']}: "
              f"{failure['category']} ({failure['artifacts']})")
        args.max_failures -= 1
        if args.max_failures == 0:
            break
    return 1 if any(row["status"] != "pass" for row in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
