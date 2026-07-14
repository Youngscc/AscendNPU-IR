# CVPipeline UB Model Post-Pipeline Design

## 1. Context

The lightweight `cvpipeline_ub_model_cpp` product accepts generic MLIR at the
compiler boundary immediately before `createCVPipeliningPass`. It currently
models CVPipelining and then enters its modeled OneShotBufferize/suffix path
directly.

The real compiler does not do that. Between CVPipelining and OneShotBufferize,
the default HIVM pipeline performs loop tiling, buffer-size propagation,
workspace planning, canonicalization, cross-core synchronization, MIX kernel
splitting, scope inlining, sub-block tiling/binding, code motion, tensor-empty
rewrites, and OTF load/store rewrites. Several of these transformations change
which operations belong to AIV, the number and physical size of local buffers,
or their lifetimes.

The current model also filters PlanMemory input to functions already marked
`AIV`. This is valid for a post-split AIV function, but it silently misses a
before-CVPipeline MIX function because the real `SplitMixKernel` pass runs
later.

The feature goal is exact UB planning, not full modeling of every NPU memory
space. L1/L0C capacity and placement remain out of scope. Cube-side buffers
must nevertheless be identified well enough to exclude them from AIV UB, and
cross-core workspace/load boundaries must be preserved because they determine
the AIV data flow.

## 2. Goals

For supported inputs and configurations, the product must exactly reproduce:

- the AIV functions created from before-CVPipeline MIX functions;
- the set of AIV UB buffers;
- each buffer's physical size and multi-buffer count;
- buffer aliasing and lifetime;
- each AIV function's local PlanMemory placement and peak;
- the module peak, defined as the maximum AIV function peak.

The product must continue to support already split, pure AIV inputs.

## 3. Input Contract

The input is restricted to generic MLIR emitted by the real compiler at the
boundary immediately before `createCVPipeliningPass`.

The model does not infer workspace insertion, load/store insertion, function
core type, or other semantics belonging to earlier compiler stages. Those must
already be present in the input in the form produced by the compiler.

The first delivery supports:

- the real compiler's default post-CVPipeline configuration;
- the CVPipeline options already exposed by the product: disable pipelining,
  pipeline depth, preload, and lazy loading;
- the suffix options already exposed by the product: auto multi-buffer and its
  local/MIX strategies;
- fixed or retry-mode PlanMemory seeds and the existing inplace restriction.

The real compiler defaults relevant to this design are:

- `tileMixVectorLoop = 2`;
- `tileMixCubeLoop = 2`;
- `enableAutoBindSubBlock = true`;
- `enableCodeMotion = true`;
- `enableUbufSaving = false`.

The product records these assumed post-CVPipeline defaults in its report.
Inputs intended for a different unexposed configuration are outside the exact
coverage contract.

## 4. Non-Goals

This delivery does not:

- calculate L1, L0A, L0B, or L0C capacity or placement;
- restore the historical suffix compiler, fixed-seed oracle, or historical
  test suite from `yy/local/memory-modeling`;
- change every product entry point to fail closed;
- link the lightweight product against the complete MLIR/bishengir compiler;
- reimplement compiler stages before the before-CVPipeline input boundary;
- support `enableUbufSaving=true` or non-default values of post-CVPipeline
  options that are not currently exposed by the product.

## 5. Architecture

The `plan-before-cvpipeline` action becomes:

```text
before-CVPipeline GenericModule
  -> existing CVPipelining model
  -> ordered post-CVPipeline UB semantic pipeline
  -> existing OneShotBufferize model
  -> existing post-bufferization/suffix model
  -> PlanMemory independently for each AIV function
  -> per-function reports and module max peak
```

The new implementation lives under:

```text
cvpipeline_ub_model_cpp/src/post_cvpipeline/
```

Its public boundary is:

```cpp
PostCVPipelineResult RunPostCVPipelineAIVProjection(
    GenericModule module,
    const PostCVPipelineOptions &options);
```

`PostCVPipelineResult` contains:

- the transformed `GenericModule` containing the AIV-side semantics needed by
  OneShotBufferize;
- a mapping from each source function to its projected AIV function;
- ordered per-stage coverage records;
- an aggregate precision of `exact` or `incomplete`;
- structured diagnostics for unsupported configurations or operation forms.

Each modeled stage has one responsibility and consumes/returns
`GenericModule`. Stage internals may use focused analysis records, but those
records do not leak across the public pipeline boundary.

## 6. Pipeline Coverage Rule

Every real compiler pass from the input boundary to local PlanMemory must have
an explicit disposition:

- `modeled`: reproduce the pass's UB-relevant semantic effect;
- `ub-invariant`: prove and test that the pass cannot change the UB buffer set,
  size, aliasing, lifetime, or peak;
- `unsupported`: continue with a best-effort result and mark precision
  `incomplete`.

