#!/usr/bin/env python3
"""Compare lightweight pre-CV MarkMultiBuffer with one native attempt."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import asdict, dataclass
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
PROFILE_ROOT = ROOT / "ub_overflow_model_cpp/config/pre_cv_profiles"
DEFAULT_PROFILES = (
    "base_default",
    "cv_manage_off",
    "mb_default_ws4",
    "mb_default_ws2",
    "mb_local_only",
    "mb_l0c_ws4",
    "mb_vector_ws4",
    "mb_unrestricted_ws4",
)


@dataclass
class Result:
    adapter: str
    profile: str
    status: str
    detail: str = ""
    artifacts: str = ""


def run(command: list[str], environment: dict[str, str] | None = None):
    return subprocess.run(
        command,
        cwd=ROOT,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def discover_adapters(adapter_root: Path, selection_root: Path) -> list[Path]:
    names = sorted(path.name for path in selection_root.iterdir() if path.is_dir())
    adapters = [adapter_root / name for name in names]
    missing = [path for path in adapters if not path.is_file()]
    if missing:
        raise RuntimeError(
            "missing adapters selected by before-CV corpus: "
            + ", ".join(str(path) for path in missing)
        )
    return adapters


def read_profile(name: str) -> list[str]:
    path = PROFILE_ROOT / f"{name}.args"
    if not path.is_file():
        raise RuntimeError(f"unknown pre-CV profile: {name}")
    return [
        line.strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]


def option_value(arguments: list[str], name: str, default: str) -> str:
    prefix = f"--{name}="
    for argument in arguments:
        if argument.startswith(prefix):
            return argument[len(prefix) :]
        if argument == f"--{name}":
            return "true"
    return default


def runner_arguments(profile_arguments: list[str]) -> list[str]:
    result: list[str] = []
    if option_value(profile_arguments, "enable-auto-multi-buffer", "false") == "true":
        result.append("--enable-auto-multi-buffer")
    if (
        option_value(
            profile_arguments, "disable-auto-cv-work-space-manage", "false"
        )
        == "true"
    ):
        result.append("--disable-auto-cv-workspace-manage")
    if (
        option_value(
            profile_arguments,
            "limit-auto-multi-buffer-only-for-local-buffer",
            "false",
        )
        == "true"
    ):
        result.append("--local-only")
    result.extend(
        [
            "--local-strategy="
            + option_value(
                profile_arguments,
                "limit-auto-multi-buffer-of-local-buffer",
                "no-l0c",
            ),
            "--mix-strategy="
            + option_value(
                profile_arguments,
                "limit-auto-multi-buffer-buffer",
                "only-cube",
            ),
            "--workspace-multi-buffer="
            + option_value(
                profile_arguments, "set-workspace-multibuffer", "4"
            ),
        ]
    )
    return result


def preserve_failure(work: Path, root: Path | None, ordinal: int) -> str:
    if root is None:
        return ""
    destination = root / f"{ordinal:05d}_{work.name}"
    shutil.copytree(work, destination)
    return str(destination)


def verify_one(
    ordinal: int,
    adapter: Path,
    profile: str,
    compiler: Path,
    runner: Path,
    temporary_root: Path,
    failure_root: Path | None,
) -> Result:
    work = temporary_root / f"case_{ordinal:05d}_{profile}_{adapter.name}"
    checkpoints = work / "checkpoints"
    work.mkdir()
    profile_arguments = read_profile(profile)
    environment = os.environ.copy()
    environment["BISHENGIR_UB_PREFIX_CHECKPOINT_DIR"] = str(checkpoints)
    environment["BISHENGIR_STOP_AFTER_UB_PREFIX_CHECKPOINTS"] = "1"
    environment.pop("BISHENGIR_UB_PREFIX_CHECKPOINT_STAGE", None)
    native = run(
        [
            str(compiler),
            str(adapter),
            *profile_arguments,
            "--enable-auto-blockify-loop=true",
            "-o",
            str(work / "unused.o"),
        ],
        environment,
    )
    attempt = checkpoints / "attempt-1"
    before_auto = attempt / "00_before_auto_blockify.mlir"
    after_auto = attempt / "01_after_auto_blockify.mlir"
    after_mark = attempt / "02_after_pre_cv_mark_multi_buffer.mlir"
    if (
        native.returncode != 0
        or not before_auto.is_file()
        or not after_auto.is_file()
        or not after_mark.is_file()
    ):
        artifacts = preserve_failure(work, failure_root, ordinal)
        diagnostic = (native.stderr or native.stdout).strip()[-2000:]
        return Result(str(adapter), profile, "NATIVE_FAILED", diagnostic, artifacts)

    mark_arguments = runner_arguments(profile_arguments)
    cumulative = run(
        [str(runner), "--apply-model", *mark_arguments, str(before_auto)]
    )
    single = run(
        [
            str(runner),
            "--apply-model",
            "--disable-auto-blockify-loop",
            *mark_arguments,
            str(after_auto),
        ]
    )
    oracle = run([str(runner), str(after_mark)])
    for label, completed in (("cumulative", cumulative), ("single", single)):
        if completed.returncode != 0:
            artifacts = preserve_failure(work, failure_root, ordinal)
            return Result(
                str(adapter),
                profile,
                "MODEL_FAILED",
                f"{label}: {completed.stderr.strip()[-1800:]}",
                artifacts,
            )
    if oracle.returncode != 0:
        artifacts = preserve_failure(work, failure_root, ordinal)
        return Result(
            str(adapter),
            profile,
            "ORACLE_SNAPSHOT_FAILED",
            oracle.stderr.strip()[-1800:],
            artifacts,
        )
    if cumulative.stdout != oracle.stdout or single.stdout != oracle.stdout:
        (work / "cumulative.snapshot.tsv").write_text(
            cumulative.stdout, encoding="utf-8"
        )
        (work / "single.snapshot.tsv").write_text(single.stdout, encoding="utf-8")
        (work / "native.snapshot.tsv").write_text(oracle.stdout, encoding="utf-8")
        artifacts = preserve_failure(work, failure_root, ordinal)
        mismatches = []
        if cumulative.stdout != oracle.stdout:
            mismatches.append("cumulative")
        if single.stdout != oracle.stdout:
            mismatches.append("single")
        return Result(
            str(adapter),
            profile,
            "MISMATCH",
            ",".join(mismatches),
            artifacts,
        )
    return Result(str(adapter), profile, "PASS")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adapter-root", default="ub_overflow_model_cpp/data/adapter")
    parser.add_argument(
        "--selection-root", default="ub_overflow_model_cpp/data/before_cvpipelining"
    )
    parser.add_argument("--compiler", default="build/bin/bishengir-compile")
    parser.add_argument(
        "--model-runner",
        default="ub_overflow_model_cpp/output/tests/pre_cv_prefix_model_runner",
    )
    parser.add_argument("--profiles", default=",".join(DEFAULT_PROFILES))
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--limit", type=int)
    parser.add_argument("--failure-dir")
    parser.add_argument("--json-report")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    compiler = Path(arguments.compiler).resolve()
    runner = Path(arguments.model_runner).resolve()
    if not compiler.is_file() or not runner.is_file():
        raise RuntimeError("compiler and model runner must already be built")
    adapters = discover_adapters(
        Path(arguments.adapter_root), Path(arguments.selection_root)
    )
    if arguments.limit is not None:
        adapters = adapters[: arguments.limit]
    profiles = [item for item in arguments.profiles.split(",") if item]
    for profile in profiles:
        read_profile(profile)
    if arguments.jobs < 1:
        raise RuntimeError("--jobs must be positive")
    failure_root = Path(arguments.failure_dir).resolve() if arguments.failure_dir else None
    if failure_root:
        failure_root.mkdir(parents=True, exist_ok=True)

    cases = [
        (adapter.resolve(), profile)
        for profile in profiles
        for adapter in adapters
    ]
    results: list[Result | None] = [None] * len(cases)
    with tempfile.TemporaryDirectory(prefix="pre-cv-mark-mb-") as temp:
        temporary_root = Path(temp)
        with ThreadPoolExecutor(max_workers=arguments.jobs) as executor:
            future_to_ordinal = {
                executor.submit(
                    verify_one,
                    ordinal,
                    adapter,
                    profile,
                    compiler,
                    runner,
                    temporary_root,
                    failure_root,
                ): ordinal
                for ordinal, (adapter, profile) in enumerate(cases)
            }
            completed_count = 0
            for future in as_completed(future_to_ordinal):
                ordinal = future_to_ordinal[future]
                results[ordinal] = future.result()
                completed_count += 1
                result = results[ordinal]
                assert result is not None
                print(
                    f"[{completed_count}/{len(cases)}] {result.status} "
                    f"{result.profile} {Path(result.adapter).name}",
                    flush=True,
                )

    concrete = [result for result in results if result is not None]
    counts: dict[str, int] = {}
    for result in concrete:
        counts[result.status] = counts.get(result.status, 0) + 1
        if result.status != "PASS":
            suffix = f": {result.detail}" if result.detail else ""
            print(
                f"[{result.status}] {result.profile} {result.adapter}{suffix} "
                f"{result.artifacts}"
            )
    report = {
        "compiler": str(compiler),
        "model_runner": str(runner),
        "profiles": profiles,
        "total": len(concrete),
        "counts": counts,
        "results": [asdict(result) for result in concrete],
    }
    if arguments.json_report:
        report_path = Path(arguments.json_report)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"summary: {json.dumps(counts, sort_keys=True)}")
    return 0 if counts == {"PASS": len(concrete)} else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"verify_pre_cv_mark_multi_buffer_pipeline: {error}", file=os.sys.stderr)
        raise SystemExit(2)
