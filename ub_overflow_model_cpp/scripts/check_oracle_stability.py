#!/usr/bin/env python3
"""Confirm cv2pm is deterministic for one case before treating it as an oracle.

Production CVPipelining and the global workspace flow still iterate
DenseMap/DenseSet keyed on object addresses, so two runs of the same input with
the same options can produce equivalent-but-permuted IR.  A model difference
caused by such a permutation is not a model bug, and chasing one costs hours.

This runs a single case N times and reports whether the PlanMemory facts the
matrix validator compares are identical across runs.  It is cheap for one case
and should be the first step before changing the model for any new mismatch.

Usage:
    check_oracle_stability.py --config preload_auto_mb \\
        --input python_tutorial_06-fused-attention.ttadapter --runs 3 --seed 0
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_cv2pm_oracle_cache import scenario_arguments  # noqa: E402
from compare_ub_plan_with_suffix_oracle import (  # noqa: E402
    parse_oracle,
    parse_oracle_contract,
)
from run_corpus_matrix import (  # noqa: E402
    DEFAULT_CV2PM,
    DEFAULT_MATRIX,
    DEFAULT_PROFILES,
    load_scenarios,
    profile_inputs,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--config", "--scenario", dest="scenario",
                        required=True)
    parser.add_argument("--input", dest="adapter", required=True)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--runs", type=int, default=3)
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument("--profiles-root", type=Path, default=DEFAULT_PROFILES)
    parser.add_argument("--cv2pm", type=Path, default=DEFAULT_CV2PM)
    parser.add_argument("--pipeline-timeout", type=int, default=900)
    parser.add_argument("--plan-timeout", type=int, default=300)
    args = parser.parse_args()
    if args.runs < 2:
        parser.error("--runs must be at least 2")
    return args


def one_run(
    args: argparse.Namespace, scenario: dict[str, str], source: Path,
    work: Path, index: int,
) -> tuple[str, ...]:
    """Run cv2pm end to end once and return the comparable facts."""
    oracle_env = os.environ.copy()
    oracle_env.pop("BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY", None)
    oracle_env.pop("BISHENGIR_DUMP_BEFORE_PLAN_MEMORY", None)
    oracle_env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
    plan = subprocess.run(
        [str(args.cv2pm.resolve()), str(source), "--mlir-disable-threading",
         *scenario_arguments(scenario), f"--plan-memory-seed={args.seed}",
         "--ub-oracle-only", "-o", os.devnull],
        text=True, capture_output=True, check=False,
        timeout=args.pipeline_timeout + args.plan_timeout, env=oracle_env,
    )
    (work / f"run.{index}.stderr").write_text(
        plan.stderr, encoding="utf-8"
    )
    if ("PLANMEM_LIVENESS_ATTEMPT\t" not in plan.stderr and
            "PLANMEM_UB_ORACLE_COMPLETE\t" not in plan.stderr):
        # A deterministic pre-PlanMemory failure is itself the fact to compare.
        return ("pipeline_failed", str(plan.returncode))
    _, peak, plan_multiset, lifetimes = parse_oracle(
        plan.stderr, args.seed, "6"
    )
    status, required, multi, inplace = parse_oracle_contract(
        plan.stderr, args.seed, "6"
    )
    return (
        f"status={status}",
        f"required={required}",
        f"peak={peak}",
        f"plan={sorted(plan_multiset.items())}",
        f"lifetime={sorted(lifetimes.items())}",
        f"multi={sorted(multi.items())}",
        f"inplace={sorted(inplace.items())}",
    )


def main() -> int:
    args = parse_args()
    try:
        scenarios = load_scenarios(args.matrix.resolve(), {args.scenario})
    except (OSError, ValueError) as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    scenario = scenarios[0]
    try:
        inputs = profile_inputs(
            args.profiles_root.resolve(), scenario["pre_cv_profile"],
            {args.adapter}, 0,
        )
    except ValueError as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2
    if not inputs:
        print(f"[ERROR] no input named {args.adapter}", file=sys.stderr)
        return 2
    adapter, source = inputs[0]

    work = Path(tempfile.mkdtemp(prefix="cvub-stability-"))
    print(f"work dir (kept): {work}")
    print(f"case: {scenario['scenario_id']} / {adapter} / seed {args.seed}")
    observations: list[tuple[str, ...]] = []
    for index in range(args.runs):
        print(f"  run {index + 1}/{args.runs} ...", flush=True)
        try:
            observations.append(one_run(args, scenario, source, work, index))
        except subprocess.TimeoutExpired:
            print("[ERROR] cv2pm timed out; this case is not usable as an "
                  "oracle at the current budget", file=sys.stderr)
            return 2

    distinct = {observation for observation in observations}
    if len(distinct) == 1:
        print(f"\nSTABLE across {args.runs} runs; a model difference on this "
              f"case is a real model difference")
        return 0
    print(f"\nUNSTABLE: {len(distinct)} distinct results across {args.runs} "
          f"runs. cv2pm itself permutes here, so do NOT change the model to "
          f"chase this difference until the permuted fields are identified.",
          file=sys.stderr)
    reference = observations[0]
    for index, observation in enumerate(observations[1:], start=2):
        for field_index, (first, other) in enumerate(
            zip(reference, observation)
        ):
            if first != other:
                label = first.split("=", 1)[0]
                print(f"  run 1 vs run {index}: {label} differs",
                      file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