No real stage may be silently omitted. A checked manifest mirrors the real
pipeline order and prevents a future compiler stage from disappearing from the
lightweight model without review.

The ordered post-CVPipeline stage registry is:

```text
TileCubeVectorLoop
InferAndSetBufferSize
WorkspaceSemanticProjection
CanonicalizationBeforeSplit
CrossCoreSyncInvariant
SplitMixKernelAIVProjection
InlineScope
TileAndBindSubBlock
FoldTensorEmpty
CanonicalizationAfterSplit
LoopInvariantCodeMotion
LoopInvariantSubsetHoisting
CloneTensorEmpty
InlineOTFLoadStore
```

The existing suffix manifest continues to cover the modeled OneShotBufferize
and post-bufferization stages. It must be audited as part of the implementation
to ensure that the new pre-bufferization output satisfies its assumptions.

## 7. Stage Semantics

### 7.1 TileCubeVectorLoop

The default `2/2` vector/cube tiling behavior is required for exact coverage.
The model reproduces the UB-relevant loop, slice, producer-fusion, and alloc
shrink effects. It preserves the attributes required by later MIX splitting.

Patterns for which the real pass rolls back must also roll back in the model.
An unmodeled successful compiler pattern produces `incomplete`, not an exact
untiled result.

### 7.2 Infer and Set Buffer Size

Buffer-size annotations are propagated and constantized for values that can
survive into the AIV projection. Dynamic allocation sizes converted to physical
views must be represented so that later bufferization obtains the same local
buffer size.

### 7.3 Workspace Semantic Projection

`GLOBAL_WORKSPACE_PLAN` plans GM workspace, not local UB. The lightweight model
does not reproduce GM placement. It preserves:

- workspace identity and alias relationships;
- result types and buffer-size information consumed by AIV load/store paths;
- the Cube-to-Vector workspace boundary.

Workspace offsets alone are tested as a UB invariant.

### 7.4 Canonicalization Before Split

The model implements the canonicalization rules that can change tensor/loop
SSA, dead operations, CSE, iter-args, or subsequent bufferization decisions.
Rules with no possible path to AIV local buffers may be classified as
UB-invariant with a test.

### 7.5 Cross-Core Synchronization

The default cross-core synchronization stages insert synchronization
operations without local-buffer operands. They may change operation numbering
but not UB gen/kill topology. The model therefore treats synchronization
insertion as UB-invariant while retaining the core classification needed by the
subsequent split.

If a future synchronization form carries a local buffer operand or creates a
local allocation, the invariant test fails and the stage requires modeling.

### 7.6 MIX to AIV Projection

`SplitMixKernelAIVProjection` uses the real `SplitMixKernel` behavior as its
semantic source but creates only the AIV side needed for UB planning.

For each MIX function:

- clone it as `<source>_mix_aiv`;
- set `func_core_type=AIV` and `part_of_mix`;
- retain Vector operations;
- remove Cube operations;
- retain neutral operations, constants, views, casts, and control flow when
  required by the AIV SSA dependency closure;
- use operation and pipelined-loop core attributes in the same precedence as
  the compiler;
- preserve cross-core workspace/load boundaries;
- run the vector-side post-split cleanup.

The original AIC body is not sent to local UB planning. Necessary declarations
remain available for valid symbol references.

Cube operation result replacement mirrors the compiler:

- destination-style results use their corresponding init values;
- removed loop results use initial iter-args;
- `scf.if` and scope results use their yielded or externally defined values;
- Cube-only tensor-empty, slice, mark, and duplicate-extract chains are removed
  after reachability cleanup.

The resulting module must contain no dangling SSA uses.

Already-AIV functions bypass MIX splitting but still pass through the required
post-split stages. AIC and host functions do not participate in UB PlanMemory.

Unknown core types, distributed custom operations without compiler-emitted core
classification, and unsupported region forms preserve the dependency closure
needed for legal SSA and add an `incomplete` diagnostic.

### 7.7 InlineScope

Inline single-block scopes in compiler order, move their body operations, map
scope results to terminator operands, and then apply the relevant inlining
cleanup. `no_inline` behavior follows the real pass options.

### 7.8 TileAndBindSubBlock

The default enabled behavior is required. The model reproduces the AIV-side
tiling/binding effects that alter slices, cloned operations, local buffer
shapes, and stores. It covers the real pass's successful, skipped, fallback,
and rollback paths for the supported input forms.

### 7.9 Tensor-Empty and Canonicalization Stages

`FoldTensorEmpty` and the following canonicalization remove redundant storage,
aliases, and iter-args before bufferization. `CloneTensorEmpty` gives each
structured destination or loop init the same distinct storage ownership as the
compiler and copies applicable buffer-size annotations.

### 7.10 Code Motion

