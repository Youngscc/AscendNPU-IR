#!/usr/bin/env python3
"""Validate C0 oracle completeness, provenance, and reproducibility."""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
import tempfile
from pathlib import Path

from cvpipeline_oracle_common import (
    canonical_ir,
    config_argv,
    sha256_bytes,
    sha256_file,
)


REQUIRED = {
    "adapter", "snapshot", "config_id", "status", "compiler_status", "input",
    "after_cvpipeline_ir", "boundary_dump", "config_json", "command_json",
    "input_sha256", "after_cvpipeline_sha256", "boundary_dump_sha256",
    "config_sha256", "config_matrix_sha256", "oracle_tool_sha256",
}


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--oracle-root", type=Path,
                        default=repo / "Output/index/c0_cvpipeline_after")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--rerun", type=int, default=0,
                        help="rerun the first N tuples; 0 disables, -1 reruns all")
    parser.add_argument("--full-suffix-cross-check", type=int, default=0,
                        help="compare stop boundary with N complete suffix runs")
    return parser.parse_args()


def load_command(path: Path) -> list[str]:
    value = json.loads(path.read_text())
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise ValueError("command JSON is not a string array")
    return value


def replace_output_paths(command: list[str], after: Path, boundary: Path,
                         stop: bool) -> list[str]:
    result: list[str] = []
    index = 0
    while index < len(command):
        item = command[index]
        if item == "--stop-after-cvpipelining":
            if stop:
                result.append(item)
        elif item.startswith("--dump-ir-after-cvpipelining="):
            result.append(f"--dump-ir-after-cvpipelining={boundary}")
        elif item == "-o":
            result.extend([item, str(after)])
            index += 1
        else:
            result.append(item)
        index += 1
    return result


def main() -> int:
    args = parse_args()
    root = args.oracle_root.resolve()
    summary = root / "summary.tsv"
    dataset_file = root / "dataset.json"
    tool = args.tool.resolve()
    if not summary.is_file() or not dataset_file.is_file() or not tool.is_file():
        raise SystemExit("[ERROR] missing C0 summary, dataset manifest, or oracle tool")
    with summary.open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if not REQUIRED.issubset(reader.fieldnames or []):
            raise SystemExit("[ERROR] C0 summary schema is incomplete")
        rows = list(reader)
    dataset = json.loads(dataset_file.read_text())
    failures: list[str] = []
    if len(rows) != dataset.get("expected_tuples"):
        failures.append("summary tuple count differs from dataset manifest")
    if dataset.get("expected_tuples") != dataset.get("input_count", 0) * dataset.get("config_count", 0):
        failures.append("dataset tuple cardinality is inconsistent")
    if dataset.get("complete_tuples") != dataset.get("expected_tuples"):
        failures.append("dataset is not marked complete")
    tool_hash = sha256_file(tool)
    matrix = Path(dataset.get("config_matrix", ""))
    matrix_hash = sha256_file(matrix) if matrix.is_file() else ""
    tuples: set[tuple[str, str, str]] = set()

    for row in rows:
        key = (row["adapter"], row["snapshot"], row["config_id"])
        if key in tuples:
            failures.append(f"duplicate tuple: {key}")
        tuples.add(key)
        paths = {name: Path(row[name]) for name in (
            "input", "after_cvpipeline_ir", "boundary_dump", "config_json", "command_json")}
        if row["status"] != "complete" or row["compiler_status"] != "0":
            failures.append(f"incomplete tuple: {key}")
            continue
        if any(not path.is_file() for path in paths.values()):
            failures.append(f"missing artifact: {key}")
            continue
        expected_hashes = {
            "input": row["input_sha256"],
            "after_cvpipeline_ir": row["after_cvpipeline_sha256"],
            "boundary_dump": row["boundary_dump_sha256"],
        }
        for name, expected in expected_hashes.items():
            if sha256_file(paths[name]) != expected:
                failures.append(f"stale {name}: {key}")
        config_text = paths["config_json"].read_text().strip()
        if sha256_bytes(config_text.encode()) != row["config_sha256"]:
            failures.append(f"stale config: {key}")
        if row["config_matrix_sha256"] != matrix_hash:
            failures.append(f"stale config matrix: {key}")
        if row["oracle_tool_sha256"] != tool_hash:
            failures.append(f"stale oracle tool: {key}")
        try:
            config = json.loads(config_text)
            command = load_command(paths["command_json"])
            if config.get("config_id") != row["config_id"]:
                failures.append(f"config id mismatch: {key}")
            if command[0] != str(tool) or command[1] != str(paths["input"]):
                failures.append(f"command input/tool mismatch: {key}")
            for option in config_argv(config):
                if option not in command:
                    failures.append(f"command does not bind {option}: {key}")
        except (ValueError, json.JSONDecodeError, IndexError) as error:
            failures.append(f"invalid config/command metadata for {key}: {error}")
        if canonical_ir(paths["after_cvpipeline_ir"].read_bytes()) != canonical_ir(paths["boundary_dump"].read_bytes()):
            failures.append(f"stop output differs from boundary dump: {key}")

    selected = rows if args.rerun < 0 else rows[:args.rerun]
    for row in selected:
        key = (row["adapter"], row["snapshot"], row["config_id"])
        with tempfile.TemporaryDirectory(prefix="cvpipeline-c0-rerun-") as tmp:
            tmp_path = Path(tmp)
            after = tmp_path / "after.mlir"
            boundary = tmp_path / "boundary.mlir"
            command = replace_output_paths(load_command(Path(row["command_json"])), after, boundary, True)
            result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            if result.returncode != 0 or not boundary.is_file():
                failures.append(f"rerun failed: {key}")
            elif boundary.read_bytes() != Path(row["boundary_dump"]).read_bytes():
                failures.append(f"non-deterministic boundary dump: {key}")

    for row in rows[:args.full_suffix_cross_check]:
        key = (row["adapter"], row["snapshot"], row["config_id"])
        with tempfile.TemporaryDirectory(prefix="cvpipeline-c0-full-") as tmp:
            tmp_path = Path(tmp)
            after = tmp_path / "suffix.mlir"
            boundary = tmp_path / "boundary.mlir"
            command = replace_output_paths(load_command(Path(row["command_json"])), after, boundary, False)
            subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if not boundary.is_file():
                failures.append(f"full suffix did not reach CVPipelining boundary: {key}")
            elif boundary.read_bytes() != Path(row["boundary_dump"]).read_bytes():
                failures.append(f"stop/full suffix boundary mismatch: {key}")

    if failures:
        for failure in failures[:50]:
            print(f"[ERROR] {failure}")
        print(f"C0_CVPIPELINE_AFTER_ORACLE=FAIL failures={len(failures)}")
        return 1
    print(f"C0_CVPIPELINE_AFTER_ORACLE=PASS tuples={len(rows)} rerun={len(selected)} full_cross_check={args.full_suffix_cross_check}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
