# CVPipeline UB Model Post-Pipeline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the lightweight before-CVPipeline product so supported compiler-emitted MIX IR is transformed through the real default post-CVPipeline UB semantics and planned exactly per AIV function.

**Architecture:** Add an ordered header-only `post_cvpipeline` pipeline over `GenericModule`. Pre-split stages model UB-affecting tiling, size, and canonicalization; MIX functions are projected into one valid AIV module each; post-split stages model default inlining, sub-block tiling, code motion, tensor-empty ownership, and OTF rewrites before reusing the existing suffix and PlanMemory models. Coverage is explicit per real compiler stage, and unsupported forms produce `precision=incomplete` diagnostics rather than an exact claim.

**Tech Stack:** C++17 header-only model, generic-form MLIR text fixtures, Bash build/test scripts, Python 3 report wrapper, existing `cvpipeline_ub_model_cpp` PlanMemory implementation.

## Global Constraints

- Input is compiler-emitted generic MLIR immediately before `createCVPipeliningPass`.
- Exact default post-CVPipeline options are vector/cube tiling `2/2`, auto sub-block binding enabled, code motion enabled, and UB-saving disabled.
- Do not calculate L1/L0A/L0B/L0C capacity or placement; only exclude non-UB Cube buffers from AIV planning.
- Plan every projected AIV function independently; module peak and required capacity are per-function maxima, never sums.
- Unsupported configurations or operation forms return a best-effort result with `precision=incomplete` and structured diagnostics.
- Invalid SSA, unresolved physical sizes, checked arithmetic failures, or malformed input remain blockers.
- Do not restore or invoke the historical suffix compiler, fixed-seed oracle, or historical test suite.
- Preserve unrelated existing workspace changes in `third-party/llvm-project`, `third-party/torch-mlir`, `.worktrees/`, `CLAUDE.md`, and `cvpipeline_ub_model_cpp/output/`.
- Follow TDD for every task and commit only the files listed by that task.

---

## File Structure

Create focused headers under `cvpipeline_ub_model_cpp/src/post_cvpipeline/`:

- `types.hpp`: options, precision, diagnostics, per-stage coverage, and projected-module result types.
- `ir_utils.hpp`: safe generic IR mutation, function extraction, dictionary updates, use replacement, and validation.
- `core_type.hpp`: AIC/AIV/neutral classification from compiler-emitted attributes and known HIVM semantics.
- `tile_cube_vector_loop.hpp`: default `2/2` UB-relevant loop/slice transformation and rollback.
- `buffer_size.hpp`: AIV-reachable buffer-size inference, constantization, and physical-view modeling.
- `canonicalization.hpp`: the pre/post-split canonicalization patterns that affect bufferization or lifetime.
- `split_mix_aiv.hpp`: MIX cloning/filtering, result replacement, dependency closure, and vector cleanup.
- `inline_scope.hpp`: scope extraction and result mapping.
- `tile_bind_subblock.hpp`: default AIV sub-block tiling/binding, skip, fallback, and rollback semantics.
- `tensor_empty.hpp`: fold/clone tensor-empty ownership rules.
- `code_motion.hpp`: LICM and subset-hoisting moves used by the default pipeline.
- `inline_otf_load_store.hpp`: unaligned last-dimension concat/store rewrite.
- `pipeline.hpp`: ordered stage orchestration and manifest/coverage validation.
- `planning.hpp`: per-function suffix invocation, PlanMemory execution, and module aggregation.

Create tests under `cvpipeline_ub_model_cpp/tests/` with one C++ runner and focused fixture files. Modify the CLI and Python wrapper only after the semantic and planning APIs are stable.

---

### Task 1: Test Harness, Coverage Types, and Manifest

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/types.hpp`
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp`
- Create: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`
- Create: `cvpipeline_ub_model_cpp/tests/test_support.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/minimal_aiv.mlir`
- Create: `cvpipeline_ub_model_cpp/tests/run_tests.sh`

**Interfaces:**
- Produces: `PostCVPipelineOptions`, `CoverageDisposition`, `Precision`, `PostCVPipelineDiagnostic`, `StageCoverage`, `ProjectedAIVModule`, `PostCVPipelineResult`, and `RunPostCVPipelineAIVProjection(GenericModule, const PostCVPipelineOptions&)`.
- Consumes: existing `cvub::GenericModule`.

- [ ] **Step 1: Write the failing manifest/default-options tests**

Add tests that load the manifest, assert the exact stage order, and assert real compiler defaults:

```cpp
CVUB_TEST(post_pipeline_defaults_match_compiler) {
  const cvub::PostCVPipelineOptions options;
  CVUB_CHECK_EQ(options.tileMixVectorLoop, 2U);
  CVUB_CHECK_EQ(options.tileMixCubeLoop, 2U);
  CVUB_CHECK(options.enableAutoBindSubBlock);
  CVUB_CHECK(options.enableCodeMotion);
  CVUB_CHECK(!options.enableUbufSaving);
}

