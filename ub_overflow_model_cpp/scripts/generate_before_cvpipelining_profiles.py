#!/usr/bin/env python3
"""Generate all named before-CVPipelining input profiles in parallel."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
import subprocess
import sys
import time


MODULE_DIR = Path(__file__).resolve().parents[1]
REPO_ROOT = MODULE_DIR.parent
DEFAULT_PROFILES = MODULE_DIR / "config/pre_cv_profiles"
DEFAULT_OUTPUT = REPO_ROOT / "Output/before_cvpipelining_profiles"
GENERATOR = MODULE_DIR / "scripts/dump_before_cvpipelining_dataset.py"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profiles", type=Path, default=DEFAULT_PROFILES)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--profile-jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=300)
    parser.add_argument("--max-files", type=int, default=0)
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()
    if args.profile_jobs <= 0:
        parser.error("--profile-jobs must be positive")
    if args.timeout < 0 or args.max_files < 0:
        parser.error("--timeout and --max-files must be non-negative")
    return args


def generate_one(
    profile: Path,
    output_root: Path,
    timeout: int,
    max_files: int,
    overwrite: bool,
) -> tuple[str, int, float]:
    destination = output_root / profile.stem
    command = [
        sys.executable,
        str(GENERATOR),
        "--frontend-args",
        str(profile),
        "--output-root",
        str(destination),
        "--timeout",
        str(timeout),
    ]
    if max_files:
        command.extend(("--max-files", str(max_files)))
    if overwrite:
        command.append("--overwrite")
    started = time.monotonic()
    log = output_root / f"{profile.stem}.generation.log"
    with log.open("wb") as stream:
        result = subprocess.run(
            command,
            cwd=REPO_ROOT,
            stdout=stream,
            stderr=subprocess.STDOUT,
            check=False,
        )
    return profile.stem, result.returncode, time.monotonic() - started


def main() -> int:
    args = parse_args()
    profiles = sorted(args.profiles.resolve().glob("*.args"))
    if not profiles:
        print(f"[ERROR] no profiles found under {args.profiles}", file=sys.stderr)
        return 2
    args.output_root.mkdir(parents=True, exist_ok=True)
    print(
        f"profiles={len(profiles)} workers={args.profile_jobs} "
        f"output={args.output_root}",
        flush=True,
    )
    failed = 0
    with ThreadPoolExecutor(max_workers=args.profile_jobs) as pool:
        futures = {
            pool.submit(
                generate_one,
                profile,
                args.output_root.resolve(),
                args.timeout,
                args.max_files,
                args.overwrite,
            ): profile.stem
            for profile in profiles
        }
        completed = 0
        for future in as_completed(futures):
            name, status, elapsed = future.result()
            completed += 1
            failed += status != 0
            print(
                f"[{completed}/{len(profiles)}] {name} "
                f"status={status} elapsed={elapsed:.1f}s",
                flush=True,
            )
    print(f"PROFILE_DATASETS complete={len(profiles) - failed} failed={failed}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
