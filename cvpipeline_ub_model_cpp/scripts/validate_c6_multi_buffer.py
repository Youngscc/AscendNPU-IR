#!/usr/bin/env python3
"""Validate C6 multi-buffer state against native pass snapshots."""

from __future__ import annotations

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


def parse_marks(path: Path) -> dict[int, tuple[int, int]]:
    marks: dict[int, tuple[int, int]] = {}
    for line in path.read_text().splitlines():
        if not line.startswith("MARK\t"):
            continue
        fields = line.split("\t")
        root = bytes.fromhex(fields[2]).decode()
        if not root.startswith("local:"):
            continue
        ordinal = int(root.removeprefix("local:"))
        value = (int(fields[3]), int(fields[4]))
        if ordinal in marks and marks[ordinal] != value:
            raise RuntimeError(f"conflicting mark for local:{ordinal}")
        marks[ordinal] = value
    return marks


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    summary = repo / "Output/index/c6_pass_oracles/summary.tsv"
    c4_objects = repo / "Output/experiments/c2_c3_pass_oracles/objects"
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    rows = list(csv.DictReader(summary.open(), delimiter="\t"))
    failures: list[str] = []
    checked = marks = additions = buffers = 0
    with tempfile.TemporaryDirectory(prefix="cvub-c6-") as temporary:
        temp = Path(temporary)
        for index, row in enumerate(rows):
            if row["status"] != "complete":
                failures.append(f"{row['after_cvpipeline_sha256']}/{row['config_id']}: incomplete")
                continue
            input_hash = row["after_cvpipeline_sha256"]
            source = (c4_objects / input_hash / "pass_tree" /
                      "builtin_module_no-symbol-name/0_one-shot-bufferize.mlir")
            tree = Path(row["pass_tree"])
            before_files = sorted(tree.rglob("*_0_hivm-mark-multi-buffer.mlir"))
            after_files = sorted(tree.rglob("*_1_hivm-mark-multi-buffer.mlir"))
            if not before_files or len(before_files) != len(after_files):
                failures.append(f"{input_hash}/{row['config_id']}: bad pass pairs")
                continue
            before = max(before_files, key=mark_count)
            after = max(after_files, key=mark_count)
            prefix = temp / str(index)
            oracle_marks = prefix.with_suffix(".oracle.tsv")
            model = prefix.with_suffix(".model.tsv")
            command = [str(tool), "--action=model-c6-multi-buffer-projection",
                       "--root", str(source), "--output", str(model),
                       "--limit-auto-multi-buffer-of-local-buffer",
                       row["local_strategy"],
                       "--limit-auto-multi-buffer-buffer",
                       row["mix_strategy"]]
            if row["enable_auto"] == "1":
                command.append("--enable-auto-multi-buffer")
            errors = [error for cmd in (
                [str(tool), "--action=dump-mark-multi-buffer-oracle",
                 "--root", str(after), "--output", str(oracle_marks)],
                command) if (error := run(cmd))]
            if errors:
                failures.append(f"{input_hash}/{row['config_id']}: {'; '.join(errors)}")
                continue
            try:
                actual = parse_marks(oracle_marks)
            except RuntimeError as error:
                failures.append(f"{input_hash}/{row['config_id']}: {error}")
                continue
            model_lines = model.read_text().splitlines()
            oracle_lines = ["C6_MULTI_BUFFER_PROJECTION\t1"]
            buffer_lines = [line for line in model_lines
                            if line.startswith("BUFFER\t")]
            for ordinal, line in enumerate(buffer_lines):
                identity = line.split("\t")[1]
                count, preload = actual.get(ordinal, (1, 0))
                oracle_lines.append(
                    f"BUFFER\t{identity}\t{count}\t{preload}")
            oracle = "\n".join(oracle_lines) + "\n"
            if model.read_text() != oracle:
                actual_lines = oracle.splitlines()
                different = next((line for line, pair in enumerate(
                    zip(model_lines, actual_lines), 1) if pair[0] != pair[1]),
                    min(len(model_lines), len(actual_lines)) + 1)
                failures.append(
                    f"{input_hash}/{row['config_id']}: mismatch line={different} "
                    f"model={model_lines[different-1] if different <= len(model_lines) else '<eof>'} "
                    f"oracle={actual_lines[different-1] if different <= len(actual_lines) else '<eof>'}")
                continue
            checked += 1
            marks += len(actual)
            additions += max(0, mark_count(after) - mark_count(before))
            buffers += len(buffer_lines)
    if failures:
        print(f"C6_MULTI_BUFFER=FAIL tuples={checked} failures={len(failures)}")
        print("\n".join(failures[:30]))
        return 1
    print(f"C6_MULTI_BUFFER=PASS tuples={checked} marks={marks} "
          f"auto_additions={additions} buffers={buffers}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
