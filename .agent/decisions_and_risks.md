# Decisions And Risks

## 已做判断

- 当前任务不依赖 CANN、`hivmc` 或真实昇腾卡。
- 当前主线是逐 pass 内存行为模拟，不是完整影子编译器。
- S0/S1 早期轻量扫描和 S8 反向 checkpoint 框架先放在根目录 `memory_modeling/`，
  用 Python 实现；后续只有在需要 MLIR typed operation walk、PlanMemory dry-run 或
  pass instrumentation 时再补 C++。
- Python 框架核心类为 `MemoryState`、`BufferRecord`、`PassEffect`、`PlanPeak`、
  `Checkpoint`、`ModelingReport`；`checkpoints.py` 先注册 S8.5 -> S8.0 的反向顺序。
- S8.5 第一版先使用文本解析和保守 interval-reuse 模型：解析 local `memref.alloc`、
  静态 size、scope、`hivm.multi_buffer` 和 SSA use lifetime，输出 predicted peak 与
  blockers；不宣称等价于真实 PlanMemory。
- `memory_modeling/run_adapters.py` 作为当前批量 validation harness：默认跑 `data/`
  下所有非隐藏普通文件，主输出为 UB 对比优先的 `comparison.csv`/`comparison.json`；
  per-case 输出保留 `memory.json`、suffix 前 IR、真实 PlanMemory/MemoryDisplay
  artifacts 和必要日志。oracle 非零返回默认记录并继续，便于把 overflow case 也纳入对照。
- 每个 pass 后都可记录 memory effect，但不一定能输出 PlanMemory 级精确峰值。
- 精确值必须尽量复用 PlanMemory 或后续 dry-run oracle，避免复制一套漂移算法。
- local `MarkMultiBuffer` 后是第一优先 exact checkpoint。
- checkpoint 往前移时，补到 PlanMemory-ready 的 suffix 会改变 buffer 集合、scope、
  lifetime、size 或 multi-buffer 倍数，因此 suffix 本身必须作为建模对象；不能把
  suffix oracle 解释成 checkpoint IR 的纯瞬时峰值。
- 早期 tensor/HFusion 阶段要用 `symbolic` 或 `concrete_structural`，不能制造假精度。

## 主要风险

### 过早精确化

bufferization、scope inference、extra buffer、multi-buffer 尚未完成时，UB/L1/L0C
峰值很可能无法和真实 PlanMemory 对齐。

缓解：按 `symbolic`、`concrete_structural`、`exact_plan` 分层输出，并记录
`blocking_dependency`。

### suffix 误解释

为了校验更早 checkpoint，可能会追加 bufferization、scope inference、extra buffer、
canonicalization、MarkMultiBuffer 等 suffix 再调用 PlanMemory。这些 suffix pass
本身会改变最终峰值。

缓解：报告中明确区分：

```text
checkpoint_structural_state
checkpoint_plus_suffix_plan_peak
```

只有前者表示当前 IR 的结构状态，后者表示“当前 IR 经过固定 suffix 后”的 oracle 峰值。

### 动态 shape

PlanMemory 精确规划依赖静态 buffer size。动态 shape 需要明确策略：

```text
拒绝 / 上界 / 符号表达 / 从 tiling 或 compile config 取具体值
```

### 和 PlanMemory 漂移

外部独立算法很容易漏掉 liveness、alias、inplace、multi-buffer、rollback、
spec-level、20 seed retry。

缓解：exact checkpoint 复用 PlanMemory 内部类，或后续重新实现 dry-run oracle。
当前 fake adapter smoke 已显示 S8.5 Python 模型偏保守：正常 fake case oracle UB
peak 512 bits，模型 768 bits；overflow fake case oracle required/peak 2097152 bits，
模型 3145728 bits。下一步应优先分析 MemoryDisplay 中 offset/lifetime 与文本 lifetime
模型的差异。

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
