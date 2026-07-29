# before-AutoBlockify 到 PlanMemory 的轻量模型扩展计划

最后更新：2026-07-30。本文是下一阶段开发的主执行计划。开始实现前必须同时完整阅读
[MEMORY.md](MEMORY.md)、[code_map.md](code_map.md)、[workflow.md](workflow.md) 和
[validation.md](validation.md)。

## 1. 目标与边界

把 embedded UB overflow 模型的输入边界从 before-CVPipelining 前移到
before-AutoBlockify：

```text
raw adapter
  -> 原生 BiSheng 前缀（典型 Triton 配置约 61 次 pass，不复刻）
  -> ModuleOp before AutoBlockify
  -> lightweight model
       -> AutoBlockify 到 before-CVPipelining 的新增轻量前缀
       -> 现有 CVPipelining 到 local PlanMemory 模型
       -> exact overflow / exact non-overflow / blocker
```

AutoBlockify 之前的 HFusion、ConvertToHIVM 和 HIVM prefix 继续由原生 BiSheng 执行，不属于
本任务。模型不直接消费 raw adapter，也不复刻前面 61 次 pass。

本任务首先追求语义完整和逐 pass 可验证；不能为了尽快得到最终 PlanMemory 一致而从结果反推
规则。每个新增逻辑必须能指出对应的 BiSheng/MLIR 源文件、匹配条件、重写顺序和失败行为。

## 2. 必须复刻的真实顺序

