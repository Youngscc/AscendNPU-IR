#!/usr/bin/env python3
"""Run named corpus differential configurations from a validated TSV matrix."""

from __future__ import annotations

import argparse
import csv
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_MATRIX = SCRIPT_DIR.parent / "config" / "corpus_test_matrix.tsv"
DEFAULT_RUNNER = SCRIPT_DIR / "run_corpus_oracle.py"
STRATEGIES = {"no-limit", "only-cube", "only-vector", "no-l0c"}
FIELDS = (
    "name",
    "required",
    "restrict_inplace_as_isa",
    "disable_cv_pipelining",
    "cv_pipeline_depth",
    "enable_preload",
    "enable_cv_lazy_loading",
    "enable_code_motion",
    "tile_mix_cube_loop",
    "tile_mix_vector_loop",
    "enable_ubuf_saving",
    "enable_auto_multi_buffer",
    "disable_align_alloc_size",
    "disable_enable_stride_align",
    "disable_infer_hivm_data_layout",
    "enable_triton_kernel_compile",
    "local_multi_buffer_strategy",
    "mix_multi_buffer_strategy",
)


def parse_zero_one(value: str, *, field: str, line: int) -> bool:
    if value not in {"0", "1"}:
        raise ValueError(f"line {line}: {field} must be 0 or 1, got {value!r}")
    return value == "1"


@dataclass(frozen=True)
class MatrixConfig:
    name: str
    required: bool
    restrict_inplace_as_isa: bool
    disable_cv_pipelining: bool
    cv_pipeline_depth: int
    enable_preload: bool
    enable_cv_lazy_loading: bool
    enable_code_motion: bool
    tile_mix_cube_loop: int
    tile_mix_vector_loop: int
    enable_ubuf_saving: bool
    enable_auto_multi_buffer: bool
    disable_align_alloc_size: bool
    disable_enable_stride_align: bool
    disable_infer_hivm_data_layout: bool
    enable_triton_kernel_compile: bool
    local_multi_buffer_strategy: str
    mix_multi_buffer_strategy: str


def load_matrix(path: Path) -> list[MatrixConfig]:
    try:
        stream = path.open(newline="", encoding="utf-8")
    except OSError as error:
        raise ValueError(f"cannot read matrix {path}: {error}") from error

    with stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != list(FIELDS):
            raise ValueError(
                "matrix header must be exactly: " + "\t".join(FIELDS))
        configs: list[MatrixConfig] = []
        names: set[str] = set()
        for line, row in enumerate(reader, start=2):
            if not any(row.values()) or row["name"].startswith("#"):
                continue
            name = row["name"].strip()
            if not name:
                raise ValueError(f"line {line}: name must not be empty")
            if name in names:
                raise ValueError(f"line {line}: duplicate configuration name {name!r}")
            names.add(name)
            try:
                depth = int(row["cv_pipeline_depth"])
            except ValueError as error:
                raise ValueError(
                    f"line {line}: cv_pipeline_depth must be an integer") from error
            if depth < -1:
                raise ValueError(
                    f"line {line}: cv_pipeline_depth must be -1 or non-negative")
            tile_factors: dict[str, int] = {}
            for field in ("tile_mix_cube_loop", "tile_mix_vector_loop"):
                try:
                    tile_factors[field] = int(row[field])
                except ValueError as error:
                    raise ValueError(
                        f"line {line}: {field} must be an integer") from error
                if tile_factors[field] <= 0:
                    raise ValueError(
                        f"line {line}: {field} must be positive")
            local_strategy = row["local_multi_buffer_strategy"]
            mix_strategy = row["mix_multi_buffer_strategy"]
            for field, strategy in (
                    ("local_multi_buffer_strategy", local_strategy),
                    ("mix_multi_buffer_strategy", mix_strategy)):
                if strategy not in STRATEGIES:
                    raise ValueError(
                        f"line {line}: {field} has invalid value {strategy!r}")
            configs.append(MatrixConfig(
                name=name,
                required=parse_zero_one(
                    row["required"], field="required", line=line),
                restrict_inplace_as_isa=parse_zero_one(
                    row["restrict_inplace_as_isa"],
                    field="restrict_inplace_as_isa", line=line),
                disable_cv_pipelining=parse_zero_one(
                    row["disable_cv_pipelining"],
                    field="disable_cv_pipelining", line=line),
                cv_pipeline_depth=depth,
                enable_preload=parse_zero_one(
                    row["enable_preload"], field="enable_preload", line=line),
                enable_cv_lazy_loading=parse_zero_one(
                    row["enable_cv_lazy_loading"],
                    field="enable_cv_lazy_loading", line=line),
                enable_code_motion=parse_zero_one(
                    row["enable_code_motion"],
                    field="enable_code_motion", line=line),
                tile_mix_cube_loop=tile_factors["tile_mix_cube_loop"],
                tile_mix_vector_loop=tile_factors["tile_mix_vector_loop"],
                enable_ubuf_saving=parse_zero_one(
                    row["enable_ubuf_saving"],
                    field="enable_ubuf_saving", line=line),
                enable_auto_multi_buffer=parse_zero_one(
                    row["enable_auto_multi_buffer"],
                    field="enable_auto_multi_buffer", line=line),
                disable_align_alloc_size=parse_zero_one(
                    row["disable_align_alloc_size"],
                    field="disable_align_alloc_size", line=line),
                disable_enable_stride_align=parse_zero_one(
                    row["disable_enable_stride_align"],
                    field="disable_enable_stride_align", line=line),
                disable_infer_hivm_data_layout=parse_zero_one(
                    row["disable_infer_hivm_data_layout"],
                    field="disable_infer_hivm_data_layout", line=line),
                enable_triton_kernel_compile=parse_zero_one(
                    row["enable_triton_kernel_compile"],
                    field="enable_triton_kernel_compile", line=line),
                local_multi_buffer_strategy=local_strategy,
                mix_multi_buffer_strategy=mix_strategy,
            ))
    if not configs:
        raise ValueError("matrix contains no configurations")
    return configs


