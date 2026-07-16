#!/usr/bin/env python3
"""Run generated suffix configs and produce fixed-seed PlanMemory oracles."""

from __future__ import annotations

import argparse
import csv
import hashlib
import os
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def parse_seeds(value: str) -> set[int]:
    if value == "0-19":
        return set(range(20))
    return {int(part) for part in value.split(",")}


def parse_modes(value: str) -> set[int]:
    return {int(part) for part in value.split(",")}


def flag(args: list[str], enabled: bool, name: str) -> None:
    if enabled:
        args.append(name)


def suffix_args(row: dict[str, str]) -> list[str]:
    args: list[str] = []
    flag(args, row["disable_cv_pipelining"] == "1", "--disable-cv-pipelining")
    flag(args, row["enable_preload"] == "1", "--enable-preload")
    args.extend(("--cv-pipeline-depth", row["cv_pipeline_depth"]))
    flag(args, row["enable_cv_lazy_loading"] == "1",
         "--enable-cv-lazy-loading")
    flag(args, row["enable_auto_multi_buffer"] == "1",
         "--enable-auto-multi-buffer")
    args.extend(("--limit-auto-multi-buffer-of-local-buffer",
                 row["local_multi_buffer_strategy"]))
    args.extend(("--limit-auto-multi-buffer-buffer",
                 row["mix_multi_buffer_strategy"]))
    args.append("--enable-code-motion=" +
                ("true" if row["enable_code_motion"] == "1" else "false"))
    flag(args, row["enable_ubuf_saving"] == "1", "--enable-ubuf-saving")
    args.extend(("--tile-mix-cube-loop", row["tile_mix_cube_loop"]))
    args.extend(("--tile-mix-vector-loop", row["tile_mix_vector_loop"]))
    flag(args, row["disable_align_alloc_size"] == "1",
         "--disable-align-alloc-size")
    args.append("--enable-hivm-auto-storage-align=" +
                ("true" if row["enable_hivm_auto_storage_align"] == "1"
                 else "false"))
    flag(args, row["disable_enable_stride_align"] == "1",
         "--disable-enable-stride-align")
    flag(args, row["disable_infer_hivm_data_layout"] == "1",
         "--disable-infer-hivm-data-layout")
    return args


def dump_complete(path: Path) -> bool:
    if not path.is_file():
        return False
    counts: dict[str, int] = {}
    with path.open(encoding="utf-8", errors="replace") as source:
        for line in source:
            tag = line.split("\t", 1)[0]
            counts[tag] = counts.get(tag, 0) + 1
    equal_pairs = (
        ("PLANMEM_LIVENESS_ATTEMPT", "PLANMEM_LIVENESS_ATTEMPT_END"),
        ("PLANMEM_PLAN_ATTEMPT", "PLANMEM_PLAN_ATTEMPT_END"),
        ("PLANMEM_BUFFER", "PLANMEM_EXACT_BUFFER"),
        ("PLANMEM_GEN", "PLANMEM_EXACT_GEN"),
        ("PLANMEM_KILL", "PLANMEM_EXACT_KILL"),
        ("PLANMEM_INPLACE", "PLANMEM_EXACT_INPLACE"),
        ("PLANMEM_MULTI", "PLANMEM_EXACT_MULTI"),
        ("PLANMEM_PLANNED_BUFFER", "PLANMEM_EXACT_PLANNED_BUFFER"),
    )
    return (
        counts.get("PLANMEM_RUN_CONFIG", 0) == 1
        and counts.get("PLANMEM_RUN_RESULT", 0) == 1
        and counts.get("PLANMEM_LIVENESS_ATTEMPT", 0) > 0
        and counts.get("PLANMEM_LIVENESS_ATTEMPT", 0)
        == counts.get("PLANMEM_PLAN_ATTEMPT", 0)
        and all(counts.get(left, 0) == counts.get(right, 0)
                for left, right in equal_pairs)
    )


def count_tag(path: Path, wanted: str) -> int:
    if not path.is_file():
        return 0
    with path.open(encoding="utf-8", errors="replace") as source:
        return sum(line.startswith(wanted + "\t") for line in source)


def run(command: list[str], cwd: Path, stdout: Path, stderr: Path,
        environment: dict[str, str]) -> int:
    with stdout.open("wb") as out, stderr.open("wb") as err:
        result = subprocess.run(command, cwd=cwd, stdout=out, stderr=err,
                                env=environment, check=False)
    return result.returncode


