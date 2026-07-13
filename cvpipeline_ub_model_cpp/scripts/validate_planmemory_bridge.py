#!/usr/bin/env python3
"""Validate the structured suffix-to-PlanMemory bridge against real PlanMemory."""

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
        raise RuntimeError(f"missing final mark-multi-buffer dump: {pass_tree}")
    return max(files, key=lambda path: path.read_bytes().count(b"hivm.multi_buffer"))


def parse_oracle(text: str) -> tuple[list[str], list[str]]:
    functions = [
        fields[1]
        for fields in (line.split("\t") for line in text.splitlines())
        if fields[0] == "PLANMEM_LIVENESS_ATTEMPT" and len(fields) >= 2
    ]
    if not functions:
        raise RuntimeError("PlanMemory oracle contains no liveness attempt")
    target_function = next(
        (name for name in functions if name.endswith("_mix_aiv")), functions[0])
    lifetime: list[str] = []
    plan: list[str] = []
    gen_kill: dict[int, tuple[list[str], list[str]]] = {}
    names: dict[int, str] = {}
    current_name = ""
    current_lifetime = False
    current_plan = False
    for line in text.splitlines():
        fields = line.split("\t")
        if fields[0] == "PLANMEM_LIVENESS_ATTEMPT" and len(fields) >= 2:
            current_lifetime = fields[1] == target_function
            continue
        if fields[0] == "PLANMEM_LIVENESS_ATTEMPT_END":
            current_lifetime = False
            continue
        if fields[0] == "PLANMEM_PLAN_ATTEMPT" and len(fields) >= 4:
            current_plan = fields[1] == target_function
            if current_plan:
                plan.append("STATUS\t" + fields[3])
            continue
        if fields[0] == "PLANMEM_PLAN_ATTEMPT_END":
            current_plan = False
            continue
        if fields[0].startswith("PLANMEM_EXACT_") or fields[0] in {
                "PLANMEM_BUFFER", "PLANMEM_GEN", "PLANMEM_KILL",
                "PLANMEM_INPLACE", "PLANMEM_MULTI"}:
            if not current_lifetime and fields[0] != "PLANMEM_EXACT_PLANNED_BUFFER":
                continue
        if fields[0] == "PLANMEM_EXACT_PLANNED_BUFFER" and not current_plan:
            continue
        if fields[0] in {"PLANMEM_PEAK", "PLANMEM_REQUIRED"} and not current_plan:
            continue
        if fields[0] == "PLANMEM_BUFFER" and len(fields) >= 3:
            current_name = fields[2]
        elif fields[0] == "PLANMEM_EXACT_BUFFER" and len(fields) >= 8:
            buffer_id = int(fields[2])
            names[buffer_id] = current_name
            lifetime.append("\t".join((
                "BUFFER", fields[2], fields[3],
                fields[5], fields[6], fields[7])))
        elif fields[0] == "PLANMEM_EXACT_GEN" and len(fields) >= 4:
            gen_kill.setdefault(int(fields[2]), ([], []))[0].append(fields[3])
        elif fields[0] == "PLANMEM_EXACT_KILL" and len(fields) >= 4:
            gen_kill.setdefault(int(fields[2]), ([], []))[1].append(fields[3])
        elif fields[0] == "PLANMEM_EXACT_INPLACE" and len(fields) >= 4:
            lifetime.append("\t".join(("INPLACE", fields[2], fields[3])))
        elif fields[0] == "PLANMEM_EXACT_MULTI" and len(fields) >= 4:
            lifetime.append("\t".join(("MULTI", fields[2], fields[3])))
        elif fields[0] == "PLANMEM_EXACT_PLANNED_BUFFER" and len(fields) >= 6:
            buffer_id = int(fields[3])
            plan.append("\t".join((
                "BUFFER", fields[3], fields[4], *fields[5:])))
        elif fields[0] == "PLANMEM_PEAK" and len(fields) >= 4 and fields[2] == "6":
            plan.append("PEAK_BITS\t" + fields[3])
        elif fields[0] == "PLANMEM_REQUIRED" and len(fields) >= 4 and fields[2] == "6":
            plan.append("REQUIRED_BITS\t" + fields[3])
    lifetime.extend(
        "\t".join(("GEN_KILL", str(index),
                    *(f"g:{value}" for value in values[0]),
                    *(f"k:{value}" for value in values[1])))
        for index, values in sorted(gen_kill.items()))
    lifetime.sort(key=lambda line: (line.split("\t", 1)[0], line))
    plan.sort(key=lambda line: (line.split("\t", 1)[0], line))
    return lifetime, plan


