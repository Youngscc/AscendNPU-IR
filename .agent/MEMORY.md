# AscendNPU-IR 项目记忆

最后核实：2026-07-30，分支 `codex/ub-overflow-model-product`，当前验证与性能基线
`389d0375ab4e`。本文件是 `.agent` 的唯一入口；源码或新测量与
本文冲突时，应先现场核实，再直接替换失效结论，不维护多代任务流水账。

## 当前唯一目标

保持 `ub_overflow_model_cpp` 是独立、轻量、可嵌入且语义精确的 UB overflow 模型。产品输入
边界已经从 before-CVPipelining 前移到 before-AutoBlockify，新增的
AutoBlockify→before-CVPipelining 原生 pass 序列和既有 CVPipelining→local PlanMemory 模型已
完成全量对齐。下一项唯一优化目标是在不改变这些 pass 语义的前提下，消除四次
ExtendedCanonicalizer 的重复 fixed-point、全量 def-use 扫描和无效 compact/semantic refresh，
优先清除 `static_range` 长尾。当前 35 场景优化速度基线已稳定快于原生 BiSheng，但收益被三个
canonicalizer 长尾输入显著稀释，仍有明确的局部优化空间。

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

2026-07-30 在 `389d0375ab4e` 上完成扩展后的
before-AutoBlockify→PlanMemory 全量现场验证。矩阵为 35 configs × 160 inputs × 20 fixed
seeds，理论规模 112000。81 个已审核、无法取得原生 oracle 的 config/input pair 对应 1620 个
attempt 未执行，实际比较 110380 项：

```text
matched                                     110183
identity_permutation                            66
ordering_equivalent                            131
different                                        0
unavailable                                      0
timeout                                          0
reported total                              110380
native-ineligible skipped                    1620
```

- 66 项 `identity_permutation` 与 131 项 `ordering_equivalent` 都要求 status、overflow、required、
  peak、multi-buffer 和逻辑 buffer/lifetime/inplace 约束保持一致；它们不计作普通 matched。
- 78 个 pair 是 13 个 auto-MB 相关场景下 6 个已知超过 360 秒的 attention 输入；另 3 个 pair 是
  `preload_auto_mb` 下原生 `SplitMixKernelPass::replaceResultWithInitOperand` 的 SIGABRT。两类均在
  运行前按审核清单排除，不能计作 matched。
- validation 中提前 non-overflow 证明仍继续执行完整模型与原生 PlanMemory；100740 个证明信号
  全部由可比较的原生结果验证为真。
- 没有新的 blocker、different、unavailable、timeout 或 native/model UB 决策差异。
- 原生 oracle 压缩缓存位于
  `output/oracle_cache/bisheng_native_35x160x20`：110389 条记录中 109185 条单-attempt 记录可直接
  回放；1204 条包含原生 fallback 的多-attempt 记录只保留审计，下次仍现场确认。

报告：`ub_overflow_model_cpp/output/validation/embedded_160x35x20_final.tsv`，仅本地生成，不提交。

## 当前性能基线

性能测试固定分为两种，禁止混用结果：

- **优化速度测试**：Release/O3、before-AutoBlockify→local PlanMemory 同边界、160 inputs × 当前
  35 个有效场景、真实 retry-only、至少 3 轮；设置 `BISHENGIR_UB_MODEL_FORCE_FULL_PLAN=1`，
  不执行 conservative non-overflow proof，所有模型 attempt 必须为 `decision_path=full_plan`。
  这是判断模型内部实现是否真正变快的主门槛。
- **真实速度测试**：Release/O3、production 默认路径、真实 retry-only，不设置
  `BISHENGIR_UB_MODEL_FORCE_FULL_PLAN`，允许 exact non-overflow proof 命中后直接返回。必须额外
  报告 fast-path 命中率、完整 PlanMemory fall-through、overflow/blocker 分布；该结果描述产品
  实际成本，不能证明基础设施本身变快。

2026-07-30 在 `389d0375ab4e` 上完成当前优化速度测试，规模为 `160×35×3=16800`：

```text
model full-plan samples                         16800 / 16800
paired native samples                           16332
known native samples skipped                      243
native unpaired samples                            225
  partial / unavailable                       78 / 147
model internal total                            338017.478 ms
model median / mean / p95 / max       3.412 / 20.120 / 36.974 / 1630.884 ms
paired model total                              306788.863 ms
native same-boundary total                      393584.083 ms
native median / mean / p95 / max       5.331 / 24.099 / 43.299 / 9539.219 ms
BiSheng / model aggregate ratio                    1.2829x
round ratios                         1.2799x / 1.2852x / 1.2836x
process wall total                               1164.537 s
peak RSS                                            72.27 MiB
```

