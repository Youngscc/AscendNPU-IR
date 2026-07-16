# UB Configuration Test Matrix

These manifests are deterministic test data generated with random seed
`20260711`. They combine exhaustive boundary coverage with constrained random
pairwise selection. Invalid combinations are normalized away: disabled
disabled CVPipelining cannot carry preload/depth/lazy options, skew mode does not carry an
unroll depth, and inactive auto multi-buffer uses its default strategies.

| Manifest | Coverage | Rows | PlanMemory runs per input before IR hash dedup |
| - | - | -: | -: |
| `plan_memory.tsv` | seeds 0..19 x restrict false/true | 40 | 40 |
| `cvpipelining.tsv` | CVPipelining off, depths -1/0/1/2/3/4, skew, lazy | 15 | 600 |
| `suffix_ub_passes.tsv` | multi-buffer, code motion, UB saving, tiling | 48 | 1920 |
| `align_storage_infer_data_layout.tsv` | exhaustive 2^4 size/alignment/layout toggles | 16 | 640 |
| `cvpipelining_suffix_plan_memory_cross.tsv` | fixed-seed random cross-pass interactions | 64 | 2560 |
| `all_configs.tsv` | all batches | 183 | 5760 |

The run count is an upper bound. The runner groups identical config digests and
deduplicates identical PlanMemory-before IR by SHA256 separately for every
input. Oracle validation still expands every unique IR over the requested
PlanMemory seeds and restrict modes.

Regenerate and verify the checked-in manifests:

```bash
python3 cvpipeline_ub_model_cpp/scripts/generate_config_test_matrix.py
python3 cvpipeline_ub_model_cpp/scripts/validate_config_test_matrix.py
```

After rebuilding the suffix compiler, start with a smoke run:

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_config_test_matrix.py \
  --matrix cvpipeline_ub_model_cpp/testdata/config_matrix/all_configs.tsv \
  --max-configs 3 --max-cases 2
```

Then remove the limits for the full run and compare every generated oracle
against the lightweight model:

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_config_test_matrix.py \
  --matrix cvpipeline_ub_model_cpp/testdata/config_matrix/all_configs.tsv
bash cvpipeline_ub_model_cpp/scripts/validate_generated_config_oracles.sh
```

Configuration variation cannot guarantee that an adapter contains
`scf.while`, `scope.scope`, `cf.br`, or preload. Dedicated structural fixtures
remain mandatory alongside this matrix.
