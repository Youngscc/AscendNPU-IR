# AscendNPU-IR 项目记忆

最后核实：2026-07-28，分支 `codex/ub-overflow-model-product`，基线提交
`4e1053359`。本文件是 `.agent` 的唯一入口；事实与当前源码或新测量冲突时，应重新核实
并直接更新当前结论，不在这里堆叠多代任务流水账。

## 当前唯一目标

把 `ub_overflow_model_cpp` 优化为嵌入 `bishengir-compile` 的高性能 UB overflow 判定
pass。模型在真实 CVPipelining 之前消费本轮 `ModuleOp` 和已经解析完成的生产参数，尽快
判断后续本地 PlanMemory 是否会 overflow：

```text
ModuleOp before CVPipelining
  -> embedded lightweight model
       |-> exact overflow: 终止本 attempt，交给 BiSheng 原有 fallback
       |-> exact non-overflow: 继续真实 CVPipelining 和后续 pipeline
       `-> incomplete/blocker: 不剪枝，继续真实 compiler
```

当前阶段优先优化真实产品路径的速度，允许模型依赖 LLVM/MLIR。目标不是缩短测试时间，
也不是继续完善 suffix/cv2pm；测试只用于证明优化没有改变 overflow 和完整 PlanMemory
语义。

## 当前产品关系

- `ub_overflow_model_cpp` 是模型的独立实现目录，后续仍在这里维护核心算法和公共结构。
- `bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp` 是生产接入 pass，位于真实
  CVPipelining 之前。
- 生产正确性 oracle 是同一 `bishengir-compile` attempt 中继续执行得到的原生本地
  PlanMemory，不是 suffix，也不是 cv2pm cache。重复开发验证可以读取 embedded native
  cache，但 cache miss、fallback、多 attempt 和发布前最终验证仍走现场 oracle。
- `bishengir-cvpipeline-suffix-compile` 已退出当前任务。
- `cv2pm-bishengir-compile` 及 schema-2 cache 只保留历史诊断价值；除非用户明确要求追溯
  历史，不再围绕它们新增任务、缓存或门禁。

## 当前实现与待实现边界

当前生产 pass 仍把 `ModuleOp` 打印成 Generic MLIR 文本，`evaluate()` 再解析为
`GenericModule`。模型内部仍经过多层状态：

```text
GenericModule
  -> BufferizedSemanticIR
  -> PostBufferizationRewriteState
  -> AfterAllocExtraBufferState
  -> AfterInlineLoadCopyState
  -> AfterMarkMultiBufferState
  -> PlanMemoryInputSemanticIR
  -> PlanMemoryInput
  -> PlanMemory
```

新的目标架构尚未实现，计划为：

```text
MLIR-backed read-only input
  -> stable-ID shadow/overlay IR
  -> unified UBBufferProgram
  -> typed PlanProgram
  -> integer-ID PlanMemory
```

不推荐把完整 `ModuleOp` 再 clone 一次并运行原生 MLIR/BiSheng pass；那会退化为接近
cv2pm 的成本。MLIR 应提供只读 `Operation/Value/Type/Attribute`、原始 use-list 和接口；
模拟变换由轻量 overlay 表达，不能污染真实 pipeline 的输入。

## 当前性能基线

2026-07-28 在同机、O3、production-default、真实 retry-only、160 个去重
before-CVPipelining 输入上，开启 stage timing 后连续三轮：

```text
internal total: 912.580 / 886.105 / 878.362 ms
逐输入中位数聚合: 888.429 ms
process wall: 1433.009 / 1392.238 / 1379.322 ms
```

主要 top-level 占比：

```text
PlanMemory                              20.06%
BuildPlanMemoryInput                    15.91%
AlignStorageAndAllocExtraBuffer         14.21%
MarkRealCoreType                         6.97%
ParseGenericIR                           5.23%
PostBufferizationRewrites                4.79%
TileAndBindSubBlock                      3.77%
AIC projection checks                    6.22%
two canonicalization pipelines           6.39%
OneShotBufferize                         3.00%
```

历史同口径 cv2pm/model 对比为 `3.309 s / 0.886 s`，模型内部约快 `3.74x`；这是历史
性能参照，不是当前正确性 oracle。

## 新性能结论与目标

生产只需要精确的 overflow 布尔结论；完整 peak/plan/lifetime/offset 仍用于 debug 和
正确性验证。不能仅通过少输出结果获得大收益：当前
`PlanMemory.MaterializeResult` 只约占 `0.14%`，主要成本在 pass 语义、liveness、retry 和
地址规划。

安全的 exact non-overflow 上界已经实现。在 `AfterMarkMultiBuffer` 后，按函数计算
所有仍存活 UB buffer 的
`AlignUp(constBits, 256) * multiBufferNum`，故意不做 inplace/reuse；若这个独立分配上界
仍不超过 UB capacity，真实 PlanMemory 必定成功。生产 `evaluate()` 命中后返回 exact
non-overflow，不再构造 PlanMemoryInput 或运行 PlanMemory，且不伪造 peak/required/seed/
plan；debug/embedded validation 观察同一个证明但强制继续完整模型与原生 PlanMemory。
2026-07-28 对 default 160 输入的先期只读实验中：

```text
145 / 160 可由该条件直接证明 non-overflow
这些输入现有 BuildPlanMemoryInput + PlanMemory 合计约 266 ms
潜在聚合降幅约 28%
```

实现覆盖 UB address space、extra buffer、InlineLoadCopy survivor、multi-buffer 和逐函数
容量；归属、大小或乘法/加法不能精确确认时直接 fall through。代表 embedded 回归为
`398 matched / 0 different / 34 unavailable`，证明模式未出现一次“上界安全但完整计划
overflow”的矛盾。真实 compiler 在 non-overflow 时仍继续后续编译，提前返回只发生在
轻量模型内部。

同日性能口径：O3、关闭 validation/dump、真实 retry-only，compile-only autotune 使用 5 个
代表 adapter、每个 4 个配置、baseline/shadow/prune 交替、5 轮、单线程：

```text
单轮成功 fused-attention：baseline 125.174 ms，fast-path 151.485 ms
  paired overhead median 26.490 ms；model 25.640 ms；serialize 0.424 ms