CVUB_TEST(post_pipeline_manifest_has_real_order) {
  const auto stages = cvub::LoadPostCVPipelineManifest(
      "cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv");
  CVUB_CHECK_EQ(cvub::StageNames(stages),
      std::vector<std::string>({
        "TileCubeVectorLoop", "InferAndSetBufferSize",
        "WorkspaceSemanticProjection", "CanonicalizationBeforeSplit",
        "CrossCoreSyncInvariant", "SplitMixKernelAIVProjection",
        "InlineScope", "TileAndBindSubBlock", "FoldTensorEmpty",
        "CanonicalizationAfterSplit", "LoopInvariantCodeMotion",
        "LoopInvariantSubsetHoisting", "CloneTensorEmpty",
        "InlineOTFLoadStore"}));
}

CVUB_TEST(ub_saving_configuration_is_reported_incomplete) {
  cvub::PostCVPipelineOptions options;
  options.enableUbufSaving = true;
  auto result = cvub::RunPostCVPipelineAIVProjection(
      ParseFixture("minimal_aiv.mlir"), options);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "configuration", "enableUbufSaving"));
}
```

- [ ] **Step 2: Run the test and verify the API is absent**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: compilation fails because `post_cvpipeline/types.hpp` and the declared types do not exist.

- [ ] **Step 3: Add the test runner and result types**

Define the public types exactly as follows:

```cpp
namespace cvub {
enum class Precision { Exact, Incomplete };
enum class CoverageDisposition { Modeled, UBInvariant, Unsupported };

struct PostCVPipelineOptions {
  unsigned tileMixVectorLoop = 2;
  unsigned tileMixCubeLoop = 2;
  bool enableAutoBindSubBlock = true;
  bool enableCodeMotion = true;
  bool enableUbufSaving = false;
};

struct PostCVPipelineDiagnostic {
  std::string pipelineStage;
  std::string function;
  int operationId = -1;
  std::string operation;
  std::string reason;
};

struct StageCoverage {
  std::string stage;
  CoverageDisposition disposition = CoverageDisposition::Unsupported;
};

struct ProjectedAIVModule {
  std::string sourceFunction;
  std::string projectedFunction;
  GenericModule module;
};

struct StageResult {
  GenericModule module;
  Precision precision = Precision::Exact;
  std::vector<PostCVPipelineDiagnostic> diagnostics;
};

struct PostCVPipelineResult {
  Precision precision = Precision::Exact;
  std::vector<ProjectedAIVModule> functions;
  std::vector<StageCoverage> coverage;
  std::vector<PostCVPipelineDiagnostic> diagnostics;
};
}
```

The Bash runner compiles with the same warning policy as the product:

```bash
c++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  -o cvpipeline_ub_model_cpp/output/tests/test_post_cvpipeline
cvpipeline_ub_model_cpp/output/tests/test_post_cvpipeline
```

Seed `post_cvpipeline_manifest.tsv` with the fourteen ordered rows and their initial dispositions. Stages implemented in later tasks start as `unsupported`; `WorkspaceSemanticProjection` and `CrossCoreSyncInvariant` start as `ub-invariant` only after their invariant tests land in Task 5.

- [ ] **Step 4: Implement manifest parsing and the initial orchestrator**

`LoadPostCVPipelineManifest` rejects duplicate stages, missing columns, and order mismatches. `RunPostCVPipelineAIVProjection` loads the compiled stage list, sets `Incomplete` when a stage is `Unsupported`, and returns a diagnostic naming that stage. It must not claim `Exact` before all encountered stages are modeled or proven invariant.

- [ ] **Step 5: Run the tests**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: all Task 1 tests print `[PASS]`; the runner exits 0.

- [ ] **Step 6: Commit**

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/types.hpp \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv \
  cvpipeline_ub_model_cpp/tests/test_support.hpp \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/tests/fixtures/minimal_aiv.mlir \
  cvpipeline_ub_model_cpp/tests/run_tests.sh
git commit -m "test: establish post-cvpipeline coverage contract"
```

### Task 2: Safe Generic IR Mutation and Function Isolation

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/ir_utils.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/after_bufferization_only.mlir`
- Modify: `cvpipeline_ub_model_cpp/src/semantic_ir/generic_rewriter.hpp`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`

**Interfaces:**
- Produces: `ReplaceAllUses`, `EraseOperationTree`, `MoveOperationBefore`, `SetDictionaryValue`, `ExtractFunctionModule`, `ValidateGenericModule`, and `ValidateBeforeCVPipelineBoundary`.
- Consumes: `GenericModule`, `GenericRewriter`, function symbol names.

- [ ] **Step 1: Write failing mutation and validation tests**

Cover all GenericOperation value-bearing fields, not just `operands`:

