#!/usr/bin/env python3
"""Dump one generic-form before-CVPipelining object per unique input."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--cvpipelining-summary", type=Path,
        default=repo / "Output/index/cvpipelining/summary.tsv")
    parser.add_argument(
        "--tool", type=Path,
        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument(
        "--output-root", type=Path,
        default=repo / "Output/index/before_cvpipelining")
    parser.add_argument("--limit", type=int, default=0)
    args = parser.parse_args()

    rows = list(csv.DictReader(args.cvpipelining_summary.open(), delimiter="\t"))
    unique: dict[str, dict[str, str]] = {}
    for row in rows:
        unique.setdefault(row["input_sha256"], row)
    selected = list(unique.items())
    if args.limit:
        selected = selected[:args.limit]

    object_root = args.output_root / "objects"
    object_root.mkdir(parents=True, exist_ok=True)
    summary_rows: list[dict[str, str]] = []
    failures: list[str] = []
    tool_hash = sha256(args.tool)
    for digest, row in selected:
        output = object_root / digest / "generic.mlir"
        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="cvub-before-cvpipelining-") as temp:
            after = Path(temp) / "after.mlir"
            command = [
                str(args.tool), row["input"], "--disable-cv-pipelining",
                "--stop-after-cvpipelining",
                f"--dump-after-cvpipelining-generic-ir={output}",
                "--mlir-disable-threading", "-o", str(after)]
            process = subprocess.run(command, text=True, capture_output=True)
        status = "complete" if process.returncode == 0 and output.is_file() else "failed"
        if status == "failed":
            failures.append(
                f"{digest}: rc={process.returncode} {process.stderr.strip()}")
        summary_rows.append({
            "input_sha256": digest,
            "input": row["input"],
            "generic_ir": str(output),
            "generic_sha256": sha256(output) if output.is_file() else "",
            "oracle_tool_sha256": tool_hash,
            "status": status,
        })

    args.output_root.mkdir(parents=True, exist_ok=True)
    summary = args.output_root / "summary.tsv"
    fields = list(summary_rows[0]) if summary_rows else [
        "input_sha256", "input", "generic_ir", "generic_sha256",
        "oracle_tool_sha256", "status"]
    with summary.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(summary_rows)
    manifest = {
        "schema_version": 1,
        "unique_inputs": len(selected),
        "complete": sum(row["status"] == "complete" for row in summary_rows),
        "cvpipelining_summary": str(args.cvpipelining_summary.resolve()),
        "cvpipelining_summary_sha256": sha256(args.cvpipelining_summary),
        "oracle_tool": str(args.tool.resolve()),
        "oracle_tool_sha256": tool_hash,
    }
    (args.output_root / "dataset.json").write_text(
        json.dumps(manifest, sort_keys=True, separators=(",", ":")) + "\n")
    if failures:
        print("\n".join(failures[:20]))
        return 1
    print(f"BEFORE_CVPIPELINING_GENERIC=PASS objects={len(summary_rows)}")
    print(summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
