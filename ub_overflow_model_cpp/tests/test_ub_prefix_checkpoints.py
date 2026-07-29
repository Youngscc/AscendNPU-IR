#!/usr/bin/env python3
"""Unit checks for the native UB prefix checkpoint driver."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/dump_ub_prefix_checkpoints.py"
SPEC = importlib.util.spec_from_file_location(
    "dump_ub_prefix_checkpoints", SCRIPT
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)

assert len(MODULE.CHECKPOINT_STAGES) == 14
assert MODULE.CHECKPOINT_STAGES[0] == "00_before_auto_blockify"
assert MODULE.CHECKPOINT_STAGES[-1] == "13_after_inline_otf_broadcast"

arguments, compiler_arguments = MODULE.parse_arguments(
    [
        "input.ttadapter",
        "--compiler",
        "compiler",
        "--output-dir",
        "checkpoints",
    ]
)
assert arguments.input == Path("input.ttadapter")
assert arguments.compiler == Path("compiler")
assert arguments.output_dir == Path("checkpoints")
assert compiler_arguments == []

arguments, compiler_arguments = MODULE.parse_arguments(
    [
        "input.ttadapter",
        "--output-dir",
        "checkpoints",
        "--",
        "--enable-auto-blockify-loop=false",
        "--mlir-disable-threading",
    ]
)
assert arguments.output_dir == Path("checkpoints")
assert compiler_arguments == [
    "--enable-auto-blockify-loop=false",
    "--mlir-disable-threading",
]

with tempfile.TemporaryDirectory(prefix="cvub-prefix-checkpoint-test-") as tmp:
    root = Path(tmp)
    attempt = root / "attempt-1"
    attempt.mkdir()
    for stage in MODULE.CHECKPOINT_STAGES:
        (attempt / f"{stage}.mlir").write_text(stage, encoding="utf-8")
    paths = MODULE.validate_attempt(attempt, None)
    assert len(paths) == 14

    selected = MODULE.CHECKPOINT_STAGES[1]
    selected_attempt = root / "attempt-2"
    selected_attempt.mkdir()
    (selected_attempt / f"{selected}.mlir").write_text(
        selected, encoding="utf-8"
    )
    assert MODULE.validate_attempt(selected_attempt, selected) == [
        selected_attempt / f"{selected}.mlir"
    ]

environment = MODULE.checkpoint_environment(Path("checkpoints"), None)
assert environment["BISHENGIR_STOP_AFTER_UB_PREFIX_CHECKPOINTS"] == "1"
assert "BISHENGIR_UB_PREFIX_CHECKPOINT_STAGE" not in environment

environment = MODULE.checkpoint_environment(
    Path("checkpoints"), "01_after_auto_blockify"
)
assert (
    environment["BISHENGIR_UB_PREFIX_CHECKPOINT_STAGE"]
    == "01_after_auto_blockify"
)

print("[PASS] native UB prefix checkpoint driver")