def build_command(args: argparse.Namespace, config: MatrixConfig) -> list[str]:
    command = [
        sys.executable,
        str(args.runner),
        "--corpus-root", str(args.corpus_root),
        "--model", str(args.model),
        "--compiler", str(args.compiler),
        "--seeds", str(args.seed),
        "--cv-pipeline-depth", str(config.cv_pipeline_depth),
        "--enable-code-motion", str(config.enable_code_motion).lower(),
        "--tile-mix-cube-loop", str(config.tile_mix_cube_loop),
        "--tile-mix-vector-loop", str(config.tile_mix_vector_loop),
        "--local-multi-buffer-strategy", config.local_multi_buffer_strategy,
        "--mix-multi-buffer-strategy", config.mix_multi_buffer_strategy,
    ]
    if config.restrict_inplace_as_isa:
        command.append("--restrict-inplace-as-isa")
    if config.disable_cv_pipelining:
        command.append("--disable-cv-pipelining")
    if config.enable_preload:
        command.append("--enable-preload")
    if config.enable_cv_lazy_loading:
        command.append("--enable-cv-lazy-loading")
    if config.enable_ubuf_saving:
        command.append("--enable-ubuf-saving")
    if config.enable_auto_multi_buffer:
        command.append("--enable-auto-multi-buffer")
    if config.disable_align_alloc_size:
        command.append("--disable-align-alloc-size")
    if config.disable_enable_stride_align:
        command.append("--disable-enable-stride-align")
    if config.disable_infer_hivm_data_layout:
        command.append("--disable-infer-hivm-data-layout")
    if config.enable_triton_kernel_compile:
        command.append("--enable-triton-kernel-compile")
    if args.max_cases is not None:
        command.extend(("--max-cases", str(args.max_cases)))
    if args.pipeline_manifest is not None:
        command.extend(("--pipeline-manifest", str(args.pipeline_manifest)))
    if args.require_all_exact:
        command.append("--require-all-exact")
    if args.quiet:
        command.append("--quiet")
    if args.no_progress:
        command.append("--no-progress")
    return command


def bounded_seed(text: str) -> int:
    value = int(text)
    if not 0 <= value <= 19:
        raise argparse.ArgumentTypeError("seed must be in [0, 19]")
    return value


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run every selected configuration from a TSV matrix once")
    parser.add_argument("--matrix", type=Path, default=DEFAULT_MATRIX)
    parser.add_argument("--runner", type=Path, default=DEFAULT_RUNNER,
                        help=argparse.SUPPRESS)
    parser.add_argument("--corpus-root", type=Path)
    parser.add_argument("--model", type=Path)
    parser.add_argument("--compiler", type=Path)
    parser.add_argument("--seed", type=bounded_seed, default=0,
                        help="single fixed PlanMemory seed (default: 0)")
    parser.add_argument("--config", action="append", default=[], metavar="NAME",
                        help="run only this named configuration; repeatable")
    parser.add_argument("--max-cases", type=int)
    parser.add_argument("--pipeline-manifest", type=Path)
    parser.add_argument("--require-all-exact", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--no-progress", action="store_true")
    parser.add_argument("--stop-on-failure", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--list", action="store_true",
                        help="list matrix configuration names and exit")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        configs = load_matrix(args.matrix)
    except ValueError as error:
        print(f"invalid test matrix: {error}", file=sys.stderr)
        return 2

    if args.list:
        for config in configs:
            print(config.name)
        return 0

    missing = [
        option for option, value in (
            ("--corpus-root", args.corpus_root),
            ("--model", args.model),
            ("--compiler", args.compiler),
        ) if value is None
    ]
    if missing:
        print("the following arguments are required: " + ", ".join(missing),
              file=sys.stderr)
        return 2
    if args.max_cases is not None and args.max_cases <= 0:
        print("--max-cases must be positive", file=sys.stderr)
        return 2

    if args.config:
        selected = set(args.config)
        known = {config.name for config in configs}
        unknown = selected - known
        if unknown:
            print("unknown matrix configuration(s): " + ", ".join(sorted(unknown)),
                  file=sys.stderr)
            return 2
        configs = [config for config in configs if config.name in selected]

    required_failures: list[str] = []
    experimental_failures: list[str] = []
    executed = 0
    for index, config in enumerate(configs, start=1):
        command = build_command(args, config)
        print(f"[{index}/{len(configs)}] {config.name}", flush=True)
        print("  " + shlex.join(command), flush=True)
        if args.dry_run:
            continue
        completed = subprocess.run(command)
        executed += 1
        if completed.returncode != 0:
            failures = required_failures if config.required else experimental_failures
            failures.append(config.name)
            classification = "required" if config.required else "experimental"
            print(f"[{classification} failure] {config.name}", file=sys.stderr)
            if args.stop_on_failure and config.required:
                break

    if args.dry_run:
        print(f"dry run complete: {len(configs)} configuration(s), seed={args.seed}")
        return 0
    print(
        f"matrix summary: selected={len(configs)} executed={executed} "
        f"passed={executed - len(required_failures) - len(experimental_failures)} "
        f"required_failed={len(required_failures)} "
        f"experimental_failed={len(experimental_failures)} seed={args.seed}")
    if required_failures:
        print("failed required configurations: " + ", ".join(required_failures),
              file=sys.stderr)
    if experimental_failures:
        print("failed experimental configurations: " +
              ", ".join(experimental_failures), file=sys.stderr)
    if required_failures:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