80 个 paired non-overflow candidate：平均模型增量 14.366 ms（20.42%）
每轮 20 candidate：baseline 中位 2.4216 s，prune 中位 2.3349 s
5 轮累计：12.1469 s -> 11.7085 s，节省 438.35 ms / 3.61%，1.037x
```

其中 80 个成功 candidate 中 60 个命中 conservative upper-bound fast path；20 个最终
overflow candidate 仍执行完整模型及 BiSheng fallback，并在 prune 模式避免进入原生后缀。

允许依赖 LLVM/MLIR 后，当前整体目标为：

```text
160-input internal total: 888 ms -> 350~500 ms
相对当前再快约 1.8~2.5x
总体降幅约 45%~60%
```

该估算包含重叠，不是各项收益直接相加。推荐优先级：

1. 直接读取当前 `ModuleOp`，删除打印/解析文本往返；
2. MLIR-backed stable-ID shadow IR，类型/属性使用 MLIRContext 句柄；
3. revisioned analysis manager、active `BitVector` 和增量 use-list；
4. 合并 post-bufferization 多层状态，删除正常路径的文本 PlanMemory bridge；
5. PlanMemory 全整数 BufferId/ValueId、seed-independent event program 和 arena outline；
6. fallback 分叉前 snapshot cache；仅在 latency mode 下考虑 seed 0 失败后的并行 retry；
7. 架构稳定后再评估 ThinLTO/PGO。

## 不可破坏的原则

- 只有 `exact overflow` 可以剪枝；incomplete/blocker 必须继续真实 compiler，且不能伪装成
  exact non-overflow。
- 未固定 seed 时，语义仍是 seed 0～19 中任何一个成功即 non-overflow，全部失败才是
  overflow。若生产只消费布尔值，可以改变不影响这个存在性结论的实现调度，但验证模式
  必须保留原生 seed 合同。
- 禁止 adapter、kernel、SSA 名、buffer 数量或 seed 特例。
- 禁止为了让模型通过而修改原生 BiSheng pass、buffer plan、遍历顺序或 fallback 语义。
- 使用 `DenseMap`、`SmallPtrSet` 等无序结构时，只能用于查找；任何影响 operation、buffer、
  RNG 或 PlanMemory 顺序的遍历必须来自显式有序容器。
- 正确性优化后仍以 embedded model 对同进程原生 PlanMemory 为准；性能使用真实
  retry-only，关闭 dump、validation 和逐 pass 快照。
- embedded native cache 只加速模型开发循环。cache identity 必须覆盖实际 pre-CV IR digest、
  resolved-options digest、seed、完整参数和 pipeline fingerprint；多 attempt/fallback 不得用
  单-attempt replay，cache mismatch 必须自动现场确认，发布前必须执行不读缓存的现场矩阵。
- 性能数字必须说明日期、构建模式、输入集合、参数、seed/retry 模式和 timing 是否开启。
- `Output/`、`ub_overflow_model_cpp/output/`、dump、cache 和临时报告均为可再生成产物，
  不提交。

## 继续工作前的阅读顺序

1. [code_map.md](code_map.md)：当前实现、热点路径和命令。
2. [workflow.md](workflow.md)：新的性能重构阶段和提交纪律。
3. [validation.md](validation.md)：当前 oracle、正确性门槛和性能测量方法。
