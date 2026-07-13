# CVPipeline UB Model C++ Core

Lightweight C++ model for the UB-visible path from before-CVPipeline generic IR
to PlanMemory local UB peak. The core code is organized by compiler pass or
semantic boundary, not by temporary planning-stage names.

## Layout

```text
src/
  common/                 # checked arithmetic and small utilities
  semantic_ir/            # generic MLIR-like parser, SSA/CFG, rewriter
  cvpipeline/             # CVPipelining analysis and rewrites
  suffix/                 # modeled suffix pass modules before PlanMemory
  planmemory/             # PlanMemory-local liveness, storage, planner
  validation/             # development-only oracle extraction helpers
  cli/
    cvpipeline_ub_model.cpp      # production CLI
    dev_validate.cpp             # development validator, historical actions
  model_core.hpp          # compatibility facade for PlanMemory-local model
tests/
  model_core_tests.cpp
  semantic_ir_tests.cpp
  inline_load_copy_tests.cpp
  mark_multi_buffer_tests.cpp
scripts/
  build.sh
  build_dev_tools.sh
  run_unit_tests.sh
  plan_precvpipeline_ub.py
  generate_ub_demo_json.py
  validate_all.sh
demo/
  ub_plan_visualizer.html
  sample_ub_plan.json
```

The suffix modules intentionally mirror compiler pass names, for example
`one_shot_analysis.hpp`, `hivm_decompose_op.hpp`, `alloc_extra_buffer.hpp`,
`inline_load_copy.hpp`, `mark_multi_buffer.hpp`, and
`planmemory_input_semantic_ir.hpp`.

## Production Entry

The user-facing wrapper starts from the before-CVPipeline generic IR boundary
and keeps CVPipeline options separate from suffix/PlanMemory options:

```bash
python3 cvpipeline_ub_model_cpp/scripts/plan_precvpipeline_ub.py \
  --pre-cvpipeline-ir=Output/index/d0_before_cvpipeline_generic/objects/<sha>/generic.mlir \
  --cv-pipeline-depth=-1 \
  --cv-enable-preload=false \
  --suffix-enable-auto-multi-buffer=false \
  --random-seed=0 \
  --format=text
```

The production binary exposes the same path directly:

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=plan-before-cvpipeline \
  --before-cvpipeline-ir=... \
  --cv-pipeline-depth=-1 \
  --cv-enable-preload=false \
  --enable-auto-multi-buffer=false \
  --random-seed=0 \
  --format=json
```

It also exposes narrower exact boundaries:

```bash
# PlanMemory-before IR -> lifetime.
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=analyze-lifetimes \
  --before-planmemory-ir=...

# PlanMemory-before IR -> local memory plan/peak.
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=plan-local-memory \
  --before-planmemory-ir=... \
  --random-seed=7 \
  --format=json

# before-OneShotBufferize generic IR -> modeled suffix -> PlanMemory.
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=plan-before-one-shot-bufferize \
  --before-one-shot-bufferize-ir=... \
  --enable-auto-multi-buffer \
  --limit-auto-multi-buffer-of-local-buffer=no-limit \
  --limit-auto-multi-buffer-buffer=no-limit \
  --random-seed=0 \
  --format=json
```

Text output prints `peak_bits`, `required_bits`, `capacity_bits`, and the
buffer plan table. JSON output includes the same summary plus a parsed `plan`
array. Unsupported exactness blockers return blocker precision instead of an
estimated peak.

## Visual Demo

Run the terminal-friendly demo:

```bash
bash cvpipeline_ub_model_cpp/scripts/run_demo_ub_plan.sh
```

By default it builds the lightweight model, runs the model, runs
`build/bin/bishengir-cvpipeline-suffix-compile` as the oracle, then compares
UB peak and `(extent_bits, offset_bytes)` buffer placement. Use `--help` to edit
CVPipeline, suffix, and PlanMemory options:

```bash
bash cvpipeline_ub_model_cpp/scripts/run_demo_ub_plan.sh --help
```

Open the static visualizer directly:

```text
cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html
```

Generate a JSON file for it from any before-CVPipeline generic IR:

```bash
python3 cvpipeline_ub_model_cpp/scripts/generate_ub_demo_json.py \
  --pre-cvpipeline-ir=Output/index/d0_before_cvpipeline_generic/objects/<sha>/generic.mlir \
  --output=cvpipeline_ub_model_cpp/output/demo/ub_plan.json
```

Then load the JSON with the page's `Open JSON` button. The page also accepts
the text output from `plan_precvpipeline_ub.py --format=text`.

## Validation

Production builds and unit tests:

```bash
bash cvpipeline_ub_model_cpp/scripts/build.sh
bash cvpipeline_ub_model_cpp/scripts/run_unit_tests.sh
```

`build.sh` and `build_dev_tools.sh` are timestamp-incremental. They return the
existing binary when no file under `src/` is newer than the output. Force a
rebuild with:

```bash
FORCE_REBUILD=1 bash cvpipeline_ub_model_cpp/scripts/build.sh
```

Development validation uses a separate binary:

```bash
bash cvpipeline_ub_model_cpp/scripts/build_dev_tools.sh
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model_dev_validate --help
```

Some validation script filenames and `dev_validate` actions still contain
historical `c1/c2/...` labels because local oracle directories and replay
scripts depend on them. They are compatibility labels only; core source types,
functions, and production actions use pass/boundary names.

Common validation gates:

```bash
bash cvpipeline_ub_model_cpp/scripts/validate_comparator_sensitivity.sh
bash cvpipeline_ub_model_cpp/scripts/validate_memory_info_replay.sh
bash cvpipeline_ub_model_cpp/scripts/validate_local_ub_allocations.sh
bash cvpipeline_ub_model_cpp/scripts/validate_lifetimes.sh
bash cvpipeline_ub_model_cpp/scripts/validate_memory_plan.sh
bash cvpipeline_ub_model_cpp/scripts/validate_fixed_seed_exact.sh
bash cvpipeline_ub_model_cpp/scripts/validate_config_matrix.sh
python3 cvpipeline_ub_model_cpp/scripts/validate_planmemory_bridge_input.py \
  --one-config-per-ir --max-failures=100
python3 cvpipeline_ub_model_cpp/scripts/validate_d1_cvpipelining.py
python3 cvpipeline_ub_model_cpp/scripts/validate_d3_end_to_end.py \
  --seeds 0 --restrict-modes 0 --compare-text-plan
```

The final acceptance protocol fixes the real PlanMemory seed and compares two
layers independently:

1. PlanMemory-input bridge: canonical PlanMemory-before input equality.
2. PlanMemory-local model: lifetime and plan canonical TSV equality.

Normalization may reorder storage paths, but it must not discard semantic
information such as buffer identity, operation order, offsets, or peak bits.

## External Input

The default validation input is the minimal suffix oracle view:

```text
cvpipeline_ub_model_cpp/input/suffix_compile/
cvpipeline_ub_model_cpp/input/suffix_compile/ub_peak_summary.tsv
```

The view contains symlinks only; real dumps live under
`Output/adapters/<adapter>/02_suffix/compiler_default`. Set
`CVPIPELINE_SUFFIX_ROOT` to point validation at another suffix dataset.

Reports are written under:

```text
cvpipeline_ub_model_cpp/output/reports/
cvpipeline_ub_model_cpp/output/snapshots/
```
