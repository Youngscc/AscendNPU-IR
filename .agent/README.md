# Agent Memory Index

这个目录只保存当前任务的 agent 记忆，不放入 `docs/`，避免影响正式文档构建。

## 当前主线

从最初 IR 输入和完整 pass config 出发，抽取并模拟每个 pass/阶段中会影响内存的
buffer/memref 行为，输出从初始 IR 到 PlanMemory 之后的内存占用变化。

这不是完整影子编译器。只模拟内存相关行为，并用仓库中 PlanMemory 能力或后续
dry-run oracle 校验模型结果。

## 核心文件

- [task_context.md](task_context.md): 当前目标、边界、已确认事实。
- [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md): 逐阶段建模方案和实施顺序。
- [code_map.md](code_map.md): 当前需要看的代码入口、pipeline 位置和调试命令。
- [pass_sequence_to_second_planmemory.md](pass_sequence_to_second_planmemory.md): 从输入到第二次
  PlanMemory 的 pass 顺序，含常见完整 config 下的 helper pipeline 展开。
- [pass_order_code_locations.md](pass_order_code_locations.md): 到第二次 PlanMemory 的细粒度
  pass 顺序及其在 pipeline 中的 addPass 代码位置。
- [decisions_and_risks.md](decisions_and_risks.md): 已做判断、风险、开放问题。
- [update_protocol.md](update_protocol.md): 后续维护规则。

已删除旧的长版快照、PlanMemory 调研长报告、代码流程长文和过时复现笔记；需要的信息已浓缩到上述文件。

## 推荐阅读顺序

1. 读本文件。
2. 读 [task_context.md](task_context.md)。
3. 推进方案或写代码前，读 [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md)。
4. 需要定位实现时，读 [code_map.md](code_map.md)。
5. 工作结束前，按 [update_protocol.md](update_protocol.md) 更新记忆。

## 原则

- 不记录用户未公开的私有 IR 内容。
- 不把 CANN、hivmc、真实昇腾卡环境当作当前任务前置条件。
- 当前任务关注编译期 pass 内存行为和 PlanMemory 静态规划，不是 NPU runtime 行为。
