#!/usr/bin/env python3
"""Require byte-exact after-CVPipelining SemanticIR equality."""

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
    parser.add_argument("--root", type=Path,
                        default=repo / "Output/index/after_cvpipelining_semantic_ir")
    parser.add_argument("--tool", type=Path,
                        default=repo / "build/bin/bishengir-cvpipeline-suffix-compile")
    parser.add_argument("--model", type=Path,
                        default=module / "output/bin/cvpipeline_ub_model_dev_validate")
    parser.add_argument("--rerun", action="store_true")
    args = parser.parse_args()
    root = args.root.resolve()
    with (root / "summary.tsv").open(newline="") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    dataset = json.loads((root / "dataset.json").read_text())
    failures: list[str] = []
    tool_hash = sha256(args.tool.resolve())
    model_hash = sha256(args.model.resolve())
    cvpipelining_summary = Path(dataset.get("cvpipelining_summary", ""))
    if (not cvpipelining_summary.is_file() or
            sha256(cvpipelining_summary) != dataset.get("cvpipelining_summary_sha256")):
        failures.append("stale CVPipelining summary provenance")
    if len(rows) != dataset.get("mapped_tuple_count") or len(rows) != dataset.get("cvpipelining_tuple_count"):
        failures.append("tuple cardinality mismatch")
    tuples: set[tuple[str, str, str]] = set()
    for row in rows:
        key = (row["adapter"], row["snapshot"], row["config_id"])
        if key in tuples:
            failures.append(f"duplicate tuple {key}")
        tuples.add(key)
        if (row["status"] != "complete" or row["compiler_status"] != "0" or
                row["model_status"] != "0"):
            failures.append(f"incomplete tuple {key}")
        after_ir = Path(row["after_cvpipelining_ir"])
        if (not after_ir.is_file() or
                sha256(after_ir) != row["after_cvpipelining_sha256"]):
            failures.append(f"stale after-CVPipelining input {key}")
        if (row["oracle_tool_sha256"] != tool_hash or
                row["model_tool_sha256"] != model_hash):
            failures.append(f"stale tuple tool provenance {key}")
    unique = {row["after_cvpipelining_sha256"]: row for row in rows}
    if len(unique) != dataset.get("unique_object_count"):
        failures.append("unique object cardinality mismatch")
    if dataset.get("complete_object_count") != len(unique):
        failures.append("incomplete unique object manifest")
    for input_hash, row in unique.items():
        oracle = Path(row["oracle_semantic"])
        modeled = Path(row["model_semantic"])
        generic = Path(row["generic_ir"])
        if row["status"] != "complete" or not all(path.is_file() for path in (oracle, modeled, generic)):
            failures.append(f"incomplete object {input_hash}")
            continue
        for field, path in (("oracle_sha256", oracle), ("model_sha256", modeled),
                            ("generic_sha256", generic)):
            if row[field] != sha256(path):
                failures.append(f"stale {field} {input_hash}")
        if row["oracle_tool_sha256"] != tool_hash or row["model_tool_sha256"] != model_hash:
            failures.append(f"stale tool provenance {input_hash}")
        if oracle.read_bytes() != modeled.read_bytes():
            failures.append(f"semantic mismatch {input_hash}")
        if args.rerun:
            with tempfile.TemporaryDirectory(prefix="after-cvpipelining-semantic-rerun-") as tmp:
                rerun = Path(tmp) / "model.tsv"
                result = subprocess.run([
                    str(args.model.resolve()), "--action=dump-after-cvpipelining-semantic-ir",
                    f"--root={generic}", f"--output={rerun}",
                ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                if result.returncode or rerun.read_bytes() != modeled.read_bytes():
                    failures.append(f"non-deterministic model {input_hash}")
    if failures:
        for failure in failures[:30]:
            print(f"[ERROR] {failure}")
        print(f"AFTER_CVPIPELINING_SEMANTIC_IR=FAIL failures={len(failures)}")
        return 1
    print(f"AFTER_CVPIPELINING_SEMANTIC_IR=PASS tuples={len(rows)} "
          f"unique_objects={len(unique)} rerun={args.rerun}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
