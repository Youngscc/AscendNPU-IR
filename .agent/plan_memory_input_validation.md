# Historical PlanMemory Input Validation

本文件保存历史大矩阵证据，避免与当前可执行门禁混淆。原来生成这些数据的多批
`validate_*.py` 脚本已不在当前工作树，因此以下数字不能声称是“本次重新运行结果”。
现行命令见 `code_map.md`。

## 边界定义

```text
after CVPipelining generic IR
  -> UB-required suffix passes
  -> canonical PlanMemory input
```

此边界检查 allocation identity、operation order、buffer 信息和 access，不涉及
PlanMemory seed。seed 只影响后续 lifetime/inplace/offset planner。

## 历史结果（2026-07-16）

```text
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

另外曾记录：

```text
CVPipelining boundary:      171 inputs x 15 configs = 2565 exact
OneShotBufferize boundary:  2565 operation/allocation/value-access exact
HIVMDecomposeOp:            378 function pairs exact
AllocExtraBuffer:           2732 UB buffer projections exact
InlineLoadCopy:             388 function pairs exact
MarkMultiBuffer:            166 objects x 7 configs = 1162 exact
PlanMemory reusable tuples: 1692 lifetime/plan canonical TSV exact
```

2250 次真实 compiler 非零返回发生在 PlanMemory 输入边界之后，诊断为 UB/CBUF
overflow；不否定输入边界逐字节一致。

## 历史根因经验

最后一批 PlanMemory 输入 mismatch 来自 sparse-attention 的真实
`TileAndBindSubBlock` 和 slice bubble-up：

1. `InsertSliceBubbleUpStrategy::handleExtractInsertExtractCase` 重建
   extract/insert/extract slice。
2. extent 链依赖 mapped loop IV。
3. pre-buffer canonicalization 组合 affine 表达式。
4. `ConvertToHIVMOpPass::applyPatternsGreedily` 只在 mapped loop 静态 domain 上
   constify 恒定表达式，并删除无用户 producer chain。
5. 模型改为从真实 `scf.for` lb/ub/step 构造有限 domain，只在所有取值一致时折叠；
   没有 adapter、buffer 名或最终数值特判。

## 使用限制

- canonical identity 使用 `b0/b1/...`，不验证 `%alloc/%alloc_N` printer 名。
- 43605 组是 PlanMemory **输入**，不是全部 seed/restrict 的最终 plan。
- 171 输入、所有配置、20 seeds、两种 restrict 的最终矩阵接近百万次，尚未完整保存
  和重新验证。
- 修改 `TileAndBindSubBlock`、canonicalization、OneShot、buffer projection、
  MarkMultiBuffer 或 PlanMemory 输入构建后，历史数字只能作为回归目标，不能替代新
  oracle。
- 新 mismatch 仍必须定位第一个真实 pass 边界，然后读取 BiSheng/MLIR 源码修复。
