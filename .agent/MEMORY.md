# AscendNPU-IR 项目记忆

最后核实：2026-07-30，分支 `codex/ub-overflow-model-product`，基线提交
`1dfddd59f3f9835e4f51fd50f79aa75454b02e27`。本文件是 `.agent` 的唯一入口；源码或新测量与
本文冲突时，应先现场核实，再直接替换失效结论，不维护多代任务流水账。

## 当前唯一目标

保持 `ub_overflow_model_cpp` 是独立、轻量、可嵌入的 UB overflow 模型，并把产品输入边界从
before-CVPipelining 前移到 before-AutoBlockify。模型新增精确复刻
AutoBlockify→before-CVPipelining 的原生 pass 序列，再接入当前已经对齐的
CVPipelining→local PlanMemory 模型。

AutoBlockify 之前的原生前缀不属于复刻范围。典型 Triton 配置下 adapter 到 AutoBlockify 前有
61 次 pass；它们继续由真实 BiSheng 执行。embedded model 直接消费这些 pass 已经生成的
before-AutoBlockify `ModuleOp`，不直接消费原始 adapter，也不复刻 HFusion/ConvertToHIVM
前缀。

当前不推进大规模基础设施替换。新增前缀优先复用现有模型自有 `GenericModule`、canonicalization
工具和已经完成的轻量 AutoBlockify；缺失语义按原生 pass 最小补齐。完成边界扩展和正确性对齐
后，再继续优化现有流水中的重复扫描、解析、索引、物化、拷贝和分配。

## 依赖边界决定

轻量模型核心继续保持 LLVM/MLIR 无关：

- 模型核心不得新增 LLVM/MLIR 类型、容器、句柄、分析或 pass 基础设施依赖；
- 不以 `Operation *`、`Value`、`Type`、`Attribute`、`DenseMap`、`SmallVector`、`BitVector` 等
  LLVM/MLIR 对象作为核心长期表示；
- 核心热路径优先使用 C++ 标准库和项目自有的紧凑、有序数据结构；
- BiSheng 接入 pass 位于 MLIR 编译器内部，因此接入层可以读取 `ModuleOp`，但必须在边界一次性
  转换为模型自有输入，LLVM/MLIR 对象不得继续渗入模型 pass；
- standalone 模型应继续能够独立构建和使用，不要求用户额外引入 LLVM/MLIR 运行时。

当前提交已经存在 `evaluateModule()`、`MLIRModuleView`、`GenericShadowOverlay` 和最低可用
revisioned analysis。它们是当前源码事实，不代表继续扩张的目标。后续冻结 MLIR-backed
overlay/通用 analysis manager 的迁移，不再为更多 pass 建第二套产品实现，也不继续为最终未必
采用的兼容层做优化。将 MLIR adapter 从模型核心拆回 BiSheng 边界属于后续依赖整理，排在
before-AutoBlockify 边界扩展和正确性对齐之后；执行前需单独设计和验收。

## 当前产品关系

```text
adapter
  -> bishengir-compile
       -> HIVM prefix
       -> UBOverflowPrediction pass
            -> lightweight model
       -> real CVPipelining and later pipeline when not pruned
       -> real local PlanMemory
```

- 核心实现目录：`ub_overflow_model_cpp`。
- 生产接入：`bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp`。
- 正确性 oracle：同一 `bishengir-compile` attempt 中继续执行得到的原生 local PlanMemory。
- 旧 suffix/cv2pm 工具已退出产品和正确性路线，不恢复、不重新维护。
- 当前产品 pass 仍在 before-CVPipelining；下一项实现任务是把它前移到
  before-AutoBlockify，并在模型内补齐两者之间的语义。

## 当前实现边界

当前核心仍以 `GenericModule` 及多层模型自有状态完成语义复刻：

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

这些类型现在不是立即整体替换的对象。先通过 profile 确认每一层的真实成本，再在不改变接口
语义的前提下做局部消重、移动传递、容量复用和查询缓存。只有连续几轮局部优化已经无法获得
稳定收益，且数据证明某个边界本身是主要成本时，才为该边界提出独立替换方案；未经用户确认
不启动全流水 ShadowIR、统一 UBBufferProgram 或全局 analysis manager 重构。

## 当前正确性基线

