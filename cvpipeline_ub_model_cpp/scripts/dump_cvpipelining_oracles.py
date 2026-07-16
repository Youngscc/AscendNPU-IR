#!/usr/bin/env python3
"""Generate exact CVPipelining output oracles with provenance."""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
from pathlib import Path

from cvpipelining_oracle_common import (
    SCHEMA_VERSION,
    canonical_ir,
    canonical_json,
    adapter_and_snapshot,
    config_argv,
    discover_inputs,
    sha256_bytes,
    sha256_file,
)


def parse_args() -> argparse.Namespace:
    script = Path(__file__).resolve()
    module = script.parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-root", type=Path,
                        default=module / "data/before_cvpipelining")
    parser.add_argument("--config-matrix", type=Path,
                        default=module / "testdata/config_matrix/cvpipelining.tsv")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--output-root", type=Path,
                        default=repo / "Output",
                        help="unified root containing adapters/ and index/")
    parser.add_argument("--max-inputs", type=int, default=0)
    parser.add_argument("--max-configs", type=int, default=0)
    parser.add_argument("--no-resume", action="store_true")
    return parser.parse_args()


def read_configs(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    if not rows or "config_id" not in rows[0]:
        raise SystemExit(f"[ERROR] invalid or empty config matrix: {path}")
    return rows


def main() -> int:
    args = parse_args()
    input_root = args.input_root.resolve()
    matrix = args.config_matrix.resolve()
    tool = args.tool.resolve()
    output_root = args.output_root.resolve()
    if not tool.is_file() or not tool.stat().st_mode & 0o111:
        raise SystemExit(f"[ERROR] oracle tool is not executable: {tool}")
    if not input_root.is_dir():
        raise SystemExit(f"[ERROR] input root is missing: {input_root}")
    if not matrix.is_file():
        raise SystemExit(f"[ERROR] config matrix is missing: {matrix}")

    inputs = discover_inputs(input_root)
    configs = read_configs(matrix)
    if args.max_inputs:
        inputs = inputs[:args.max_inputs]
    if args.max_configs:
        configs = configs[:args.max_configs]
    if not inputs or not configs:
        raise SystemExit(
            "[ERROR] CVPipelining requires at least one input and one config")

    adapter_root = output_root / "adapters"
    index_root = output_root / "index/cvpipelining"
    adapter_root.mkdir(parents=True, exist_ok=True)
    index_root.mkdir(parents=True, exist_ok=True)
    tool_hash = sha256_file(tool)
    matrix_hash = sha256_file(matrix)
    summary_fields = [
        "adapter", "snapshot", "config_id", "status", "compiler_status", "input",
        "after_cvpipelining_ir", "boundary_dump", "config_json", "command_json",
        "input_sha256", "after_cvpipelining_sha256", "boundary_dump_sha256",
        "config_sha256", "config_matrix_sha256", "oracle_tool_sha256",
    ]
    summary_rows: list[dict[str, object]] = []
    complete = 0

    for input_path in inputs:
        adapter, snapshot = adapter_and_snapshot(input_path, input_root)
        input_hash = sha256_file(input_path)
        for config in configs:
            config_id = config["config_id"]
            config_text = canonical_json(config)
            config_hash = sha256_bytes(config_text.encode())
            run_dir = (adapter_root / adapter / "cvpipelining/oracle" /
                       snapshot / config_id)
            run_dir.mkdir(parents=True, exist_ok=True)
            after_ir = run_dir / "after_cvpipelining.mlir"
            boundary_dump = run_dir / "boundary_dump.mlir"
            stdout_log = run_dir / "stdout.log"
            stderr_log = run_dir / "stderr.log"
            config_file = run_dir / "config.json"
            command_file = run_dir / "command.json"
            marker = run_dir / "complete.json"
            command = [
                str(tool), str(input_path), "--stop-after-cvpipelining",
                f"--dump-ir-after-cvpipelining={boundary_dump}",
                "--mlir-disable-threading", *config_argv(config),
                "-o", str(after_ir),
            ]
            fingerprint = {
                "schema_version": SCHEMA_VERSION,
                "input_sha256": input_hash,
                "config_sha256": config_hash,
                "config_matrix_sha256": matrix_hash,
                "oracle_tool_sha256": tool_hash,
            }
            reusable = False
            if not args.no_resume and marker.is_file():
                try:
                    reusable = json.loads(marker.read_text()) == fingerprint
                    reusable = reusable and after_ir.is_file() and boundary_dump.is_file()
                    reusable = reusable and canonical_ir(after_ir.read_bytes()) == canonical_ir(boundary_dump.read_bytes())
                except (OSError, json.JSONDecodeError):
                    reusable = False

            compiler_status = 0
            if not reusable:
                config_file.write_text(config_text + "\n")
                command_file.write_text(canonical_json(command) + "\n")
                with stdout_log.open("wb") as stdout, stderr_log.open("wb") as stderr:
                    compiler_status = subprocess.run(command, stdout=stdout, stderr=stderr).returncode
                valid = (
                    compiler_status == 0 and after_ir.is_file() and boundary_dump.is_file()
                    and canonical_ir(after_ir.read_bytes()) == canonical_ir(boundary_dump.read_bytes())
                )
                if valid:
                    marker.write_text(canonical_json(fingerprint) + "\n")
                elif marker.exists():
                    marker.unlink()
            else:
                config_file.write_text(config_text + "\n")
                command_file.write_text(canonical_json(command) + "\n")

            status = "complete" if marker.is_file() else "incomplete"
            if status == "complete":
                complete += 1
            summary_rows.append({
                "adapter": adapter,
                "snapshot": snapshot,
                "config_id": config_id,
                "status": status,
                "compiler_status": compiler_status,
                "input": input_path,
                "after_cvpipelining_ir": after_ir,
                "boundary_dump": boundary_dump,
                "config_json": config_file,
                "command_json": command_file,
                "input_sha256": input_hash,
                "after_cvpipelining_sha256": sha256_file(after_ir) if after_ir.is_file() else "",
                "boundary_dump_sha256": sha256_file(boundary_dump) if boundary_dump.is_file() else "",
                "config_sha256": config_hash,
                "config_matrix_sha256": matrix_hash,
                "oracle_tool_sha256": tool_hash,
            })

    summary = index_root / "summary.tsv"
    with summary.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=summary_fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(summary_rows)
    manifest = {
        "schema_version": SCHEMA_VERSION,
        "input_root": str(input_root),
        "config_matrix": str(matrix),
        "oracle_tool": str(tool),
        "input_count": len(inputs),
        "config_count": len(configs),
        "expected_tuples": len(inputs) * len(configs),
        "complete_tuples": complete,
    }
    (index_root / "dataset.json").write_text(canonical_json(manifest) + "\n")
    print(f"inputs={len(inputs)} configs={len(configs)} expected={len(summary_rows)} complete={complete}")
    print(summary)
    return 0 if complete == len(summary_rows) else 1


if __name__ == "__main__":
    raise SystemExit(main())
