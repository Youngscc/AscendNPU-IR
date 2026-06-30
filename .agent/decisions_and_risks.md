# Decisions And Risks

## 已做判断

- 当前任务不依赖 CANN、`hivmc` 或真实昇腾卡。
- 当前主线是逐 pass 内存行为模拟，不是完整影子编译器。
- 每个 pass 后都可记录 memory effect，但不一定能输出 PlanMemory 级精确峰值。
- 精确值必须尽量复用 PlanMemory 或后续 dry-run oracle，避免复制一套漂移算法。
- local `MarkMultiBuffer` 后是第一优先 exact checkpoint。
- 早期 tensor/HFusion 阶段要用 `symbolic` 或 `concrete_structural`，不能制造假精度。

## 主要风险

### 过早精确化

bufferization、scope inference、extra buffer、multi-buffer 尚未完成时，UB/L1/L0C
峰值很可能无法和真实 PlanMemory 对齐。

缓解：按 `symbolic`、`concrete_structural`、`exact_plan` 分层输出，并记录
`blocking_dependency`。

### 动态 shape

PlanMemory 精确规划依赖静态 buffer size。动态 shape 需要明确策略：

```text
拒绝 / 上界 / 符号表达 / 从 tiling 或 compile config 取具体值
```

### 和 PlanMemory 漂移

外部独立算法很容易漏掉 liveness、alias、inplace、multi-buffer、rollback、
spec-level、20 seed retry。

缓解：exact checkpoint 复用 PlanMemory 内部类，或后续重新实现 dry-run oracle。

### 预测输出误触发 fallback

外层 retry policy 会匹配类似 `ub overflow`、`cbuf overflow`、`cc overflow` 的文本。

缓解：预测输出使用 `predicted_over_capacity`，不要打印原始 overflow 短语。

### `requires` 误解

PlanMemory 的 `requires N bits` 不是裸 buffer size，也不是所有 UB buffer 求和；它来自
失败后重新规划失败 scope 得到的峰值地址边界。

缓解：使用 PlanMemory 或后续 dry-run oracle 的结果作为基准。

## 开放问题

- 逐 pass trace 用 PassManager instrumentation，还是显式 checkpoint pass？
- early symbolic buffer 和最终 concrete memref 是否需要稳定 lineage ID？
- dynamic shape 的策略选哪一种？
- 输出只报告风险，还是最终要反馈到 tiling/autoschedule 自动调参？