```cpp
CVUB_TEST(replace_all_uses_updates_dps_and_block_edges) {
  auto module = ParseFixture("generic_rewrite.mlir");
  const int oldValue = FindNamedValue(module, "%old");
  const int newValue = FindNamedValue(module, "%new");
  cvub::ReplaceAllUses(module, oldValue, newValue);
  CVUB_CHECK(!cvub::ModuleReferencesValue(module, oldValue));
  cvub::ValidateGenericModule(module);
}

CVUB_TEST(extract_function_keeps_required_declarations_only) {
  auto module = ParseFixture("two_mix_functions.mlir");
  auto isolated = cvub::ExtractFunctionModule(module, "first_mix_aiv");
  CVUB_CHECK_EQ(cvub::DeviceFunctionNames(isolated),
                std::vector<std::string>({"first_mix_aiv"}));
  cvub::ValidateGenericModule(isolated);
}

CVUB_TEST(boundary_validation_rejects_post_bufferization_only_input) {
  auto module = ParseFixture("after_bufferization_only.mlir");
  CVUB_CHECK_THROWS_CONTAINS(
      cvub::ValidateBeforeCVPipelineBoundary(module),
      "expected tensor-level before-CVPipeline device function");
}
```

- [ ] **Step 2: Verify the tests fail**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: compilation fails on the missing mutation helpers.

- [ ] **Step 3: Implement complete value replacement and compaction fixes**

`ReplaceAllUses` must update `operands`, `dpsInputs`, `dpsInits`, block
arguments passed through successor operands represented by the parser, and any
stage-owned value maps. Update `CompactGenericModule` so remapped value IDs are
also applied to `dpsInputs` and `dpsInits`; otherwise later split/clone stages
will carry stale IDs.

Use a single helper for all three operation vectors:

```cpp
inline void RemapValues(std::vector<int> &values,
                        const std::map<int, int> &mapping) {
  for (int &value : values) {
    const auto found = mapping.find(value);
    if (found != mapping.end())
      value = found->second;
  }
}
```

- [ ] **Step 4: Implement validation and extraction**

`ValidateGenericModule` verifies unique IDs, valid parent/region/block links,
operation ordinals, defined operands, valid successors, and matching DPS value
types. `ValidateBeforeCVPipelineBoundary` requires at least one non-host MIX or
AIV device function, a function core-type attribute, and tensor-level
operations expected at this boundary; it rejects an input containing only
post-bufferization local allocations. `ExtractFunctionModule` clones the
module root, the named function, and only transitively referenced declarations,
then compacts and validates the result.

- [ ] **Step 5: Run focused and regression tests**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Run: `bash cvpipeline_ub_model_cpp/build.sh`

Expected: tests pass and the product builds without warnings.

- [ ] **Step 6: Commit**

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/ir_utils.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/after_bufferization_only.mlir \
  cvpipeline_ub_model_cpp/src/semantic_ir/generic_rewriter.hpp \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp
git commit -m "feat: add safe generic IR mutation utilities"
```

### Task 3: Core Classification and MIX-to-AIV Projection

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/core_type.hpp`
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/split_mix_aiv.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/simple_mix_before_split.mlir`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/control_flow_mix_before_split.mlir`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`

**Interfaces:**
- Produces: `CoreKind ClassifyCore(const GenericModule&, const GenericOperation&)` and `PostCVPipelineResult ProjectMixFunctionsToAIV(GenericModule)`.
- Consumes: Task 2 mutation/validation helpers.

- [ ] **Step 1: Add failing simple MIX projection tests**

The fixture contains `mmadL1 -> fixpipe/workspace -> load -> vadd`. Assert:

```cpp
CVUB_TEST(split_mix_keeps_vector_and_workspace_boundary) {
  auto result = cvub::ProjectMixFunctionsToAIV(
      ParseFixture("simple_mix_before_split.mlir"));
  CVUB_CHECK_EQ(result.functions.size(), 1U);
  const auto &projected = result.functions.front();
  CVUB_CHECK_EQ(projected.projectedFunction, "kernel_mix_aiv");
  CVUB_CHECK(!HasOperation(projected.module, "hivm.hir.mmadL1"));
  CVUB_CHECK(!HasOperation(projected.module, "hivm.hir.fixpipe"));
  CVUB_CHECK(HasOperation(projected.module, "hivm.hir.load"));
  CVUB_CHECK(HasOperation(projected.module, "hivm.hir.vadd"));
  cvub::ValidateGenericModule(projected.module);
}
```

- [ ] **Step 2: Run and confirm the missing projection failure**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: compilation fails because `ProjectMixFunctionsToAIV` is absent.

- [ ] **Step 3: Implement core classification**

Classification precedence is:

1. `hivm.loop_core_type` on `scf.for`/`scope.scope`;
2. explicit operation `tcoretype`/core-type attributes emitted by the compiler;
3. known Cube operations (`mmadL1`, `batchMmadL1`, Conv1d/2d/3dL1,
   `fixpipe` where compiler semantics classify it Cube);
4. known Vector operations from the existing HIVM destination-style registry;
5. structural/arith/tensor/memref/annotation operations as `Neutral`;
6. `Unknown`, which adds an incomplete diagnostic.

Use:

```cpp
enum class CoreKind { Cube, Vector, Neutral, Unknown };
struct CoreClassification {
  CoreKind kind = CoreKind::Unknown;
  std::string reason;
};
```

- [ ] **Step 4: Implement function cloning and filtering**

Clone each MIX function as `<name>_mix_aiv`, update its function attributes,
walk children post-order, and remove Cube operations. Result replacement uses
this exact decision order:

```cpp
std::vector<int> ReplacementValues(const GenericModule &module,
                                   const GenericOperation &operation) {
  if (!operation.dpsInits.empty())
    return operation.dpsInits;
  if (operation.name == "scf.for" || operation.name == "scf.while")
    return LoopInitialValues(module, operation);
  if (operation.name == "scf.if" || operation.name == "scope.scope")
    return YieldedOrExternalValues(module, operation);
  if (operation.results.empty())
    return {};
  throw UnsupportedProjection(operation.id, operation.name,
                              "no result replacement rule");
}
```

Retain the dependency closure for neutral definitions, preserve workspace/load
edges, run vector-side mark/extract cleanup, compact, isolate one AIV function
per module, and validate SSA.

- [ ] **Step 5: Add control-flow and incomplete tests**

Cover DPS init replacement, Cube loop iter-arg replacement, `scf.if` yields,
single-block scope yields, already-AIV passthrough, multiple MIX functions, and
an unknown custom op that preserves legal SSA while setting `Incomplete`.

- [ ] **Step 6: Run tests and build**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Run: `bash cvpipeline_ub_model_cpp/build.sh`

Expected: all projection tests pass; build succeeds.

- [ ] **Step 7: Mark the split stage modeled and commit**

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/core_type.hpp \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/split_mix_aiv.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/simple_mix_before_split.mlir \
  cvpipeline_ub_model_cpp/tests/fixtures/control_flow_mix_before_split.mlir \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv
git commit -m "feat: project MIX functions to AIV semantics"
```