唯一顺序来源是
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp` 当前源码：

```text
1  AutoBlockifyParallelLoop                         [conditional]
2  MarkMultiBuffer                                  [workspace manage enabled]
3  ExtendedCanonicalizer
4  ArithToAffine
5  CanonicalizeIterArg                              [func]
6  ExtendedCanonicalizer
7  SCFForLoopCanonicalization
8  CSE
9  ExtendedCanonicalizer                            [func]
10 HIVMOptSinglePoint                               [func]
11 ExtendedCanonicalizer                            [func]
12 MemrefDeadStoreElimination                       [func]
13 InlineOTFBroadcast                               [func]
-- existing model boundary --
14 CVPipelining                                     [workspace manage enabled]
```

第 4～12 项是一次 `canonicalizationHIVMPipeline`，共 9 个 pass。第 3 项是它之前额外运行的
module-level ExtendedCanonicalizer，不能与第 6/9/11 项合并。

条件必须逐字复刻：

- AutoBlockify 仅在 `enableTritonKernelCompile && enableAutoBlockifyLoop` 时执行；
- CV 前 MarkMultiBuffer 仅在 `!disableAutoCVWorkSpaceManage` 时执行；
- CVPipelining 同样受 `!disableAutoCVWorkSpaceManage` 控制；
- dump、trace 和 UB prediction 是辅助 pass，不属于原生语义序列。

## 3. 不可破坏的实现原则

- 对应原生源码是唯一语义来源；测试结果只能定位问题，不能用来编造规则。
- 不修改 BiSheng 原生 pass、pattern、遍历顺序、fallback 或 PlanMemory 行为。
- 不增加 kernel、adapter、SSA 名、buffer 数量、配置名或 seed 特例。
- 未覆盖的原生分支必须返回 blocker/incomplete 并 fail open，不能猜测为 no-op。
- 不能因为 corpus 中某 pass 暂时没有改变 IR，就删除或永久写成 no-op；必须从原生匹配条件证明
  当前支持域内不可达，并保留未知分支保护。
- 保持原生 greedy rewrite 的 worklist、user/use-list、插入点、erase 和遍历顺序；任何影响后续
  operation/value identity 的变化都需要 checkpoint 验证。
- 模型核心不新增 LLVM/MLIR 类型或运行时依赖。BiSheng adapter 可以读取 `ModuleOp`，但新增
  pass 使用模型自有 `GenericModule` 和现有公共工具。
- 不启动全流水 ShadowIR、通用 analysis manager 或 UBBufferProgram 替换。现有 AutoBlockify
  overlay 只作为已经完成的实现复用，不扩大成全局迁移项目。

## 4. 原生源码与现有实现盘点

| pass | 原生语义来源 | 当前轻量实现 | 本阶段处理 |
|---|---|---|---|
| AutoBlockify | `HIVM/Transforms/AutoBlockifyParallelLoop.cpp` | `passes/auto_blockify_parallel_loop.hpp`，已有 legacy/overlay 与 fixture | 复核源码后直接接入；只做输入合同、参数和必要适配 |
| CV 前 MarkMultiBuffer | `HIVM/Transforms/MarkMultiBuffer.cpp` | 现有 `passes/mark_multi_buffer.hpp` 是 PlanMemory 前 local-buffer 阶段 | 新建 pre-CV 语义层；只复用枚举、解析和经证明相同的 helper |
| ExtendedCanonicalizer | `Transforms/ExtendedCanonicalizer.cpp` + 所有已加载 dialect/op canonicalization pattern | `canonicalization_hivm_pipeline.hpp` 有受限投影 | 先记录 reachable pattern，再逐 pattern 对照原生实现补齐；未知 pattern blocker |
| ArithToAffine | `Conversion/ArithToAffine/ArithToAffine.cpp` | 已有 `RunArithToAffineConversion` | 源码复核后复用，并在新输入阶段单独差分 |
| CanonicalizeIterArg | `Dialect/SCF/Transforms/CanonicalizeIterArg.cpp` | 已有 iter-arg 受限实现 | 对照原生 For/While、dead iter arg、effect 判定逐分支补齐 |
| SCFForLoopCanonicalization | `third-party/llvm-project/mlir/.../LoopCanonicalization.cpp` | 当前聚合 canonicalization 覆盖部分效果 | 拆出可独立调用/验证的 stage，按原生 pattern 实现 |
| CSE | `third-party/llvm-project/mlir/lib/Transforms/CSE.cpp` | 已有受限、确定顺序 CSE | 复核 region/dominance/effect/equivalence 条件后在新阶段复用 |
| HIVMOptSinglePoint | `HIVM/Transforms/HIVMOptSinglePoint.cpp` | 现有实现绑定 post-bufferization `BufferizedSemanticIR` | 新增 pre-CV GenericModule 版本或证明分支不可达；不得直接复用错误阶段状态 |
| Memref DSE | `Dialect/MemRef/Transforms/DeadStoreElimination.cpp` | 没有独立 pre-CV stage | 按原生 alias/access/region 条件实现；不可达分支必须有证明与 guard |
| InlineOTFBroadcast | `HIVM/Transforms/InlineOTFBroadcast.cpp` | 没有独立实现 | 按单一 `VBrcInlinePattern` 逐句复刻 |

AutoBlockify 原生约 192 行、已有轻量实现约 573 行；MarkMultiBuffer 原生约 330 行；
CanonicalizeIterArg 原生约 1710 行，是本次风险最高的单体之一。ExtendedCanonicalizer 驱动本身
很小，但动态收集所有已加载 dialect/op pattern，是覆盖面最大的风险，不能按 93 行驱动代码
低估工作量。

## 5. 阶段 0：冻结旧边界并建立逐 pass checkpoint

状态：**2026-07-30 已完成**。

### 目标

在移动产品入口前，保存 `1dfddd59` 的 before-CV 正确性/性能基线，并建立不改变原生语义的
before-AutoBlockify 与每个新增 pass 后 checkpoint。

### 执行方式

1. 保留当前产品 prediction pass 位置和剪枝行为不变。
2. 在真实 pipeline 增加默认关闭的观察点：
   - before AutoBlockify；
   - 每个上述 pass 后；
   - before CVPipelining。
3. checkpoint 只用于测试：输出稳定结构记录，不改变 pass 输入、遍历或 insertion order。
4. 稳定结构记录至少覆盖：
   - operation/block/region 顺序与层级；
   - result/operand/iter_arg/yield 对应关系；
   - type、shape、layout、address space；
   - memory effects；
   - AutoBlockify、workspace、multi-buffer、broadcast、core-type 属性。
5. 对相同 ModuleOp，原生 pass 链和轻量 pass 链必须从同一个 before-AutoBlockify checkpoint
   分叉，禁止用不同编译产生的输入冒充逐 pass 对比。

### 完成条件

- checkpoint 默认关闭，普通构建/运行无输出和可测开销；
- 能单独停止并取得任意新增 pass 前后的原生结构；
- 当前 before-CV embedded 回归未改变。

### 完成证据

- `BISHENGIR_UB_PREFIX_CHECKPOINT_DIR` 默认未设置时不插入任何 dump pass；
- 共生成 14 个固定 checkpoint，覆盖 before AutoBlockify、13 个 pass 后和最终 before-CV；
- `BISHENGIR_UB_PREFIX_CHECKPOINT_STAGE` 可精确选择单个 stage；
- `BISHENGIR_STOP_AFTER_UB_PREFIX_CHECKPOINTS=1` 在最终 checkpoint 后停止，不加入
  CVPipelining/PlanMemory；
- vector-add 的最终 checkpoint 与修改前 `BISHENGIR_DUMP_BEFORE_CVPIPELINING` 输出逐字节一致；
- 默认产品路径的 before-CV dump 与修改前基线逐字节一致；
- `bishengir-compile` Release 构建和 `ub_overflow_model_cpp/tests/run_tests.sh` 全部通过。

### 提交边界

独立的测试基础设施提交，不混入任何 pass 语义。

## 6. 阶段 1：接入已有 AutoBlockify

状态：**2026-07-30 已完成**。

### 实现

1. 完整复核原生 `AutoBlockifyParallelLoop.cpp` 与当前
   `auto_blockify_parallel_loop.hpp`，列出每个条件和操作顺序的一一对应关系。
2. 生产轻量前缀调用已有实现；优先使用当前已经验证的入口，不重写第二套实现。
3. 支持禁用分支精确 no-op；读取真实 resolved `enableTritonKernelCompile`、
   `enableAutoBlockifyLoop` 和模块 device/core-count 信息。
4. 如需微调，只能修复与当前原生源码不一致之处；旧实现保留到新 checkpoint 对齐后再清理。

### 验证

- 现有 fixture/legacy-overlay 差分全部通过；
- `verify_auto_blockify.py` 对 160 输入重新现场验证；
- 新 before-AutoBlockify 输入上逐字段比较 after-AutoBlockify checkpoint；
- 覆盖禁用、VECTOR、CUBE、MIX、factor、缺失 logical mark 和失败路径。

### 闸门

after-AutoBlockify checkpoint 完全对齐后才能开始 MarkMultiBuffer，不允许先串起完整前缀再回头
定位。

### 完成证据

- `AutoBlockifyPrefixOptions` 只表达原生两个 gate，启用路径调用已有 stable-ID 实现，禁用路径
  不修改 module；
- 现有 legacy/stable-ID fixture 和完整模型测试套件通过；
- 同一个真实 compiler attempt 的 before/after checkpoint 在 160 个去重 adapter 上
  `160 PASS / 0 mismatch / 0 unavailable`；
- `enableAutoBlockifyLoop=false` 的原生 before/after IR 逐字节一致，模型禁用路径结构一致；
- vector-add 与 fused-attention、production-default、seeds `{0,13}` 的最终 embedded
  PlanMemory 小回归为 `matched=4, different=0, unavailable=0, timeout=0`；
- production prediction pass 位置和现有 before-CV API 未移动，本阶段不会重复运行
  AutoBlockify。

## 7. 阶段 2：实现 CV 前 MarkMultiBuffer

状态：**2026-07-30 已完成**。

### 实现

按原生文件逐个迁移下列语义：

1. workspace traceback：AllocWorkspace、ToTensor、ViewLike 链；
2. 已有 annotation mark 的识别、校验和跳过；
3. `MarkScopeMultiBuffer` 的 preload/core/V1-use 条件；
4. ND2NZ/Fixpipe/Load/Store 的 local-buffer mark 条件；
5. scf.for/scf.while parent-loop 限制；
6. MIX core 下 only-cube/only-vector/no-l0c 策略；
7. workspace multi-buffer 的 Store/Fixpipe、loop 和数量条件；
8. greedy pattern 注册与应用顺序。

必须新增/传递真实 resolved options：

```text
enableAutoMultiBuffer
limitAutoMultiBufferOnlyForLocalBuffer
limitAutoMultiBufferOfLocalBuffer
limitMixAutoMultiBufferBuffer
setWorkspaceMultibuffer
disableAutoCVWorkSpaceManage
```

现有 PlanMemory 前 `ModelMarkMultiBuffer` 不能直接调用，因为它消费
`AfterInlineLoadCopyState`，而 CV 前 pass 运行在不同 IR 阶段。公共 enum/helper 只有在逐项证明
行为相同后才能提取复用。

### 验证

- 每个 pattern 单独 fixture；
- enable-auto=false、local-only、四种 strategy、MIX/AIV/AIC、workspace 数量、已有 mark、
  preload scope、unsupported loop 全覆盖；
- 比较 annotation 的位置、属性、source identity、operation 顺序和完整 checkpoint；
- 通过后再执行 AutoBlockify+MarkMultiBuffer 累积差分。

### 完成证据

- `src/passes/pre_cv_mark_multi_buffer.hpp` 独立建模原生 pre-CV pass，没有复用阶段错误的
  post-bufferization `ModelMarkMultiBuffer`；
- 已覆盖 workspace `AllocWorkspace/ToTensor/ViewLike` traceback、local scf.for/scf.while/if
  traceback、`getParentLoop` consumption/yield 传播、scope preload/V1-use、已有 mark 校验和全部
  strategy/workspace 数量选项；
- 原生 `applyPatternsGreedily` 隐含执行的 `muli/div/trunc(ext)` folding、dead arith 清理和
  constant hoist/unique 被作为本 pass 的可观察语义复刻；
- `pre_cv_mark_multi_buffer.mlir` 经真实 `bishengir-opt` 与模型比较，Load、Fixpipe、scope preload
  的 mark source、数量、属性和插入位置完全一致；
- 8 个相关 pre-CV profiles × 160 个去重 adapter 的单 pass 与
  AutoBlockify+MarkMultiBuffer 累计同-attempt checkpoint 均为 `1280/1280 PASS`；
- `ub_overflow_model_cpp/tests/run_tests.sh` 全部通过；
- vector-add、attn-fwd × production-default/auto-MB × seeds `{0,13}` 的下游 embedded
  PlanMemory 为 `matched=8, different=0, unavailable=0, timeout=0`。

### 提交边界

独立提交 `feat(ub-model): model pre-cv multi-buffer marking`，不混入 outer canonicalizer。

## 8. 阶段 3：实现外层 ExtendedCanonicalizer

状态：**2026-07-30 已完成**。

这个 pass 位于 CV 前 MarkMultiBuffer 后、九步 canonicalization pipeline 前。它主要为后续
InlineOTFBroadcast 清理冗余 1-to-1 broadcast，但真实行为来自全部已加载 pattern，不能只凭该
注释实现一个 broadcast 特例。

### 实现方法

1. 默认关闭地记录原生 pass 在当前支持域内实际触发的 pattern 名、目标 op 和 rewrite 次序；
2. 从对应 dialect/op 的 canonicalization 源码读取每个 reachable pattern；
3. 在模型中逐 pattern 复刻匹配、替换、erase 和 greedy convergence；
4. 为尚未支持但可能匹配的 op/pattern 建 capability guard，命中时 blocker/fail open；
5. 不用“当前 160 输入没变化”证明所有输入 no-op。

### 验证

- pattern 级 fixture；
- 单 pass checkpoint；
- AutoBlockify+MarkMultiBuffer+outer canonicalizer 累积 checkpoint；
- greedy rewrite 次数、最终 op 顺序和 use replacement 一致。

### 完成证据

- 实现直接对应原生 `ExtendedCanonicalizer.cpp`、MLIR greedy driver/FoldUtils、Arith folds、
  HIVM `RedudantVBrcOp`/`RedudantVReduceInitOp` 与 Annotation
  `FoldUselessBufferSizeMarkOp`；
- fixture 同时经过真实 `bishengir-opt --canonicalize-ext` 与轻量 runner，稳定结构完全一致；
- 8 profiles × 160 inputs 的单 pass `02 -> outer canonicalizer` 与累计
  `00 -> AutoBlockify -> MarkMultiBuffer -> outer canonicalizer` 均精确匹配原生 `03`，结果
  `1280/1280 PASS`；
- 完整 `ub_overflow_model_cpp/tests/run_tests.sh` 通过；
- vector-add、attn-fwd × production-default/auto-MB × seeds `{0,13}` 的下游 embedded
  PlanMemory 为 `matched=8, different=0, unavailable=0, timeout=0`。

### 提交边界

独立提交 outer ExtendedCanonicalizer，不混入阶段 4.1 ArithToAffine。

## 9. 阶段 4：逐个实现九步 canonicalizationHIVMPipeline

当前子阶段：**4.5 CSE**。

九个 pass 必须按下面九个子阶段依次实现和关闸。每个子阶段都执行：源码审阅→单元 fixture→
单 pass checkpoint→从 AutoBlockify 开始的累积 checkpoint；通过后才进入下一个。

### 4.1 ArithToAffine

状态：**2026-07-30 已完成**。

- 复核 affine legality、constant/index 处理和 canonicalization 顺序；
- 优先复用 `RunArithToAffineConversion`，但必须证明 before-AutoBlockify 输入覆盖一致。

完成证据：独立 pass 入口只执行原生 conversion 范围，不调用后续 affine canonicalization；
fixture 与真实 `bishengir-opt --convert-arith-to-affine` 精确一致；8 profiles × 160 inputs 的
`03 -> 04` 单 pass 与 `00 -> 04` 累计 checkpoint 为 `1280/1280 PASS`；完整测试通过；代表
PlanMemory seeds `{0,13}` 为 `8/8 matched`。

### 4.2 CanonicalizeIterArg

状态：**2026-07-30 已完成**。

- 逐项覆盖 scf.for/scf.while、iteration-independent、dead iter arg、effect/speculation、
  result/yield/region-arg 一致置换；
- 现有实现只作为起点，原生 1710 行源码是判定标准。

完成证据：fixture 与真实 `bishengir-opt --scf-canonicalize-iter-arg` 精确一致，覆盖外部 tensor
yield、嵌套 SCF unchanged、For/While dead channel 以及 vector-function backward gate；内部 CSE
只在原生 For/While pattern 调用点执行，保守 `none` effect 不再被当成 pure。8 profiles × 160
inputs 的 `04 -> 05` 单 pass及 `00 -> 05` 累计 checkpoint 为 `1280/1280 PASS`；完整测试通过；
代表 PlanMemory seeds `{0,13}` 为 `8/8 matched`。

### 4.3 ExtendedCanonicalizer（module）

状态：**2026-07-30 已完成**。

- 复用阶段 3 的 pattern registry/capability 机制；
- 重新记录这一 IR 阶段的 reachable patterns，不能沿用外层 pass 的 pattern 命中集合。

完成证据：新增独立 `RunModuleExtendedCanonicalizer` 阶段入口；按原生 greedy driver 的迭代顺序
覆盖此边界新暴露的 affine.apply/min/max 组合、常量与 identity fold、表达式/结果排序、CSE/DCE、
semi-affine 重建以及相关 Arith/slice fold。fixture 覆盖 fresh-constant 生命周期和 semi-affine local
项顺序；8 profiles × 160 inputs 的 `05 -> 06` 单 pass 与 `00 -> 06` 累计 checkpoint 为
`1280/1280 PASS`；4.2 的 1280 项回归、完整测试和代表 PlanMemory `8/8 matched` 均通过。

### 4.4 SCFForLoopCanonicalization

状态：**2026-07-30 已完成**。

- 按上游 `LoopCanonicalization.cpp` 的 pattern 集合迁移；
- 独立验证 loop bounds、iter args、body replacement 和 erase 顺序。

完成证据：逐句覆盖 tensor/memref 的 iter-arg 与 loop-result dim folder、递归
`tensor.insert_slice`/nested `scf.for` shape-preserving proof，以及 affine.min/max 对 `scf.for`、
`scf.parallel`、`scf.forall` 已知范围的约束化简。上游成功、部分化简、nested、no-change、
parallel/forall fixture 与真实 `bishengir-opt --scf-for-loop-canonicalization` 精确一致；8 profiles ×
160 inputs 的 `06 -> 07` 单 pass 与 `00 -> 07` 累计 checkpoint 为 `1280/1280 PASS`；完整测试
通过，代表 PlanMemory 为 `8/8 matched`。

### 4.5 CSE

状态：**2026-07-30 已完成**。

- 保持原生 dominance、region visibility、memory-effect 和 operation equivalence 条件；
- 哈希结构只用于 lookup，替换顺序必须来自原生有序 traversal。

完成证据：新增独立 `RunPreCVCSE`，复刻上游 nested-region-first、region-scoped known-values、
isolated function、commutative equivalence、read-only/write barrier 和 deferred erase 行为。定向 fixture
与真实 `bishengir-opt --cse` 精确一致；8 profiles × 160 inputs 的 `07 -> 08` 单 pass 与
`00 -> 08` 累计 checkpoint 为 `1280/1280 PASS`；完整测试通过，代表 PlanMemory 为
`8/8 matched`。当前支持域所有 region 均为 single block；multi-block SSACFG 显式 fail closed。

### 4.6 ExtendedCanonicalizer（func）

状态：**2026-07-30 已完成**。

- 与 4.3 共用实现但使用 func scope；
- 单独验证 scope 对 pattern worklist 和 region simplification 的影响。

完成证据：新增独立 `RunFirstFuncExtendedCanonicalizer` 入口；源码证明共享实现的 folder 和 rewrite
均按 enclosing function 隔离。双函数 fixture 与真实
`builtin.module(func.func(canonicalize-ext))` 精确一致；8 profiles × 160 inputs 的 `08 -> 09`
单 pass 与 `00 -> 09` 累计 checkpoint 为 `1280/1280 PASS`；完整测试通过，代表 PlanMemory
为 `8/8 matched`。

### 4.7 HIVMOptSinglePoint

- 对照原生 VBrc、Copy/Load、VAdd/VSub/VMul/VDiv/VAbs/VSqrt/VMax/VMin pattern；
- 区分 pure tensor 与 pure buffer semantics；
- 当前 post-bufferization 实现不能直接替代该 pre-CV stage；若当前输入全部 tensor 语义导致
  pattern 不触发，也必须由源码条件和 capability guard 证明。

### 4.8 ExtendedCanonicalizer（func）

- 重新运行同一实现；不能因为 4.6 已执行就省略，因为 4.7 会创建新的 arith/load/store op。

### 4.9 MemrefDeadStoreElimination

- 对照原生 alias、access、forwarding 和 region/control-flow 条件；
- 若 before-CV 支持域内不可达，使用明确的 IR capability 检查证明 no-op；未知 memref access
  必须 blocker，不能静默忽略。

### 阶段闸门

九个 checkpoint 和完整 cumulative checkpoint 全部通过后，才允许进入 InlineOTFBroadcast。

## 10. 阶段 5：实现 InlineOTFBroadcast

原生 pass 当前只有一个 `VBrcInlinePattern`，按源码逐句迁移：

- 必须是 pure tensor semantics；
- source 必须是 tensor；
- 只支持单 broadcast axis；
- i64/i1 不 inline；
- LAST axis 使用原生白名单和 VAbs element-type 限制；
- 其他 axis 要求 BroadcastableOTF、binary、HIVMStructured；
- 合并用户已有 broadcast dims；
- 按原始 user 顺序替换 DPS input；
- 至少一个有效 user 才算 rewrite success；
- 保持 VBrc 对其他不合法 users 的用途。

验证覆盖每个成功/失败条件、多 user、部分可 inline user、已有 dims 和 user 顺序。最终比较
before-CVPipelining checkpoint。

## 11. 阶段 6：组合前缀与 API/input contract

### 参数和合同

1. 输入合同从 before-CVPipelining 升级为 before-AutoBlockify，提升 contract version 和
   pipeline fingerprint。
2. 从同一个 `HIVMPipelineOptions` resolved instance 传入所有新增分支参数，禁止模型侧重新
   推导默认值。
3. 至少新增或确认：
   - `enableTritonKernelCompile`；
   - `enableAutoBlockifyLoop`；
   - `limitAutoMultiBufferOnlyForLocalBuffer`；
   - `setWorkspaceMultibuffer`；
   - 现有 auto-MB strategy 和 workspace-manage 参数。
4. standalone before-CV 兼容入口可暂时保留为测试工具，但不得与新产品入口混淆。

### 组合流水

```text
before-AutoBlockify GenericModule
  -> AutoBlockify [conditional]
  -> pre-CV MarkMultiBuffer [conditional]
  -> outer ExtendedCanonicalizer
  -> canonicalizationHIVMPipeline [9 exact stages]
  -> InlineOTFBroadcast
  -> existing CVPipelining-to-PlanMemory pipeline