正式结论：同一完整边界上模型是原生 BiSheng 的 `1.2829x` 速度，亦即配对模型内部耗时低
`22.05%`；三轮稳定同向。单 case 比值中位数为 `1.6414x`，`97.65%` 的配对 case 模型更快。
聚合倍率受三个输入的 canonicalizer 长尾压低：`static_range`、`inline_asm_elementwise`、`randn`
分别比原生多耗时 137.586 s、21.401 s、10.569 s；排除三者后的诊断倍率为 `3.2302x`，只用于
热点归因，不替代正式全量倍率。

一轮 `160×35=5600` 的 stage 诊断（有额外计时开销）分解为：输入桥 0.492 s、pre-CV 新前缀
76.917 s、既有 CV→PlanMemory 35.664 s。四个 ExtendedCanonicalizer 合计 68.874 s，占模型
总时间 60.59%、占 pre-CV 前缀 89.54%；AutoBlockify 本体仅 0.220 s，占模型总时间 0.19%。
`static_range` 的 pre-CV 前缀 48.224 s，而原生同输入整个 AutoBlockify→PlanMemory 边界仅
3.362 s，已经严格证明异常成本来自模型 canonicalizer 实现，而不是 AutoBlockify 算法本身。

正式报告：`output/performance/optimization_fullplan_389d0375_rebuilt_160x35x3.{tsv,json}`；stage
诊断：`output/performance/optimization_stages_389d0375_rebuilt_160x35.{tsv,json}`。均为本地生成物，
不提交。

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
剪枝。扩展后的 35 场景全量为 110183 matched、66 identity permutation、131 ordering
equivalent、0 different/unavailable/timeout；81 个 native-ineligible pair 在运行前排除。同边界
160×35×3 优化速度测试为 `BiSheng/model=1.2829x`。

## 当前实现优先级

1. 优先优化 pre-CV ExtendedCanonicalizer 的重复全量 fixed point；四次顶层调用实际会通过
   module canonicalizer 再嵌套调用 outer canonicalizer，合计占 stage 诊断总时间 60.59%，并造成
   `triton.language.static_range` 极端长尾。先让一次 canonicalizer invocation 共享现有
   `PipelineAnalysisContext`，把 value replacement、has-use、definition lookup 和 DCE 从全模型
   线性扫描改成有序增量索引；不得省略原生 pass 次序、worklist 顺序或 pattern。
2. 为 canonicalizer 优化建立相同 160×35×3 full-plan A/B，并重跑 160×35×20 正确性；目标是
   先消除 `static_range` 长尾，再确认正式倍率稳定高于当前 `1.2829x`。
3. canonicalizer 的第二批安全优化是 dirty-operation semantics refresh、批量 DCE/erase，以及用
   mutation/tombstone 标志让确定无变化的 `CompactGenericModule` 走 O(1) no-op；不能跨原生 pass
   边界复用尚未证明仍有效的 fixed-point 结果。
4. 在现有数据结构内优化 `BuildPlanMemoryInput`：优先处理 `EmitOperations`、`IndexValues` 与
   `Normalize` 的重复中间记录和二次 materialize；随后处理
   `AlignStorageAndAllocExtraBuffer` 的重复 type/layout/attribute 解析。
5. PlanMemory 已经把 `PreparedMemLivenessAnalysis` 和首次 `PreparedStorageEntryAnalysis` 提到
   retry 外，后续只能继续分离严格 seed-independent 的 base facts，并保持 mt19937 调用次数、
   候选顺序、inplace 选择和 retry 合同完全不变。
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
- 正确性 oracle 必须源自同进程原生 PlanMemory、fixed seeds 0～19 和完整合同。允许回放身份中
  包含 adapter 内容、完整参数和原生源码 fingerprint 的已验证缓存；缺失、过期、多-attempt 或
  缓存比较不一致时必须现场运行原生 pipeline。
- 性能只比较模型单轮成本；默认关闭 dump、validation、stage artifact 和详细计时。优化速度测试
  强制 full-plan，真实速度测试允许 non-overflow 提前返回；两个倍率必须分开命名、分开报告。
- 提前 non-overflow 在 production 可直接返回；debug/validation 必须继续完整模型与原生
  PlanMemory，验证证明真实成立。
- 性能数字必须说明日期、commit、构建、输入、配置、seed/retry、提前判定状态和 timing 口径。
- `Output/`、`ub_overflow_model_cpp/output/`、dump、cache 和临时报告均不提交。

## 继续工作前的阅读顺序

1. [implementation_plan.md](implementation_plan.md)：当前阶段和禁止事项。
2. [code_map.md](code_map.md)：真实代码路径和热点。
3. [workflow.md](workflow.md)：每批修改、回退和提交纪律。
4. [validation.md](validation.md)：现场正确性与完整路径性能口径。