### Task 4: Default Cube/Vector Loop Tiling

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/tile_cube_vector_loop.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/tile_vector_loop.mlir`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/tile_cube_loop.mlir`
- Modify: `cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`

**Interfaces:**
- Produces: `StageResult RunTileCubeVectorLoop(GenericModule, unsigned vectorTripCount, unsigned cubeTripCount)`.
- Consumes: compiler-emitted pipelined-loop attributes and Task 2 IR utilities.

- [ ] **Step 1: Add failing vector/cube tiling golden tests**

Assert default trip counts produce the expected ceil-div tile size, slice
shapes, fused producer placement, and shrunk destination allocation. Add a
second fixture whose real legality checks require rollback and assert byte-for-
byte equivalent serialized IR after compaction.

```cpp
CVUB_TEST(default_vector_tiling_shrinks_local_destination) {
  auto result = cvub::RunTileCubeVectorLoop(
      ParseFixture("tile_vector_loop.mlir"), 2, 2);
  CVUB_CHECK_EQ(ResultTypeOf(result.module, "hivm.hir.vadd"),
                "tensor<1x64xf16>");
  CVUB_CHECK_EQ(AllocationShape(result.module, "%vector_dst"),
                std::vector<int64_t>({1, 64}));
}
```

- [ ] **Step 2: Verify tests fail**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: compilation fails on `RunTileCubeVectorLoop`.

- [ ] **Step 3: Implement candidate collection and legality analysis**

Match only loops/scopes carrying `hivm.loop_core_type`. Compute a single tiling
dimension, `tileSize = ceilDiv(extent, tripCount)`, and collect producers that
the real pass fuses into the tiled body. The analysis returns either a complete
`TilingPlan` or a reason; a reason that corresponds to a real rollback leaves
IR unchanged and remains exact, while an unimplemented successful pattern
marks the stage incomplete.

- [ ] **Step 4: Implement transactional rewrite**

Rewrite a cloned module, validate it, shrink eligible allocs, and commit the
clone only on success:

```cpp
GenericModule candidate = module;
ApplyTilingPlan(candidate, plan);
ShrinkTiledAllocations(candidate, plan);
candidate = CompactGenericModule(std::move(candidate));
ValidateGenericModule(candidate);
module = std::move(candidate);
```

Preserve slice offsets/sizes/strides and the loop core attributes consumed by
Task 3.

- [ ] **Step 5: Run tests, mark stage modeled, and commit**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/tile_cube_vector_loop.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/tile_vector_loop.mlir \
  cvpipeline_ub_model_cpp/tests/fixtures/tile_cube_loop.mlir \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv
git commit -m "feat: model default CV loop tiling"
```

### Task 5: Buffer Sizes, Canonicalization, Workspace, and Sync Dispositions

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/buffer_size.hpp`
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/canonicalization.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/dynamic_buffer_size.mlir`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/workspace_sync_invariant.mlir`
- Modify: `cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`

