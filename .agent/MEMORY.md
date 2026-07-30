# AscendNPU-IR 项目记忆

最后核实：2026-07-30，分支 `codex/ub-overflow-model-product`，当前产品提交
`8ebd4f1203f1c2d751721be576b90eb9e00500f2`。本文件是 `.agent` 的唯一入口；源码或新测量与
本文冲突时，应先现场核实，再直接替换失效结论，不维护多代任务流水账。

## 当前唯一目标

保持 `ub_overflow_model_cpp` 是独立、轻量、可嵌入且语义精确的 UB overflow 模型。产品输入
边界已经从 before-CVPipelining 前移到 before-AutoBlockify，新增的
AutoBlockify→before-CVPipelining 原生 pass 序列和既有 CVPipelining→local PlanMemory 模型已
完成全量对齐。下一项唯一优化目标是在不改变这些 pass 语义的前提下，消除四次
ExtendedCanonicalizer 的重复 fixed-point 工作和 `static_range` 长尾，使新同边界全量倍率从
当前基本持平变为稳定快于原生 BiSheng。

AutoBlockify 之前的原生前缀不属于复刻范围。典型 Triton 配置下 adapter 到 AutoBlockify 前有
61 次 pass；它们继续由真实 BiSheng 执行。embedded model 直接消费这些 pass 已经生成的
before-AutoBlockify `ModuleOp`，不直接消费原始 adapter，也不复刻 HFusion/ConvertToHIVM
前缀。

当前不推进大规模基础设施替换。性能优化继续使用现有模型自有 `GenericModule`、canonicalization
工具和已经完成的轻量 AutoBlockify，先基于 profile 消除局部重复扫描、索引和 fixed point；不以
删除 pattern、合并原生 pass 次序或减少验证范围获得加速。

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
- 当前产品 pass 已位于 before-AutoBlockify；模型内部先执行已验证的 13-pass pre-CV 前缀，再
  执行既有 CVPipelining→local PlanMemory 路径。Exact overflow 已恢复产品剪枝；validation
  仍强制完整模型和原生 PlanMemory。

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

2026-07-30 在提交 `8ebd4f120` 上完成 before-AutoBlockify→PlanMemory 新边界的现场验证。有效
矩阵为 26 configs × 160 inputs × 20 fixed seeds = 83200；66 个已审核原生长超时 pair 对应
1320 个 attempt 未执行，实际报告 81880 项：

```text
matched                                      81789
identity_permutation                            31
different                                        0
unavailable                                     60
timeout                                          0
reported total                               81880
known-timeout skipped                         1320
```

- 31 项是 `python_tutorial_06-fused-attention` 的等尺寸 buffer identity permutation；只有在
  status/required/peak/multi 一致，且去掉 offset 后 extent/lifetime/inplace 全图一致时才归入
  该类别，不算普通 matched。
- 60 项仍是 `preload_auto_mb` 下 3 个 matmul 输入的原生 SIGABRT（3×20）；不是模型差异。
- 新增 `auto_blockify` 配置单独为 3200/3200 strict matched。
- 没有新的 blocker、unavailable、timeout 或 native/model 行为差异。

上述正式基线完成后，默认矩阵新增 4 个非笛卡尔积交互场景：AutoBlockify 分别与 preload、
default auto-MB、local-only auto-MB、unrestricted auto-MB 组合。矩阵现为 32 组，其中 30 组可
用于 embedded correctness/同边界性能；下一轮理论规模为正确性 `160×30×20=96000`、三轮性能
`160×30×3=14400`。4 场景 × 4 代表输入 × seeds `{0,13}` 小回归为 31 matched、1 个已知严格
identity permutation、0 different/unavailable/timeout；这不是 30 配置全量结论。

报告：`ub_overflow_model_cpp/output/validation/before_auto_8ebd4f120_160x26x20.tsv`，仅本地生成，
不提交。

## 当前性能基线

主口径为 Release/O3、before-AutoBlockify→local PlanMemory 同边界、26 configs × 160 inputs、
真实 retry-only、3 轮、关闭提前 non-overflow 返回及其证明计算，所有模型 attempt 都走
`decision_path=full_plan`。2026-07-30 的 12480 项结果：

