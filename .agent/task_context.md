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
- 不是每个 pass 后都有 PlanMemory 级精确峰值。早期阶段应输出 symbolic 或
  concrete structural 状态，不能伪装成 exact。
- local `MarkMultiBuffer` 后最接近真实 local PlanMemory 输入，是第一优先校验点。

## 不要再分心的问题

- `hivmc` 找不到。
- CANN 没安装。
- 没有昇腾卡。
- macOS full compile/full test 不通过。
- 旧的 fake ttadapter、run.sh、IR tree dump 细节。

这些可能影响完整编译或旧实验，但不是当前逐 pass 内存建模的 blocker。
