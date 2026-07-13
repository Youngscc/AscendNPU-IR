# CVPipeline UB Model C++ Core

This directory is the home of the lightweight C++ core for CVPipeline UB
modeling.

## Layout

```text
src/
  common/checked_math.hpp       # overflow-checked UB arithmetic
  semantic_ir/                  # parser, SSA/CFG, types and op registry
  planmemory/                   # normalize, alias, liveness, storage and planner
  suffix/                       # reserved for independent C-stage pass modules
  validation/                   # development-only replay/compare helpers
  cli/                          # production and development executables
  model_core.hpp                # compatibility facade for PlanMemory model
tests/
  model_core_tests.cpp          # B-stage semantic boundary tests
  semantic_ir_tests.cpp         # standalone shared-IR/parser test
testdata/planmemory_b/
  exact/                       # named exact B-stage MLIR fixtures
  manifest.tsv                 # case origin and expected mode
scripts/
  build.sh                     # builds final-facing binary
  build_dev_tools.sh           # builds development-only validator
  validate_memory_info_replay.sh      # MemoryDisplay offset replay
  validate_local_ub_allocations.sh    # PlanMemory-before IR identity
  validate_lifetimes.sh               # lifetime analysis
  validate_planmemory_attempts.sh     # fixed-seed internal-state checks
  validate_memory_plan.sh             # extent/offset/UB peak
  validate_extended_memory_plan.sh    # extended exact-peak regression
  dump_fixed_seed_oracles.sh          # 166 inputs x 20 seeds real oracle
  validate_fixed_seed_lifetimes.sh    # exact lifetime/state acceptance
  validate_fixed_seed_plans.sh        # exact layout/peak acceptance
  validate_fixed_seed_exact.sh        # both fixed-seed acceptance checks
  dump_restrict_fixed_seed_oracles.sh # non-default config oracle
  dump_restrict_fixed_seed_oracles_parallel.sh # sharded 166-case oracle
  validate_config_matrix.sh           # default + restrict exact checks
  generate_config_test_matrix.py      # deterministic constrained config data
  run_config_test_matrix.py           # suffix transform + IR hash dedup + oracle
  validate_generated_config_oracles.sh # byte-exact model comparison
  generate_planmemory_b_fixtures.py    # named focused B-stage MLIR corpus
  run_planmemory_b_fixture_oracles.sh  # dynamically sized focused oracle
  validate_planmemory_b_fixtures.sh    # lifetime and plan focused gate
  run_vv_fixed_seed_oracles.sh         # five vv adapters x 20 x 2
  validate_vv_fixed_seed_oracles.sh    # byte-exact vv acceptance
  run_unit_tests.sh                   # no LLVM/MLIR dependency
  verify_hivm_op_registry.sh          # TableGen trait drift check
  validate_comparator_sensitivity.sh  # rejects mutated lifetime/offset oracle
  validate_oracle_provenance.sh       # input/tool hash freshness gate
  dump_cvpipeline_after_oracles.py     # C0 CVPipelining-after oracle dataset
  validate_cvpipeline_after_oracles.py # C0 exact/provenance/reproducibility gate
  validate_cvpipeline_oracle_sensitivity.py # C0 mutation rejection test
  dump_c1_semantic_oracles.py      # C1 real-object SemanticIR oracle dataset
  validate_c1_semantic_oracles.py  # C1 byte-exact/provenance/determinism gate
  validate_c1_semantic_sensitivity.py # C1 field mutation rejection test
  validate_c1_focused_cfg.sh       # C1 forward-successor focused oracle
  validate_post_bufferization_normalization.py # C2-to-C3 canonicalization
  validate_hivm_decompose_op.py    # first decompose allocation/rewrite exact gate
  validate_non_contiguous_reshape_to_copy.py # real + focused copy gate
  validate_c3_semantic_sensitivity.py # rejects C3 type/buffer mutations
  validate_all.sh                     # unit + exact acceptance regressions
output/
  bin/                         # local build artifact
  reports/                     # validation TSV reports
  snapshots/                   # canonical oracle/model text snapshots
input/
  suffix_compile -> ../../Output/views/suffix_compiler_default
```

## External Input

The current validation input is the minimal suffix oracle output, exposed from
this module through `input/suffix_compile`:

```text
cvpipeline_ub_model_cpp/input/suffix_compile/
cvpipeline_ub_model_cpp/input/suffix_compile/ub_peak_summary.tsv
```

The view contains symlinks only; real dumps live once under
`Output/adapters/<adapter>/02_suffix/compiler_default`. To point validation at
another suffix dataset, set `CVPIPELINE_SUFFIX_ROOT`.

## Common Commands

```bash
bash cvpipeline_ub_model_cpp/scripts/build.sh
bash cvpipeline_ub_model_cpp/scripts/build_dev_tools.sh
bash cvpipeline_ub_model_cpp/scripts/run_unit_tests.sh
bash cvpipeline_ub_model_cpp/scripts/validate_comparator_sensitivity.sh
bash cvpipeline_ub_model_cpp/scripts/validate_memory_info_replay.sh
bash cvpipeline_ub_model_cpp/scripts/validate_local_ub_allocations.sh
bash cvpipeline_ub_model_cpp/scripts/validate_lifetimes.sh
bash cvpipeline_ub_model_cpp/scripts/validate_memory_plan.sh
bash cvpipeline_ub_model_cpp/scripts/validate_all.sh

# Small historical per-attempt regression.
PLAN_MEMORY_SEED=7 \
  bash cvpipeline_ub_model_cpp/scripts/dump_planmemory_attempt_oracle.sh
bash cvpipeline_ub_model_cpp/scripts/validate_planmemory_attempts.sh

# Exact acceptance: 166 PlanMemory-before inputs x seeds 0..19 = 3320 runs.
bash cvpipeline_ub_model_cpp/scripts/dump_fixed_seed_oracles.sh
bash cvpipeline_ub_model_cpp/scripts/validate_oracle_provenance.sh
bash cvpipeline_ub_model_cpp/scripts/validate_fixed_seed_exact.sh

# Exact non-default config matrix (an additional 3320 oracle runs).
bash cvpipeline_ub_model_cpp/scripts/dump_restrict_fixed_seed_oracles.sh
bash cvpipeline_ub_model_cpp/scripts/validate_config_matrix.sh

# Rich config matrix: 183 rows across five deterministic batches.
python3 cvpipeline_ub_model_cpp/scripts/validate_config_test_matrix.py
python3 cvpipeline_ub_model_cpp/scripts/run_config_test_matrix.py \
  --matrix cvpipeline_ub_model_cpp/testdata/config_matrix/all_configs.tsv
bash cvpipeline_ub_model_cpp/scripts/validate_generated_config_oracles.sh

# C0: 171 reachable adapters x 15 CVPipeline configs = 2565 tuples.
python3 cvpipeline_ub_model_cpp/scripts/dump_cvpipeline_after_oracles.py
python3 cvpipeline_ub_model_cpp/scripts/validate_cvpipeline_after_oracles.py \
  --rerun -1 --full-suffix-cross-check 2565
python3 cvpipeline_ub_model_cpp/scripts/validate_cvpipeline_oracle_sensitivity.py

# C1: all 2565 tuples map to 166 unique SemanticIR objects.
python3 cvpipeline_ub_model_cpp/scripts/dump_c1_semantic_oracles.py
python3 cvpipeline_ub_model_cpp/scripts/validate_c1_semantic_oracles.py --rerun
python3 cvpipeline_ub_model_cpp/scripts/validate_c1_semantic_sensitivity.py
bash cvpipeline_ub_model_cpp/scripts/validate_c1_focused_cfg.sh

# Focused B-stage semantic corpus: 35 cases x 20 seeds x 2 restrict modes.
python3 cvpipeline_ub_model_cpp/scripts/generate_planmemory_b_fixtures.py
bash cvpipeline_ub_model_cpp/scripts/run_planmemory_b_fixture_oracles.sh
bash cvpipeline_ub_model_cpp/scripts/validate_planmemory_b_fixtures.sh

# Five vv adapters: default/restrict x all 20 seeds = 200 runs.
bash cvpipeline_ub_model_cpp/scripts/run_vv_fixed_seed_oracles.sh
bash cvpipeline_ub_model_cpp/scripts/validate_vv_fixed_seed_oracles.sh
```

Reports are written to:

```text
cvpipeline_ub_model_cpp/output/reports/memory_info_replay_validation.tsv
cvpipeline_ub_model_cpp/output/reports/local_ub_allocations_validation.tsv
cvpipeline_ub_model_cpp/output/reports/lifetime_validation.tsv
cvpipeline_ub_model_cpp/output/reports/memory_plan_validation.tsv
cvpipeline_ub_model_cpp/output/reports/fixed_seed_lifetime_validation.tsv
cvpipeline_ub_model_cpp/output/reports/fixed_seed_plan_validation.tsv
```

The final-facing binary is:

```text
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model
```

The user-facing end-to-end wrapper starts from the before-CVPipeline generic IR
boundary and keeps CVPipeline options separate from suffix/PlanMemory options:

```bash
python3 cvpipeline_ub_model_cpp/scripts/plan_precvpipeline_ub.py \
  --pre-cvpipeline-ir=Output/index/d0_before_cvpipeline_generic/objects/<sha>/generic.mlir \
  --cv-pipeline-depth=-1 \
  --cv-enable-preload=false \
  --suffix-enable-auto-multi-buffer=false \
  --random-seed=0 \
  --format=text
```

Text output prints `peak_bits`, `required_bits`, `capacity_bits`, and the
buffer plan table. JSON output includes the same summary plus a parsed `plan`
array.

It exposes the production PlanMemory and initial C-suffix surfaces:

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=analyze-lifetimes \
  --before-planmemory-ir=...

cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=plan-local-memory \
  --before-planmemory-ir=... \
  --random-seed=7 \
  --format=json

# Initial C-stage delivery: C1 generic IR -> modeled suffix -> PlanMemory.
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=plan-c1-suffix \
  --c1-generic-ir=... \
  --enable-auto-multi-buffer \
  --limit-auto-multi-buffer-of-local-buffer=no-limit \
  --limit-auto-multi-buffer-buffer=no-limit \
  --random-seed=0 \
  --format=json