```text
model full-plan samples                         12480 / 12480
paired native samples                           12174
model internal total                            273096.841 ms
model median / mean / p95 / max       3.370 / 21.883 / 37.088 / 3056.203 ms
paired model total                              249990.946 ms
native same-boundary total                      249146.574 ms
native median / mean / p95 / max       5.640 / 20.465 / 44.419 / 9669.407 ms
BiSheng / model aggregate ratio                    0.9966x
round ratios                         1.0215x / 0.9787x / 0.9870x
process wall total                                859.586 s
peak RSS                                           111.219 MiB
```

正式结论：新 before-AutoBlockify 边界下模型和原生 BiSheng 聚合时间基本持平；`0.9966x` 表示
模型约慢 0.34%，三轮波动跨过 1.0，不能宣称稳定加速。模型中位数更快，但
`triton.language.static_range` 的模型总时间 123.267 s、原生 7.952 s，单一长尾占配对模型总量
约 49.3%；排除该输入后比值为 1.9033x。这个排除结果只用于定位热点，不能替代正式全量倍率。

一轮 4160 项的 stage 诊断（有额外计时开销）分解为：输入桥 0.369 s、pre-CV 新前缀
61.053 s、既有 CV→PlanMemory 25.981 s。四个 ExtendedCanonicalizer 合计约 55.18 s，占模型
总时间约 63%；`static_range` 的 39.487 s 中约 91.8% 同样来自四个 canonicalizer。因此下一轮
性能优化应优先消除 canonicalizer 的重复全量 fixed point，不能删除任何语义 pattern。

正式报告：`output/performance/before_auto_8ebd4f120_160x26x3.{tsv,json}`；stage 诊断：
`output/performance/before_auto_8ebd4f120_stage_160x26.{tsv,json}`。均为本地生成物，不提交。

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
production/auto-MB × seeds `{0,13}` 的下游 embedded PlanMemory 为 `8/8 matched`。

阶段 4.1 已于 2026-07-30 完成：复核并复用现有 `ArithToAffine` 分析，但新增严格的独立 pass
入口，禁止调用会提前执行后续 affine canonicalization 的组合入口；物化结果改为与原生 MLIR
一致的标准 `affine_map`，覆盖 Add/Sub/Mul/CeilDiv/Div/Rem 与 signed/unsigned Min/Max 的 index
分支，非 index arithmetic 保持不变。8 profiles × 160 inputs 的单 pass 与累计 checkpoint 为
`1280/1280 PASS`；真实 `bishengir-opt --convert-arith-to-affine` fixture 完全一致；完整模型测试
通过；代表下游 PlanMemory 为 `8/8 matched`。

阶段 4.2 已于 2026-07-30 完成：`CanonicalizeIterArg` 严格保留原生 greedy 驱动语义，覆盖
tensor.empty folding、仅由 For/While pattern 触发的父函数 CSE、外部 tensor yield、直接及嵌套
SCF unchanged channel、For/While dead channel、effect/use closure 和 vector-function 禁用 backward
pattern。修正了把保守 `none` effect 当纯操作及无循环函数误跑 CSE 的旧聚合行为。fixture 与真实
`bishengir-opt --scf-canonicalize-iter-arg` 结构完全一致；8 profiles × 160 inputs 的单 pass 与累计
checkpoint 为 `1280/1280 PASS`；完整测试通过；代表 PlanMemory 为 `8/8 matched`。

阶段 4.3 已于 2026-07-30 完成：canonicalization pipeline 内的 module-level
`ExtendedCanonicalizer` 现在有独立的 `RunModuleExtendedCanonicalizer` 入口，复刻原生 greedy
fixed point 在该边界新触发的 affine composition、min/max canonicalization、fresh constant
materialization、下一轮 OperationFolder CSE/hoist、DCE、Arith/slice fold 和 semi-affine 重建。
修复过程没有复用旧常量来伪造操作顺序，而是保留原生两轮常量生命周期。8 profiles × 160 inputs
的 `05 -> 06` 单 pass 与 `00 -> 06` 累计 checkpoint 为 `1280/1280 PASS`；4.2 全量回归、完整
测试和代表 PlanMemory `8/8 matched` 通过。随后进入阶段 4.4
`SCFForLoopCanonicalization`。

