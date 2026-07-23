#!/usr/bin/env python3
"""Run corpus-wide real-compiler/final-PlanMemory differential validation."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from compare_ub_plan_with_suffix_oracle import (
    canonical_function_name,
    model_multi_and_inplace,
    normalized_lifetimes_from_model,
    parse_oracle,
    parse_oracle_contract,
    parse_oracle_retry,
    plan_multiset_from_model,
)
from cvpipeline_oracle_common import discover_inputs, safe_name


def parse_bool(text: str) -> bool:
    value = text.lower()
    if value in {"1", "true", "yes", "on"}:
        return True
    if value in {"0", "false", "no", "off"}:
        return False
    raise argparse.ArgumentTypeError(
        "expected one of: 0, 1, true, false, yes, no, on, off")


def positive_int(text: str) -> int:
    value = int(text)
    if value <= 0:
        raise argparse.ArgumentTypeError("expected a positive integer")
    return value


@dataclass(frozen=True)
class RuntimeTimingRecord:
    tool: str
    kind: str
    name: str
    occurrence: int
    nanoseconds: int


def parse_runtime_timing(stderr: str) -> list[RuntimeTimingRecord]:
    """Parse opt-in timing emitted by the model or suffix compiler itself."""
    records: list[RuntimeTimingRecord] = []
    for line in stderr.splitlines():
        fields = line.split("\t")
        if not fields or fields[0] != "CVPIPELINE_TIMING":
            continue
        if len(fields) != 7 or fields[1] != "1":
            raise ValueError(f"invalid CVPIPELINE_TIMING record: {line}")
        tool, kind, name = fields[2:5]
        if tool not in {"model", "suffix_compile"}:
            raise ValueError(f"unknown timing tool {tool!r}")
        if kind not in {"TOTAL", "STAGE", "PASS"}:
            raise ValueError(f"unknown timing kind {kind!r}")
        try:
            occurrence = int(fields[5])
            nanoseconds = int(fields[6])
        except ValueError as error:
            raise ValueError(
                f"non-integer CVPIPELINE_TIMING value: {line}") from error
        if occurrence < 0 or nanoseconds < 0:
            raise ValueError(f"negative CVPIPELINE_TIMING value: {line}")
        records.append(RuntimeTimingRecord(
            tool, kind, name, occurrence, nanoseconds))
    return records


def require_runtime_timing(
    stderr: str, expected_tool: str,
) -> list[RuntimeTimingRecord]:
    records = parse_runtime_timing(stderr)
    unexpected = {record.tool for record in records
                  if record.tool != expected_tool}
    if unexpected:
        raise ValueError(
            f"unexpected timing tool(s): {', '.join(sorted(unexpected))}")
    totals = [record for record in records if record.kind == "TOTAL"]
    if len(totals) != 1:
        raise ValueError(
            f"expected one {expected_tool} TOTAL timing record, got "
            f"{len(totals)}")
    return records


def write_runtime_timing(
    path: Path,
    rows: list[tuple[str, int, RuntimeTimingRecord]],
) -> dict[str, int]:
    """Write per-input/per-stage records and return summed tool totals."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow((
            "input", "seed", "tool", "kind", "name", "occurrence",
            "nanoseconds", "milliseconds",
        ))
        for input_name, seed, record in rows:
            writer.writerow((
                input_name,
                seed,
                record.tool,
                record.kind,
                record.name,
                record.occurrence,
                record.nanoseconds,
                f"{record.nanoseconds / 1_000_000:.6f}",
            ))
    totals: dict[str, int] = {}
    for _, _, record in rows:
        if record.kind == "TOTAL":
            totals[record.tool] = totals.get(record.tool, 0) + record.nanoseconds
    return totals


