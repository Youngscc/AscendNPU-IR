#!/usr/bin/env python3
"""Run named corpus differential configurations from a validated TSV matrix."""

from __future__ import annotations

import argparse
import contextlib
import csv
import hashlib
import shlex
import subprocess
import sys
import tempfile
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


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


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


def build_command(
    args: argparse.Namespace,
    config: MatrixConfig,
    runtime_timing_output: Path | None = None,
) -> list[str]:
    command = [
        sys.executable,
        str(args.runner),
        "--corpus-root", str(args.corpus_root),
        "--model", str(args.model),
        "--compiler", str(args.compiler),
        "--cv-pipeline-depth", str(config.cv_pipeline_depth),
        "--enable-code-motion", str(config.enable_code_motion).lower(),
        "--tile-mix-cube-loop", str(config.tile_mix_cube_loop),
        "--tile-mix-vector-loop", str(config.tile_mix_vector_loop),
        "--local-multi-buffer-strategy", config.local_multi_buffer_strategy,
        "--mix-multi-buffer-strategy", config.mix_multi_buffer_strategy,
    ]
    if getattr(args, "retry_only", False):
        command.append("--retry-only")
    else:
        selected_seeds = getattr(args, "seeds", None)
        command.extend((
            "--seeds",
            selected_seeds if selected_seeds is not None
            else str(args.seed if args.seed is not None else 0),
        ))
    if getattr(args, "suffix_cache_dir", None) is not None:
        command.extend((
            "--suffix-cache-dir", str(args.suffix_cache_dir),
            "--suffix-cache-mode", args.suffix_cache_mode,
        ))
        compiler_digest = getattr(args, "suffix_compiler_sha256", None)
        if compiler_digest is not None:
            command.extend(("--suffix-compiler-sha256", compiler_digest))
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
    if getattr(args, "case_start", 0):
        command.extend(("--case-start", str(args.case_start)))
    if args.pipeline_manifest is not None:
        command.extend(("--pipeline-manifest", str(args.pipeline_manifest)))
    if runtime_timing_output is not None:
        command.extend((
            "--runtime-timing-output", str(runtime_timing_output)))
        if getattr(args, "runtime_timing_exclude_dumps", False):
            command.append("--runtime-timing-exclude-dumps")
        command.append("--suppress-runtime-timing-summary")
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
    seed_group = parser.add_mutually_exclusive_group()
    seed_group.add_argument(
        "--seed", type=bounded_seed,
        help="single fixed PlanMemory seed (default: 0)")
    seed_group.add_argument(
        "--seeds",
        help="PlanMemory seed selection, for example 0-19",
    )
    seed_group.add_argument(
        "--retry-only", action="store_true",
        help="Run only the default PlanMemory seed-retry mode",
    )
    parser.add_argument(
        "--suffix-cache-dir", type=Path,
        help=("Share a content-addressed suffix oracle cache across all "
              "matrix configurations"),
    )
    parser.add_argument(
        "--suffix-cache-mode",
        choices=("read-write", "read-only", "refresh"),
        default="read-write",
        help=("read-write fills misses (default); read-only never invokes "
              "suffix; refresh replaces matching entries"),
    )
    parser.add_argument("--config", action="append", default=[], metavar="NAME",
                        help="run only this named configuration; repeatable")
    parser.add_argument(
        "--case-start", type=int, default=0,
        help="Skip this many selected corpus inputs before --max-cases",
    )
    parser.add_argument("--max-cases", type=int)
    parser.add_argument("--pipeline-manifest", type=Path)
    parser.add_argument(
        "--runtime-timing-output", type=Path,
        help=("Collect executable-internal timing from every selected "
              "configuration into one TSV"),
    )
    timing_dump_group = parser.add_mutually_exclusive_group()
    timing_dump_group.add_argument(
        "--runtime-timing-exclude-dumps", action="store_true",
        dest="runtime_timing_exclude_dumps", default=True,
        help=("Exclude suffix oracle/debug dump layers from measured time "
              "using a separate suffix invocation (default)"),
    )
    timing_dump_group.add_argument(
        "--runtime-timing-include-dumps", action="store_false",
        dest="runtime_timing_exclude_dumps",
        help="Include suffix oracle/debug dump overhead in measured time",
    )
    parser.add_argument("--require-all-exact", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--no-progress", action="store_true")
    parser.add_argument("--stop-on-failure", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--list", action="store_true",
                        help="list matrix configuration names and exit")
    return parser.parse_args()


def merge_runtime_timings(
    output: Path,
    parts: list[tuple[str, Path]],
) -> tuple[int, int, int]:
    expected_header = [
        "input", "seed", "tool", "kind", "name", "occurrence",
        "nanoseconds", "milliseconds",
    ]
    output.parent.mkdir(parents=True, exist_ok=True)
    model_total = 0
    suffix_total = 0
    record_count = 0
    with output.open("w", newline="", encoding="utf-8") as target:
        writer = csv.writer(target, delimiter="\t", lineterminator="\n")
        writer.writerow(["configuration", *expected_header])
        for configuration, part in parts:
            if not part.is_file():
                continue
            with part.open(newline="", encoding="utf-8") as source:
                reader = csv.reader(source, delimiter="\t")
                header = next(reader, None)
                if header != expected_header:
                    raise ValueError(
                        f"invalid runtime timing header in {part}")
                for row in reader:
                    if len(row) != len(expected_header):
                        raise ValueError(
                            f"invalid runtime timing row in {part}: {row}")
                    writer.writerow([configuration, *row])
                    record_count += 1
                    if row[3] == "TOTAL":
                        if row[2] == "model":
                            model_total += int(row[6])
                        elif row[2] == "suffix_compile":
                            suffix_total += int(row[6])
    return model_total, suffix_total, record_count


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
    if args.case_start < 0:
        print("--case-start must be non-negative", file=sys.stderr)
        return 2
    if args.suffix_cache_mode != "read-write" and args.suffix_cache_dir is None:
        print("--suffix-cache-mode requires --suffix-cache-dir", file=sys.stderr)
        return 2
    if args.suffix_cache_dir is not None:
        try:
            args.suffix_compiler_sha256 = sha256_file(args.compiler)
        except OSError as error:
            print(f"cannot hash suffix compiler {args.compiler}: {error}",
                  file=sys.stderr)
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
    timing_parts: list[tuple[str, Path]] = []
    timing_context = (
        tempfile.TemporaryDirectory(prefix="cvub-matrix-timing-")
        if args.runtime_timing_output else contextlib.nullcontext(None)
    )
    with timing_context as timing_directory:
        for index, config in enumerate(configs, start=1):
            timing_part = (
                Path(timing_directory) / f"{index:03d}-{config.name}.tsv"
                if timing_directory else None
            )
            command = build_command(args, config, timing_part)
            print(f"[{index}/{len(configs)}] {config.name}", flush=True)
            print("  " + shlex.join(command), flush=True)
            if args.dry_run:
                continue
            completed = subprocess.run(command)
            executed += 1
            if timing_part is not None:
                timing_parts.append((config.name, timing_part))
            if completed.returncode != 0:
                failures = (required_failures if config.required
                            else experimental_failures)
                failures.append(config.name)
                classification = ("required" if config.required
                                  else "experimental")
                print(f"[{classification} failure] {config.name}",
                      file=sys.stderr)
                if (args.stop_on_failure and
                        (config.required or args.require_all_exact)):
                    break

        if args.runtime_timing_output and not args.dry_run:
            try:
                model_ns, suffix_ns, timing_records = merge_runtime_timings(
                    args.runtime_timing_output, timing_parts)
            except ValueError as error:
                print(f"failed to merge runtime timing: {error}",
                      file=sys.stderr)
                return 2
            ratio = suffix_ns / model_ns if model_ns else float("inf")
            print(
                "matrix runtime timing: "
                f"model_total_ms={model_ns / 1_000_000:.3f} "
                f"suffix_compile_total_ms={suffix_ns / 1_000_000:.3f} "
                f"suffix_over_model={ratio:.3f} "
                f"records={timing_records} "
                f"output={args.runtime_timing_output}"
            )

    if args.dry_run:
        seed_mode = ("retry" if args.retry_only else
                     (args.seeds if args.seeds is not None else
                      str(args.seed if args.seed is not None else 0)))
        print(
            f"dry run complete: {len(configs)} configuration(s), "
            f"seed={seed_mode}")
        return 0
    seed_mode = ("retry" if args.retry_only else
                 (args.seeds if args.seeds is not None else
                  str(args.seed if args.seed is not None else 0)))
    print(
        f"matrix summary: selected={len(configs)} executed={executed} "
        f"passed={executed - len(required_failures) - len(experimental_failures)} "
        f"required_failed={len(required_failures)} "
        f"experimental_failed={len(experimental_failures)} seed={seed_mode}")
    if required_failures:
        print("failed required configurations: " + ", ".join(required_failures),
              file=sys.stderr)
    if experimental_failures:
        print("failed experimental configurations: " +
              ", ".join(experimental_failures), file=sys.stderr)
    if required_failures or (args.require_all_exact and experimental_failures):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
