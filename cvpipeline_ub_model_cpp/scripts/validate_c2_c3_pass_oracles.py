#!/usr/bin/env python3
"""Validate C2/C3 native PassManager snapshots and provenance."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

from dump_c2_c3_pass_oracles import snapshots_complete, tree_sha256


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path,
                        default=repo / "Output/index/c2_c3_pass_oracles")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--rerun", type=int, default=0,
                        help="rerun first N unique objects; -1 reruns all")
    args = parser.parse_args()
    root = args.root.resolve()
    with (root / "summary.tsv").open(newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    dataset = json.loads((root / "dataset.json").read_text())
    tool_hash = sha256(args.tool.resolve())
    failures: list[str] = []
    c0_summary = Path(dataset.get("c0_summary", ""))
    if (not c0_summary.is_file() or
            sha256(c0_summary) != dataset.get("c0_summary_sha256")):
        failures.append("stale C0 summary provenance")
    if len(rows) != dataset.get("tuple_count"):
        failures.append("tuple cardinality mismatch")
    tuples: set[tuple[str, str, str]] = set()
    objects: dict[str, dict[str, str]] = {}
    for row in rows:
        key = (row["adapter"], row["snapshot"], row["config_id"])
        if key in tuples:
            failures.append(f"duplicate tuple {key}")
        tuples.add(key)
        input_path = Path(row["after_cvpipeline_ir"])
        tree = Path(row["pass_tree"])
        if row["status"] != "complete":
            failures.append(f"incomplete tuple {key}")
        if not input_path.is_file() or sha256(input_path) != row["after_cvpipeline_sha256"]:
            failures.append(f"stale input {key}")
        if row["oracle_tool_sha256"] != tool_hash:
            failures.append(f"stale tool {key}")
        if not tree.is_dir() or not snapshots_complete(tree):
            failures.append(f"incomplete pass tree {key}")
        elif tree_sha256(tree) != row["pass_tree_sha256"]:
            failures.append(f"stale pass tree {key}")
        previous = objects.setdefault(row["after_cvpipeline_sha256"], row)
        if previous["pass_tree_sha256"] != row["pass_tree_sha256"]:
            failures.append(f"inconsistent object mapping {key}")
    if len(objects) != dataset.get("unique_object_count"):
        failures.append("unique object cardinality mismatch")
    if dataset.get("complete_object_count") != len(objects):
        failures.append("incomplete object manifest")

    selected = list(sorted(objects.items()))
    if args.rerun >= 0:
        selected = selected[:args.rerun]
    generator = Path(__file__).with_name("dump_c2_c3_pass_oracles.py")
    for input_hash, row in selected:
        with tempfile.TemporaryDirectory(prefix="c2-c3-rerun-") as tmp:
            temp_root = Path(tmp)
            mini_summary = temp_root / "c0.tsv"
            with mini_summary.open("w", newline="") as stream:
                fields = ["adapter", "snapshot", "config_id", "status",
                          "after_cvpipeline_sha256", "after_cvpipeline_ir"]
                writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
                writer.writeheader()
                writer.writerow({name: row[name] for name in fields})
            result = subprocess.run([
                "python3", str(generator), "--c0-summary", str(mini_summary),
                "--tool", str(args.tool.resolve()), "--output-root", str(temp_root),
                "--no-resume"], stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL)
            rerun_tree = (temp_root / "experiments/c2_c3_pass_oracles/objects" /
                          input_hash / "pass_tree")
            if (result.returncode or not rerun_tree.is_dir() or
                    tree_sha256(rerun_tree) != row["pass_tree_sha256"]):
                failures.append(f"non-deterministic pass tree {input_hash}")
    if failures:
        for failure in failures[:30]:
            print(f"[ERROR] {failure}")
        print(f"C2_C3_PASS_ORACLES=FAIL failures={len(failures)}")
        return 1
    print(f"C2_C3_PASS_ORACLES=PASS tuples={len(rows)} "
          f"unique_objects={len(objects)} rerun={len(selected)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
