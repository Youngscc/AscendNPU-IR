# yy UB Capability Parity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the UB-safety capabilities unique to the removed post-CVPipeline implementation onto yy's product architecture without restoring a parallel pipeline.

**Architecture:** yy's `src/ir`, `src/passes`, and `src/pipeline` remain authoritative. Missing passes return a shared stage result; the module pipeline propagates Incomplete before authoritative planning. Existing yy pass implementations stay in place and gain migrated boundary tests.

**Tech Stack:** C++17 header-only semantic model, Python 3 oracle/report tests, Bash build wrappers.

## Global Constraints

- Only `precision=exact` may expose authoritative UB planning fields.
- Unsupported or unproved paths fail closed.
- Do not recreate `src/post_cvpipeline`.
- Preserve yy's more complete TileAndBindSubBlock and SplitMix implementations.
- Keep `cvpipeline_ub_model_cpp/output/` untracked.

---

### Task 1: Capability inventory and shared contracts

**Files:**
- Create: `cvpipeline_ub_model_cpp/tests/capability_parity.tsv`
- Create: `cvpipeline_ub_model_cpp/src/pipeline/modeling_result.hpp`
- Create: `cvpipeline_ub_model_cpp/src/ir/post_pipeline_ir_utils.hpp`

- [ ] Map every old `CVUB_TEST` to migrated, yy-covered, or blocker evidence.
- [ ] Add failing verifier tests for use-before-def and cross-function values.
- [ ] Port the shared diagnostic/result and structural validation helpers.
- [ ] Run focused tests and confirm they pass.

### Task 2: Restore TileCubeVectorLoop in yy pipeline

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/passes/tile_cube_vector_loop.hpp`
- Modify: `cvpipeline_ub_model_cpp/src/pipeline/cvpipelining_ub_pipeline.hpp`
- Test: `cvpipeline_ub_model_cpp/tests/test_capability_parity.cpp`

- [ ] Add failing default vector/cube tiling and ambiguous-axis blocker tests.
- [ ] Port the proven old implementation to the new include/layout contracts.
- [ ] Invoke it immediately after CVPipelining with default 2/2 options.
- [ ] Propagate Incomplete to module blocker.

### Task 3: Restore SubsetHoisting and strict inline behavior

**Files:**
- Create: `cvpipeline_ub_model_cpp/src/passes/loop_invariant_subset_hoisting.hpp`
- Extend: `cvpipeline_ub_model_cpp/src/passes/inline_scope.hpp`
- Modify: `cvpipeline_ub_model_cpp/src/pipeline/cvpipelining_ub_pipeline.hpp`

- [x] Add failing tests for static subset rebuild, mismatched subset blocker,
  private multi-result call and recursive-call blocker.
- [x] Port the transactional subset matcher and private-call inliner.
- [x] Run subset after LICM and propagate stage precision.

### Task 4: Restore safety regression coverage

**Files:**
- Modify: `cvpipeline_ub_model_cpp/tests/test_capability_parity.cpp`
- Modify: `cvpipeline_ub_model_cpp/tests/run_tests.sh`
- Modify: `cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv`

- [x] Add yy-path tests for Task7 successful tiling/early canonicalization, OTF extra
  users/dynamic concat, tensor-empty ownership and multi-AIV seed aggregation.
- [x] Add the missing Ascend950 tightly-coupled-buffer stage to the oracle and
  product guard; gate Triton-only DPS optimization with the real option.
- [ ] Ensure every manifest stage has a product call site and positive/blocker
  evidence.
- [ ] Keep paths without evidence blocked.

### Task 5: Full verification and delivery

**Files:**
- Modify: `cvpipeline_ub_model_cpp/HANDOFF.md`

- [ ] Run product build and all C++/Python tests.
- [ ] Rebuild the real oracle and validate 15 post stages plus 11 suffix groups.
- [ ] Run the demo and all 171 corpus inputs for seed 0.
- [ ] Run scoped diff checks and confirm no unmerged or generated files.
- [ ] Amend the merge commit and push `jskhub/cvpipeline-ub-post-model`.