2026-07-29 在提交 `1dfddd59` 上完成了 160 inputs × 27 configs × 20 seeds 的 embedded
现场验证。配置中 66 个已知原生长超时 pair，共 1320 个 attempt 按既定规则没有执行，因此
实际报告 85080 项：

```text
matched                                      78583
strict different                                37
unavailable                                   6460
timeout                                          0
total                                         85080
```

- 37 个 strict difference 全部是已知 fused-attention 等尺寸 buffer 身份置换，仅影响
  plan/lifetime/inplace；status、overflow、required、peak、multi-buffer 一致。
- 6400 个 unavailable 来自两个 workspace-manage-off 配置下产品 prediction pass 不插入。
- 60 个 unavailable 来自 3 个 matmul 输入在 `preload_auto_mb` 下的原生 SIGABRT。
- 不得把 unavailable 或身份置换写成“86400 全通过”。

原始报告：
`ub_overflow_model_cpp/output/validation/full_1dfddd59_160x27x20.tsv`，属于本地生成物，不提交。

## 当前性能基线

性能主口径已经改为：Release/O3、160 × 27 read-only 配置、真实 retry-only、关闭模型的提前
non-overflow 判定及其证明计算，模型全部执行到完整 PlanMemory。这样测量的是完整
before-CVPipelining→PlanMemory 模型本身，不混入提前判定收益。

2026-07-29，4320 个模型 case 全部执行：

```text
model internal total                         23836.604 ms
per-case median / mean / p95 / max       2.187 / 5.518 / 22.089 / 93.149 ms
model process wall                           111.509 s
model peak RSS                                51.266 MiB
decision path                              4320 full_plan
```

原生 BiSheng 侧按既定规则跳过 66 个已知长超时 pair，可配对 4254 项：

```text
paired model internal total                  22621.361 ms
native CVPipelining->local PlanMemory wall   89363.428 ms
BiSheng / model aggregate ratio                  3.9504x
```

若只统计原生单 attempt 的 4219 项，比例为 `3.7568x`。因此当前对“完整
CVPipelining→PlanMemory 路径”的正式结论是：模型约为原生 BiSheng 的 `3.95x` 速度。这个比值
以模型内部时间对原生边界 wall delta 计算，报告时必须保留口径说明。

作为产品行为参考，production-default 允许 exact non-overflow fast path 时曾测得约 `6.81x`；
它包含 147/160 的提前判定命中，不可用来代表完整 PlanMemory 路径的基础设施速度。关闭提前
判定、只测 production-default 时为约 `4.006x`。

原始报告：
`ub_overflow_model_cpp/output/performance/1dfddd59_read_only_160x27.tsv`，属于本地生成物，不提交。

## 当前阶段状态

阶段 0 已于 2026-07-30 完成：原生 before-AutoBlockify 到 before-CVPipelining 的 14 个
checkpoint 已建立，默认关闭；显式停止会在 `13_after_inline_otf_broadcast` 后结束，不运行
CVPipelining 或更后的 pass。普通产品路径的 before-CV IR 与阶段 0 修改前逐字节一致，完整模型
测试套件通过。

阶段 1 已于 2026-07-30 完成：已有 AutoBlockify stable-ID 实现通过最小条件入口接入新增轻量
前缀，精确使用原生 `enableTritonKernelCompile && enableAutoBlockifyLoop` gate。同一次原生编译
attempt 生成的 before/after checkpoint 在去重后的 160 个 adapter 上全部结构一致；禁用分支为
精确 no-op；代表输入 seeds `{0,13}` 最终 PlanMemory 为 4/4 matched。

阶段 2 已于 2026-07-30 完成：新增独立的 pre-CV `MarkMultiBuffer` 模型，逐句复刻 local、
workspace、scope preload、已有 mark 校验、memref traceback、parent-loop、MIX strategy 和
workspace 数量语义，并保留原生 greedy driver 同时触发的 ordinary folding/DCE。8 个相关
pre-CV profile × 160 inputs 的单 pass 与累计前缀同-attempt checkpoint 为 `1280/1280 PASS`；
真实 `bishengir-opt` fixture 的 Load/Fixpipe/preload-scope 输出完全一致；完整模型测试通过；
代表输入 × production/auto-MB × seeds `{0,13}` 的下游 embedded PlanMemory 为 `8/8 matched`。

