#!/usr/bin/env python3
"""Validate the C7 canonical PlanMemory-before AIV/UB buffer snapshot."""

from __future__ import annotations

import argparse
import csv
import subprocess
import tempfile
from pathlib import Path


def run(command: list[str]) -> str:
    completed = subprocess.run(command, text=True, capture_output=True)
    return "" if completed.returncode == 0 else (
        completed.stderr.strip() or completed.stdout.strip())


def mark_count(path: Path) -> int:
    return path.read_bytes().count(b"hivm.multi_buffer")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--with-accesses", action="store_true",
        help="also run the C8-precheck operation-access comparison")
    args = parser.parse_args()
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    rows = list(csv.DictReader(
        (repo / "Output/index/c6_pass_oracles/summary.tsv").open(),
        delimiter="\t"))
    c4_objects = repo / "Output/experiments/c2_c3_pass_oracles/objects"
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    failures: list[str] = []
    checked = buffers = accesses = 0
    with tempfile.TemporaryDirectory(prefix="cvub-c7-") as temporary:
        temp = Path(temporary)
        for index, row in enumerate(rows):
            input_hash = row["after_cvpipeline_sha256"]
            label = f"{input_hash}/{row['config_id']}"
            source = (c4_objects / input_hash / "pass_tree" /
                      "builtin_module_no-symbol-name/0_one-shot-bufferize.mlir")
            after_files = sorted(Path(row["pass_tree"]).rglob(
                "*_1_hivm-mark-multi-buffer.mlir"))
            if row["status"] != "complete" or not after_files:
                failures.append(f"{label}: incomplete oracle")
                continue
            after = max(after_files, key=mark_count)
            model = temp / f"{index}.model.tsv"
            raw_oracle = temp / f"{index}.oracle.tsv"
            model_access = temp / f"{index}.model-access.tsv"
            raw_access = temp / f"{index}.oracle-access.tsv"
            model_command = [
                str(tool), "--action=model-planmemory-input-buffers",
                "--root", str(source), "--output", str(model),
                "--limit-auto-multi-buffer-of-local-buffer",
                row["local_strategy"], "--limit-auto-multi-buffer-buffer",
                row["mix_strategy"]]
            if row["enable_auto"] == "1":
                model_command.append("--enable-auto-multi-buffer")
            access_command = model_command.copy()
            access_command[1] = "--action=model-planmemory-input-access-debug"
            access_command[5] = str(model_access)
            commands = [
                model_command,
                [str(tool), "--action=dump-planmemory-input-buffer-oracle",
                 "--root", str(after), "--output", str(raw_oracle)]]
            if args.with_accesses:
                commands.extend((
                    access_command,
                    [str(tool), "--action=dump-planmemory-input-access-oracle",
                     "--root", str(after), "--output", str(raw_access)]))
            errors = [error for command in commands
                if (error := run(command))]
            if errors:
                failures.append(f"{label}: {'; '.join(errors)}")
                continue
            model_lines = model.read_text().splitlines()
            model_by_ordinal = {
                int(fields[1]): fields
                for line in model_lines if line.startswith("BUFFER\t")
                for fields in [line.split("\t")]}
            oracle_lines = ["PLANMEMORY_INPUT_BUFFERS\t1"]
            malformed = False
            for line in raw_oracle.read_text().splitlines():
                if not line.startswith("BUFFER\t"):
                    continue
                fields = line.split("\t")
                ordinal = int(fields[1])
                modeled = model_by_ordinal.get(ordinal)
                if modeled is None:
                    failures.append(f"{label}: oracle ordinal {ordinal} missing")
                    malformed = True
                    break
                oracle_lines.append("\t".join((
                    "BUFFER", fields[1], modeled[2], fields[2], fields[3],
                    fields[4], modeled[6], fields[5], fields[6])))
            if malformed:
                continue
            oracle = "\n".join(oracle_lines) + "\n"
            if model.read_text() != oracle:
                actual_lines = oracle.splitlines()
                different = next((line for line, pair in enumerate(
                    zip(model_lines, actual_lines), 1) if pair[0] != pair[1]),
                    min(len(model_lines), len(actual_lines)) + 1)
                failures.append(
                    f"{label}: mismatch line={different} "
                    f"model={model_lines[different-1] if different <= len(model_lines) else '<eof>'} "
                    f"oracle={actual_lines[different-1] if different <= len(actual_lines) else '<eof>'}")
                continue
            if not args.with_accesses:
                checked += 1
                buffers += len(model_by_ordinal)
                continue
            identity_by_ordinal = {
                ordinal: fields[2]
                for ordinal, fields in model_by_ordinal.items()}
            access_oracle_lines = ["PLANMEMORY_INPUT_ACCESSES\t1"]
            malformed_access = False
            for line in raw_access.read_text().splitlines():
                if not line.startswith("EVENT\t"):
                    continue
                fields = line.split("\t")
                rewritten = fields[:4]
                for access in fields[4:]:
                    encoded, effect = access.rsplit(":", 1)
                    root = bytes.fromhex(encoded).decode()
                    if not root.startswith("local:"):
                        failures.append(f"{label}: malformed access root {root}")
                        malformed_access = True
                        break
                    identity = identity_by_ordinal.get(
                        int(root.removeprefix("local:")))
                    if identity is None:
                        failures.append(f"{label}: access root missing {root}")
                        malformed_access = True
                        break
                    rewritten.append(f"{identity}:{effect}")
                if malformed_access:
                    break
                access_oracle_lines.append("\t".join(rewritten))
            if malformed_access:
                continue
            access_oracle = "\n".join(access_oracle_lines) + "\n"
            debug_access_lines = model_access.read_text().splitlines()
            access_sources: dict[int, str] = {}
            model_access_lines = ["PLANMEMORY_INPUT_ACCESSES\t1"]
            for debug_line in debug_access_lines:
                if debug_line.startswith("ACCESS_BLOCKER\t"):
                    model_access_lines.append(debug_line)
                    continue
                if not debug_line.startswith("EVENT\t"):
                    continue
                fields = debug_line.split("\t")
                access_sources[len(model_access_lines) + 1] = (
                    f"{fields[3]}:{bytes.fromhex(fields[4]).decode()}")
                model_access_lines.append("\t".join(fields[:3] + fields[5:]))
            model_access_text = "\n".join(model_access_lines) + "\n"
            if model_access_text != access_oracle:
                actual_access_lines = access_oracle.splitlines()
                different = next((line for line, pair in enumerate(
                    zip(model_access_lines, actual_access_lines), 1)
                    if pair[0] != pair[1]),
                    min(len(model_access_lines), len(actual_access_lines)) + 1)
                failures.append(
                    f"{label}: access mismatch line={different} "
                    f"source_op={access_sources.get(different, '-')} "
                    f"model={model_access_lines[different-1] if different <= len(model_access_lines) else '<eof>'} "
                    f"oracle={actual_access_lines[different-1] if different <= len(actual_access_lines) else '<eof>'}")
                continue
            checked += 1
            buffers += len(model_by_ordinal)
            accesses += len(access_oracle_lines) - 1
    if failures:
        print(f"C7_PLANMEMORY_INPUT=FAIL tuples={checked} failures={len(failures)}")
        representatives: dict[str, str] = {}
        for failure in failures:
            representatives.setdefault(failure.split("/", 1)[0], failure)
        print("\n".join(list(representatives.values())[:30]))
        return 1
    suffix = f" accesses={accesses}" if args.with_accesses else ""
    print(f"C7_PLANMEMORY_INPUT=PASS tuples={checked} buffers={buffers}{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
