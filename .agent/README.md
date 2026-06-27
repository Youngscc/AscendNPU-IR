# Agent Memory Index

这个目录是给后续 agent/Codex 续接当前任务用的记忆系统，刻意放在
`.agent/` 下，不放入 `docs/`，避免影响项目正式文档构建。

仓库根目录的 `AGENTS.md` 是给全新 agent 的入口文件，会指向本索引。

## 当前核心任务

在 BiShengIR 真实触发 PlanMemory overflow 之前，根据当前编译选项和 IR
状态，精确、轻量地预测未来本地内存占用，提前发现 UB/L1/L0C 等 local memory
是否会超容量，从而避免真实编译环境里发生 overflow 并触发 fallback/rollback。

## 文件说明

- [task_context.md](task_context.md): 用户目标、边界、已确认事实。
- [code_map.md](code_map.md): 和 PlanMemory/overflow/编译选项相关的代码地图。
- [reproduction.md](reproduction.md): macOS 上复现 PlanMemory overflow 的命令和注意事项。
- [estimation_design.md](estimation_design.md): 精确内存估算器的推荐设计。
- [decisions_and_risks.md](decisions_and_risks.md): 已做判断、风险点、开放问题。
- [update_protocol.md](update_protocol.md): 后续 agent 更新这个记忆系统的规则。
- [overflow_memory_estimation_agent_memory.md](overflow_memory_estimation_agent_memory.md): 完整长版快照，保留全部上下文。

## 后续 agent 阅读顺序

1. 先读本文件。
2. 再读 [task_context.md](task_context.md)。
3. 如果要改代码，读 [code_map.md](code_map.md) 和 [estimation_design.md](estimation_design.md)。
4. 如果要复现实验，读 [reproduction.md](reproduction.md)。
5. 工作结束前，按 [update_protocol.md](update_protocol.md) 更新记忆。

## 重要原则

- 不把这些任务记忆放进 `docs/`。
- 不记录用户未公开的私有 IR 文件内容。
- 不把 CANN、hivmc、真实昇腾卡环境当作当前任务前置条件。
- 当前任务关注的是 PlanMemory 前的静态内存预测，不是 NPU runtime 行为。
