#!/usr/bin/env python3
"""Compare the model bridge output with PlanMemory's direct structured input."""

from __future__ import annotations

import argparse
import csv
import os
import subprocess
import tempfile
from pathlib import Path


def final_ir(pass_tree: str) -> Path:
    files = sorted(Path(pass_tree).rglob("*_1_hivm-mark-multi-buffer.mlir"))
    if not files:
        raise RuntimeError(f"missing final MarkMultiBuffer dump: {pass_tree}")
    return max(files, key=lambda path: path.read_bytes().count(b"hivm.multi_buffer"))


def first_difference(model: list[str], oracle: list[str]) -> str:
    for index, pair in enumerate(zip(model, oracle)):
        if pair[0] != pair[1]:
            return f"line={index + 1} model={pair[0]} oracle={pair[1]}"
    index = min(len(model), len(oracle))
    model_line = model[index] if index < len(model) else "<eof>"
    oracle_line = oracle[index] if index < len(oracle) else "<eof>"
    return f"line={index + 1} model={model_line} oracle={oracle_line}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--artifacts", type=Path)
    parser.add_argument("--only-hash", action="append", default=[])
    parser.add_argument("--one-config-per-ir", action="store_true")
    parser.add_argument("--max-failures", type=int, default=100)
    args = parser.parse_args()
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    compiler = repo / "build/bin/bishengir-cvpipeline-suffix-compile"
    rows = list(csv.DictReader(
        (repo / "Output/index/c6_pass_oracles/summary.tsv").open(),
        delimiter="\t"))
    if args.only_hash:
        selected = set(args.only_hash)
        rows = [row for row in rows
                if row["after_cvpipeline_sha256"] in selected]
    if args.one_config_per_ir:
        unique_rows = {}
        for row in rows:
            unique_rows.setdefault(row["after_cvpipeline_sha256"], row)
        rows = list(unique_rows.values())
    if args.limit:
        rows = rows[:args.limit]

    failures: list[str] = []
    checked = 0
    temporary = None
    if args.artifacts:
        temp = args.artifacts.resolve()
        temp.mkdir(parents=True, exist_ok=True)
    else:
        temporary = tempfile.TemporaryDirectory(prefix="cvub-planmemory-input-")
        temp = Path(temporary.name)
    try:
        for index, row in enumerate(rows):
            label = f"{row['after_cvpipeline_sha256']}/{row['config_id']}"
            source = (repo / "Output/experiments/c2_c3_pass_oracles/objects" /
                      row["after_cvpipeline_sha256"] / "pass_tree" /
                      "builtin_module_no-symbol-name/0_one-shot-bufferize.mlir")
            before = temp / f"{index}.before.mlir"
            after = temp / f"{index}.after.mlir"
            env = os.environ.copy()
            env["BISHENGIR_DUMP_BEFORE_PLAN_MEMORY"] = str(before)
            compile_result = subprocess.run(
                [str(compiler), str(final_ir(row["pass_tree"])),
                 "--local-plan-memory-only", "--plan-memory-seed=0",
                 "--mlir-disable-threading", "--enable-memory-display=false",
                 "-o", str(after)], text=True, capture_output=True, env=env)
            if not before.exists():
                diagnostic = (compile_result.stderr.strip() or
                              compile_result.stdout.strip())
                failures.append(
                    f"{label}: direct dump failed rc={compile_result.returncode} "
                    f"{diagnostic}")
                continue

            oracle = temp / f"{index}.oracle.tsv"
            model = temp / f"{index}.model.tsv"
            oracle_result = subprocess.run(
                [str(tool), "--action=dump-planmemory-canonical-input-oracle",
                 "--root", str(before), "--output", str(oracle)],
                text=True, capture_output=True)
            config = [
                "--limit-auto-multi-buffer-of-local-buffer",
                row["local_strategy"],
                "--limit-auto-multi-buffer-buffer", row["mix_strategy"]]
            if row["enable_auto"] == "1":
                config.append("--enable-auto-multi-buffer")
            model_result = subprocess.run(
                [str(tool), "--action=model-planmemory-canonical-input",
                 "--root", str(source), "--output", str(model), *config],
                text=True, capture_output=True)
            if oracle_result.returncode != 0 or model_result.returncode != 0:
                failures.append(
                    f"{label}: oracle={oracle_result.returncode} "
                    f"model={model_result.returncode} "
                    f"{oracle_result.stderr.strip()} {model_result.stderr.strip()}")
                continue
            model_lines = model.read_text().splitlines()
            oracle_lines = oracle.read_text().splitlines()
            if model_lines != oracle_lines:
                failures.append(
                    f"{label}: {first_difference(model_lines, oracle_lines)}")
                continue
            checked += 1
    finally:
        if temporary is not None:
            temporary.cleanup()

    if failures:
        print(f"PLANMEMORY_BRIDGE_INPUT=FAIL checked={checked} "
              f"failures={len(failures)}")
        displayed = failures if args.max_failures == 0 else failures[:args.max_failures]
        print("\n".join(displayed))
        return 1
    print(f"PLANMEMORY_BRIDGE_INPUT=PASS tuples={checked}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
