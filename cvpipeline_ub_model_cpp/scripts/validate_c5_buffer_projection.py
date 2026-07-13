#!/usr/bin/env python3
"""Validate connected C4-to-C5 InlineLoadCopy buffer projection."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


PASS_SUFFIX = "hivm-inline-load-copy.mlir"


def allocation_count(path: Path) -> int:
    return path.read_bytes().count(b'"memref.alloc"')


def allocation_tokens(path: Path) -> list[tuple[str, ...]]:
    result = []
    for line in path.read_text().splitlines():
        if line.startswith("ALLOC\t"):
            fields = line.split("\t")
            result.append(tuple(fields[2:]))
    return result


def lcs_survivors(before: list[tuple[str, ...]],
                  after: list[tuple[str, ...]]) -> set[int]:
    rows = len(before) + 1
    columns = len(after) + 1
    table = [[0] * columns for _ in range(rows)]
    for i in range(len(before) - 1, -1, -1):
        for j in range(len(after) - 1, -1, -1):
            table[i][j] = (1 + table[i + 1][j + 1]
                           if before[i] == after[j]
                           else max(table[i + 1][j], table[i][j + 1]))
    survivors: set[int] = set()
    i = j = 0
    while i < len(before) and j < len(after):
        if before[i] == after[j] and table[i][j] == 1 + table[i + 1][j + 1]:
            survivors.add(i)
            i += 1
            j += 1
        elif table[i + 1][j] >= table[i][j + 1]:
            i += 1
        else:
            j += 1
    if len(survivors) != len(after):
        raise RuntimeError("C5 oracle allocation sequence is not a subsequence")
    return survivors


def run(command: list[str]) -> str:
    completed = subprocess.run(command, text=True, capture_output=True)
    if completed.returncode:
        return completed.stderr.strip() or completed.stdout.strip()
    return ""


def main() -> int:
    module = Path(__file__).resolve().parent.parent
    repo = module.parent
    c4_objects = repo / "Output/experiments/c2_c3_pass_oracles/objects"
    c5_objects = repo / "Output/experiments/c5_pass_oracles/objects"
    tool = module / "output/bin/cvpipeline_ub_model_dev_validate"
    failures: list[str] = []
    checked = function_pairs = changed_pairs = rewrites = buffers = 0
    with tempfile.TemporaryDirectory(prefix="cvub-c5-") as temporary:
        temp = Path(temporary)
        for directory in sorted(c5_objects.iterdir()):
            if not directory.is_dir():
                continue
            source = (c4_objects / directory.name / "pass_tree" /
                      "builtin_module_no-symbol-name/0_one-shot-bufferize.mlir")
            c3_endpoint = source.with_name(
                "5_convert-non-contiguous-reshape-to-copy.mlir")
            c4_endpoint = source.with_name("9_hivm-infer-mem-scope.mlir")
            tree = directory / "pass_tree"
            before_files = sorted(tree.rglob(f"*_0_{PASS_SUFFIX}"))
            after_files = sorted(tree.rglob(f"*_1_{PASS_SUFFIX}"))
            if not before_files or len(before_files) != len(after_files):
                failures.append(f"{directory.name}: incomplete pass pairs")
                continue
            function_pairs += len(before_files)
            for before, after in zip(before_files, after_files):
                before_body = b"\n".join(before.read_bytes().splitlines()[1:])
                after_body = b"\n".join(after.read_bytes().splitlines()[1:])
                changed_pairs += before_body != after_body

            initial = max(before_files, key=allocation_count)
            final = min(after_files, key=allocation_count)
            prefix = temp / directory.name
            model = prefix.with_suffix(".model.tsv")
            semantic = prefix.with_suffix(".semantic.tsv")
            c4_oracle = prefix.with_suffix(".c4.tsv")
            before_alloc = prefix.with_suffix(".before.tsv")
            after_alloc = prefix.with_suffix(".after.tsv")
            candidates = prefix.with_suffix(".candidates.tsv")
            commands = (
                [str(tool), "--action=model-c5-buffer-projection",
                 "--root", str(source), "--output", str(model)],
                [str(tool), "--action=model-c5-semantic-ir",
                 "--root", str(source), "--output", str(semantic)],
                [str(tool), "--action=dump-c4-buffer-projection-oracle",
                 "--root", str(source), "--attempt-root", str(c3_endpoint),
                 "--after-ir", str(c4_endpoint),
                 "--expected-summary", str(source.parents[1]),
                 "--output", str(c4_oracle)],
                [str(tool), "--action=dump-c2-allocation-oracle",
                 "--root", str(initial), "--output", str(before_alloc)],
                [str(tool), "--action=dump-c2-allocation-oracle",
                 "--root", str(final), "--output", str(after_alloc)],
                [str(tool), "--action=dump-inline-load-copy-candidates",
                 "--root", str(initial), "--output", str(candidates)],
            )
            errors = [error for command in commands if (error := run(command))]
            if errors:
                failures.append(f"{directory.name}: {'; '.join(errors)}")
                continue

            before = allocation_tokens(before_alloc)
            after = allocation_tokens(after_alloc)
            c4_lines = [line for line in c4_oracle.read_text().splitlines()
                        if line.startswith("BUFFER\t")]
            if len(before) != len(c4_lines):
                failures.append(
                    f"{directory.name}: C4/before count {len(c4_lines)} != "
                    f"{len(before)}")
                continue
            try:
                survivors = lcs_survivors(before, after)
            except RuntimeError as error:
                failures.append(f"{directory.name}: {error}")
                continue
            oracle = "C5_BUFFER_PROJECTION\t1\n" + "".join(
                line + "\n" for index, line in enumerate(c4_lines)
                if index in survivors)
            model_text = model.read_text()
            if model_text != oracle:
                model_lines = model_text.splitlines()
                oracle_lines = oracle.splitlines()
                different = next(
                    (index for index, pair in enumerate(
                        zip(model_lines, oracle_lines), 1)
                     if pair[0] != pair[1]),
                    min(len(model_lines), len(oracle_lines)) + 1)
                failures.append(
                    f"{directory.name}: mismatch line={different}\n"
                    f"  oracle={oracle_lines[different - 1] if different <= len(oracle_lines) else '<eof>'}\n"
                    f"  model={model_lines[different - 1] if different <= len(model_lines) else '<eof>'}")
                continue
            object_rewrites = sum(
                line.startswith("INLINE\t")
                for line in semantic.read_text().splitlines())
            removed = len(before) - len(after)
            if object_rewrites != removed:
                failures.append(
                    f"{directory.name}: rewrites={object_rewrites} "
                    f"removed={removed}")
                continue
            removed_ordinals = set(range(len(before))) - survivors
            oracle_signatures = []
            invalid_destination = False
            for line in candidates.read_text().splitlines():
                if not line.startswith("CANDIDATE\t"):
                    continue
                fields = line.split("\t")
                middle = bytes.fromhex(fields[3]).decode()
                destination = bytes.fromhex(fields[4]).decode()
                if not middle.startswith("local:"):
                    continue
                middle_ordinal = int(middle.removeprefix("local:"))
                if middle_ordinal not in removed_ordinals:
                    continue
                if not destination.startswith("local:"):
                    failures.append(
                        f"{directory.name}: non-local inline destination")
                    invalid_destination = True
                    break
                destination_ordinal = int(
                    destination.removeprefix("local:"))
                oracle_signatures.append(
                    (fields[1], c4_lines[middle_ordinal].split("\t")[1],
                     c4_lines[destination_ordinal].split("\t")[1]))
            if invalid_destination:
                continue
            model_signatures = []
            for line in semantic.read_text().splitlines():
                if line.startswith("INLINE\t"):
                    fields = line.split("\t")
                    model_signatures.append((fields[1], fields[5], fields[6]))
            if sorted(model_signatures) != sorted(oracle_signatures):
                failures.append(
                    f"{directory.name}: rewrite signatures differ "
                    f"oracle={sorted(oracle_signatures)} "
                    f"model={sorted(model_signatures)}")
                continue
            checked += 1
            rewrites += object_rewrites
            buffers += len(after)

    if failures:
        print(f"C5_BUFFER_PROJECTION=FAIL objects={checked} "
              f"failures={len(failures)}")
        print("\n".join(failures[:30]))
        return 1
    print(f"C5_BUFFER_PROJECTION=PASS objects={checked} "
          f"function_pairs={function_pairs} changed_pairs={changed_pairs} "
          f"rewrites={rewrites} buffers={buffers}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
