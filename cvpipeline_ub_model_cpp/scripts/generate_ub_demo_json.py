#!/usr/bin/env python3
"""Generate JSON suitable for the UB plan visualizer demo."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the before-CVPipeline UB model and save JSON for the demo.")
    parser.add_argument("--pre-cvpipeline-ir", required=True,
                        help="Generic IR captured before CVPipelining.")
    parser.add_argument("--output", required=True,
                        help="Destination JSON file.")
    parser.add_argument("--cv-pipeline-depth", default="-1")
    parser.add_argument("--cv-disable-pipelining", action="store_true")
    parser.add_argument("--cv-enable-preload", action="store_true")
    parser.add_argument("--cv-enable-lazy-loading", action="store_true")
    parser.add_argument("--suffix-enable-auto-multi-buffer", action="store_true")
    parser.add_argument("--suffix-local-multi-buffer-strategy", default="no-limit")
    parser.add_argument("--suffix-mix-multi-buffer-strategy", default="no-limit")
    parser.add_argument("--restrict-inplace-as-isa", action="store_true")
    parser.add_argument("--random-seed", default=None)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = repo_root()
    wrapper = root / "cvpipeline_ub_model_cpp" / "scripts" / "plan_precvpipeline_ub.py"
    output = Path(args.output)
    command = [
        sys.executable,
        str(wrapper),
        f"--pre-cvpipeline-ir={args.pre_cvpipeline_ir}",
        f"--cv-pipeline-depth={args.cv_pipeline_depth}",
        f"--cv-disable-pipelining={str(args.cv_disable_pipelining).lower()}",
        f"--cv-enable-preload={str(args.cv_enable_preload).lower()}",
        f"--cv-enable-lazy-loading={str(args.cv_enable_lazy_loading).lower()}",
        f"--suffix-enable-auto-multi-buffer={str(args.suffix_enable_auto_multi_buffer).lower()}",
        f"--suffix-local-multi-buffer-strategy={args.suffix_local_multi_buffer_strategy}",
        f"--suffix-mix-multi-buffer-strategy={args.suffix_mix_multi_buffer_strategy}",
        "--format=json",
    ]
    if args.restrict_inplace_as_isa:
        command.append("--restrict-inplace-as-isa")
    if args.random_seed is not None:
        command.append(f"--random-seed={args.random_seed}")
    completed = subprocess.run(
        command,
        cwd=root,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if completed.returncode != 0:
        sys.stderr.write(completed.stderr)
        return completed.returncode
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(completed.stdout, encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
