# Adapter Test Inputs

This directory is the flat input corpus consumed by
`tools/dump_cvpipeline_before_ir.sh`.

## Corpus

- `attn_fwd.ttadapter`: original attention baseline.
- `s0_*.ttadapter` through `s9_*.ttadapter`: the established CVPipeline
  optimization sequence used by the current exactness fixtures.
- `triton.language.*.ttadapter`: Triton language and CANN extension coverage.
- `ascend_tutorial_*.ttadapter` and `python_tutorial_*.ttadapter`: Ascend and
  Python tutorial kernel coverage.
- `kernels_vllm_*.ttadapter`: VLLM kernel coverage.
- `*_bench.ttadapter` and the remaining named kernels: focused operator and
  benchmark coverage.
- `vv_*.ttadapter`: five unique vector-add, softmax, and layernorm inputs
  imported from `vv/*.ttadapter.mlir`; the suffix records the static tile size.

The current corpus contains 174 adapters. The original import contributed 138
files from `source_archives/ttadapter_148.zip`; ten entries matching `s0`
through `s9` were not imported because their only difference was the random
temporary source path in MLIR location metadata. The later
`source_archives/ttadapter_168.zip` import added 20 tutorial adapters. Of its
remaining 148 entries, 137 were byte-identical to the corpus, ten differed only
in temporary source paths, and the existing compiler-compatible
`triton.language.assume.ttadapter` was retained intentionally.
The later `vv/` import added five content-unique adapters.

Keep adapters at this directory's top level. Source archives belong under
`source_archives/`; the batch dump script intentionally scans only top-level
`*.ttadapter` files.

## Compiler Compatibility

`triton.language.assume.ttadapter` and
`python_tutorial_03-matrix-multiplication.ttadapter` use the generic LLVM
intrinsic syntax accepted by this repository's MLIR revision. Their source
archives used a newer custom assembly form.

The following adapters currently do not reach local PlanMemory with this
compiler revision and must not be used as exact UB oracle cases:

- `triton.language.extra.cann.extension.gather_out_to_ub.ttadapter`
- `triton.language.extra.cann.extension.index_put.ttadapter`
- `triton.language.extra.cann.extension.scatter_ub_to_out.ttadapter`

Their external CANN helper declarations lack core-type metadata. Adding AIV
metadata gets past `InferFuncCoreType`, but later lowering still violates the
helpers' required GM/UB address-space contract. Resolve that in the matching
adapter/lowering implementation rather than rewriting pointer spaces in these
fixtures.
