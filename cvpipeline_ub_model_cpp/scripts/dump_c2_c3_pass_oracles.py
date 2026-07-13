#!/usr/bin/env python3
"""Capture native PassManager C2/C3 snapshots without pipeline changes."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


PRINT_PASSES = (
    "one-shot-bufferize",
    "hivm-decompose-op",
    "convert-non-contiguous-reshape-to-copy",
    "hivm-infer-mem-scope",
    "hivm-aggregated-decompose-op",
    "hivm-alloc-extra-buffer",
)


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
    parser.add_argument("--c0-summary", type=Path,
                        default=repo / "Output/index/c0_cvpipeline_after/summary.tsv")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--output-root", type=Path, default=repo / "Output")
    parser.add_argument("--max-objects", type=int, default=0)
    parser.add_argument("--no-resume", action="store_true")
    return parser.parse_args()


def count_files(tree: Path, pass_name: str) -> int:
    return sum(path.name.endswith(f"_{pass_name}.mlir")
               for path in tree.rglob("*.mlir"))


def snapshots_complete(tree: Path) -> bool:
    expected_minimums = {
        "one-shot-bufferize": 2,
        "drop-equivalent-buffer-results": 2,
        "hivm-decompose-op": 4,
        "convert-non-contiguous-reshape-to-copy": 2,
        "hivm-infer-mem-scope": 4,
        "hivm-aggregated-decompose-op": 14,
        "hivm-alloc-extra-buffer": 2,
    }
    return all(count_files(tree, name) >= count
               for name, count in expected_minimums.items())


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

    index_root = output_root / "index/c2_c3_pass_oracles"
    object_root = output_root / "experiments/c2_c3_pass_oracles/objects"
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
            "print_passes": PRINT_PASSES,
        }
        reusable = False
        if not args.no_resume and metadata.is_file() and tree.is_dir():
            try:
                reusable = json.loads(metadata.read_text()) == fingerprint
                reusable = reusable and snapshots_complete(tree)
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
                "--mlir-print-ir-before=one-shot-bufferize",
                "--mlir-print-ir-after=one-shot-bufferize",
                "--mlir-print-ir-after=drop-equivalent-buffer-results",
            ]
            for pass_name in PRINT_PASSES[1:]:
                command.extend((f"--mlir-print-ir-before={pass_name}",
                                f"--mlir-print-ir-after={pass_name}"))
            command.extend(("-o", str(directory / "suffix.mlir")))
            with (directory / "stdout.log").open("wb") as stdout, \
                    (directory / "stderr.log").open("wb") as stderr:
                compiler_status = subprocess.run(
                    command, stdout=stdout, stderr=stderr).returncode
            if snapshots_complete(tree):
                metadata.write_text(json.dumps(fingerprint, sort_keys=True) + "\n")
        complete = tree.is_dir() and snapshots_complete(tree)
        states[input_hash] = {
            "status": "complete" if complete else "incomplete",
            "compiler_status": str(compiler_status),
            "pass_tree": str(tree),
            "pass_tree_sha256": tree_sha256(tree) if complete else "",
            "provenance": str(metadata),
        }

    fields = ["adapter", "snapshot", "config_id", "status",
              "compiler_status", "after_cvpipeline_sha256",
              "after_cvpipeline_ir", "pass_tree", "provenance",
              "pass_tree_sha256", "oracle_tool_sha256"]
    output_rows = []
    for row in rows:
        state = states.get(row["after_cvpipeline_sha256"])
        if state is None:
            continue
        output_rows.append({
            "adapter": row["adapter"], "snapshot": row["snapshot"],
            "config_id": row["config_id"],
            "after_cvpipeline_sha256": row["after_cvpipeline_sha256"],
            "after_cvpipeline_ir": row["after_cvpipeline_ir"],
            "oracle_tool_sha256": tool_hash, **state,
        })
    with (index_root / "summary.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(output_rows)
    manifest = {
        "schema_version": 1,
        "c0_summary": str(c0_summary),
        "c0_summary_sha256": sha256(c0_summary),
        "tuple_count": len(output_rows),
        "unique_object_count": len(selected),
        "complete_object_count": sum(
            state["status"] == "complete" for state in states.values()),
        "oracle_tool_sha256": tool_hash,
    }
    (index_root / "dataset.json").write_text(
        json.dumps(manifest, sort_keys=True) + "\n")
    print(f"tuples={len(output_rows)} unique_objects={len(selected)} "
          f"complete_objects={manifest['complete_object_count']}")
    return 0 if manifest["complete_object_count"] == len(selected) else 1


if __name__ == "__main__":
    raise SystemExit(main())
