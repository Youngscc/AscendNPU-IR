#!/usr/bin/env python3
"""Contract tests for cv2pm/model stage projection and invariants."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/bisect_stage_divergence.py"
SPEC = importlib.util.spec_from_file_location("bisect_stage_divergence", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)

mixed = '''"builtin.module"() ({
  "func.func"() ({
    "hivm.hir.mmad"() : () -> ()
  }) {hivm.func_core_type = #hivm.func_core_type<AIC>} : () -> ()
  "func.func"() ({
    "hivm.hir.add"() : () -> ()
  }) {hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
'''
projection = MODULE.ub_relevant_mlir_projection(mixed)
assert "hivm.hir.add" in projection
assert "hivm.hir.mmad" not in projection
ops, _ = MODULE.invariants_from_mlir(mixed)
assert ops == {"func.func": 1, "hivm.hir.add": 1}, ops

unsplit = '''"builtin.module"() ({
  "func.func"() ({
    "hivm.hir.add"() : () -> ()
  }) {hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> ()
}) : () -> ()
'''
assert MODULE.ub_relevant_mlir_projection(unsplit) == unsplit
ops, _ = MODULE.invariants_from_mlir(unsplit)
assert ops == {"func.func": 1, "hivm.hir.add": 1}, ops

print("[PASS] stage divergence uses the UB-relevant AIV projection")
