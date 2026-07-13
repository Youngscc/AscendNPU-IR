#!/usr/bin/env python3
"""Compare production D1+C+B planning with saved after-CVPipeline C+B oracle."""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
from pathlib import Path


def run_json(command: list[str]) -> tuple[int, dict, str]:
    process = subprocess.run(command, text=True, capture_output=True)
    try:
        payload = json.loads(process.stdout)
    except json.JSONDecodeError as exc:
        raise RuntimeError(
            f"invalid JSON from {' '.join(command)}\nstdout={process.stdout}\n"
            f"stderr={process.stderr}") from exc
    return process.returncode, payload, process.stderr.strip()


def run_text(command: list[str]) -> tuple[int, list[str], str]:
    process = subprocess.run(command, text=True, capture_output=True)
    return process.returncode, process.stdout.splitlines(), process.stderr.strip()


def comparable(payload: dict) -> dict:
    return {
        "status": payload.get("status"),
        "overflow": payload.get("overflow"),
        "ub_peak_bits": payload.get("ub_peak_bits"),
        "required_bits": payload.get("required_bits"),
        "capacity_bits": payload.get("capacity_bits"),
        "selected_seed": payload.get("selected_seed"),
    }


def comparable_plan_lines(lines: list[str]) -> list[str]:
    kept: list[str] = []
    for line in lines:
        if line.startswith("precision\t"):
            continue
        kept.append(line)
    return kept


def bool_arg(value: str) -> str:
    return "true" if value in {"1", "true", "True"} else "false"


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--max-failures", type=int, default=20)
    parser.add_argument("--seeds", default="0")
    parser.add_argument("--restrict-modes", default="0")
    parser.add_argument("--compare-text-plan", action="store_true")
    args = parser.parse_args()

    tool = module / "output/bin/cvpipeline_ub_model"
    before_rows = {
        row["input_sha256"]: row
        for row in csv.DictReader(
            (repo / "Output/index/d0_before_cvpipeline_generic/summary.tsv").open(),
            delimiter="\t",
        )
    }
    c0_rows = list(
        csv.DictReader(
            (repo / "Output/index/c0_cvpipeline_after/summary.tsv").open(),
            delimiter="\t",
        )
    )
    c1_rows = {
        (row["adapter"], row["snapshot"], row["config_id"]): row
        for row in csv.DictReader(
            (repo / "Output/index/c1_semantic_ir/summary.tsv").open(),
            delimiter="\t",
        )
    }
    if args.limit:
        c0_rows = c0_rows[: args.limit]
    seeds = [int(seed) for seed in args.seeds.split(",") if seed]
    restrict_modes = [mode in {"1", "true", "True"}
                      for mode in args.restrict_modes.split(",") if mode]

    checked = 0
    failures: list[str] = []
    for row in c0_rows:
        key = (row["adapter"], row["snapshot"], row["config_id"])
        before = before_rows[row["input_sha256"]]["generic_ir"]
        c1 = c1_rows[key]["generic_ir"]
        config = json.loads(Path(row["config_json"]).read_text())
        common_suffix = [
            f"--enable-auto-multi-buffer={bool_arg(config['enable_auto_multi_buffer'])}",
            "--limit-auto-multi-buffer-of-local-buffer",
            config["local_multi_buffer_strategy"],
            "--limit-auto-multi-buffer-buffer",
            config["mix_multi_buffer_strategy"],
        ]
        cv_options = [
            "--cv-pipeline-depth",
            config["cv_pipeline_depth"],
            f"--disable-cv-pipelining={bool_arg(config['disable_cv_pipelining'])}",
            f"--enable-preload={bool_arg(config['enable_preload'])}",
            f"--enable-cv-lazy-loading={bool_arg(config['enable_cv_lazy_loading'])}",
        ]
        for restrict in restrict_modes:
            restrict_arg = ["--restrict-inplace-as-isa"] if restrict else []
            for seed in seeds:
                seed_arg = [f"--random-seed={seed}"]
                d_command = [
                    str(tool),
                    "--action=plan-before-cvpipeline",
                    "--before-cvpipeline-ir",
                    before,
                    *cv_options,
                    *common_suffix,
                    *restrict_arg,
                    *seed_arg,
                ]
                c_command = [
                    str(tool),
                    "--action=plan-c1-suffix",
                    "--c1-generic-ir",
                    c1,
                    *common_suffix,
                    *restrict_arg,
                    *seed_arg,
                ]
                d_status, d_payload, d_stderr = run_json(
                    [*d_command, "--format=json"])
                c_status, c_payload, c_stderr = run_json(
                    [*c_command, "--format=json"])
                if comparable(d_payload) != comparable(c_payload):
                    failures.append(
                        f"{row['adapter']}/{row['config_id']}/seed={seed}/"
                        f"restrict={int(restrict)}: "
                        f"D={comparable(d_payload)} C={comparable(c_payload)} "
                        f"Derr={d_stderr} Cerr={c_stderr}")
                elif (d_status == 0) != (c_status == 0):
                    failures.append(
                        f"{row['adapter']}/{row['config_id']}/seed={seed}/"
                        f"restrict={int(restrict)}: return D={d_status} C={c_status}")
                elif args.compare_text_plan:
                    d_text_status, d_text, d_text_stderr = run_text(d_command)
                    c_text_status, c_text, c_text_stderr = run_text(c_command)
                    if (d_text_status == 0) != (c_text_status == 0):
                        failures.append(
                            f"{row['adapter']}/{row['config_id']}/seed={seed}/"
                            f"restrict={int(restrict)}: text return "
                            f"D={d_text_status} C={c_text_status}")
                    elif comparable_plan_lines(d_text) != comparable_plan_lines(c_text):
                        failures.append(
                            f"{row['adapter']}/{row['config_id']}/seed={seed}/"
                            f"restrict={int(restrict)}: text plan mismatch "
                            f"Derr={d_text_stderr} Cerr={c_text_stderr}")
                    else:
                        checked += 1
                else:
                    checked += 1
                if len(failures) >= args.max_failures:
                    break
            if len(failures) >= args.max_failures:
                break
        if len(failures) >= args.max_failures:
            break

    if failures:
        print(f"D3_END_TO_END=FAIL checked={checked} failures={len(failures)}")
        print("\n".join(failures[: args.max_failures]))
        return 1
    print(f"D3_END_TO_END=PASS tuples={checked}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