```

先在模型内部启用新入口，真实 BiSheng prediction pass 仍保持旧位置；通过 before-CV checkpoint
后，再进入 embedded observe-only。

## 12. 阶段 7：embedded observe-only 与最终切换

### observe-only

1. 把测试用 prediction hook 放到真实 AutoBlockify 前；
2. 模型运行完整新路径，但无论结果如何都不剪枝；
3. 原生 pipeline 继续运行 AutoBlockify、CVPipelining 和 local PlanMemory；
4. 同一 attempt 比较模型与原生最终合同；
5. 保留旧 before-CV model 作为短期迁移 oracle，仅限测试，不长期运行两次产品模型。

### 验证顺序

1. 单 pass fixture；
2. 每个 pass 的 160-input checkpoint；
3. 代表算子 × 所有相关配置 × seeds `{0,13}`；
4. 代表算子 × 所有相关配置 × seeds `0-19`；
5. 当前有意义配置的 160 × configs × 20 全量 embedded 验证。

比较字段：status、overflow、required、peak、plan、lifetime、multi-buffer、inplace。已知等尺寸
buffer identity 置换单独分类，不能掩盖新增前缀造成的差异。

### 产品切换

只有全部满足时才允许：

- before-CV cumulative checkpoint 对齐；
- 20-seed 最终合同无新增差异；
- blocker 正确 fail open；
- 新增参数与真实 compiler resolved values 一致；
- debug/validation 默认关闭；
- observe-only 稳定后，才允许 exact overflow 剪枝；
- 删除旧 before-CVPipelining 产品调用，确保一个 attempt 只运行一次模型。

## 13. 性能测量

边界前移增加了真实工作，旧 `CVPipelining→PlanMemory` 的 `3.95x` 只能保留为历史基线。新阶段
必须同时报告：

```text
旧模型 CVPipelining->PlanMemory internal total
新增 AutoBlockify->before-CVPipelining prefix internal total
新模型 before-AutoBlockify->PlanMemory internal total
原生 BiSheng AutoBlockify->local PlanMemory wall delta
BiSheng/model ratio（同一新边界）
per-case median / mean / p95 / max
peak RSS
pass/stage breakdown
```

结构性能主测量使用 Release/O3、真实 retry-only、160 × 有意义配置、关闭提前 non-overflow
返回及证明计算，使所有 case 执行到完整 PlanMemory。production fast path 另测，不能掩盖新增
前缀回退。

功能阶段不能为了性能省略真实 pass。若某 pass 成为热点，必须在完整对齐后另开优化批次，仍以
原生语义为边界。

## 14. 提交与汇报边界

建议提交顺序：

1. `test(ub-model): add pre-autoblockify native checkpoints`
2. `feat(ub-model): integrate autoblockify prefix stage`
3. `feat(ub-model): model pre-cv multi-buffer marking`
4. `feat(ub-model): model pre-cv outer canonicalization`
5. canonicalization 九个子阶段按实际独立性分 3～9 个提交
6. `feat(ub-model): model inline otf broadcast`
7. `refactor(ub-model): define before-autoblockify input contract`
8. `test(ub-model): validate pre-autoblockify embedded pipeline`
9. `feat(bishengir): move UB prediction before autoblockify`

每个 pass 闸门通过后向用户报告：源码对应、实现范围、checkpoint 结果、最终合同小回归、工作树
和下一 pass。未经用户要求不 push。半成品需要跨会话保存时可做明确的本地 checkpoint commit。

## 15. 延后事项

以下不与本功能扩展混合：

- 全流水 stable-ID/ShadowIR 迁移；
- 通用 revisioned analysis manager；
- 统一 UBBufferProgram/PlanProgram；
- PlanMemory retry 基础设施重写；
- LLVM/MLIR core dependency 拆分；
- LTO/PGO、seed 并行和 autotune 总收益优化。

## 16. 完成定义

1. 产品模型输入位于真实 AutoBlockify 前；
2. AutoBlockify 前 61 次原生 pass 不在模型中重复实现；
3. AutoBlockify 到 before-CV 的 13 个 pass 按真实条件和顺序复刻；
4. 现有 AutoBlockify 实现得到复用并重新通过真实 checkpoint；
5. 每个新增 pass 都有独立原生 checkpoint 差分证据；
6. 模型准确接收这段流水使用的全部 resolved options；
7. fixed seeds 0～19 的完整 PlanMemory 合同对齐；
8. 没有通过 kernel/seed/config 特例或修改原生逻辑获得一致；
9. blocker/fail-open、exact overflow 和提前 non-overflow 合同正确；
10. 新边界有独立的正确性、单轮时间、stage 分布和 BiSheng/model 比值报告；
11. 普通运行不输出调试日志、不生成 checkpoint、不运行两次模型；
12. `.agent`、API 文档和真实代码行为一致。

始终记住：逐 pass 对齐是硬门槛。不得先追求最终结果一致，再用结果倒推出一套与 BiSheng
源码无对应关系的逻辑。
