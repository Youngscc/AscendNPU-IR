# PlanMemory Input Validation

模型输入为真实 compiler 的 `CVPipelining` 后 generic IR，运行 UB 必需 Pass，输出
`PlanMemory` canonical input；oracle 来自真实 `PlanMemory` 前 IR 的同一 canonical
投影。该边界不涉及 seed。

## 当前结果

```text
date=2026-07-16
inputs=171
cvpipelining_configs=15
unique_suffix_configs=17
cases=43605
exact=43605
mismatch=0
model_blocker=0
compiler_success=41355
compiler_overflow_after_plan_memory_boundary=2250
```

```bash
bash cvpipeline_ub_model_cpp/scripts/build_dev_tools.sh

python3 cvpipeline_ub_model_cpp/scripts/validate_plan_memory_input_matrix.py \
  --full-cross-product --jobs=8 \
  --artifacts=/tmp/cvub-plan-memory-input-cross-product
```

`--reuse-existing-oracle` 只重跑模型；重新确认 compiler dump 时去掉该参数或使用新的
artifacts 目录。不要提交 case IR。

## 最后根因链

1. 五个 sparse-attention adapters 曾有 165 个 mismatch，首差异位于真实
   `hivm-bind-sub-block`，oracle 产生 `8x?` subview，模型产生 `16x?`。
2. `BubbleUpExtractSlice/Pattern.cpp::
   InsertSliceBubbleUpStrategy::handleExtractInsertExtractCase` 为
   extract/insert/extract 重建 slice，并产生依赖 mapped loop IV 的 extent 链。
3. pre-buffer canonicalization 组合 affine 表达式；`ConvertToHIVMOpPass` 的
   `applyPatternsGreedily` 在 mapped loop 静态 domain 上 constify 恒定 affine min，
   并删除无用户 producer chain。
4. 轻量模型从真实 `scf.for` lb/ub/step 构造有限 domain，只在所有 domain 取值一致
   时折叠。实现不含 adapter、buffer 名或最终数值特判。
5. 定向 165/165 exact 后，配置轴矩阵达到 5556/5556 exact；随后所有输入与全部
   CVPipelining/multi-buffer 配置对的笛卡尔积达到 43605/43605 exact。

## 证据与限制

- C++ 单元测试和 `verify_hivm_op_registry.sh` 通过。
- canonical 格式使用 `b0/b1/...`，不保留 `%alloc/%alloc_N` printer 展示名。
- 2250 个真实 compiler 非零返回均发生在该边界之后，诊断为预期的 UB/CBUF
  overflow；不影响 PlanMemory 输入逐字节一致性的结论。
- 代表样例的 20 seeds x 2 restrict 检查中，peak、lifetime、extent、offset 数值一致。
- 新 mismatch 必须先找第一个真实 pass 差异，再读对应 BiSheng 源码；suffix compiler
  只允许增加只读 dump。
