# AscendNPU-IR 项目记忆

最后核实：2026-07-28，分支 `codex/ub-overflow-model-product`，阶段 0 模型基线提交
`75809a33d`。本文件是 `.agent` 的唯一入口；事实与当前源码或新测量冲突时，应重新核实
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

当前阶段优先优化真实产品路径的速度，允许模型依赖 LLVM/MLIR。测试只用于证明优化没有改变
overflow 和完整 PlanMemory 语义。

下一产品边界已经确定：在 direct-MLIR、stable-ID shadow overlay 和最小增量分析基础设施
完成后，把 prediction pass 从 before-CVPipelining 前移到 before-AutoBlockify，并复刻
AutoBlockify、CV 前 MarkMultiBuffer、canonicalizationHIVMPipeline、InlineOTFBroadcast 到
CVPipelining 的真实语义。完整执行顺序和验收门槛见
[implementation_plan.md](implementation_plan.md)。

轻量 AutoBlockify 已经存在于仓库，后续应基于现有实现改造到新基础设施并接入主链路，不能
无理由从零重写。实施方案的每个阶段完成代码和测试后，都必须向用户报告一次临时正确性、
性能、工作树状态、遗留风险和下一步；若出现差异、回退或需要触碰原生逻辑，先暂停征询。

## 当前产品关系

- `ub_overflow_model_cpp` 是模型的独立实现目录，后续仍在这里维护核心算法和公共结构。
- `bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp` 是生产接入 pass，位于真实
  CVPipelining 之前。
- 生产正确性 oracle 是同一 `bishengir-compile` attempt 中继续执行得到的原生本地
  PlanMemory。旧的两个独立后缀编译器及其脚本、配置和缓存已删除；需要追溯只能查看 Git
  历史。
- 正确性脚本不读取原生结果缓存。每个场景和输入必须执行 seeds 0～19，并且每个 seed 都在
  同一次真实 BiSheng 执行中完成 embedded model 与原生本地 PlanMemory 的完整合同对比。

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

不推荐把完整 `ModuleOp` 再 clone 一次并运行原生 MLIR/BiSheng pass；那会退化为完整后缀
编译的成本。MLIR 应提供只读 `Operation/Value/Type/Attribute`、原始 use-list 和接口；
模拟变换由轻量 overlay 表达，不能污染真实 pipeline 的输入。

## 当前性能基线

阶段 0 已增加 `scripts/measure_embedded_model.py`，通过真实 BiSheng prefix 调用 production
`evaluate()`，关闭 validation/dump，在 prediction 后用默认关闭的环境边界停止。160 个与
before-CV corpus 配对的 adapter、真实 retry-only、O3、三轮结果为：

```text
prediction total: 741.444 / 725.845 / 722.965 ms
model total:      715.850 / 700.471 / 697.682 ms
serialize total:   25.595 /  25.373 /  25.283 ms
process wall:    4719.278 / 4580.319 / 4625.515 ms
per-input prediction median / mean / p95 / max:
                   1.598 / 4.563 / 15.415 / 90.005 ms
peak RSS:          44.4 MB
fast-path hits:    147 / 160 per round
```

本地报告位于 `output/performance/stage0/`，不提交。代表 embedded 20-seed 基线共 140 个
attempt，结果 `140 matched / 0 different / 0 unavailable / 0 timeout`；AutoBlockify 独立
160-input 验证全部通过。

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

历史完整后缀/模型对比为 `3.309 s / 0.886 s`，模型内部约快 `3.74x`；这里只作为旧性能
参照，不是当前正确性 oracle。

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

当前性能目标只比较轻量模型自身的单轮速度，不再评估 autotune 总收益或原生 compiler 被
剪枝后节省的时间。统一口径为同一输入集合、同一 resolved options、O3、关闭
validation/dump、真实 retry-only，交替测量原模型和新模型的每输入 `totalTimeNs`、单轮
internal total、process wall 和峰值 RSS。入口前移后实际工作量增加时必须单列，不能把不同
边界的总时间包装成严格 A/B 加速比。

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
- 正确性优化后仍以 embedded model 对同进程原生 PlanMemory 为准；性能使用原模型/新模型
  同机交替的真实 retry-only 单轮测量，关闭 dump、validation 和逐 pass 快照；不再使用
  autotune 或 compiler 剪枝收益作为性能验收指标。
- 正确性验证禁止使用缓存代替原生 PlanMemory。提前 non-overflow 证明在验证模式中只作为
  信号，模型仍生成完整计划，真实 BiSheng 仍执行到本地 PlanMemory；二者按 seed 比较最终
  完整合同。
- 性能数字必须说明日期、构建模式、输入集合、参数、seed/retry 模式和 timing 是否开启。
- `Output/`、`ub_overflow_model_cpp/output/`、dump、cache 和临时报告均为可再生成产物，
  不提交。

## 继续工作前的阅读顺序

1. [code_map.md](code_map.md)：当前实现、热点路径和命令。
2. [workflow.md](workflow.md)：新的性能重构阶段和提交纪律。
3. [validation.md](validation.md)：当前 oracle、正确性门槛和性能测量方法。
4. [implementation_plan.md](implementation_plan.md)：交给后续实现者的逐阶段任务、执行方式和
   验收标准。