阶段 3 已于 2026-07-30 完成：新增 outer module-level `ExtendedCanonicalizer` 的轻量投影，按
原生 MLIR greedy driver 的“先注册/CSE 常量、再处理 rewrite worklist”顺序执行，覆盖当前支持域
实际到达的 Arith identity folds、constant CSE/hoist、`RedudantVBrcOp`、
`RedudantVReduceInitOp`、`FoldUselessBufferSizeMarkOp` 和相应 DCE。8 个 pre-CV profile × 160
inputs 的单 pass 与累计前缀同-attempt checkpoint 为 `1280/1280 PASS`；原生
`bishengir-opt --canonicalize-ext` fixture 结构完全一致；完整模型测试通过；代表输入 ×
production/auto-MB × seeds `{0,13}` 的下游 embedded PlanMemory 为 `8/8 matched`。当前进入阶段
4.1：只实现 `ArithToAffine`，不得提前实现后续 canonicalization 子阶段。

## 当前实现优先级

1. 从阶段 4.1 `ArithToAffine` 开始，依次补齐九步 `canonicalizationHIVMPipeline` 和
   `InlineOTFBroadcast`；阶段 1 AutoBlockify、阶段 2 pre-CV MarkMultiBuffer 与阶段 3 outer
   ExtendedCanonicalizer 已完成。
2. 使用阶段 0 已建立的原生 checkpoint 做每个新增 pass 的差分；最终以同进程原生 local
   PlanMemory 做 seeds 0～19 完整合同验证。
3. 对齐后把 production prediction pass 前移到 before-AutoBlockify；在完成代表与全量验证前
   只能 observe-only，不能用新增路径剪枝。
4. 在现有数据结构内优化 `BuildPlanMemoryInput`、`AlignStorageAndAllocExtraBuffer` 和连续
   post-bufferization 状态之间的重复解析、扫描、拷贝和分配。
5. 在不改变 seed/RNG/遍历顺序的前提下，把 PlanMemory 的 seed-independent 工作移出 retry，
   复用 scratch capacity 和只读事件信息。
6. 只为 profile 证明的热点增加局部、明确生命周期的索引或 cache；不先建设通用全局 analysis
   manager。
7. 收益稳定后清理被真正替代的 wrapper、重复工具和死代码；禁止先增加一套平行架构再等待
   将来删除旧实现。
8. 边界扩展和当前路径优化接近收益上限后，再单独评审 core/MLIR adapter 的依赖拆分。

详细阶段见 [implementation_plan.md](implementation_plan.md)，执行纪律见
[workflow.md](workflow.md)，正确性与性能口径见 [validation.md](validation.md)，代码位置见
[code_map.md](code_map.md)。

## 不可破坏的原则

- 只有 `Precision::Exact && overflow == true` 可以剪枝；blocker/incomplete 必须 fail open。
- 未固定 seed 时保持原生 retry 合同：seeds 0～19 任一成功即 non-overflow，全部失败才 overflow。
- 不修改 BiSheng 原生 pass、buffer plan、liveness、inplace、multi-buffer、fallback 或遍历语义。
- 不增加 adapter、kernel、SSA、buffer 数量、配置名或 seed 特例。
- 任何影响 op、buffer、RNG、lifetime 或地址规划顺序的遍历必须来自显式有序容器。
- 正确性使用同进程原生 PlanMemory、fixed seeds 0～19 和完整合同；不得用缓存替代 oracle。
- 性能只比较模型单轮成本；默认关闭 dump、validation、stage artifact 和详细计时。
- 提前 non-overflow 在 production 可直接返回；debug/validation 必须继续完整模型与原生
  PlanMemory，验证证明真实成立。
- 性能数字必须说明日期、commit、构建、输入、配置、seed/retry、提前判定状态和 timing 口径。
- `Output/`、`ub_overflow_model_cpp/output/`、dump、cache 和临时报告均不提交。

## 继续工作前的阅读顺序

1. [implementation_plan.md](implementation_plan.md)：当前阶段和禁止事项。
2. [code_map.md](code_map.md)：真实代码路径和热点。
3. [workflow.md](workflow.md)：每批修改、回退和提交纪律。
4. [validation.md](validation.md)：现场正确性与完整路径性能口径。
