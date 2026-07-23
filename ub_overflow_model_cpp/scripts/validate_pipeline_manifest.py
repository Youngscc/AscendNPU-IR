#!/usr/bin/env python3
"""Validate model stage manifests against the real suffix compiler registry."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler-manifest", required=True, type=Path)
    parser.add_argument("--post-manifest", required=True, type=Path)
    parser.add_argument("--suffix-manifest", required=True, type=Path)
    parser.add_argument("--expected-pipeline-sha256", required=True, type=Path)
    return parser.parse_args()


def checked_stage_names(path: Path, expected_column: str) -> list[str]:
    lines = [line for line in path.read_text(encoding="utf-8").splitlines()
             if line.strip()]
    if not lines:
        raise ValueError(f"{path}: empty manifest")
    header = lines[0].split("\t")
    try:
        index = header.index(expected_column)
    except ValueError as error:
        raise ValueError(f"{path}: missing {expected_column!r} column") from error
    names: list[str] = []
    for line_number, line in enumerate(lines[1:], 2):
        fields = line.split("\t")
        if index >= len(fields) or not fields[index]:
            raise ValueError(f"{path}:{line_number}: missing stage name")
        names.append(fields[index])
    return names


def compiler_manifest(path: Path) -> tuple[str, dict[str, list[str]]]:
    pipeline_hash = ""
    stages: dict[str, list[str]] = {"post": [], "suffix": []}
    schema_seen = False
    for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), 1):
        fields = line.split("\t")
        if fields[:2] == ["ORACLE_PIPELINE_SCHEMA", "1"]:
            schema_seen = True
        elif fields and fields[0] == "PIPELINE_SHA256" and len(fields) == 2:
            pipeline_hash = fields[1]
        elif fields and fields[0] == "STAGE":
            if len(fields) != 4 or fields[1] not in stages:
                raise ValueError(f"{path}:{line_number}: malformed STAGE row")
            expected_order = len(stages[fields[1]]) + 1
            if fields[2] != str(expected_order):
                raise ValueError(f"{path}:{line_number}: non-contiguous stage order")
            stages[fields[1]].append(fields[3])
    if not schema_seen:
        raise ValueError(f"{path}: missing schema")
    if not pipeline_hash:
        raise ValueError(f"{path}: missing pipeline hash")
    return pipeline_hash, stages


def main() -> int:
    args = parse_args()
    try:
        actual_hash, actual = compiler_manifest(args.compiler_manifest)
        expected_post = checked_stage_names(args.post_manifest, "stage")
        expected_suffix = checked_stage_names(
            args.suffix_manifest, "compiler_pass_group")
        expected_hash = args.expected_pipeline_sha256.read_text(
            encoding="utf-8").strip()
        errors: list[str] = []
        if actual["post"] != expected_post:
            errors.append(
                f"post stages differ: compiler={actual['post']!r} model={expected_post!r}")
        if actual["suffix"] != expected_suffix:
            errors.append(
                f"suffix stages differ: compiler={actual['suffix']!r} model={expected_suffix!r}")
        if actual_hash != expected_hash:
            errors.append(
                f"textual pipeline hash differs: compiler={actual_hash} expected={expected_hash}")
        if errors:
            print("\n".join(errors), file=sys.stderr)
            return 1
        return 0
    except (OSError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
