# Modeled Suffix Modules

Each modeled suffix pass gets its own similarly named `.hpp`/`.cpp` pair in
this directory. `suffix_pipeline` only owns pass ordering and configuration;
pass implementations mutate the shared `semantic_ir` representation and expose
an independent canonical snapshot boundary.

Implemented modules are source-forward ports of the corresponding compiler
logic:

- `one_shot_analysis.hpp`: OneShotAnalysis alias/read/write/reverse-use-def/RaW
  decisions and UB allocation materialization.
- `bufferized_semantic_ir.hpp`: post-OneShotBufferize allocation identities,
  shaped-value bindings and operand-to-buffer use bindings.
- `hivm_decompose_op.hpp`: source-forward HIVMDecomposeOp buffer materialization
  and UB-relevant operation rewrites.
- `hivm_opt_single_point.hpp`: post-bufferization `CanonicalizeIterArg`,
  singleton vector-to-scalar rewrites and dead local-buffer elimination.
- `convert_non_contiguous_reshape_to_copy.hpp`: conditional contiguous temp
  buffer/copy insertion for annotated collapse-shape operations.
- `post_bufferization_rewrites.hpp`: post-OneShotBufferize canonicalization,
  decomposition and reshape-copy state before memory-scope inference.
- `infer_hivm_mem_scope.hpp`: function/core-derived allocation memory scope.
- `align_storage.hpp`: `AlignAllocSize`, `MarkStrideAlign` and
  `EnableStrideAlign` UB projection.
- `alloc_extra_buffer.hpp`: source-forward HIVM `AllocExtraBuffer` formulas.
- `after_alloc_extra_buffer.hpp`: alignment, post-flatten trace size,
  extra-buffer ownership and physical buffer projection.
- `inline_load_copy.hpp`: source-forward `InlineLoadCopy` ordering, alias and
  intervening-memory-effect checks.
- `after_inline_load_copy.hpp`: direct-load rewrites and dead middle buffer
  projection after `InlineLoadCopy`.
- `mark_multi_buffer.hpp`: source-forward `MarkMultiBuffer` strategy, loop,
  existing-mark and preload semantics.
- `after_mark_multi_buffer.hpp`: multi-buffer projection after `MarkMultiBuffer`.
- `planmemory_input_semantic_ir.hpp`: canonical PlanMemory-before AIV/UB
  physical buffer records and access projection.

The post-bufferization rewrite path ends immediately before the first
`InferHIVMMemScope`: `BufferizedSemanticIR` -> post-bufferization
canonicalization -> first `HIVMDecomposeOp` ->
`ConvertNonContiguousReshapeToCopy` -> `PostBufferizationRewriteState`.

The AllocExtraBuffer projection is validated byte-for-byte on buffer identity,
owner, memory scope, physical bits and extra-buffer status. The InlineLoadCopy
projection additionally validates all 388 function pass pairs and the final
survivor set after `InlineLoadCopy`. The MarkMultiBuffer projection validates
166 objects across seven native strategy classes. The PlanMemory-input bridge
validates 1162 tuples and 17836 final AIV/UB buffers; optional access checking
remains exact-or-blocker.

Oracle extraction and byte-for-byte comparison remain in `scripts/` and the
development CLI; they are not part of the model API.
