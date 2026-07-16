# yy UB 模型能力保全设计

## 目标

以 `yy/cvpipeline-ub-model-product@5528b27c` 的 `ir/passes/pipeline` 为唯一产品架构，
保留 yy 更完整的 pass 实现，同时迁移旧 `src/post_cvpipeline` 中独有且会影响 UB
正确性的能力。不得通过恢复第二套 pipeline 达成兼容。

## 审计结论

旧文件并非全部重复。能力分为三类：

1. yy 更完整：SplitMix、TileAndBindSubBlock、buffer-size/canonicalization 主体。
2. 已成功移植：多 AIV 独立 PlanMemory、模块 max、Exact/Incomplete 报告、
   `debug_estimate`、inplace blocker、oracle/corpus 工具。
3. 明确缺失：TileCubeVectorLoop、LoopInvariantSubsetHoisting、private call inline、
   Generic IR 结构校验，以及覆盖 manifest 与运行时 Exact 的绑定。

此外，旧 166 项 C++ 回归被删到只剩少量场景，不能以最终 corpus 的 0 failures
替代 pass 级适用性证明。

## 架构

- 通用诊断、阶段精度和覆盖类型放入 `src/pipeline/modeling_result.hpp`。
- 通用 IR 安全操作与 verifier 放入 `src/ir/post_pipeline_ir_utils.hpp`。
- 独有 pass 分别进入 `src/passes/tile_cube_vector_loop.hpp`、
  `src/passes/loop_invariant_subset_hoisting.hpp` 和严格 inline 支持；不创建
  `src/post_cvpipeline`。
- `cvpipelining_ub_pipeline.hpp` 按真实顺序调用这些 pass。任一 StageResult 为
  Incomplete 时立即返回模块级 blocker，不继续产生权威规划。
- yy 已有的 TileAndBindSubBlock 等实现不替换；通过迁移旧回归测试来锁定安全边界。

## Exact 合同

- 只有全部影响 UB 的阶段都成功建模，结果才是 Exact。
- 未建模 applicability、结构不合法、动态信息不足或事务性改写无法证明时返回
  Incomplete。
- blocker 的权威 peak、required、functions 继续为空。
- manifest 必须同时列出真实执行阶段；只在 TSV 中出现而产品 pipeline 未执行的阶段
  是测试失败。

## 验收

- TileCubeVectorLoop、SubsetHoisting、private inline 和 verifier 均有先失败后通过的
  新架构测试。
- 旧回归逐项映射为：迁移测试、yy 等价测试或明确 blocker 测试，不允许无解释删除。
- 轻量构建、C++/Python 测试、15+11 manifest、demo 和 171 corpus oracle 全部通过。

真实 builder 的当前版本还在 SplitMixKernel 前增加了仅 Ascend950 生效的
`MarkTightlyCoupledBuffer` 与 `HoistTightlyCoupledAlloc`，并且
`OptimizeDpsOpWithYieldedInsertSlice` 只在 Triton 模式执行。两点都必须进入
模型选项和 oracle 阶段，不能沿用 yy 初版的无条件/遗漏行为。