def discover_inputs(root: Path) -> list[Path]:
    return sorted(
        path.resolve()
        for path in root.rglob("before_cvpipelining_*.mlir")
        if ".raw." not in path.name
        and ".before_local_plan_memory." not in path.name
        and ".after_local_plan_memory." not in path.name
    )


def group_configs(rows: list[dict[str, str]]) -> list[dict[str, object]]:
    grouped: dict[str, dict[str, object]] = {}
    for row in rows:
        digest = row["config_digest"]
        if digest not in grouped:
            grouped[digest] = {
                "row": row,
                "config_ids": [],
                "seeds": set(),
                "restrict_modes": set(),
            }
        grouped[digest]["config_ids"].append(row["config_id"])
        grouped[digest]["seeds"].update(parse_seeds(row["plan_memory_seeds"]))
        grouped[digest]["restrict_modes"].update(
            parse_modes(row["restrict_modes"])
        )
    return list(grouped.values())


def main() -> int:
    module = Path(__file__).resolve().parents[1]
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--matrix", type=Path, required=True)
    parser.add_argument("--input-root", type=Path,
                        default=repo / "Output/views/cvpipeline_compiler_default")
    parser.add_argument("--output-root", type=Path,
                        default=module / "output/config_matrix")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--max-configs", type=int, default=0)
    parser.add_argument("--max-cases", type=int, default=0)
    parser.add_argument("--transform-only", action="store_true")
    parser.add_argument("--no-resume", action="store_true")
    args = parser.parse_args()

    tool = args.tool.resolve()
    if not tool.is_file() or not os.access(tool, os.X_OK):
        parser.error(f"tool is not executable: {tool}")
    rows = read_rows(args.matrix)
    configs = group_configs(rows)
    inputs = discover_inputs(args.input_root)
    if args.max_configs:
        configs = configs[: args.max_configs]
    if args.max_cases:
        inputs = inputs[: args.max_cases]
    if not configs or not inputs:
        parser.error("matrix and input set must both be non-empty")

    output_root = args.output_root.resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    tool_hash = sha256(tool)
    transform_rows: list[list[object]] = []
    oracle_rows: list[list[object]] = []
    hashes_by_case: dict[str, dict[str, str]] = {}

    for input_path in inputs:
        case = input_path.parent.name + "__" + input_path.stem
        hashes_by_case.setdefault(case, {})
        input_hash = sha256(input_path)
        for config in configs:
            row = config["row"]
            config_id = str(config["config_ids"][0])
            run_dir = output_root / "transforms" / config_id / case
            run_dir.mkdir(parents=True, exist_ok=True)
            before_plan = run_dir / "before_local_plan_memory.mlir"
            after_plan = run_dir / "after_local_plan_memory.mlir"
            transform_stdout = run_dir / "stdout.log"
            transform_stderr = run_dir / "stderr.log"
            transform_marker = run_dir / "complete.marker"
            transform_status_file = run_dir / "compiler_status.txt"
            transform_fingerprint = (
                f"input={input_hash} tool={tool_hash} "
                f"config={row['config_digest']}"
            )
            resume_transform = (
                not args.no_resume and before_plan.is_file()
                and transform_marker.is_file()
                and transform_marker.read_text(encoding="utf-8").strip()
                == transform_fingerprint
                and transform_status_file.is_file()
            )
            if resume_transform:
                status = int(transform_status_file.read_text().strip())
            else:
                command = [
                    str(tool), str(input_path), "--mlir-disable-threading",
                    "--plan-memory-seed", "0", "-o", str(after_plan),
                ]
                command.extend(suffix_args(row))
                environment = os.environ.copy()
                environment["BISHENGIR_DUMP_BEFORE_PLAN_MEMORY"] = str(before_plan)
                status = run(command, run_dir, transform_stdout,
                             transform_stderr, environment)
                transform_status_file.write_text(f"{status}\n", encoding="utf-8")
                if before_plan.is_file() and before_plan.stat().st_size > 0:
                    transform_marker.write_text(transform_fingerprint + "\n",
                                                encoding="utf-8")
                else:
                    transform_marker.unlink(missing_ok=True)
            if not before_plan.is_file() or before_plan.stat().st_size == 0:
                transform_rows.append([
                    case, config_id, "failed", status, "", "", input_path,
                    before_plan, input_hash, tool_hash,
                ])
                continue
            ir_hash = sha256(before_plan)
            duplicate_of = hashes_by_case[case].get(ir_hash, "")
            if not duplicate_of:
                hashes_by_case[case][ir_hash] = config_id
            transform_rows.append([
                case, config_id, "duplicate" if duplicate_of else "unique",
                status, ir_hash, duplicate_of, input_path, before_plan,
                input_hash, tool_hash,
            ])
            if duplicate_of or args.transform_only:
                continue

            for restrict in sorted(config["restrict_modes"]):
                for seed in sorted(config["seeds"]):
                    oracle_dir = (
                        output_root / "oracles" / config_id / case
                        / f"restrict_{restrict}" / f"seed_{seed:02d}"
                    )
                    oracle_dir.mkdir(parents=True, exist_ok=True)
                    dump = oracle_dir / "oracle.tsv"
                    output_ir = oracle_dir / "after_local_plan_memory.mlir"
                    stdout = oracle_dir / "stdout.log"
                    marker = oracle_dir / "complete.marker"
                    status_file = oracle_dir / "compiler_status.txt"
                    fingerprint = (
                        f"input={ir_hash} tool={tool_hash} seed={seed} "
                        f"restrict={restrict}"
                    )
                    resume_oracle = (
                        not args.no_resume and marker.is_file()
                        and marker.read_text(encoding="utf-8").strip()
                        == fingerprint and status_file.is_file()
                        and dump_complete(dump)
                    )
                    if resume_oracle:
                        compiler_status = int(status_file.read_text().strip())
                    else:
                        command = [
                            str(tool), str(before_plan),
                            "--local-plan-memory-only", "--enable-memory-display",
                            "--mlir-disable-threading", "--plan-memory-seed",
                            str(seed), "-o", str(output_ir),
                        ]
                        if restrict:
                            command.append("--restrict-inplace-as-isa")
                        environment = os.environ.copy()
                        environment["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
                        compiler_status = run(command, oracle_dir, stdout, dump,
                                              environment)
                        status_file.write_text(f"{compiler_status}\n",
                                               encoding="utf-8")
                    complete = dump_complete(dump)
                    if complete:
                        marker.write_text(fingerprint + "\n", encoding="utf-8")
                    else:
                        marker.unlink(missing_ok=True)
                    oracle_rows.append([
                        case, seed, config_id, restrict,
                        "complete" if complete else "incomplete",
                        compiler_status,
                        count_tag(dump, "PLANMEM_LIVENESS_ATTEMPT"),
                        count_tag(dump, "PLANMEM_PLAN_ATTEMPT"),
                        before_plan, dump, ir_hash,
                        tool_hash,
                    ])

    with (output_root / "transform_summary.tsv").open(
        "w", newline="", encoding="utf-8"
    ) as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow((
            "case", "config_id", "status", "compiler_status", "ir_sha256",
            "duplicate_of", "input", "before_plan_memory_ir", "input_sha256",
            "oracle_tool_sha256",
        ))
        writer.writerows(transform_rows)
    with (output_root / "oracle_summary.tsv").open(
        "w", newline="", encoding="utf-8"
    ) as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow((
            "case", "seed", "config_id", "restrict_inplace_as_isa", "status",
            "compiler_status", "liveness_attempts", "plan_attempts", "input",
            "dump", "input_sha256", "oracle_tool_sha256",
        ))
        writer.writerows(oracle_rows)

    unique = sum(row[2] == "unique" for row in transform_rows)
    duplicates = sum(row[2] == "duplicate" for row in transform_rows)
    failures = sum(row[2] == "failed" for row in transform_rows)
    incomplete = sum(row[4] != "complete" for row in oracle_rows)
    print(f"configs={len(configs)} cases={len(inputs)}")
    print(f"unique_ir={unique} duplicate_ir={duplicates} transform_failures={failures}")
    print(f"oracle_runs={len(oracle_rows)} oracle_incomplete={incomplete}")
    print(output_root / "transform_summary.tsv")
    print(output_root / "oracle_summary.tsv")
    return 0 if failures == 0 and incomplete == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
