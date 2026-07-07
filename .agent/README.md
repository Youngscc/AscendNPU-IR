# Agent Memory Index

这个目录只保存当前任务的 agent 记忆，不放入 `docs/`，避免影响正式文档构建。

## 当前焦点

当前短期任务是围绕第二次 PlanMemory，也就是 local `PlanMemory(LOCAL_MEM_PLAN)`：

- dump `data/` 下 IR 在 local PlanMemory 前的快照，必要时额外抽取 local PlanMemory
  后的快照。
- 保留 `memory_info*.json`、stdout/stderr，方便观察 overflow。
- 阅读 PlanMemory 代码，解释 before IR 如何经过 local PlanMemory 变成 after IR。
- 做第一阶段 UB peak pass-group 消融实验，先用现有 `bishengir-compile` 开关观察
  pass group 对 local UB 峰值的影响。

早前“逐 pass 内存建模 / Python estimator”的主线已暂停，相关文件只作为背景资料保留；
不要把它当成当前默认工作入口。

## 核心文件

- [task_context.md](task_context.md): 当前目标、边界、已确认事实。
- [code_map.md](code_map.md): 当前需要看的代码入口、pipeline 位置和调试命令。
- [pass_sequence_to_second_planmemory.md](pass_sequence_to_second_planmemory.md): 从输入到第二次
  PlanMemory 的 pass 顺序，含常见完整 config 下的 helper pipeline 展开。
- [pass_order_code_locations.md](pass_order_code_locations.md): 到第二次 PlanMemory 的细粒度
  pass 顺序及其在 pipeline 中的 addPass 代码位置。
- [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md): 已暂停的逐阶段建模方案；
  只有恢复 estimator 工作时才需要读。
- [decisions_and_risks.md](decisions_and_risks.md): 已做判断、风险、开放问题。
- [update_protocol.md](update_protocol.md): 后续维护规则。

已删除旧的长版快照、PlanMemory 调研长报告、代码流程长文和过时复现笔记；需要的信息已浓缩到上述文件。

## 推荐阅读顺序

1. 读本文件。
2. 读 [task_context.md](task_context.md)。
3. 需要定位实现或命令时读 [code_map.md](code_map.md)。
4. 需要完整 pass 顺序时读 [pass_sequence_to_second_planmemory.md](pass_sequence_to_second_planmemory.md)
   或 [pass_order_code_locations.md](pass_order_code_locations.md)。
5. 只有恢复 estimator/逐 pass 建模时，才读 [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md)。
6. 工作结束前，按 [update_protocol.md](update_protocol.md) 更新记忆。

## 当前常用入口

```bash
# 只 dump local PlanMemory before、memory_info 和日志；默认不创建 full ir_tree。
bash tools/dump_planmemory_ir.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/planmemory_ir_dumps \
  --allow-failures

# 额外抽取 local PlanMemory after；full ir_tree 用完即删。
bash tools/dump_planmemory_ir.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/planmemory_ir_dumps \
  --drop-full-tree \
  --allow-failures

# 保留每个 pass 后的 full IR tree。
bash tools/dump_planmemory_ir.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/planmemory_ir_dumps \
  --dump-full-ir-tree \
  --allow-failures

# 第一阶段 UB peak pass-group 消融，默认不创建 full ir_tree。
bash tools/run_ub_peak_pass_ablation.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/ub_peak_pass_ablation
```

## 原则

- 不记录用户未公开的私有 IR 内容。
- 不把 CANN、hivmc、真实昇腾卡环境当作当前任务前置条件。
- 当前任务关注编译期 pass 内存行为和 PlanMemory 静态规划，不是 NPU runtime 行为。
