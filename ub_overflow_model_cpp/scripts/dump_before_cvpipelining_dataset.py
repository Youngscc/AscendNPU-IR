#!/usr/bin/env python3
"""Regenerate before-CVPipelining bytecode from the real BiSheng compiler.

The semantic front-end options are read from an argument file.  The production
validation dump pass writes MLIR bytecode at the exact CVPipelining boundary;
this preserves SSA use-list order that textual MLIR cannot represent.  A
failed compile after that boundary does not invalidate an already captured
snapshot.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys
import time
from typing import Iterable


MODULE_DIR = Path(__file__).resolve().parents[1]
REPO_ROOT = MODULE_DIR.parent
DEFAULT_ARGS_FILE = MODULE_DIR / "config/bisheng_frontend_to_cvpipeline.args"
DEFAULT_ADAPTER_ROOT = MODULE_DIR / "data/adapter"
DEFAULT_OUTPUT_ROOT = REPO_ROOT / "Output/before_cvpipelining_regenerated"
DEFAULT_COMPILER = REPO_ROOT / "build/bin/bishengir-compile"


def parse_command_line() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compile .ttadapter files with the real BiSheng pipeline and "
            "capture the IR immediately before CVPipelining."
        )
    )
    parser.add_argument("--adapter-root", type=Path, default=DEFAULT_ADAPTER_ROOT)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--compiler", type=Path, default=DEFAULT_COMPILER)
    parser.add_argument("--frontend-args", type=Path, default=DEFAULT_ARGS_FILE)
    parser.add_argument(
        "--input",
        action="append",
        type=Path,
        default=[],
        help="Capture one adapter; may be repeated. Defaults to all adapters.",
    )
    parser.add_argument(
        "--input-list",
        type=Path,
        help=(
            "Text file containing one adapter filename or path per line. "
            "Relative filenames are resolved below --adapter-root."
        ),
    )
    parser.add_argument("--max-files", type=int, default=0)
    parser.add_argument("--timeout", type=int, default=300)
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Replace adapter directories already present under --output-root.",
    )
    args = parser.parse_args()
    if args.max_files < 0:
        parser.error("--max-files must be non-negative")
    if args.timeout < 0:
        parser.error("--timeout must be non-negative")
    return args


def read_argument_file(path: Path) -> list[str]:
    result: list[str] = []
    for line_number, original in enumerate(path.read_text().splitlines(), 1):
        line = original.strip()
        if not line or line.startswith("#"):
            continue
        values = shlex.split(line, comments=True, posix=True)
        if len(values) != 1:
            raise ValueError(
                f"{path}:{line_number}: expected one complete argument per line"
            )
        result.extend(values)
    return result


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run_until_snapshot(
    command: list[str],
    cwd: Path,
    stdout_path: Path,
    stderr_path: Path,
    timeout: int,
    snapshot: Path,
    env: dict[str, str] | None = None,
) -> tuple[int, bool, bool]:
    with stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
        process = subprocess.Popen(
            command,
            cwd=cwd,
            stdout=stdout,
            stderr=stderr,
            start_new_session=True,
            env=env,
        )
        deadline = None if timeout == 0 else time.monotonic() + timeout
        previous_size = -1
        stable_observations = 0
        while True:
            status = process.poll()
            if status is not None:
                return status, False, snapshot.is_file()
            try:
                size = snapshot.stat().st_size
            except FileNotFoundError:
                size = -1
            if size > 0 and size == previous_size:
                stable_observations += 1
            else:
                stable_observations = 0
            previous_size = size
            # The dump stream is closed at the end of the validation pass.
            # Requiring three stable observations avoids terminating while a
            # larger bytecode module is still being written.
            if stable_observations >= 3:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
                return 0, False, True
            if deadline is not None and time.monotonic() >= deadline:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
                stderr.write(f"\n[TIMEOUT] Compile exceeded {timeout}s.\n".encode())
                return 124, True, snapshot.is_file()
            time.sleep(0.05)


def select_inputs(args: argparse.Namespace) -> list[Path]:
    inputs = [path.resolve() for path in args.input]
    if args.input_list:
        for original in args.input_list.read_text().splitlines():
            line = original.strip()
            if not line or line.startswith("#"):
                continue
            path = Path(line)
            if not path.is_absolute():
                path = args.adapter_root / path
            inputs.append(path.resolve())
    if not inputs:
        inputs = sorted(args.adapter_root.resolve().glob("*.ttadapter"))
    if args.max_files:
        inputs = inputs[: args.max_files]
    return inputs


def compiler_version(compiler: Path) -> str:
    result = subprocess.run(
        [str(compiler), "--version"], capture_output=True, text=True, check=False
    )
    text = result.stdout + result.stderr
    return "\n".join(line.rstrip() for line in text.splitlines()) + "\n"


def write_frontend_metadata(
    output_root: Path, args_file: Path, frontend_args: Iterable[str], compiler: Path
) -> None:
    shutil.copyfile(args_file, output_root / "frontend_args.txt")
    (output_root / "frontend_command.txt").write_text(
        "bishengir-compile INPUT -o OUTPUT "
        + " ".join(shlex.quote(value) for value in frontend_args)
        + "\n"
    )
    (output_root / "compiler_version.txt").write_text(compiler_version(compiler))


def main() -> int:
    args = parse_command_line()
    compiler = args.compiler.resolve()
    args_file = args.frontend_args.resolve()
    output_root = args.output_root.resolve()
    if not compiler.is_file() or not os.access(compiler, os.X_OK):
        print(f"[ERROR] compiler is not executable: {compiler}", file=sys.stderr)
        return 2
    if not args_file.is_file():
        print(f"[ERROR] front-end argument file is missing: {args_file}", file=sys.stderr)
        return 2
    frontend_args = read_argument_file(args_file)
    inputs = select_inputs(args)
    if not inputs:
        print("[ERROR] no adapter inputs found", file=sys.stderr)
        return 2

    output_root.mkdir(parents=True, exist_ok=True)
    write_frontend_metadata(output_root, args_file, frontend_args, compiler)
    compiler_hash = sha256(compiler)
    rows: list[dict[str, object]] = []

    for position, adapter in enumerate(inputs, 1):
        if not adapter.is_file():
            print(f"[WARN] missing adapter: {adapter}", file=sys.stderr)
            continue
        destination = output_root / adapter.name
        if destination.exists():
            if not args.overwrite:
                print(
                    f"[ERROR] output exists (pass --overwrite): {destination}",
                    file=sys.stderr,
                )
                return 2
            shutil.rmtree(destination)
        destination.mkdir(parents=True)
        case_dir = destination / "_capture"
        case_dir.mkdir(parents=True)
        print(f"[{position}/{len(inputs)}] {adapter.name}", flush=True)

        snapshot = destination / "before_cvpipelining.mlirbc"
        compile_env = os.environ.copy()
        compile_env["BISHENGIR_DUMP_BEFORE_CVPIPELINING"] = str(snapshot)
        compile_env["BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY"] = "1"

        command = [
            str(compiler),
            str(adapter),
            "-o",
            str(case_dir / "output.o"),
            *frontend_args,
        ]
        status, timed_out, captured = run_until_snapshot(
            command,
            case_dir,
            case_dir / "compile.stdout.log",
            case_dir / "compile.stderr.log",
            args.timeout,
            snapshot,
            env=compile_env,
        )

        snapshots = [snapshot.name] if snapshot.is_file() else []
        dump_status = "complete" if snapshots else "failed"
        rows.append(
            {
                "adapter": adapter.name,
                "dump_status": dump_status,
                "compile_status": (
                    "captured"
                    if captured
                    else (
                        "timeout"
                        if timed_out
                        else ("ok" if status == 0 else f"fail:{status}")
                    )
                ),
                "snapshot_count": len(snapshots),
                "adapter_sha256": sha256(adapter),
                "compiler_sha256": compiler_hash,
                "snapshot_format": "mlir-bytecode-v1",
                "before_cvpipelining_files": ",".join(snapshots),
            }
        )
        print(
            f"  compile={rows[-1]['compile_status']} snapshots={len(snapshots)}",
            flush=True,
        )

    columns = [
        "adapter",
        "dump_status",
        "compile_status",
        "snapshot_count",
        "adapter_sha256",
        "compiler_sha256",
        "snapshot_format",
        "before_cvpipelining_files",
    ]
    with (output_root / "manifest.tsv").open("w", newline="") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=columns, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        writer.writerows(rows)
    complete = sum(row["dump_status"] == "complete" for row in rows)
    print(
        f"BEFORE_CVPIPELINE_DATASET total={len(rows)} "
        f"complete={complete} failed={len(rows) - complete}"
    )
    print(output_root / "manifest.tsv")
    return 0 if complete else 1


if __name__ == "__main__":
    raise SystemExit(main())