阶段 4.4 已于 2026-07-30 完成：`RunSCFForLoopCanonicalization` 复刻上游六类 cross-dialect
pattern，覆盖 tensor/memref 的 iter-arg/loop-result dim folding、`tensor.insert_slice` 与 nested
`scf.for` 的递归 shape-preserving proof，以及 affine.min/max 在 `scf.for`、`scf.parallel`、
`scf.forall` 常量可证范围内的化简。对于无法证明的动态/非纯 affine 情况保持原操作，不推测。
上游 fixture 的成功、部分成功、nested、no-change、parallel/forall 分支均与真实
`bishengir-opt` 精确一致；8 profiles × 160 inputs 的 `06 -> 07` 单 pass 与 `00 -> 07` 累计
checkpoint 为 `1280/1280 PASS`；完整测试通过，代表 PlanMemory 为 `8/8 matched`。随后进入阶段
4.5 CSE。

阶段 4.5 已于 2026-07-30 完成：新增独立 `RunPreCVCSE`，按上游 `CSEDriver` 保留 region
scope、isolated-from-above、nested-region-first traversal、交换律 operation equivalence、只读操作
同块且中间无 write/unknown 的限制，以及延迟 erase 的 DCE/CSE 顺序。定向 fixture 与真实
`bishengir-opt --cse` 结构精确一致，覆盖纯操作、read/write barrier、嵌套/兄弟 region、region-op
等价与 dead op；8 profiles × 160 inputs 的 `07 -> 08` 单 pass 与 `00 -> 08` 累计 checkpoint 为
`1280/1280 PASS`；完整测试通过，代表 PlanMemory 为 `8/8 matched`。当前 160 输入只到达
single-block structured region；multi-block SSACFG 保持显式 blocker，不做错误线性化。随后进入阶段
4.6 func-scoped ExtendedCanonicalizer。

阶段 4.6 已于 2026-07-30 完成：新增独立 `RunFirstFuncExtendedCanonicalizer` 阶段入口；经源码
复核，共享 canonicalizer 中所有 modeled rewrite 均按 enclosing function 隔离，OperationFolder
本来就是逐 function 建表，因而复用 4.3 的 fixed-point 实现而不复制第二套 pattern。双函数 fixture
与真实 `builtin.module(func.func(canonicalize-ext))` 精确一致，证明 constant/worklist 不跨函数；
8 profiles × 160 inputs 的 `08 -> 09` 单 pass 与 `00 -> 09` 累计 checkpoint 为
`1280/1280 PASS`；完整测试通过，代表 PlanMemory 为 `8/8 matched`。

阶段 4.7 已于 2026-07-30 完成：新增 pre-CV `RunPreCVHIVMOptSinglePoint`，逐项复刻原生
VBrc、Copy/Load 以及 VAdd/VSub/VMul/VDiv/VAbs/VSqrt/VMax/VMin 的 pure-buffer、单点形状、
f32/i64、host、no-IO-alias、memref traceback 和 memory-user 条件；生成的 constant、load、scalar
arith/math、store 顺序及 greedy folding 与真实 `bishengir-opt` 精确一致。原生对合法
`VMax/VMin<ui64>` 会生成 verifier 拒绝的 scalar op，模型将该已证明的原生失败显式 fail open。
8 profiles × 160 inputs 的 `09 -> 10` 单 pass 与 `00 -> 10` 累计 checkpoint 为
`1280/1280 PASS`；完整测试通过，代表 PlanMemory 为 `8/8 matched`。随后进入阶段 4.8 第二次
func-scoped ExtendedCanonicalizer。

阶段 4.8 已于 2026-07-30 完成：原生在 HIVMOptSinglePoint 后重新注册相同的
`func.func(canonicalize-ext)`，模型以独立 `RunSecondFuncExtendedCanonicalizer` 入口复用已证明按
function 隔离的 fixed-point 实现，未复制第二套 pattern。定向 fixture 与真实嵌套 canonicalizer
精确一致；8 profiles × 160 inputs 的 `10 -> 11` 单 pass 与 `00 -> 11` 累计 checkpoint 为
`1280/1280 PASS`；完整测试通过，代表 PlanMemory 为 `8/8 matched`。随后进入阶段 4.9
MemrefDeadStoreElimination。

