#!/usr/bin/env python3
"""Dump native BiSheng checkpoints from before AutoBlockify to before CV."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_COMPILER = ROOT / "build/bin/bishengir-compile"
CHECKPOINT_STAGES = (
    "00_before_auto_blockify",
    "01_after_auto_blockify",
    "02_after_pre_cv_mark_multi_buffer",
    "03_after_outer_extended_canonicalizer",
    "04_after_arith_to_affine",
    "05_after_canonicalize_iter_arg",
    "06_after_extended_canonicalizer_module",
    "07_after_scf_for_loop_canonicalization",
    "08_after_cse",
    "09_after_extended_canonicalizer_func_1",
    "10_after_hivm_opt_single_point",
    "11_after_extended_canonicalizer_func_2",
    "12_after_memref_dead_store_elimination",
    "13_after_inline_otf_broadcast",
)
DEFAULT_COMPILER_ARGS = (
    "--enable-hfusion-compile=true",
    "--enable-triton-kernel-compile=true",
    "--enable-auto-blockify-loop=true",
    "--mlir-disable-threading",
)


def checkpoint_environment(
    output_directory: Path, stage: str | None
) -> dict[str, str]:
    environment = os.environ.copy()
    environment["BISHENGIR_UB_PREFIX_CHECKPOINT_DIR"] = str(
        output_directory.resolve()
    )
    environment["BISHENGIR_STOP_AFTER_UB_PREFIX_CHECKPOINTS"] = "1"
    if stage is None:
        environment.pop("BISHENGIR_UB_PREFIX_CHECKPOINT_STAGE", None)
    else:
        environment["BISHENGIR_UB_PREFIX_CHECKPOINT_STAGE"] = stage
    return environment


def validate_attempt(attempt: Path, stage: str | None) -> list[Path]:
    expected_stages = CHECKPOINT_STAGES if stage is None else (stage,)
    expected = [attempt / f"{name}.mlir" for name in expected_stages]
    missing = [path for path in expected if not path.is_file()]
    if missing:
        raise RuntimeError(
            "missing UB prefix checkpoints: "
            + ", ".join(str(path) for path in missing)
        )
    actual = sorted(attempt.glob("*.mlir"))
    unexpected = [path for path in actual if path not in expected]
    if unexpected:
        raise RuntimeError(
            "unexpected UB prefix checkpoints: "
            + ", ".join(str(path) for path in unexpected)
        )
    return expected


def parse_arguments(
    argv: list[str] | None = None,
) -> tuple[argparse.Namespace, list[str]]:
    raw_arguments = list(sys.argv[1:] if argv is None else argv)
    if "--" in raw_arguments:
        separator = raw_arguments.index("--")
        driver_arguments = raw_arguments[:separator]
        compiler_arguments = raw_arguments[separator + 1 :]
    else:
        driver_arguments = raw_arguments
        compiler_arguments = []

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="Input .ttadapter file")
    parser.add_argument("--compiler", type=Path, default=DEFAULT_COMPILER)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--stage", choices=CHECKPOINT_STAGES)
    arguments = parser.parse_args(driver_arguments)
    return arguments, compiler_arguments


def main() -> int:
    arguments, compiler_args = parse_arguments()
    compiler = arguments.compiler.resolve()
    input_path = arguments.input.resolve()
    output_directory = arguments.output_dir.resolve()
    if not compiler.is_file():
        raise RuntimeError(f"compiler does not exist: {compiler}")
    if not input_path.is_file():
        raise RuntimeError(f"input does not exist: {input_path}")
    output_directory.mkdir(parents=True, exist_ok=True)
    existing_attempts = set(output_directory.glob("attempt-*"))

    if not compiler_args:
        compiler_args = list(DEFAULT_COMPILER_ARGS)

    with tempfile.TemporaryDirectory(prefix="cvub-prefix-checkpoint-") as tmp:
        command = [
            str(compiler),
            str(input_path),
            *compiler_args,
            "-o",
            str(Path(tmp) / "checkpoint.o"),
        ]
        completed = subprocess.run(
            command,
            cwd=ROOT,
            env=checkpoint_environment(output_directory, arguments.stage),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
    if completed.returncode != 0:
        sys.stderr.write(completed.stderr)
        return completed.returncode

    new_attempts = sorted(
        set(output_directory.glob("attempt-*")) - existing_attempts
    )
    if len(new_attempts) != 1:
        raise RuntimeError(
            "expected one new UB prefix checkpoint attempt, found "
            f"{len(new_attempts)}"
        )
    files = validate_attempt(new_attempts[0], arguments.stage)
    print(f"checkpoint attempt: {new_attempts[0]}")
    for path in files:
        print(path)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
