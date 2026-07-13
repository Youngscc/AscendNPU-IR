#!/usr/bin/env python3
"""Capture native InlineLoadCopy before/after snapshots for C5."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


PASS_NAME = "hivm-inline-load-copy"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree_sha256(root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted(root.rglob("*.mlir")):
        digest.update(path.relative_to(root).as_posix().encode())
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def parse_args() -> argparse.Namespace:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--c0-summary", type=Path,
        default=repo / "Output/index/c0_cvpipeline_after/summary.tsv")
    parser.add_argument(
        "--tool", type=Path,
        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--output-root", type=Path, default=repo / "Output")
    parser.add_argument("--max-objects", type=int, default=0)
    parser.add_argument("--no-resume", action="store_true")
    return parser.parse_args()


def snapshot_pairs(tree: Path) -> int:
    before = set()
    after = set()
    for path in tree.rglob(f"*_{PASS_NAME}.mlir"):
        relative = path.relative_to(tree)
        stem = path.name.removesuffix(f"_{PASS_NAME}.mlir")
        key = relative.parent / stem.rsplit("_", 1)[0]
        (before if stem.endswith("_0") else after).add(key)
    return len(before) if before == after else 0


def main() -> int:
    args = parse_args()
    c0_summary = args.c0_summary.resolve()
    tool = args.tool.resolve()
    output_root = args.output_root.resolve()
    with c0_summary.open(newline="") as stream:
        rows = [row for row in csv.DictReader(stream, delimiter="\t")
                if row["status"] == "complete"]
    inputs: dict[str, Path] = {}
    for row in rows:
        inputs.setdefault(row["after_cvpipeline_sha256"],
                          Path(row["after_cvpipeline_ir"]))
    selected = dict(sorted(inputs.items()))
    if args.max_objects:
        selected = dict(list(selected.items())[:args.max_objects])

    index_root = output_root / "index/c5_pass_oracles"
    object_root = output_root / "experiments/c5_pass_oracles/objects"
    index_root.mkdir(parents=True, exist_ok=True)
    object_root.mkdir(parents=True, exist_ok=True)
    tool_hash = sha256(tool)
    states: dict[str, dict[str, str]] = {}
    for input_hash, input_path in selected.items():
        directory = object_root / input_hash
        tree = directory / "pass_tree"
        metadata = directory / "provenance.json"
        fingerprint = {
            "schema_version": 1,
            "input_sha256": input_hash,
            "oracle_tool_sha256": tool_hash,
            "pass": PASS_NAME,
        }
        reusable = False
        if not args.no_resume and metadata.is_file() and tree.is_dir():
            try:
                reusable = json.loads(metadata.read_text()) == fingerprint
                reusable = reusable and snapshot_pairs(tree) > 0
            except (OSError, json.JSONDecodeError):
                reusable = False
        compiler_status = 0
        if not reusable:
            if tree.exists():
                shutil.rmtree(tree)
            directory.mkdir(parents=True, exist_ok=True)
            command = [
                str(tool), str(input_path), "--disable-cv-pipelining",
                "--enable-memory-display=false", "--mlir-disable-threading",
                "--mlir-print-op-generic", "--mlir-print-ir-module-scope",
                f"--mlir-print-ir-tree-dir={tree}",
                f"--mlir-print-ir-before={PASS_NAME}",
                f"--mlir-print-ir-after={PASS_NAME}",
                "-o", str(directory / "suffix.mlir"),
            ]
            with (directory / "stdout.log").open("wb") as stdout, \
                    (directory / "stderr.log").open("wb") as stderr:
                compiler_status = subprocess.run(
                    command, stdout=stdout, stderr=stderr).returncode
            if snapshot_pairs(tree) > 0:
                metadata.write_text(json.dumps(fingerprint, sort_keys=True) +
                                    "\n")
        pairs = snapshot_pairs(tree) if tree.is_dir() else 0
        states[input_hash] = {
            "status": "complete" if pairs else "incomplete",
            "compiler_status": str(compiler_status),
            "snapshot_pairs": str(pairs),
            "pass_tree": str(tree),
            "pass_tree_sha256": tree_sha256(tree) if pairs else "",
            "provenance": str(metadata),
        }

    fields = ["after_cvpipeline_sha256", "after_cvpipeline_ir", "status",
              "compiler_status", "snapshot_pairs", "pass_tree",
              "pass_tree_sha256", "provenance", "oracle_tool_sha256"]
    with (index_root / "summary.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        for input_hash, input_path in selected.items():
            writer.writerow({"after_cvpipeline_sha256": input_hash,
                             "after_cvpipeline_ir": input_path,
                             "oracle_tool_sha256": tool_hash,
                             **states[input_hash]})
    complete = sum(state["status"] == "complete" for state in states.values())
    print(f"objects={len(selected)} complete={complete} "
          f"function_pairs={sum(int(s['snapshot_pairs']) for s in states.values())}")
    return 0 if complete == len(selected) else 1


if __name__ == "__main__":
    raise SystemExit(main())
