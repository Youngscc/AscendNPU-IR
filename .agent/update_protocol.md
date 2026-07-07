# Agent Memory Update Protocol

## 什么时候更新

以下情况需要更新 `.agent/`：

- 用户澄清目标、输入格式、输出格式或边界。
- 发现新的关键代码路径或 pass 依赖。
- 新增/修改 estimator、trace、JSON schema 或测试。
- 新实验推翻了已有判断。

## 更新规则

- 不写入 `docs/`。
- 不记录用户私有 IR 的具体内容，除非用户明确允许。
- 优先精简：把结论写进现有核心文件，不再新增长版历史报告。
- 新代码路径更新 [code_map.md](code_map.md)。
- 方案变化更新对应核心文件；只有恢复逐 pass estimator 主线时才更新
  [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md)。
- 目标/边界变化更新 [task_context.md](task_context.md)。
- 风险/开放问题变化更新 [decisions_and_risks.md](decisions_and_risks.md)。

## 接手流程

1. 读 [README.md](README.md)。
2. 读 [task_context.md](task_context.md)。
3. 需要定位代码时读 [code_map.md](code_map.md)。
4. 只有恢复 estimator/逐 pass 建模时，才读
   [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md)。
5. 完成后按本协议更新记忆。

## 不要做

- 不把 CANN/hivmc/真实卡问题当成当前任务 blocker。
- 不把早期 symbolic 阶段写成 exact PlanMemory 结论。
- 不恢复旧的完整快照或长篇调研报告，除非用户明确要求。