**Interfaces:**
- Produces: `RunInferAndSetBufferSize`, `RunPreSplitCanonicalization`, `VerifyWorkspaceUBInvariant`, and `VerifyCrossCoreSyncUBInvariant`.
- Consumes: Task 3 AIV reachability/core classification.

- [ ] **Step 1: Write failing physical-size and invariant tests**

Cover dynamic memref annotation inference, conflicting marks, static-size mark
removal, iter-arg simplification, dead tensor-empty removal, CSE, workspace
offset changes, and inserted operandless sync operations.

```cpp
CVUB_TEST(workspace_offsets_and_operandless_sync_do_not_change_ub_projection) {
  auto baseline = ParseFixture("workspace_sync_invariant.mlir");
  auto changed = baseline;
  ChangeWorkspaceOffsets(changed);
  InsertOperandlessCrossCoreSync(changed);
  CVUB_CHECK_EQ(ProjectedUBSignature(baseline), ProjectedUBSignature(changed));
}
```

- [ ] **Step 2: Verify the new tests fail**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: compilation fails on the new stage functions.

- [ ] **Step 3: Implement buffer-size semantics**

For functions carrying the compiler auto-size marker, derive element count from
an existing `buffer_size_in_byte` annotation, annotate unmarked dynamic allocs,
constantize supported affine expressions, and represent `SetBufferSize` as the
same physical view signature consumed by existing suffix size parsing.
Conflicting sizes remain blockers.

- [ ] **Step 4: Implement targeted canonicalization**

Add fixed-point patterns for constant folding used by size/slice expressions,
unused tensor-empty/mark removal, iter-arg init/yield equality, single-point
loops, dead pure operations, and exact-key CSE. Apply before split in compiler
order; post-split reuse is wired in Task 6.

- [ ] **Step 5: Implement and test invariant guards**

`VerifyWorkspaceUBInvariant` rejects any workspace rewrite that changes a
local result type, AIV load result shape, or alias source. `VerifyCrossCoreSyncUBInvariant`
accepts only sync operations with no local buffer operand/result. A violated
guard marks precision incomplete and names the operation.

- [ ] **Step 6: Run tests, update manifest, and commit**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/buffer_size.hpp \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/canonicalization.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/dynamic_buffer_size.mlir \
  cvpipeline_ub_model_cpp/tests/fixtures/workspace_sync_invariant.mlir \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv
git commit -m "feat: model pre-split size and canonicalization semantics"
```

### Task 6: Scope, Tensor-Empty, and Post-Split Canonicalization

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/inline_scope.hpp`
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/tensor_empty.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/scope_tensor_empty.mlir`
- Modify: `cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`

**Interfaces:**
- Produces: `RunInlineScope`, `RunFoldTensorEmpty`, `RunCloneTensorEmpty`, and reuse of `RunPostSplitCanonicalization`.
- Consumes: Task 2 IR mutation and Task 5 canonicalization.

- [ ] **Step 1: Add failing scope/fold/clone tests**

The fixture contains a single-block scope with two results, an empty insert
slice, two HIVM structured ops sharing one destination empty, and loop/while
empty inits. Assert scope body order, result replacement, folded dead empty,
and one distinct empty per destination owner.

- [ ] **Step 2: Confirm failure**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: missing stage function compilation failures.

- [ ] **Step 3: Implement scope extraction**

Skip `no_inline` scopes. For every other one-block scope, move all body
operations except the terminator immediately before the scope, replace each
scope result with its matching terminator operand, erase the scope tree, and
validate operation order and SSA.

- [ ] **Step 4: Implement tensor-empty ownership rules**

Fold empty-source insert slices to their destinations. Clone tensor-empty
destinations immediately before each HIVM structured operation, loop init, and
tensor insert. Copy only `buffer_size_in_byte` marks to the clone, matching the
compiler pass.

- [ ] **Step 5: Run post-split canonicalization and tests**

Call Task 5 canonicalization after fold and before code motion. Assert the
serialized fixture matches its checked golden output.

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

- [ ] **Step 6: Update manifest and commit**

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/inline_scope.hpp \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/tensor_empty.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/scope_tensor_empty.mlir \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv
git commit -m "feat: model scope and tensor-empty rewrites"
```

### Task 7: Default AIV Sub-Block Tiling and Binding

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/tile_bind_subblock.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/subblock_bind_success.mlir`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/subblock_bind_fallback.mlir`
- Modify: `cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`

**Interfaces:**
- Produces: `StageResult RunTileAndBindSubBlock(GenericModule, bool enableTile)`.
- Consumes: isolated AIV modules from Task 3 and Task 2 transactional rewrites.

- [ ] **Step 1: Add failing success, skip, fallback, and rollback tests**

Base fixtures on the existing compiler tests in
`bishengir/test/Dialect/HIVM/tile-and-bind-sub-block.mlir`. Check resulting
slice shapes, cloned loop/store structure, sub-block restriction marks, and
physical local destination sizes.