Default LICM and subset hoisting move definitions and uses across loop
boundaries. The model reproduces these moves because operation order and loop
placement directly affect PlanMemory lifetimes and peak overlap.

### 7.11 InlineOTFLoadStore

The model reproduces the real unaligned last-dimension `vconcat + store`
rewrite, including insert-slice construction and removal of obsolete
buffer-size marks. This prevents dead concat destinations from being counted as
UB allocations.

## 8. Per-Function Planning

After bufferization and suffix modeling, PlanMemory input is grouped by AIV
function. Each function is planned independently with the existing capacity,
seed, and inplace policy.

The module does not concatenate or sum function-local plans. Its peak and
required capacity are the maxima across function results because the functions
are independently executed and planned.

## 9. Output Contract

The report separates planning status from semantic coverage:

- `status`: `success`, `overflow`, or `blocker`;
- `precision`: `exact` or `incomplete`.

Each function result includes:

- projected and source function names;
- status;
- UB peak and required bits;
- selected PlanMemory seed;
- its buffer plan.

Module aggregation follows these rules:

- module peak is the maximum function peak;
- any function overflow makes the module status `overflow`;
- module required bits are the maximum function requirement;
- function-local buffers are never summed;
- flattened compatibility output adds a function field to disambiguate buffer
  names.

Existing top-level peak, required, capacity, overflow, and inplace fields are
retained where their meaning remains unambiguous. Function-specific fields are
added without reusing a single global seed when independently planned
functions selected different seeds.

Coverage diagnostics include:

- pipeline stage;
- source/projected function;
- operation ID and name when applicable;
- the unsupported semantic condition;
- the assumed configuration that caused or exposed the gap.

## 10. Error Handling

Unsupported configurations and operation forms do not stop the command. The
model emits a best-effort plan with `precision=incomplete` and structured
diagnostics. `incomplete` alone does not change the process exit code.

Conditions that prevent a meaningful plan are blockers:

- malformed or non-before-CVPipeline input;
- an inability to construct valid SSA;
- an unresolved physical buffer size;
- checked arithmetic overflow;
- an internal pipeline invariant failure.

Blockers use status `blocker` and exit code 1. Successful plans retain exit code
0. UB capacity overflow retains exit code 2.

## 11. Tests

This feature adds focused tests rather than restoring the historical oracle
infrastructure.

### 11.1 Stage Tests

Before/after fixtures cover:

- default `TileCubeVectorLoop(2,2)`, including alloc shrink and rollback;
- dynamic buffer-size inference and propagation;
- MIX-to-AIV filtering, DPS replacement, loop/if/scope replacement, and
  vector-side cleanup;
- scope inlining;
- sub-block tiling/binding success, skip, fallback, and rollback;
- tensor-empty folding and cloning;
- LICM and subset hoisting lifetime changes;
- the unaligned `vconcat + store` rewrite.

### 11.2 Coverage Tests

The checked post-CVPipeline manifest must match the intended real pipeline
order. Every stage has exactly one of the three coverage dispositions. Tests
fail when a stage has no disposition.

UB-invariant tests cover workspace offset changes and operandless cross-core
synchronization insertion.

### 11.3 End-to-End Tests

Fixtures cover:

- `mmadL1 -> fixpipe/workspace -> load -> vector`;
- CVPipeline depth, preload, and lazy-loading settings;
- default loop tiling, sub-block binding, and code motion;
- multiple MIX/AIV functions, with module max rather than sum;
- both existing pure-AIV product demos;
- unsupported custom/core-type forms producing `incomplete` diagnostics;
- overflow, dynamic size, and malformed SSA status behavior.

Reference expectations come from reviewed, checked-in golden fixtures based on
the repository's compiler pass semantics and existing compiler tests. Test
execution does not dynamically build or invoke the historical suffix oracle.

### 11.4 PlanMemory Invariants

Tests assert:

- every planned buffer belongs to an AIV function and UB address space;
- no Cube L1/L0C buffer appears in an AIV UB plan;
- overlapping lifetimes do not receive overlapping placements;
- multi-buffer extent and offset counts agree;
- module peak equals the maximum function peak;
- workspace-only offsets and operandless sync operations do not alter UB
  results.

## 12. Acceptance Criteria

The feature is complete when:

1. the lightweight model builds successfully;
2. every post-CVPipeline stage has a tested coverage disposition;
3. all new stage, pipeline, output, and invariant tests pass;
4. both existing AIV demos retain their expected results;
5. the new MIX demo reports exact AIV UB buffers and peak under real default
   post-CVPipeline options;
6. multi-function reports use independent plans and a module maximum;
7. unsupported variants at an otherwise valid before-CVPipeline boundary
   produce `incomplete` diagnostics without being mislabeled exact;
8. text and JSON consumers preserve the existing top-level result contract.