阶段 4.9 已于 2026-07-30 完成：新增 pre-CV `RunPreCVMemrefDeadStoreElimination`，逐句复刻
原生 `ValueDependencyAnalyzer` 的 preorder value 编号、ViewLike alias 并查集与
`OpsOfAlloc` 顺序，以及 `DeadStoreElimination.cpp` 的同层 store-to-load forwarding、精确 SSA
index/全 1 memref 匹配、write/call/nested-region cache barrier、310B/950 尾部无用 HIVM Load
清理和 MLIR `eraseDeadAllocAndStores` 规则。普通与 reg-based 定向 fixture 分别与真实
`builtin.module(func.func(memref-dse))` 精确一致，覆盖 view alias、嵌套区域、call、递归 subview
死链及保留间接使用；最小 fixture 证明原生 pass 在 reg-based 模式发生 store-to-load forwarding
后会因已删除 operation 的悬空记录 SIGSEGV，模型在该组合上显式 fail open，而不复现未定义
行为。8 profiles × 160 inputs 的 `11 -> 12` 单 pass 与
`00 -> 12` 累计 checkpoint 为 `1280/1280 PASS`；完整测试通过，代表 PlanMemory 为
`8/8 matched`。

阶段 5 已于 2026-07-30 完成：新增 `RunPreCVInlineOTFBroadcast`，逐句复刻原生
`VBrcInlinePattern` 的 pure-tensor、axis/type、LAST 白名单、Ascend950 shift、非 LAST
BroadcastableOTF+binary+structured、DPS input replacement 和 broadcast dims 合并语义；无效
user 保留 VBrc。普通/Ascend950 fixture 与原生结构 `2/2` 精确一致；8 profiles × 160 inputs 的
`12 -> 13` 单 pass与 `00 -> 13` 累计 checkpoint 均为 `1280/1280 PASS`；完整测试通过，代表
PlanMemory `8/8 matched`。随后进入阶段 6 combined before-CV 与 API/input contract。

阶段 6 已于 2026-07-30 完成：新增唯一 `RunPreCVPrefixPipeline`，按原生顺序组合 checkpoint
01～13 的全部 pass；API 升级为 options v5，并增加 before-AutoBlockify input contract v2 与
独立 fingerprint，legacy before-CV v1 只作迁移测试。新增 4 个 pre-CV resolved option 字段均
由同一 `HIVMPipelineOptions` 映射。组合入口在 8 profiles × 160 inputs 上与同一原生 attempt
的 checkpoint 13 达到 `1280/1280 PASS`；完整测试和两个 embedded build target 均通过。当前
随后进入阶段 7 embedded observe-only。

阶段 7 已于 2026-07-30 完成：prediction pass 位于 checkpoint 00 后、原生 AutoBlockify 前，使用
contract v2；验证命令显式 `prune=false`，产品源码不再强制 observe-only，Exact overflow 恢复
剪枝。26 个有效配置 × 160 inputs × 20 seeds 的理论规模为 83200，排除 1320 个已审核长超时后
现场执行 81880 项，结果为 81789 matched、31 identity permutation、60 个既有原生 SIGABRT、
0 different、0 timeout。完整模型测试、Release build 和同边界 160×26×3 性能测量均完成。

## 当前实现优先级

1. 优先优化 pre-CV ExtendedCanonicalizer 的重复全量 fixed point；四次调用占 stage 诊断总时间
   约 63%，并造成 `triton.language.static_range` 的极端长尾。只能复用分析、worklist、常量索引和
   已收敛信息，不得省略原生 pass 次序或 pattern。
2. 为 canonicalizer 优化建立相同 160×30×3 full-plan A/B，并重跑 160×30×20 正确性；目标是
   先消除 `static_range` 长尾，再判断新边界是否获得稳定加速。
3. 在现有数据结构内优化 `BuildPlanMemoryInput`、`AlignStorageAndAllocExtraBuffer` 和连续
   post-bufferization 状态之间的重复解析、扫描、拷贝和分配。
4. 在不改变 seed/RNG/遍历顺序的前提下，把 PlanMemory 的 seed-independent 工作移出 retry，
   复用 scratch capacity 和只读事件信息。
5. 只为 profile 证明的热点增加局部、明确生命周期的索引或 cache；不先建设通用全局 analysis
   manager。
6. 收益稳定后清理被真正替代的 wrapper、重复工具和死代码；禁止先增加一套平行架构再等待
   将来删除旧实现。
7. 边界扩展和当前路径优化接近收益上限后，再单独评审 core/MLIR adapter 的依赖拆分。

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