- [ ] **Step 2: Verify failure**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: compilation fails on `RunTileAndBindSubBlock`.

- [ ] **Step 3: Implement early patterns and eligibility analysis**

Mirror the real decision order: disable attribute, collect AIV/AIC MIX parts,
apply early patterns, reject batch-matmul/implicit-transpose/same-address
hazards, derive AIV tiling dimension, and produce a complete transactional
plan. `enableTile=false` still applies `limitUniqueSubBlockToStore`.

- [ ] **Step 4: Implement AIV tiling transaction and fallback**

Apply the plan to a clone, bubble subviews from tiling, limit stores to one
sub-block, validate, and replace the original only on success. On a recognized
real-pass failure, restore the original and apply the same store restriction.
On an unmodeled potentially successful form, keep legal IR and emit an
incomplete diagnostic.

- [ ] **Step 5: Run tests, mark modeled, and commit**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/tile_bind_subblock.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/subblock_bind_success.mlir \
  cvpipeline_ub_model_cpp/tests/fixtures/subblock_bind_fallback.mlir \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv
git commit -m "feat: model default AIV sub-block binding"
```

### Task 8: Code Motion and OTF Load/Store Rewrites

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/code_motion.hpp`
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/inline_otf_load_store.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/code_motion_lifetime.mlir`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/unaligned_concat_store.mlir`
- Modify: `cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`

**Interfaces:**
- Produces: `RunLoopInvariantCodeMotion`, `RunLoopInvariantSubsetHoisting`, and `RunInlineOTFLoadStore`.
- Consumes: valid isolated AIV modules after Task 7.

- [ ] **Step 1: Add failing lifetime and concat tests**

For code motion, assert invariant pure definitions move before the loop while
dependent operations remain, and assert the expected before/after gen-kill
interval. For concat/store, cover aligned no-op and unaligned rewrite; assert
the concat destination and its size mark become dead only in the unaligned
case.

- [ ] **Step 2: Verify failure**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: missing function compilation failures.

- [ ] **Step 3: Implement LICM and subset hoisting**

Move only side-effect-free operations whose operands are defined outside the
loop or by already-hoisted operations. For subset operations, hoist invariant
source/view construction while preserving dynamic index dependencies and
yield order. Iterate to a fixed point and validate after each loop.

- [ ] **Step 4: Implement unaligned concat/store rewrite**

Match a pure-tensor `vconcat` feeding `hivm.hir.store` on the last dimension.
If every cumulative input extent in bytes is block aligned, leave it unchanged.
Otherwise build the insert-slice accumulator, replace the store source, remove
`buffer_size_in_byte` marks on concat, and run dead-operation cleanup.

- [ ] **Step 5: Run tests, update manifest, and commit**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/code_motion.hpp \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/inline_otf_load_store.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/code_motion_lifetime.mlir \
  cvpipeline_ub_model_cpp/tests/fixtures/unaligned_concat_store.mlir \
  cvpipeline_ub_model_cpp/src/post_cvpipeline/pipeline.hpp \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv
git commit -m "feat: model post-split lifetime rewrites"
```

### Task 9: Per-Function Suffix Planning and Module Aggregation

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/post_cvpipeline/planning.hpp`
- Modify: `cvpipeline_ub_model_cpp/src/suffix/planmemory_bridge.hpp`
- Modify: `cvpipeline_ub_model_cpp/src/suffix/suffix_pipeline.hpp`
- Create: `cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`

**Interfaces:**
- Produces: `FunctionPlanResult`, `ModulePlanResult`, and `PlanProjectedAIVFunctions(const PostCVPipelineResult&, const SuffixPipelineOptions&, std::optional<uint32_t>, bool)`.
- Consumes: `ProjectedAIVModule`, existing `BuildSuffixPlanMemoryInput`, `PlanLocalMemory`, and `PlanLocalMemoryForSeed`.

- [ ] **Step 1: Add failing independent-planning tests**

```cpp
CVUB_TEST(module_peak_is_max_not_sum) {
  auto projected = MakeTwoAIVModulesWithPeaks(65536, 98304);
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK_EQ(result.peakBits, 98304U);
  CVUB_CHECK_EQ(result.requiredBits, 98304U);
}
```

Also test one overflow plus one success, different selected retry seeds, fixed
seed propagation, duplicate buffer names in different functions, and empty AIV
modules.

- [ ] **Step 2: Confirm the API is absent**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected: compilation fails on `PlanProjectedAIVFunctions`.

- [ ] **Step 3: Add result types and plan each function independently**

```cpp
struct FunctionPlanResult {
  std::string sourceFunction;
  std::string function;
  PlanMemoryModelResult plan;
};

struct ModulePlanResult {
  Precision precision = Precision::Exact;
  bool success = true;
  bool overflow = false;
  uint64_t peakBits = 0;
  uint64_t requiredBits = 0;
  uint64_t capacityBits = kUBCapacityBits;
  std::vector<FunctionPlanResult> functions;
  std::vector<PostCVPipelineDiagnostic> diagnostics;
};
```

