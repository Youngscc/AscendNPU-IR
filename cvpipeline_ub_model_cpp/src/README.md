# Source Layout

```text
main.cpp
  -> pipeline/cvpipelining_ub_pipeline.hpp
       -> passes/cvpipelining/
       -> passes/infer_and_set_buffer_size.hpp
       -> passes/global_workspace_plan.hpp
       -> passes/fold_tensor_empty.hpp
       -> passes/canonicalization_hivm_pipeline.hpp
       -> passes/mark_real_core_type.hpp
       -> passes/cross_core_gss.hpp
       -> passes/split_mix_kernel.hpp
       -> passes/inline_scope.hpp
       -> passes/tile_and_bind_sub_block.hpp
       -> passes/loop_invariant_code_motion.hpp
       -> passes/clone_tensor_empty.hpp
       -> passes/hivm_inline_otf_load_store.hpp
       -> passes/optimize_dps_op_with_yielded_insert_slice.hpp
       -> passes/one_shot_bufferize.hpp
       -> passes/hivm_decompose_op.hpp
       -> passes/infer_hivm_mem_scope.hpp
       -> passes/align_storage.hpp
       -> passes/alloc_extra_buffer.hpp
       -> passes/inline_load_copy.hpp
       -> passes/mark_multi_buffer.hpp
       -> passes/plan_memory/
```

- `passes/` 只存放真实 Pass 或真实 Pass 内部实现。
- `pipeline/` 存放调用顺序、Pass 边界状态和 PlanMemory 输入构建。
- `ir/` 与 `analysis/` 是 Pass 共用的轻量表示和分析。
- `tools/` 与 `validation/` 仅用于开发验证，不进入生产主程序。
