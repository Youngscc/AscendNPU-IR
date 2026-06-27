# Agent Memory Update Protocol

## 什么时候更新

后续 agent 在以下情况需要更新本目录：

- 发现新的关键代码路径。
- 确认新的编译选项会影响内存估算。
- 跑了新的复现实验。
- 实现了新的 pass、工具或 JSON schema。
- 用户澄清了目标、输入格式或环境。
- 某个之前的判断被推翻。

## 更新规则

1. 不把任务记忆写入 `docs/`。
2. 不记录用户私有 IR 的具体内容，除非用户明确同意。
3. 每次更新尽量写清楚：
   - 日期或上下文；
   - 做了什么；
   - 得到什么结论；
   - 哪些仍然不确定。
4. 新增代码后，要更新 [code_map.md](code_map.md)。
5. 新增命令或实验结果后，要更新 [reproduction.md](reproduction.md)。
6. 设计变化后，要更新 [estimation_design.md](estimation_design.md)。
7. 关键判断和风险变化后，要更新 [decisions_and_risks.md](decisions_and_risks.md)。

## 推荐续接流程

后续 agent 接手时：

1. 读 [README.md](README.md)。
2. 读 [task_context.md](task_context.md)。
3. 根据任务类型选择：
   - 要写代码：读 [code_map.md](code_map.md)、[estimation_design.md](estimation_design.md)。
   - 要复现：读 [reproduction.md](reproduction.md)。
   - 要判断方案：读 [decisions_and_risks.md](decisions_and_risks.md)。
4. 开始操作仓库。
5. 完成后更新对应记忆文件。

## 不要做的事

- 不要把 CANN/hivmc 问题当成当前估算任务的 blocker。
- 不要为了记忆系统修改项目正式文档。
- 不要删除已有用户改动。
- 不要把估算器做成完全脱离 PlanMemory 的重复实现，除非用户明确只需要粗估。
