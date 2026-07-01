# Memory Modeling

Lightweight Python module for early memory-state snapshots and pass-effect
modeling.

This module intentionally starts outside the C++ pass pipeline. It provides the
Python-side scaffolding for the reverse modeling workflow:

```text
S8.5 MarkMultiBuffer后 -> PlanMemory(local)
S8.4 InlineLoadCopy后  -> MarkMultiBuffer -> PlanMemory(local)
...
S0 initial IR/config snapshot
```

It does not try to replace `PlanMemory`. Exact local memory planning should still
reuse the C++ implementation or a future dry-run oracle.

## Core Classes

```text
Checkpoint
  Describes a modeling point and the suffix needed to reach an oracle.

MemoryState
  Memory-relevant state at a checkpoint: IR level, precision, visible buffers,
  functions, address spaces, blockers, and eventual PlanMemory peaks.

BufferRecord
  Stable description of a buffer-like value. Early scans only fill type/op/scope;
  later MLIR-backed collectors can fill exact size, alignment, lineage, and
  multi-buffer metadata.

PassEffect
  Per-pass delta: created/deleted/resized buffers, scope/layout/lifetime changes,
  copy insertion, extra buffer insertion, multi-buffer changes, reuse changes.

PlanPeak
  Peak returned by PlanMemory or a dry-run oracle for a memory scope.

ModelingReport
  Top-level JSON report containing input metadata, checkpoint, MemoryState,
  compile config, and pass effects.
```

## Current Scope

```text
S0 text scan
  - identify rough IR level
  - count memory-relevant ops
  - collect obvious function/module attributes
  - emit JSON for later trace/modeling

S8 checkpoint registry
  - encode the reverse implementation order
  - make suffix effects explicit
  - prevent treating suffix passes as peak-neutral

S8.5 text model
  - parse MarkMultiBuffer-after dump IR
  - collect local memref.alloc buffers, static sizes, scopes, multi-buffer attrs
  - approximate lifetime from textual SSA use order
  - compute conservative per-function/per-scope interval-reuse peaks
  - emit blockers for PlanMemory details not yet simulated
```

## Usage

```bash
python3 -m memory_modeling.s0_snapshot input.mlir
python3 -m memory_modeling.s0_snapshot input.mlir --pretty
python3 -m memory_modeling.s0_snapshot input.mlir --config-json config.json
python3 -m memory_modeling.cli --list-checkpoints --pretty
python3 -m memory_modeling.cli --reverse-plan --pretty
python3 -m memory_modeling.cli input.mlir --checkpoint S8.5 --pretty
python3 -m memory_modeling.run_adapters --data-dir data --pretty
```

The scanner is text-based for now. It should only produce `symbolic` or
`concrete_structural` facts unless the input is explicitly a PlanMemory-ready
checkpoint whose peak is supplied by the real C++ oracle.

`run_adapters` is the batch validation entry. It scans all non-hidden regular
files under `data/` by default, runs the S8.5 Python model, then invokes
`build/bin/bishengir-opt` with local PlanMemory and MemoryDisplay enabled. A run
directory contains a top-level `comparison.csv`/`comparison.json` focused on UB:
model peak, AscendIR peak computed from max address offset, delta, overflow
required bits, and capacity. Each per-file directory keeps `memory.json`,
`suffix_input.mlir`, `after_plan.mlir` when planning succeeds, raw
`memory_info*.json`, and `oracle.log` only when the oracle prints output.

## Reverse Modeling Rule

`MarkMultiBuffer` after is the first exact checkpoint. When a checkpoint moves
earlier, the suffix back to PlanMemory is not peak-neutral. It is part of the
model:

```text
checkpoint_structural_state
checkpoint_plus_suffix_plan_peak
```

Keep these two concepts separate in reports.

## Future C++ Boundary

Move or mirror logic into C++ only when we need one of these:

```text
MLIR Operation walk
typed dialect attributes
MemLivenessAnalysis
MemPlan / PlanMemory dry-run
pipeline instrumentation
```