```

The initial C-stage boundary is the generic IR captured immediately before
OneShotBufferize. It is not CVPipeline-before IR; modeling CVPipelining itself
belongs to stage D. The C bridge is byte-exact on 165 of 166 unique validation
IRs (99.4%). The remaining dynamic stride-aligned allocation class is rejected
as a blocker by the production suffix entry instead of returning an estimated
UB peak. The frozen initial pass manifest is
`config/initial_suffix_manifest.tsv`.

Memory-info replay, allocation inspection and operation dumps are development
checks and remain in `dev_validate.cpp`/validation scripts rather than the
production CLI.

Development validation uses a separate binary:

```text
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model_dev_validate
```

The model targets the supported static AIV/UB PlanMemory-before IR contract.
The final acceptance protocol fixes the real PlanMemory seed and compares two
layers independently. Each side is independently normalized into the same TSV
schema and acceptance uses byte-for-byte string equality. The source dumps may
have different organization, but normalization does not discard information.
Buffer IDs and raw SSA names are both retained; order-sensitive gen/kill,
inplace, and offsets are never sorted.

Lifetime snapshots contain config, function name, stable buffer identity and
name, size/scope/ignore-inplace, alloc/free time, ordered final gen/kill,
ordered initial inplace pairs, and multi-buffer state. Transient non-buffer SSA
live sets are debugging inputs to `OpKillHandle`, not part of the lifetime output
contract. Plan snapshots contain config, function name, success/overflow, every
buffer's identity/name/const bits/ordered offsets, exact UB peak on success,
exact required bits on overflow, and capacity. The validator also checks oracle
completeness, rejects duplicate tuples, and requires exactly 3320 rows.

The older 11/30-attempt and 144-case checks remain useful development
regressions, but they are not a substitute for the fixed-seed acceptance run.
`scf.while`, `scope.scope`, `cf.br`/`cf.cond_br`, and preload lifetime expansion
follow the corresponding `MemLivenessAnalysis` paths. Dynamic local extent is
rejected because real `PlanMemory::GetBufferInfo` requires a static shape;
unknown local-memory operations also remain explicit blockers. Float
index-reduce and cumulative scalar classification now
reproduce the source unit-collapse, barrier, explicit-stride, and annotation
stride-alignment parts of `FlattenInterface`.
Zero AIV functions means an exact zero AIV UB peak. More than one AIV function
is rejected instead of silently merging independent PlanMemory invocations.
The non-default PlanMemory option `restrict-inplace-as-isa=true` is threaded
through every production lifetime/plan action and follows the source
`IsReuseHIVMOp` cutoff.

The checked-in HIVM operation registry is derived from the generated TableGen
declarations. Only real `DestinationStyleOpInterface` operations are handled as
DPS operations. `OptMemPlanForDma` classification follows structured/single-pipe
MTE2/MTE3 direction and `shouldLowerToScalarLoops`; unrecognized pipe or op
semantics are blockers.

The extended `Output/views/planmemory_all` check has exact UB peak for all
144 AIV cases. The planner follows the source `MemoryBound`/`PlanRecord` outline,
`MultiSpecPlan`/`SpecAlloc` levels 0-3, pipeline conflicts, multi-buffer
relations, rollback, retry, and level-0 overflow fallback.

The production liveness path intentionally keeps only UB-relevant PlanMemory
state.
Its names track the compiler source where practical:

```text
PlanMemory.cpp                      lightweight core
OpInfo / BufferStatus              OpInfo / BufferStatus
linearOperation                    linearOperation
buffer2status                      buffer2status
buffer2AliasVec                    BufferAliasMap / buffer2AliasVec
UpdateLinearOperation              UpdateLinearOperation
GetLiveBuffersInLoop               GetLiveBuffersInLoop
UpdateBufferAlias                  UpdateBufferAlias
UpdateOpGenInfo                    UpdateOpGenInfo
UpdateOperandGenInfo               UpdateOperandGenInfo
OpKillHandle                       OpKillHandle
UpdateOpKillInfo                   UpdateOpKillInfo
AllDeadAfter                       AllDeadAfter
GenerateBufferLife                 GenerateBufferLife
InitializeInplacePairList          InitializeInplacePairList
MemPlan::GenerateInplaceList       GenerateInplaceList
MemPlan::IsReuseHIVMOp             IsReuseHIVMOp
```

Legacy 11-case regression after the exact validator was tightened:

```text
validated_plan_cases=11
peak_failures=0
layout_failures=0
extended_cases=144
extended_peak_failures=0
```

The memory-plan validator includes layout in its exit status and is part of
`validate_all.sh`. The older 30-attempt oracle is retained only as a standalone
historical diagnostic because its semantic snapshot predates the exact oracle.

Current fixed-seed result:

```text
oracle complete: 3320 / 3320
lifetime byte exact: 3320 / 3320
plan byte exact: 3320 / 3320
both layers exact: 3320 / 3320
```

The current default `summary.tsv` predates provenance columns. It remains a
strong semantic regression, but `validate_oracle_provenance.sh` intentionally
rejects it. The highest-confidence acceptance run is: rebuild the suffix tool,
regenerate the oracle so every tuple records `input_sha256` and
`oracle_tool_sha256`, pass provenance validation, then run the two byte-exact
comparators. `validate_comparator_sensitivity.sh` verifies that changing one
lifetime field or one planned offset makes the corresponding comparator fail.

This result covers the default config. The separate restrict-config oracle and
validator use the same canonical byte-exact contract; both config summaries
must pass before claiming the full 6640-tuple config matrix.

The focused `testdata/planmemory_b` suite is a stricter coverage extension, not
a replacement for the 166-input baseline. Its real oracle is complete at
1400/1400 valid PlanMemory-before runs. The current model result is 266/1400
byte-identical lifetime snapshots and 358/1400 byte-identical plan snapshots,
so the newly exposed control-flow/normalization paths remain open.
See `testdata/planmemory_b/README.md` and the generated reports for the case
breakdown.

The 166-input `restrict-inplace-as-isa=true` oracle is also complete at
3320/3320 with current input/tool hashes. Model validation is 3060/3320 exact
for both lifetime and plan; the 260 mismatches are 13 attention-family inputs
times all 20 seeds. Restrict reports use the `_restrict.tsv` suffix so they do
not overwrite default-config reports.

Five content-unique adapters imported from `vv/` add another 200 current,
hash-bound tuples. All five reach local PlanMemory; their eventual compiler
failure is only the unavailable external `hivmc`. Model lifetime and plan
validation are both 200/200 byte exact. The accepted adapter corpus is now 171
inputs, or 6840 tuples across 20 seeds and two restrict modes. Including the 35
focused MLIRs, the complete stored test matrix contains 8240 tuples.
