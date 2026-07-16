#!/usr/bin/env python3
"""Compare lightweight CVPipelining semantics with the real pass."""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
import tempfile
from pathlib import Path


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--max-failures", type=int, default=20)
    args = parser.parse_args()
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    before_rows = {
        row["input_sha256"]: row for row in csv.DictReader(
            (repo / "Output/index/before_cvpipelining/summary.tsv").open(),
            delimiter="\t")}
    cvpipelining_rows = list(csv.DictReader(
        (repo / "Output/index/cvpipelining/summary.tsv").open(),
        delimiter="\t"))
    after_cvpipelining_rows = {
        (row["adapter"], row["snapshot"], row["config_id"]): row
        for row in csv.DictReader(
            (repo / "Output/index/after_cvpipelining_semantic_ir/summary.tsv").open(),
            delimiter="\t")}
    if args.limit:
        cvpipelining_rows = cvpipelining_rows[:args.limit]
    failures: list[str] = []
    checked = 0
    with tempfile.TemporaryDirectory(prefix="cvub-cvpipelining-") as temp:
        for index, row in enumerate(cvpipelining_rows):
            before = before_rows[row["input_sha256"]]["generic_ir"]
            oracle = after_cvpipelining_rows[(row["adapter"], row["snapshot"],
                              row["config_id"])]["model_semantic"]
            config = json.loads(Path(row["config_json"]).read_text())
            output = Path(temp) / f"{index}.tsv"
            command = [
                str(tool), "--action=model-cvpipelining-semantic-ir",
                "--root", before, "--output", str(output),
                "--cv-pipeline-depth", config["cv_pipeline_depth"],
                f"--disable-cv-pipelining={config['disable_cv_pipelining']}",
                f"--enable-preload={config['enable_preload']}",
                f"--enable-cv-lazy-loading={config['enable_cv_lazy_loading']}"]
            process = subprocess.run(command, text=True, capture_output=True)
            if process.returncode != 0:
                failures.append(
                    f"{row['adapter']}/{row['config_id']}: {process.stderr.strip()}")
            elif output.read_bytes() != Path(oracle).read_bytes():
                model_lines = output.read_text().splitlines()
                oracle_lines = Path(oracle).read_text().splitlines()
                difference = next(
                    (line for line, pair in enumerate(zip(model_lines, oracle_lines), 1)
                     if pair[0] != pair[1]),
                    min(len(model_lines), len(oracle_lines)) + 1)
                failures.append(
                    f"{row['adapter']}/{row['config_id']}: line={difference}")
            else:
                checked += 1
    if failures:
        print(f"CVPIPELINING=FAIL checked={checked} failures={len(failures)}")
        print("\n".join(failures[:args.max_failures]))
        return 1
    print(f"CVPIPELINING=PASS tuples={checked}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
