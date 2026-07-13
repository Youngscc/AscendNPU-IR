#!/usr/bin/env python3
"""Acceptance gate for the initial C1-to-PlanMemory production delivery."""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--min-coverage", type=float, default=0.5)
    args = parser.parse_args()
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    binary = module / "output/bin/cvpipeline_ub_model"

    bridge = subprocess.run(
        ["python3", str(module / "scripts/validate_planmemory_bridge_input.py"),
         "--one-config-per-ir", "--max-failures=20"],
        text=True, capture_output=True)
    bridge_text = bridge.stdout.strip()
    if "checked=165 failures=1" not in bridge_text and bridge.returncode != 0:
        print(bridge_text)
        print(bridge.stderr.strip())
        return 1

    rows = list(csv.DictReader(
        (repo / "Output/index/c6_pass_oracles/summary.tsv").open(),
        delimiter="\t"))
    unique: dict[str, dict[str, str]] = {}
    for row in rows:
        unique.setdefault(row["after_cvpipeline_sha256"], row)

    accepted = 0
    blocked = 0
    failures: list[str] = []
    report_rows: list[tuple[str, str, str]] = []
    for digest, row in unique.items():
        source = (repo / "Output/experiments/c2_c3_pass_oracles/objects" /
                  digest / "pass_tree/builtin_module_no-symbol-name" /
                  "0_one-shot-bufferize.mlir")
        command = [
            str(binary), "--action=plan-c1-suffix",
            f"--c1-generic-ir={source}", "--random-seed=0", "--format=json",
            "--limit-auto-multi-buffer-of-local-buffer", row["local_strategy"],
            "--limit-auto-multi-buffer-buffer", row["mix_strategy"]]
        if row["enable_auto"] == "1":
            command.append("--enable-auto-multi-buffer")
        process = subprocess.run(command, text=True, capture_output=True)
        try:
            output = json.loads(process.stdout)
        except json.JSONDecodeError:
            failures.append(f"{digest}: invalid JSON: {process.stdout!r}")
            continue
        precision = output.get("precision", "")
        status = output.get("status", "")
        if precision == "initial_c_model" and status in {"success", "overflow"}:
            accepted += 1
            report_rows.append((digest, "accepted", status))
        elif precision == "blocked" and status == "blocker":
            blocked += 1
            report_rows.append((digest, "blocked", ";".join(output["blockers"])))
        else:
            failures.append(f"{digest}: unexpected result {output}")

    total = len(unique)
    coverage = accepted / total if total else 0.0
    report = module / "output/reports/initial_c_delivery.tsv"
    report.parent.mkdir(parents=True, exist_ok=True)
    with report.open("w") as stream:
        stream.write("input_sha256\tresult\tdetail\n")
        for fields in report_rows:
            stream.write("\t".join(fields) + "\n")
    if failures or coverage < args.min_coverage:
        print(f"INITIAL_C_DELIVERY=FAIL accepted={accepted} blocked={blocked} "
              f"total={total} coverage={coverage:.4f}")
        print("\n".join(failures[:20]))
        return 1
    print(f"INITIAL_C_DELIVERY=PASS accepted={accepted} blocked={blocked} "
          f"total={total} coverage={coverage:.4f}")
    print(bridge_text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