def parse_model(path: Path) -> list[str]:
    raw_lines = path.read_text().splitlines()[1:]
    success = "STATUS\tsuccess" in raw_lines
    lines = []
    for line in raw_lines:
        fields = line.split("\t")
        if fields[0] == "REQUIRED_BITS" and success:
            continue
        if fields[0] == "BUFFER" and len(fields) >= 4:
            line = "\t".join(fields[:2] + fields[3:])
        lines.append(line)
    return sorted(lines, key=lambda line: (line.split("\t", 1)[0], line))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--seeds", default="0")
    args = parser.parse_args()
    seeds = [int(value) for value in args.seeds.split(",")]
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    compiler = repo / "build/bin/bishengir-cvpipeline-suffix-compile"
    rows = list(csv.DictReader(
        (repo / "Output/index/c6_pass_oracles/summary.tsv").open(),
        delimiter="\t"))
    if args.limit:
        rows = rows[:args.limit]
    failures: list[str] = []
    checked = 0
    with tempfile.TemporaryDirectory(prefix="cvub-planmemory-bridge-") as temp_dir:
        temp = Path(temp_dir)
        for row_index, row in enumerate(rows):
            source = (repo / "Output/experiments/c2_c3_pass_oracles/objects" /
                      row["after_cvpipeline_sha256"] / "pass_tree" /
                      "builtin_module_no-symbol-name/0_one-shot-bufferize.mlir")
            after = final_ir(row["pass_tree"])
            config = [
                "--limit-auto-multi-buffer-of-local-buffer", row["local_strategy"],
                "--limit-auto-multi-buffer-buffer", row["mix_strategy"]]
            if row["enable_auto"] == "1":
                config.append("--enable-auto-multi-buffer")
            for seed in seeds:
                label = f"{row['after_cvpipeline_sha256']}/{row['config_id']}/{seed}"
                env = os.environ.copy()
                env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
                oracle = subprocess.run(
                    [str(compiler), str(after), "--local-plan-memory-only",
                     f"--plan-memory-seed={seed}", "--mlir-disable-threading",
                     "--enable-memory-display=false", "-o", str(temp / "after.mlir")],
                    text=True, capture_output=True, env=env)
                if oracle.returncode != 0:
                    failures.append(f"{label}: compiler={oracle.returncode}")
                    continue
                oracle_lifetime, oracle_plan = parse_oracle(
                    oracle.stdout + oracle.stderr)
                outputs = []
                for kind in ("lifetime", "plan"):
                    path = temp / f"{row_index}-{seed}-{kind}.tsv"
                    command = [
                        str(tool),
                        f"--action=model-planmemory-structured-{kind}",
                        "--root", str(source), "--output", str(path),
                        "--random-seed", str(seed), *config]
                    model = subprocess.run(command, text=True, capture_output=True)
                    if model.returncode != 0:
                        failures.append(
                            f"{label}: model-{kind}=" +
                            (model.stderr.strip() or model.stdout.strip()))
                        outputs = []
                        break
                    outputs.append(parse_model(path))
                if not outputs:
                    continue
                expected = (oracle_lifetime, oracle_plan)
                mismatch = next((kind for kind in range(2)
                                 if outputs[kind] != expected[kind]), None)
                if mismatch is not None:
                    model_set = outputs[mismatch]
                    oracle_set = expected[mismatch]
                    different = next((index for index, pair in enumerate(
                        zip(model_set, oracle_set)) if pair[0] != pair[1]),
                        min(len(model_set), len(oracle_set)))
                    failures.append(
                        f"{label}: {'lifetime' if mismatch == 0 else 'plan'} "
                        f"model={model_set[different] if different < len(model_set) else '<eof>'} "
                        f"oracle={oracle_set[different] if different < len(oracle_set) else '<eof>'}")
                    continue
                checked += 1
    if failures:
        print(f"PLANMEMORY_BRIDGE=FAIL checked={checked} failures={len(failures)}")
        print("\n".join(failures[:100]))
        return 1
    print(f"PLANMEMORY_BRIDGE=PASS tuples={checked}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