For each isolated module, run the existing suffix once, then PlanMemory once.
Remove the bridge's multiple-AIV blocker only by enforcing the new invariant
that each supplied module contains exactly one AIV function; do not merge
function plans.

- [ ] **Step 4: Implement aggregation**

Set module peak/required to the maxima. A successful function's effective
requirement is `max(plan.peakBits, plan.requiredBits)`; an overflowing
function's effective requirement is `plan.requiredBits`. Module overflow is
true when any function overflows; module success requires all functions to
succeed. Preserve each function's selected seed rather than inventing a module
seed when they differ.

- [ ] **Step 5: Run tests and existing demos**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Run: `bash cvpipeline_ub_model_cpp/build.sh`

Run: `cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model --action=plan-before-cvpipeline --before-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_vector_add_before_cvpipeline.mlir --format=json`

Expected: tests pass; the pure-AIV vector demo still reports `65536` peak bits.

- [ ] **Step 6: Commit**

```bash
git add cvpipeline_ub_model_cpp/src/post_cvpipeline/planning.hpp \
  cvpipeline_ub_model_cpp/src/suffix/planmemory_bridge.hpp \
  cvpipeline_ub_model_cpp/src/suffix/suffix_pipeline.hpp \
  cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp
git commit -m "feat: plan projected AIV functions independently"
```

### Task 10: Product CLI, Structured Diagnostics, and Report Compatibility

**Files:**
- Modify: `cvpipeline_ub_model_cpp/src/cli/cvpipeline_ub_model.cpp`
- Modify: `cvpipeline_ub_model_cpp/scripts/plan_precvpipeline_ub.py`
- Modify: `cvpipeline_ub_model_cpp/scripts/print_ub_plan_summary.py`
- Modify: `cvpipeline_ub_model_cpp/scripts/render_ub_demo_html.py`
- Modify: `cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Create: `cvpipeline_ub_model_cpp/tests/test_report.py`

**Interfaces:**
- Produces: stable text and JSON reports with module and per-function results.
- Consumes: Task 9 `ModulePlanResult`.

- [ ] **Step 1: Add failing JSON/text contract tests**

Expected JSON shape:

```json
{
  "precision": "exact",
  "status": "success",
  "ub_peak_bits": 98304,
  "required_bits": 98304,
  "assumed_post_cvpipeline_options": {
    "tile_mix_vector_loop": 2,
    "tile_mix_cube_loop": 2,
    "enable_auto_bind_sub_block": true,
    "enable_code_motion": true,
    "enable_ubuf_saving": false
  },
  "functions": [],
  "diagnostics": []
}
```

Text buffer rows gain a leading `function` column. Tests also cover
`incomplete+success`, `exact+overflow`, and blocker JSON.

- [ ] **Step 2: Run report tests and confirm failure**

Run: `python3 cvpipeline_ub_model_cpp/tests/test_report.py`

Expected: assertions fail because per-function and diagnostic fields are absent.

- [ ] **Step 3: Wire the new pipeline into only `plan-before-cvpipeline`**

Replace the direct CVPipeline-to-suffix call with:

```cpp
module = cvub::RunCVPipeliningPass(std::move(module),
                                   cvPipeliningOptions(opts));
const cvub::PostCVPipelineResult projected =
    cvub::RunPostCVPipelineAIVProjection(
        std::move(module), cvub::PostCVPipelineOptions{});
const cvub::ModulePlanResult result = cvub::PlanProjectedAIVFunctions(
    projected, suffixPipelineOptions(opts), opts.randomSeed,
    opts.restrictInplaceAsISA);
```

Leave `plan-before-one-shot-bufferize` and `plan-local-memory` behavior
unchanged.

- [ ] **Step 4: Emit compatible reports**

Keep current top-level capacity/peak/required/overflow/inplace fields. Add
function arrays, diagnostics, per-stage coverage, and assumed defaults. Return
0 for success, 2 for overflow, and 1 for blockers; incomplete precision does
not alter the status-derived code.

- [ ] **Step 5: Update wrapper and visualizer**

Teach the Python parser the new text function column and JSON fields. Group
buffers by function in the summary and visualizer; display module max rather
than summing chart extents across functions.

- [ ] **Step 6: Run report, C++, and build tests**

Run: `python3 cvpipeline_ub_model_cpp/tests/test_report.py`

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Run: `bash cvpipeline_ub_model_cpp/build.sh`

Expected: all pass with no warnings.

- [ ] **Step 7: Commit**

```bash
git add cvpipeline_ub_model_cpp/src/cli/cvpipeline_ub_model.cpp \
  cvpipeline_ub_model_cpp/scripts/plan_precvpipeline_ub.py \
  cvpipeline_ub_model_cpp/scripts/print_ub_plan_summary.py \
  cvpipeline_ub_model_cpp/scripts/render_ub_demo_html.py \
  cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/tests/test_report.py
