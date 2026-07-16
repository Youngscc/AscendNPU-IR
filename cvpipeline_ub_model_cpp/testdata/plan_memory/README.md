# PlanMemory Fixtures

This directory contains named PlanMemory-before MLIR fixtures for focused
lifetime and local UB planning validation. Every exact fixture must be checked
with seeds 0 through 19 and both values of `restrict-inplace-as-isa`.

## Naming

| Prefix | Coverage |
| - | - |
| `b_alias_` | conditional, region-result, and view aliases |
| `b_control_` | `scf.while`, `scope`, `cf` CFG, and preload handling |
| `b_liveness_` | ordered live sets and nested-region death points |
| `b_planner_` | reuse, inplace, multi-buffer, and SpecAlloc paths |
| `b_boundary_` | alignment and UB-capacity boundaries |
| `b_type_` | memref rank and element-type boundaries |

`manifest.tsv` records whether a case is copied from the real
`bishengir/test/Dialect/HIVM/plan-memory.mlir` suite or designed specifically
for a boundary. Source-derived fixtures retain their source function name.

## Scale

```text
35 exact fixtures x 20 seeds x 2 restrict modes = 1400 exact runs
```

All fixtures satisfy the real PlanMemory-before input contract. Dynamic local
extent is excluded because the upstream suffix guarantees static local-buffer
shapes before PlanMemory; defensive rejection of malformed input is tested at
the C++ unit level instead of being counted as focused PlanMemory coverage.

## Commands

```bash
python3 cvpipeline_ub_model_cpp/scripts/generate_plan_memory_fixtures.py
bash cvpipeline_ub_model_cpp/scripts/run_plan_memory_fixture_oracles.sh
bash cvpipeline_ub_model_cpp/scripts/validate_plan_memory_fixtures.sh
```

Oracle generation writes to
`cvpipeline_ub_model_cpp/output/plan_memory_fixtures/`. The validator always
runs both lifetime and plan comparisons, then returns failure if either layer
has any non-identical canonical snapshot.

## Current Result

The oracle corpus is complete: all 1400 runs reached PlanMemory. The lightweight
model does not yet pass this new coverage gate:

```text
lifetime: 266 / 1400 byte-identical, 1134 mismatches
plan:     358 / 1400 byte-identical, 1042 mismatches
```

These counts include repeated seeds/configs. They must not be interpreted as
1134 or 1042 independent implementation defects. The reports under
`output/plan_memory_fixtures/reports/` identify the failing case/config/seed.
Known root categories include MLIR operand display-name normalization,
PlanMemory's greedy loop-iterator normalization side effects, and remaining
control-flow/zero-sized-extra-buffer liveness differences. A failure remains a
failure; the suite does not compare peak only or discard names/order to pass.

The 12-case structural extension adds source-derived `scf.if`/loop-result and
multi-buffer-count-4 modules plus designed two-result `scf.while`, two-result
`scope.scope`, two-preload-buffer scope, and `cf` branches containing
`scf.for`.