def print_runtime_timing_comparison(
    rows: list[tuple[str, int, RuntimeTimingRecord]],
) -> None:
    by_input: dict[tuple[str, int], dict[str, object]] = {}
    for input_name, seed, record in rows:
        case = by_input.setdefault((input_name, seed), {
            "totals": {},
            "steps": {"model": {}, "suffix_compile": {}},
            "counts": {"model": {}, "suffix_compile": {}},
            "model_order": [],
        })
        totals = case["totals"]
        steps = case["steps"]
        counts = case["counts"]
        if record.kind == "TOTAL":
            totals[record.tool] = record.nanoseconds
            continue
        tool_steps = steps[record.tool]
        tool_counts = counts[record.tool]
        if record.tool == "model" and record.name not in tool_steps:
            case["model_order"].append(record.name)
        tool_steps[record.name] = (
            tool_steps.get(record.name, 0) + record.nanoseconds)
        tool_counts[record.name] = tool_counts.get(record.name, 0) + 1
    if not by_input:
        return
    print("runtime timing comparison (same-name steps are aggregated):")
    for (input_name, seed), case in by_input.items():
        totals = case["totals"]
        steps = case["steps"]
        counts = case["counts"]
        model_steps = steps["model"]
        suffix_steps = steps["suffix_compile"]
        shared_names = [
            name for name in case["model_order"] if name in suffix_steps]
        model_ns = totals.get("model", 0)
        suffix_ns = totals.get("suffix_compile", 0)
        seed_text = "retry" if seed == -1 else str(seed)
        print(f"\n[{seed_text}] {input_name}")
        step_width = max(
            len("Step"),
            min(40, max((len(name) for name in shared_names), default=5)),
        )

        def short_name(name: str) -> str:
            if len(name) <= step_width:
                return name
            return name[:step_width - 3] + "..."

        header = (
            f"{'Step':<{step_width}} | {'M#':>3} | {'Model ms':>11} | "
            f"{'S#':>3} | {'Suffix ms':>11} | {'Delta ms':>11} | {'S/M':>8}")
        print(header)
        print("-" * len(header))

        def print_row(name: str, model_count: int, model_value: int,
                      suffix_count: int, suffix_value: int) -> None:
            delta_ms = (suffix_value - model_value) / 1_000_000
            ratio = suffix_value / model_value if model_value else float("inf")
            print(
                f"{short_name(name):<{step_width}} | {model_count:>3} | "
                f"{model_value / 1_000_000:>11.3f} | {suffix_count:>3} | "
                f"{suffix_value / 1_000_000:>11.3f} | {delta_ms:>11.3f} | "
                f"{ratio:>8.3f}")

        print_row("TOTAL", 1 if model_ns else 0, model_ns,
                  1 if suffix_ns else 0, suffix_ns)
        for name in shared_names:
            print_row(
                name,
                counts["model"][name],
                model_steps[name],
                counts["suffix_compile"][name],
                suffix_steps[name],
            )
        model_only = len(model_steps) - len(shared_names)
        suffix_only = len(suffix_steps) - len(shared_names)
        if model_only or suffix_only:
            print(
                f"unmapped: model-only stages={model_only}, "
                f"suffix-only passes={suffix_only}; see the TSV for details")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus-root", required=True, type=Path)
    parser.add_argument(
        "--input", action="append", type=Path, default=[], metavar="PATH",
        help=("Run only this input; repeatable. Relative paths are resolved "
              "against --corpus-root unless they exist from the current "
              "directory."),
    )
    parser.add_argument("--model", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument(
        "--suffix-cache-dir", type=Path,
        help=("Cache suffix oracle results by compiler, input contents, seed, "
              "and pipeline options"),
    )
    parser.add_argument(
        "--suffix-cache-mode",
        choices=("read-write", "read-only", "refresh"),
        default="read-write",
        help=("read-write fills misses (default); read-only never invokes "
              "suffix; refresh replaces matching entries"),
    )
    parser.add_argument(
        "--suffix-compiler-sha256",
        help=argparse.SUPPRESS,
    )
    parser.add_argument("--seeds", default="0-19")
    parser.add_argument("--restrict-inplace-as-isa", action="store_true")
    parser.add_argument("--disable-cv-pipelining", action="store_true")
    parser.add_argument("--cv-pipeline-depth", type=int, default=-1)
    parser.add_argument("--enable-preload", action="store_true")
    parser.add_argument("--enable-cv-lazy-loading", action="store_true")
    parser.add_argument("--enable-code-motion", type=parse_bool, default=True,
                        metavar="BOOL")
    parser.add_argument("--tile-mix-cube-loop", type=positive_int, default=2)
    parser.add_argument("--tile-mix-vector-loop", type=positive_int, default=2)
    parser.add_argument("--enable-ubuf-saving", action="store_true")
    parser.add_argument("--enable-auto-multi-buffer", action="store_true")
    parser.add_argument("--enable-triton-kernel-compile", action="store_true")
    parser.add_argument("--disable-align-alloc-size", action="store_true")
    parser.add_argument("--disable-enable-stride-align", action="store_true")
    parser.add_argument("--disable-infer-hivm-data-layout", action="store_true")
    parser.add_argument("--local-multi-buffer-strategy", default="no-l0c",
                        choices=("no-limit", "only-cube", "only-vector", "no-l0c"))
    parser.add_argument("--mix-multi-buffer-strategy", default="only-cube",
                        choices=("no-limit", "only-cube", "only-vector", "no-l0c"))
    parser.add_argument("--stage-snapshots-dir", type=Path)
    parser.add_argument("--pipeline-manifest", type=Path)
    parser.add_argument(
        "--runtime-timing-output", type=Path,
        help=("Enable internal runtime timing in both executables and write "
              "the emitted total/stage/pass records as TSV"),
    )
    timing_dump_group = parser.add_mutually_exclusive_group()
    timing_dump_group.add_argument(
        "--runtime-timing-exclude-dumps", action="store_true",
        dest="runtime_timing_exclude_dumps", default=True,
        help=("Measure suffix compilation in a separate invocation with "
              "oracle/debug dumps disabled (default); correctness still "
              "uses the normal dump-enabled invocation"),
    )
    timing_dump_group.add_argument(
        "--runtime-timing-include-dumps", action="store_false",
        dest="runtime_timing_exclude_dumps",
        help="Include suffix oracle/debug dump overhead in measured time",
    )
    parser.add_argument(
        "--suppress-runtime-timing-summary", action="store_true",
        help=argparse.SUPPRESS,
    )
    parser.add_argument(
        "--case-start", type=int, default=0,
        help="Skip this many selected corpus inputs before --max-cases",
    )
    parser.add_argument("--max-cases", type=int)
    parser.add_argument("--require-all-exact", action="store_true")
    parser.add_argument("--check-retry", action="store_true",
                        help="Also compare the default retry-selected plan")
    parser.add_argument(
        "--retry-only", action="store_true",
        help=("Skip fixed seeds and run only the default retry-selected "
              "PlanMemory plan"),
    )
    parser.add_argument("--quiet", action="store_true",
                        help="Only print the summary and failures")
    parser.add_argument("--no-progress", action="store_true",
                        help="Disable the interactive progress bar")
    return parser.parse_args()


def seeds(text: str) -> list[int]:
    result: set[int] = set()
    for token in text.split(","):
        token = token.strip()
        if "-" in token:
            first, last = (int(value) for value in token.split("-", 1))
            result.update(range(first, last + 1))
        elif token:
            result.add(int(token))
    if not result or min(result) < 0 or max(result) > 19:
        raise ValueError("--seeds must select values in [0, 19]")
    return sorted(result)


def select_inputs(corpus_root: Path, requested: list[Path]) -> list[Path]:
    root = corpus_root.resolve()
    if not requested:
        return discover_inputs(root)
    selected: list[Path] = []
    seen: set[Path] = set()
    for value in requested:
        candidate = value
        if not candidate.is_absolute() and not candidate.is_file():
            candidate = root / candidate
        candidate = candidate.resolve()
        try:
            candidate.relative_to(root)
        except ValueError as error:
            raise ValueError(
                f"--input must be inside --corpus-root: {value}") from error
        if not candidate.is_file():
            raise ValueError(f"--input is not a file: {value}")
        if candidate not in seen:
            selected.append(candidate)
            seen.add(candidate)
    return selected


def run(command: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, text=True, capture_output=True, env=env)


SUFFIX_CACHE_SCHEMA = 1


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


@dataclass
class SuffixCacheStats:
    hits: int = 0
    misses: int = 0
    writes: int = 0
    bypasses: int = 0


class SuffixResultCache:
    """Content-addressed cache for suffix oracle stdout/stderr and status."""

    def __init__(
        self,
        root: Path,
        compiler: Path,
        mode: str,
        compiler_digest: str | None = None,
    ) -> None:
        self.root = root
        self.mode = mode
        self.compiler_digest = (
            compiler_digest if compiler_digest is not None
            else sha256_file(compiler)
        )
        self.stats = SuffixCacheStats()
        self._input_digests: dict[Path, str] = {}

    def _input_digest(self, path: Path) -> str:
        resolved = path.resolve()
        digest = self._input_digests.get(resolved)
        if digest is None:
            digest = sha256_file(resolved)
            self._input_digests[resolved] = digest
        return digest

    @staticmethod
    def _normalized_arguments(
        command: list[str], input_path: Path,
    ) -> list[str]:
        """Remove temporary/output paths while retaining semantic options."""
        normalized: list[str] = []
        skip_next = False
        resolved_input = input_path.resolve()
        for argument in command[1:]:
            if skip_next:
                skip_next = False
                continue
            if argument == "-o":
                skip_next = True
                continue
            try:
                is_input = Path(argument).resolve() == resolved_input
            except (OSError, RuntimeError):
                is_input = False
            if is_input:
                normalized.append("<INPUT>")
            else:
                normalized.append(argument)
        return normalized

    def _key_and_record(
        self,
        command: list[str],
        input_path: Path,
        env: dict[str, str] | None,
    ) -> tuple[str, dict[str, Any]]:
        effective_env = os.environ if env is None else env
        cache_env = {
            name: effective_env.get(name)
            for name in (
                "BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS",
                "BISHENGIR_DUMP_BEFORE_PLAN_MEMORY",
            )
            if effective_env.get(name) is not None
        }
        identity: dict[str, Any] = {
            "schema": SUFFIX_CACHE_SCHEMA,
            "compiler_sha256": self.compiler_digest,
            "input_sha256": self._input_digest(input_path),
            "arguments": self._normalized_arguments(command, input_path),
            "environment": cache_env,
        }
        encoded = json.dumps(
            identity, sort_keys=True, separators=(",", ":")).encode("utf-8")
        return hashlib.sha256(encoded).hexdigest(), identity

    def _path(self, key: str) -> Path:
        return self.root / self.compiler_digest[:16] / key[:2] / f"{key}.json"

    def load_or_run(
        self,
        command: list[str],
        input_path: Path,
        *,
        env: dict[str, str] | None = None,
        allow_cache: bool = True,
    ) -> subprocess.CompletedProcess[str]:
        if not allow_cache:
            self.stats.bypasses += 1
            return run(command, env=env)

        key, identity = self._key_and_record(command, input_path, env)
        cache_path = self._path(key)
        if self.mode != "refresh":
            try:
                payload = json.loads(cache_path.read_text(encoding="utf-8"))
                if payload.get("identity") != identity:
                    raise ValueError("cache identity mismatch")
                returncode = int(payload["returncode"])
                stdout = str(payload["stdout"])
                stderr = str(payload["stderr"])
            except (OSError, ValueError, KeyError, TypeError,
                    json.JSONDecodeError):
                self.stats.misses += 1
            else:
                self.stats.hits += 1
                return subprocess.CompletedProcess(
                    command, returncode, stdout, stderr)
        else:
            self.stats.misses += 1

        if self.mode == "read-only":
            return subprocess.CompletedProcess(
                command,
                125,
                "",
                f"suffix cache miss: {cache_path}\n",
            )

        completed = run(command, env=env)
        cacheable_result = (
            completed.returncode == 0
            or re.search(
                r"\berror:\s+[A-Za-z][A-Za-z0-9_-]* overflow, requires\s+",
                completed.stderr,
                re.IGNORECASE,
            ) is not None
        )
        if not cacheable_result:
            return completed

        cache_path.parent.mkdir(parents=True, exist_ok=True)
        record = {
            "identity": identity,
            "returncode": completed.returncode,
            "stdout": completed.stdout,
            "stderr": completed.stderr,
        }
        temporary_path: Path | None = None
        try:
            with tempfile.NamedTemporaryFile(
                mode="w",
                encoding="utf-8",
                dir=cache_path.parent,
                prefix=f".{key}.",
                suffix=".tmp",
                delete=False,
            ) as stream:
                temporary_path = Path(stream.name)
                json.dump(record, stream, sort_keys=True, separators=(",", ":"))
                stream.write("\n")
            os.replace(temporary_path, cache_path)
            self.stats.writes += 1
        finally:
            if temporary_path is not None:
                try:
                    temporary_path.unlink()
                except FileNotFoundError:
                    pass
        return completed


def run_suffix(
    command: list[str], *, env: dict[str, str] | None = None,
    exclude_dumps_from_timing: bool = False,
    cache: SuffixResultCache | None = None,
    input_path: Path | None = None,
    allow_cache: bool = True,
) -> tuple[subprocess.CompletedProcess[str], subprocess.CompletedProcess[str]]:
    """Run the oracle command and, when requested, a clean timing invocation."""
    if cache is not None:
        if input_path is None:
            raise ValueError("input_path is required when suffix cache is enabled")
        compiled = cache.load_or_run(
            command, input_path, env=env, allow_cache=allow_cache)
    else:
        compiled = run(command, env=env)
    if not exclude_dumps_from_timing:
        return compiled, compiled

    timing_command = [
        argument for argument in command
        if argument != "--show-runtime-timing"
        and not argument.startswith("--dump-stage-oracle-dir=")
        and not argument.startswith("--dump-pipeline-stage-manifest=")
    ]
    timing_command.extend((
        "--show-runtime-timing", "--runtime-timing-exclude-dumps"))
    timing_env = dict(os.environ if env is None else env)
    timing_env.pop("BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS", None)
    timing_env.pop("BISHENGIR_DUMP_BEFORE_PLAN_MEMORY", None)
    return compiled, run(timing_command, env=timing_env)


def bool_text(value: bool) -> str:
    return "true" if value else "false"


def overflow_address_space(stderr: str) -> str | None:
    """Return the compiler address space named by an overflow diagnostic."""
    match = re.search(
        r"\berror:\s+([A-Za-z][A-Za-z0-9_-]*) overflow, requires\s+",
        stderr,
        re.IGNORECASE,
    )
    return match.group(1).lower() if match else None


def is_ub_overflow(address_space: str | None) -> bool:
    """Accept both spellings emitted by supported compiler revisions."""
    return address_space in {"ub", "ubuf"}


def shared_pipeline_options(args: argparse.Namespace) -> list[str]:
    return [
        f"--disable-cv-pipelining={bool_text(args.disable_cv_pipelining)}",
        f"--cv-pipeline-depth={args.cv_pipeline_depth}",
        f"--enable-preload={bool_text(args.enable_preload)}",
        f"--enable-cv-lazy-loading={bool_text(args.enable_cv_lazy_loading)}",
        f"--enable-code-motion={bool_text(args.enable_code_motion)}",
        f"--tile-mix-cube-loop={args.tile_mix_cube_loop}",
        f"--tile-mix-vector-loop={args.tile_mix_vector_loop}",
        f"--enable-ubuf-saving={bool_text(args.enable_ubuf_saving)}",
        f"--enable-auto-multi-buffer={bool_text(args.enable_auto_multi_buffer)}",
        f"--enable-triton-kernel-compile={bool_text(args.enable_triton_kernel_compile)}",
        f"--disable-align-alloc-size={bool_text(args.disable_align_alloc_size)}",
        f"--disable-enable-stride-align={bool_text(args.disable_enable_stride_align)}",
        f"--disable-infer-hivm-data-layout={bool_text(args.disable_infer_hivm_data_layout)}",
        f"--limit-auto-multi-buffer-of-local-buffer={args.local_multi_buffer_strategy}",
        f"--limit-auto-multi-buffer-buffer={args.mix_multi_buffer_strategy}",
    ]


class ProgressBar:
    def __init__(self, total: int, enabled: bool) -> None:
        self.total = max(total, 1)
        self.enabled = enabled
        self.started = time.monotonic()
        self.last_width = 0

    @staticmethod
    def format_duration(seconds: float) -> str:
        seconds = max(int(seconds), 0)
        minutes, seconds = divmod(seconds, 60)
        hours, minutes = divmod(minutes, 60)
        if hours:
            return f"{hours:d}:{minutes:02d}:{seconds:02d}"
        return f"{minutes:02d}:{seconds:02d}"

    def update(self, completed: int, label: str) -> None:
        if not self.enabled:
            return
        completed = min(max(completed, 0), self.total)
        ratio = completed / self.total
        width = 24
        filled = min(int(ratio * width), width)
        bar = "#" * filled + "-" * (width - filled)
        elapsed = time.monotonic() - self.started
        eta = (elapsed * (self.total - completed) / completed
               if completed else 0.0)
        timing = (f"elapsed {self.format_duration(elapsed)}"
                  if completed == 0 else
                  f"ETA {self.format_duration(eta)}")
        line = (f"[{bar}] {completed:>{len(str(self.total))}}/{self.total} "
                f"{ratio * 100:6.2f}% {timing}  {label}")
        columns = shutil.get_terminal_size(fallback=(120, 24)).columns
        line = line[:max(columns - 1, 20)]
        padding = " " * max(self.last_width - len(line), 0)
        sys.stderr.write(f"\r{line}{padding}")
        sys.stderr.flush()
        self.last_width = len(line)

    def newline(self) -> None:
        if self.enabled and self.last_width:
            sys.stderr.write("\n")
            sys.stderr.flush()
            self.last_width = 0

    def finish(self) -> None:
        self.update(self.total, "complete")
        self.newline()


def main() -> int:
    args = parse_args()
    if args.cv_pipeline_depth < -1:
        print("--cv-pipeline-depth must be -1 or non-negative", file=sys.stderr)
        return 2
    if args.retry_only and args.check_retry:
        print("--retry-only cannot be combined with --check-retry",
              file=sys.stderr)
        return 2
    if args.suffix_cache_mode != "read-write" and args.suffix_cache_dir is None:
        print("--suffix-cache-mode requires --suffix-cache-dir", file=sys.stderr)
        return 2
    if (args.suffix_compiler_sha256 is not None and
            re.fullmatch(r"[0-9a-f]{64}", args.suffix_compiler_sha256) is None):
        print("--suffix-compiler-sha256 must be 64 lowercase hex digits",
              file=sys.stderr)
        return 2
    if args.case_start < 0:
        print("--case-start must be non-negative", file=sys.stderr)
        return 2
    if args.max_cases is not None and args.max_cases <= 0:
        print("--max-cases must be positive", file=sys.stderr)
        return 2
    selected_seeds = [] if args.retry_only else seeds(args.seeds)
    run_retry = args.check_retry or args.retry_only
    try:
        inputs = select_inputs(args.corpus_root, args.input)
    except ValueError as error:
        print(error, file=sys.stderr)
        return 2
    inputs = inputs[args.case_start:]
    if args.max_cases is not None:
        inputs = inputs[:args.max_cases]
    if not inputs:
        print("no before_cvpipelining_*.mlir inputs found", file=sys.stderr)
        return 1
    for executable in (args.model, args.compiler):
        if not executable.is_file() or not os.access(executable, os.X_OK):
            print(f"not an executable: {executable}", file=sys.stderr)
            return 1

    suffix_cache = (
        SuffixResultCache(
            args.suffix_cache_dir.resolve(),
            args.compiler.resolve(),
            args.suffix_cache_mode,
            args.suffix_compiler_sha256,
        )
        if args.suffix_cache_dir is not None else None
    )
    cache_safe_for_run = (
        args.runtime_timing_output is None
        and args.stage_snapshots_dir is None
        and args.pipeline_manifest is None
    )

    failures: list[str] = []
    blockers = 0
    reported_exact = 0
    matched = 0
    invalid = 0
    oracle_unavailable = 0
    unavailable_reasons: list[str] = []
    timing_rows: list[tuple[str, int, RuntimeTimingRecord]] = []
    manifest_written = False
    tasks_per_case = len(selected_seeds) + int(run_retry)
    progress = ProgressBar(
        len(inputs) * tasks_per_case,
        not args.no_progress and sys.stderr.isatty())
    progress.update(0, "starting")
    with tempfile.TemporaryDirectory(prefix="cvub-corpus-oracle-") as temp:
        temp_root = Path(temp)
        for case_index, input_path in enumerate(inputs):
            failure_count_before_case = len(failures)
            relative = input_path.relative_to(args.corpus_root.resolve())
            case_name = safe_name(str(relative))
            case_was_blocker = False
            case_reported_exact = True
            case_oracle_unavailable = False
            case_task_base = case_index * tasks_per_case
            for seed_index, seed in enumerate(selected_seeds):
                progress.update(
                    case_task_base + seed_index,
                    f"seed={seed} {relative}")
                model_command = [
                    str(args.model),
                    f"--before-cvpipelining-ir={input_path}",
                    f"--random-seed={seed}",
                    "--format=json",
                    *shared_pipeline_options(args),
                ]
                if args.restrict_inplace_as_isa:
                    model_command.append("--restrict-inplace-as-isa")
                if args.runtime_timing_output:
                    model_command.append("--show-runtime-timing")
                modeled = run(model_command)
                if args.runtime_timing_output:
                    try:
                        timing_rows.extend(
                            (str(relative), seed, record)
                            for record in require_runtime_timing(
                                modeled.stderr, "model")
                        )
                    except ValueError as error:
                        failures.append(f"{relative} seed {seed}: {error}")
                try:
                    payload = json.loads(modeled.stdout)
                except json.JSONDecodeError:
                    case_reported_exact = False
                    failures.append(
                        f"{relative} seed {seed}: model did not emit JSON: {modeled.stderr.strip()}")
                    break

                if payload.get("precision") != "exact":
                    case_reported_exact = False
                    if modeled.returncode != 1 or \
                            payload.get("status") != "blocker" or \
                            payload.get("ub_peak_bits") is not None or \
                            payload.get("required_bits") is not None or \
                            payload.get("functions") != []:
                        failures.append(
                            f"{relative} seed {seed}: incomplete report violates blocker contract")
                    # A blocker is safe, but the corpus gate must still prove
                    # the real pipeline accepts the input and emit its stage
                    # snapshots for coverage auditing.
                    if seed == selected_seeds[0]:
                        blocker_output = temp_root / f"{case_name}.blocker.mlir"
                        blocker_command = [
                            str(args.compiler), str(input_path), "-o",
                            str(blocker_output), f"--plan-memory-seed={seed}",
                            "--mlir-disable-threading",
                            *shared_pipeline_options(args),
                        ]
                        if args.restrict_inplace_as_isa:
                            blocker_command.append("--restrict-inplace-as-isa")
                        if (args.runtime_timing_output and
                                not args.runtime_timing_exclude_dumps):
                            blocker_command.extend((
                                "--show-runtime-timing",
                                "--runtime-timing-include-dumps"))
                        if args.stage_snapshots_dir:
                            blocker_command.append(
                                f"--dump-stage-oracle-dir={args.stage_snapshots_dir / case_name}")
                        if not manifest_written and args.pipeline_manifest:
                            blocker_command.append(
                                f"--dump-pipeline-stage-manifest={args.pipeline_manifest}")
                            manifest_written = True
                        compiled_blocker, timed_blocker = run_suffix(
                            blocker_command,
                            cache=suffix_cache,
                            input_path=input_path,
                            allow_cache=cache_safe_for_run,
                            exclude_dumps_from_timing=
                            bool(args.runtime_timing_output) and
                            args.runtime_timing_exclude_dumps)
                        if args.runtime_timing_output:
                            try:
                                timing_rows.extend(
                                    (str(relative), seed, record)
                                    for record in require_runtime_timing(
                                        timed_blocker.stderr,
                                        "suffix_compile")
                                )
                            except ValueError as error:
                                failures.append(
                                    f"{relative} seed {seed}: {error}")
                        expected_capacity_failure = re.search(
                            r"error: [^\n]* overflow, requires ",
                            compiled_blocker.stderr) is not None
                        if (compiled_blocker.returncode != 0 and
                                not expected_capacity_failure):
                            failures.append(
                                f"{relative}: real compiler failed on model blocker: "
                                f"{compiled_blocker.stderr[-2000:]}")
                    if seed != selected_seeds[0] and not case_was_blocker:
                        failures.append(
                            f"{relative} seed {seed}: precision changed from "
                            "exact to blocker across seeds")
                    case_was_blocker = True
                    # Blocker safety is a per-seed contract.  Continue running
                    # the model for every requested seed; only the expensive
                    # real compiler acceptance/snapshot check is seed 0.
                    continue

                if case_was_blocker:
                    failures.append(
                        f"{relative} seed {seed}: precision changed from "
                        "blocker to exact across seeds")
                    continue
                expected_model_rc = 2 if payload.get("status") == "overflow" else 0
                if modeled.returncode != expected_model_rc:
                    failures.append(
                        f"{relative} seed {seed}: exact {payload.get('status')} "
                        f"returned {modeled.returncode}, expected {expected_model_rc}")
                    break

                oracle_output = temp_root / f"{case_name}.{seed}.mlir"
                compiler_command = [
                    str(args.compiler), str(input_path), "-o", str(oracle_output),
                    f"--plan-memory-seed={seed}", "--mlir-disable-threading",
                    "--ub-oracle-only",
                    *shared_pipeline_options(args),
                ]
                if args.restrict_inplace_as_isa:
                    compiler_command.append("--restrict-inplace-as-isa")
                if (args.runtime_timing_output and
                        not args.runtime_timing_exclude_dumps):
                    compiler_command.extend((
                        "--show-runtime-timing",
                        "--runtime-timing-include-dumps"))
                if seed == selected_seeds[0] and args.stage_snapshots_dir:
                    stage_dir = args.stage_snapshots_dir / case_name
                    compiler_command.append(f"--dump-stage-oracle-dir={stage_dir}")
                if not manifest_written and args.pipeline_manifest:
                    compiler_command.append(
                        f"--dump-pipeline-stage-manifest={args.pipeline_manifest}")
                    manifest_written = True
                oracle_env = dict(os.environ)
                oracle_env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
                compiled, timed_compiled = run_suffix(
                    compiler_command, env=oracle_env,
                    cache=suffix_cache,
                    input_path=input_path,
                    allow_cache=cache_safe_for_run,
                    exclude_dumps_from_timing=
                    bool(args.runtime_timing_output) and
                    args.runtime_timing_exclude_dumps)
                if args.runtime_timing_output:
                    try:
                        timing_rows.extend(
                            (str(relative), seed, record)
                            for record in require_runtime_timing(
                                timed_compiled.stderr, "suffix_compile")
                        )
                    except ValueError as error:
                        failures.append(f"{relative} seed {seed}: {error}")
                overflow_space = overflow_address_space(compiled.stderr)
                if (compiled.returncode != 0 and overflow_space is not None and
                        not is_ub_overflow(overflow_space)):
                    case_oracle_unavailable = True
                    unavailable_reasons.append(
                        f"{relative} seed {seed}: real compiler stopped on "
                        f"{overflow_space} overflow before a UB oracle was available")
                    break
                if (compiled.returncode != 0 and
                        not (payload.get("status") == "overflow" and
                             is_ub_overflow(overflow_space))):
                    failures.append(
                        f"{relative} seed {seed}: real compiler failed: {compiled.stderr[-2000:]}")
                    break
                oracle_tsv = temp_root / f"{case_name}.{seed}.oracle.tsv"
                oracle_tsv.write_text(compiled.stderr, encoding="utf-8")
                _, oracle_peak, oracle_plan, oracle_lifetimes = parse_oracle(
                    oracle_tsv, seed, "6")
                oracle_status, oracle_required, oracle_multi, oracle_inplace = (
                    parse_oracle_contract(oracle_tsv, seed, "6"))
                model_peak = int(payload["ub_peak_bits"])
                model_plan = plan_multiset_from_model(payload)
                model_lifetimes = normalized_lifetimes_from_model(payload)
                model_multi, model_inplace = model_multi_and_inplace(payload)
                differences = []
                if oracle_status != payload.get("status"):
                    differences.append("status")
                if oracle_required != int(payload["required_bits"]):
                    differences.append("required")
                if oracle_peak != model_peak:
                    differences.append("peak")
                if oracle_plan != model_plan:
                    differences.append("plan")
                if oracle_lifetimes != model_lifetimes:
                    differences.append("lifetime")
                if oracle_multi != model_multi:
                    differences.append("multi")
                if oracle_inplace != model_inplace:
                    differences.append("inplace")
                if differences:
                    failures.append(
                        f"{relative} seed {seed}: {','.join(differences)} differ "
                        f"(model peak={model_peak}, oracle peak={oracle_peak}, "
                        f"model buffers={sum(model_plan.values())}, "
                        f"oracle buffers={sum(oracle_plan.values())})")
                    break
            if not case_was_blocker and run_retry:
                progress.update(
                    case_task_base + len(selected_seeds),
                    f"retry {relative}")
                retry_model_command = [
                    str(args.model),
                    f"--before-cvpipelining-ir={input_path}", "--format=json",
                    *shared_pipeline_options(args),
                ]
                if args.restrict_inplace_as_isa:
                    retry_model_command.append("--restrict-inplace-as-isa")
                if args.runtime_timing_output:
                    retry_model_command.append("--show-runtime-timing")
                modeled_retry = run(retry_model_command)
                if args.runtime_timing_output:
                    try:
                        timing_rows.extend(
                            (str(relative), -1, record)
                            for record in require_runtime_timing(
                                modeled_retry.stderr, "model")
                        )
                    except ValueError as error:
                        failures.append(f"{relative} retry: {error}")
                try:
                    retry_payload = json.loads(modeled_retry.stdout)
                except json.JSONDecodeError:
                    failures.append(
                        f"{relative} retry: model did not emit JSON: "
                        f"{modeled_retry.stderr.strip()}")
                else:
                    if retry_payload.get("precision") != "exact":
                        if args.retry_only:
                            case_reported_exact = False
                            case_was_blocker = True
                            if (modeled_retry.returncode != 1 or
                                    retry_payload.get("status") != "blocker" or
                                    retry_payload.get("ub_peak_bits") is not None or
                                    retry_payload.get("required_bits") is not None or
                                    retry_payload.get("functions") != []):
                                failures.append(
                                    f"{relative} retry: incomplete report "
                                    "violates blocker contract")
                        else:
                            failures.append(
                                f"{relative} retry: fixed-seed Exact became "
                                "blocker")
                    else:
                        expected_retry_rc = (
                            2 if retry_payload.get("status") == "overflow"
                            else 0)
                        if modeled_retry.returncode != expected_retry_rc:
                            failures.append(
                                f"{relative} retry: exact "
                                f"{retry_payload.get('status')} returned "
                                f"{modeled_retry.returncode}, expected "
                                f"{expected_retry_rc}")
                        retry_output = temp_root / f"{case_name}.retry.mlir"
                        retry_compiler_command = [
                            str(args.compiler), str(input_path), "-o",
                            str(retry_output), "--plan-memory-seed=-1",
                            "--mlir-disable-threading", "--ub-oracle-only",
                            *shared_pipeline_options(args),
                        ]
                        if args.restrict_inplace_as_isa:
                            retry_compiler_command.append("--restrict-inplace-as-isa")
                        if (args.runtime_timing_output and
                                not args.runtime_timing_exclude_dumps):
                            retry_compiler_command.extend((
                                "--show-runtime-timing",
                                "--runtime-timing-include-dumps"))
                        oracle_env = dict(os.environ)
                        oracle_env["BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS"] = "1"
                        compiled_retry, timed_retry = run_suffix(
                            retry_compiler_command, env=oracle_env,
                            cache=suffix_cache,
                            input_path=input_path,
                            allow_cache=cache_safe_for_run,
                            exclude_dumps_from_timing=
                            bool(args.runtime_timing_output) and
                            args.runtime_timing_exclude_dumps)
                        if args.runtime_timing_output:
                            try:
                                timing_rows.extend(
                                    (str(relative), -1, record)
                                    for record in require_runtime_timing(
                                        timed_retry.stderr,
                                        "suffix_compile")
                                )
                            except ValueError as error:
                                failures.append(f"{relative} retry: {error}")
                        retry_overflow_space = overflow_address_space(
                            compiled_retry.stderr)
                        if (compiled_retry.returncode != 0 and
                                not (retry_payload.get("status") == "overflow" and
                                     is_ub_overflow(retry_overflow_space))):
                            failures.append(
                                f"{relative} retry: real compiler failed: "
                                f"{compiled_retry.stderr[-2000:]}")
                        else:
                            retry_oracle = temp_root / f"{case_name}.retry.tsv"
                            retry_oracle.write_text(compiled_retry.stderr,
                                                    encoding="utf-8")
                            selected_by_function, retry_peak, retry_plan, retry_lifetimes = (
                                parse_oracle_retry(retry_oracle, "6"))
                            model_selected = {
                                canonical_function_name(str(function["function"])):
                                int(function["selected_seed"])
                                for function in retry_payload.get("functions", [])
                                if (function.get("buffers") or
                                    int(function.get("ub_peak_bits", 0)) != 0)
                            }
                            if (retry_peak != int(retry_payload["ub_peak_bits"]) or
                                    retry_plan != plan_multiset_from_model(retry_payload) or
                                    retry_lifetimes != normalized_lifetimes_from_model(
                                        retry_payload) or
                                    selected_by_function != model_selected):
                                failures.append(
                                    f"{relative} retry: selected-seed/peak/plan/"
                                    "lifetime differ")
            if case_oracle_unavailable:
                oracle_unavailable += 1
            elif case_was_blocker:
                blockers += 1
            elif case_reported_exact:
                reported_exact += 1
                if len(failures) == failure_count_before_case:
                    matched += 1
            else:
                invalid += 1
            status = ("ORACLE_UNAVAILABLE" if case_oracle_unavailable else
                      "BLOCKER" if case_was_blocker else
                      "INVALID" if not case_reported_exact else
                      "MATCHED" if len(failures) == failure_count_before_case else
                      "MISMATCH")
            progress.update((case_index + 1) * tasks_per_case,
                            f"{status} {relative.name}")
            if not args.quiet:
                progress.newline()
                print(f"[{case_index + 1}/{len(inputs)}] {status} {relative}")

    progress.finish()
    if args.require_all_exact and blockers:
        failures.append(f"{blockers} corpus inputs returned blocker")
    print(
        f"summary: inputs={len(inputs)} reported_exact={reported_exact} "
        f"matched={matched} blockers={blockers} invalid={invalid} "
        f"oracle_unavailable={oracle_unavailable} failures={len(failures)}")
    if suffix_cache is not None:
        stats = suffix_cache.stats
        print(
            "suffix cache: "
            f"mode={args.suffix_cache_mode} hits={stats.hits} "
            f"misses={stats.misses} writes={stats.writes} "
            f"bypasses={stats.bypasses} dir={args.suffix_cache_dir}"
        )
    if args.runtime_timing_output:
        totals = write_runtime_timing(args.runtime_timing_output, timing_rows)
        model_ns = totals.get("model", 0)
        suffix_ns = totals.get("suffix_compile", 0)
        ratio = suffix_ns / model_ns if model_ns else float("inf")
        if not args.suppress_runtime_timing_summary:
            print(
                "runtime timing: "
                f"model_total_ms={model_ns / 1_000_000:.3f} "
                f"suffix_compile_total_ms={suffix_ns / 1_000_000:.3f} "
                f"suffix_over_model={ratio:.3f} "
                f"records={len(timing_rows)} "
                f"output={args.runtime_timing_output}"
            )
            print_runtime_timing_comparison(timing_rows)
    for reason in unavailable_reasons:
        print(f"[UNAVAILABLE] {reason}", file=sys.stderr)
    for failure in failures:
        print(f"[FAIL] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