git commit -m "feat: expose per-function CVPipeline UB plans"
```

### Task 11: End-to-End MIX Fixture, Suffix Audit, and Documentation

**Files:**
- Create: `cvpipeline_ub_model_cpp/examples/inputs/demo_mix_before_cvpipeline.mlir`
- Modify: `cvpipeline_ub_model_cpp/config/initial_suffix_manifest.tsv`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`
- Modify: `cvpipeline_ub_model_cpp/README.md`
- Modify: `cvpipeline_ub_model_cpp/run_demo_ub_plan.sh`
- Modify: `cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp`
- Modify: `cvpipeline_ub_model_cpp/tests/test_report.py`

**Interfaces:**
- Produces: final product demo and checked evidence that every stage has a coverage disposition.
- Consumes: all previous tasks.

- [ ] **Step 1: Add the failing end-to-end MIX acceptance test**

The demo must contain a real compiler-boundary MIX function with Cube,
workspace, load, Vector, pipeline loop, and enough structure to exercise
default `2/2` tiling, split, sub-block binding, code motion, and tensor-empty
ownership. The test asserts the reviewed golden AIV buffer names, extents,
multi-buffer counts, lifetimes, placements, function peak, and module peak.

- [ ] **Step 2: Run the acceptance test and inspect the first mismatch**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Expected before final adjustments: the new MIX golden test fails with a
specific buffer, lifetime, or placement mismatch.

- [ ] **Step 3: Audit the existing suffix manifest against the new boundary**

For every suffix row, confirm its input assumption after the post-CVPipeline
pipeline. Add an `input_contract` column documenting required operation forms
and ensure a test exercises each row on an AIV module. If a suffix stage sees
an unsupported new form, add a structured incomplete diagnostic at the owning
stage rather than silently dropping the buffer.

- [ ] **Step 4: Resolve acceptance mismatches at their owning stage**

Change only the stage responsible for each mismatch and add a focused
regression assertion before rerunning the full MIX test. Do not compensate in
PlanMemory for incorrect pre-bufferization semantics.

- [ ] **Step 5: Update product documentation and demo command**

Document the exact input boundary, real assumed defaults, per-function max
semantics, exact/incomplete meanings, unsupported UB-saving/non-default
post-pipeline configurations, and the new MIX demo command. Remove wording
that implies only CVPipelining and suffix are modeled.

- [ ] **Step 6: Run the complete verification set**

Run: `bash cvpipeline_ub_model_cpp/tests/run_tests.sh`

Run: `python3 cvpipeline_ub_model_cpp/tests/test_report.py`

Run: `bash cvpipeline_ub_model_cpp/build.sh`

Run: `cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model --action=plan-before-cvpipeline --before-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_vector_add_before_cvpipeline.mlir --format=json`

Expected: `status=success`, `precision=exact`, `ub_peak_bits=65536`.

Run: `cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model --action=plan-before-cvpipeline --before-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir --format=json`

Expected: `status=success`, `precision=exact`, `ub_peak_bits=23904`, `required_bits=24064` if the current output contract continues reporting both values for successful retry planning.

Run: `cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model --action=plan-before-cvpipeline --before-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_mix_before_cvpipeline.mlir --format=json`

Expected: `status=success`, `precision=exact`, one `kernel_mix_aiv` function, and module peak equal to that function peak.

- [ ] **Step 7: Inspect workspace scope and commit only feature files**

Run: `git status --short`

Expected: feature files plus the pre-existing unrelated changes listed in
Global Constraints; none of those unrelated paths are staged.

```bash
git add cvpipeline_ub_model_cpp/examples/inputs/demo_mix_before_cvpipeline.mlir \
  cvpipeline_ub_model_cpp/config/initial_suffix_manifest.tsv \
  cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv \
  cvpipeline_ub_model_cpp/README.md \
  cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  cvpipeline_ub_model_cpp/tests/test_post_cvpipeline.cpp \
  cvpipeline_ub_model_cpp/tests/test_report.py
git commit -m "docs: complete CVPipeline MIX UB workflow"
```

---

## Final Review Checklist

- Every real stage between CVPipeline and OneShotBufferize has a tested
  `modeled`, `ub-invariant`, or `unsupported` disposition.
- Supported default MIX inputs report `precision=exact`; no unsupported path
  can leave precision exact.
- All projected modules pass `ValidateGenericModule` before suffix modeling.
- Only AIV UB buffers reach local PlanMemory; Cube L1/L0C buffers are absent.
- Physical sizes, multi-buffer counts, lifetimes, and offsets are asserted in
  the MIX golden fixture.
- Multiple AIV functions are independently planned and aggregated by maximum.
- Existing pure-AIV demo results remain unchanged.
- Text, JSON, summary, and visualization consumers agree on per-function and
  module semantics.
- Historical oracle infrastructure remains untouched.
- Unrelated dirty workspace paths remain unmodified and unstaged.
