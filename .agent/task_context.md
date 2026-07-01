# Task Context

## 目标

当前任务的输入是：

```text
初始 IR 代码 + 完整 pass pipeline config
```

输出是：

```text
每个 pass/阶段之后的内存状态，直到 PlanMemory 之后
```

核心思路：

- 不做完整影子编译器。
- 只 1:1 模拟会影响内存的 buffer/memref 行为。
- 对每个 pass 抽取创建、删除、reshape、copy、scope 推断、layout、lifetime、
  inplace、multi-buffer 等内存相关 effect。
- 用真实 PlanMemory 或后续 dry-run oracle 对比模型峰值。

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
- local `MarkMultiBuffer` 后最接近真实 local PlanMemory 输入，是第一优先校验点。
- 第一版 exact 校验以 local `MarkMultiBuffer` 后 IR 作为起点，suffix 基本只有
  `PlanMemory(local)`。把起点往前推时，后续 suffix pass 不是“无影响尾巴”，而是
  必须一起建模的内存管理优化逻辑；oracle 峰值表示 checkpoint IR 经同一 suffix
  到 PlanMemory 后的结果。
- `memory_modeling/` 已有 Python 框架骨架：核心 dataclass、S8 反向 checkpoint
  注册表、text scanner、CLI，以及兼容旧用法的 `s0_snapshot` 入口。当前仍是轻量
  structural scan，不是 MLIR typed parser 或 PlanMemory dry-run。
- `memory_modeling/s8_local_model.py` 已实现 S8.5 第一版文本模型：输入为
  `createMarkMultiBufferPass` 后 dump 出来的 IR，解析 local `memref.alloc` 的
  scope/static size/multi-buffer/lifetime，并给出保守 predicted peak。它不模拟
  NormalizeLoopIterator、完整 alias/inplace、dmaFirstPipelineOpt、spec-level allocation
  或 20 seed retry，必须继续用真实 PlanMemory oracle 校验。
- `memory_modeling/run_adapters.py` 是当前批量验证入口：默认扫描 `data/` 下所有
  非隐藏普通文件，对每个文件运行 S8.5 Python 模型和 `bishengir-opt` local
  PlanMemory + MemoryDisplay oracle。主输出是 UB 对比优先的
  `comparison.json`/`comparison.csv`，每个 case 目录保留 `memory.json`、
  `model_memory_info.json`、`suffix_input.mlir`、成功时的 `after_plan.mlir`、原始
  `memory_info*.json` 和必要时的 `oracle.log`。`model_memory_info.json` 是模型侧逐
  buffer 视图，包含 model offset、extent、end address、lifetime、size/aligned size
  和 multi-buffer copy index。2026-07-01 smoke：`fake_attn_fwd.ttadapter` oracle UB
  peak 为 512 bits，当前模型预测 768 bits；overflow fake adapter oracle UB
  required/peak 为 2097152 bits，当前模型预测 3145728 bits。

## 不要再分心的问题

- `hivmc` 找不到。
- CANN 没安装。
- 没有昇腾卡。
- macOS full compile/full test 不通过。
- 旧的 fake ttadapter、run.sh、IR tree dump 细节。

这些可能影响完整编译或旧实验，但不是当前逐 pass 内存建模的 blocker。
