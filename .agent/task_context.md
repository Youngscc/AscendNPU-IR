# Task Context

## 目标

当前任务的输入是：

```text
data/ 下的 .ttadapter/MLIR 输入，或已经 dump 出来的 PlanMemory 前后 IR
```

输出是：

```text
local PlanMemory 前后的 IR、memory_info*.json、overflow 日志，以及代码逻辑解释
```

核心思路：

- 以真实 `bishengir-compile` / `bishengir-opt` 和 PlanMemory dump 为准。
- 重点观察第二次 PlanMemory，即 local `PlanMemory(LOCAL_MEM_PLAN)`。
- 解释 IR 时只记录结构和代码路径，不把用户私有 IR 内容写入 `.agent/`。
- 早前逐 pass estimator / Python 建模主线已暂停，不作为当前默认路线。

## 当前边界

优先关注 local memory：

```text
UB
L1 / CBUF
L0C / CC
```

global workspace 也和 PlanMemory 相关，但目前是次要目标。需要时单独建模，不能和
local peak 混算。

## 已确认事实

- PlanMemory overflow 是 BiShengIR 编译期静态规划问题，不需要 CANN、`hivmc`
  或真实昇腾卡。
- 如果输入已到 HIVM/memref 层级，可以直接用 PlanMemory 分析，后续也适合接入
  dry-run oracle。
- 如果输入还在 tensor/TTIR/HFusion 层级，必须先经过 lowering、bufferization、
  mem scope 推断等阶段。
- `bishengir-compile` 直接输入应是可解析为 MLIR `ModuleOp` 的 IR 文本；Python
  算子文件不能直接作为该 pipeline 输入，需先由前端转换成 Torch/HFusion/HIVM 等
  MLIR IR。
- 不是每个 pass 后都有 PlanMemory 级精确峰值。早期阶段应输出 symbolic 或
  concrete structural 状态，不能伪装成 exact。
- local `MarkMultiBuffer` 后最接近真实 local PlanMemory 输入。
- `createDumpIRBeforePlanMemoryPass` 位于 `MarkMultiBuffer` 和 local PlanMemory 之间。
  现在必须同时满足 `--enable-dump-ir-before-plan-memory` 和
  `BISHENGIR_DUMP_BEFORE_PLAN_MEMORY=<path>`，才会写出 before dump。普通编译默认不再
  插入该 debug pass。
- `--enable-dump-ir-before-plan-memory` 是最近新增的 compile option；修改后未在本轮
  完整 build，使用新脚本前需要用户重新 build `bishengir-compile`，让 TableGen 生成的
  option/config 代码和二进制同步。
- `tools/dump_planmemory_ir.sh` 是当前批量 dump 入口，默认扫描 `data/`，为每个 case
  保留 before local PlanMemory IR、`.raw.mlir`、`memory_info*.json`、stdout/stderr
  和顶层 `summary.tsv`；默认不再创建完整 `ir_tree/`。after local PlanMemory IR 和
  `plan_memory_after_dumps/` 依赖 `--dump-full-ir-tree` 或 `--drop-full-tree`。
- `tools/run_ub_peak_pass_ablation.sh` 是第一阶段 UB peak pass-group 消融入口，默认对
  `data/attn_fwd.ttadapter` 跑现有编译开关矩阵，并汇总 `summary.tsv` /
  `memory_metrics.tsv`；默认不创建 full `ir_tree/`，加 `--keep-full-tree` 才保留 full
  pass IR tree。
- `memory_info*.json` 中 `offset` 是 byte offset，`extent` 是 bit；PlanMemory
  内部以 bit 规划，rewrite 到 IR 时 offset 会转成 byte。
- 2026-07-07 的第一阶段 `attn_fwd` 实验输出位于
  `Output/ub_peak_pass_ablation/20260707_102925`；该轮里只有
  `--disable-auto-cv-work-space-manage=true` 让 AIV UB memory display 从 overflow 变为
  success，其余现有开关配置仍保持 `1716224 bits required / 1572864 bits available`。
- `Output/planmemory_ir_dumps/data_attn_fwd.ttadapter/planmemory_code_walkthrough_report.md`
  是当前示例报告，解释了 `data/attn_fwd.ttadapter` 的 local PlanMemory before/after。
  `.agent/` 中不要复制该报告里的用户 IR 片段。

## 不要再分心的问题

- `hivmc` 找不到。
- CANN 没安装。
- 没有昇腾卡。
- macOS full compile/full test 不通过。
- 旧实验 smoke 对比。
- 旧的 Python estimator 偏差调试，除非用户明确恢复该主线。
- 旧辅助脚本的 IR tree dump 细节。

这些可能影响完整编译或旧实验，但不是当前 PlanMemory dump/解释任务的 blocker。
