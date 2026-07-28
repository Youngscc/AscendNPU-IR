# UB Overflow 模型性能重构与 before-AutoBlockify 前移执行方案

本文是交给后续实现者的主执行方案。它描述的是产品实现顺序，不是测试脚本优化计划。
执行者开始工作前必须同时阅读 [MEMORY.md](MEMORY.md)、[code_map.md](code_map.md)、
[workflow.md](workflow.md) 和 [validation.md](validation.md)。源码、真实测量与本文冲突时，先用
源码和同机实验重新核实，再更新本文，禁止带着已经失效的假设继续实现。

## 1. 最终目标

将轻量 UB overflow 模型作为真实 `bishengir-compile` 中的一个同步 Module pass，入口从当前
before-CVPipelining 前移到 before-AutoBlockify：

```text
ModuleOp before AutoBlockify
  -> embedded lightweight model
       -> 模拟 AutoBlockify 到 CVPipelining 之间的真实语义
       -> 模拟 CVPipelining 到 local PlanMemory 的 UB 相关语义
       -> exact overflow：终止当前 attempt，交给 BiSheng 原有 fallback
       -> exact non-overflow：继续真实 compiler pipeline
       -> incomplete/blocker：fail open，继续真实 compiler pipeline
```

首要指标是轻量模型自身的单轮速度：在相同输入集合、相同 resolved options、相同
retry-only 语义和相同构建模式下，比较原模型与新模型的每输入耗时、单轮聚合耗时和峰值
内存。当前任务不评估 autotune 总收益，也不把原生 compiler 被剪枝后节省的时间计入模型
性能收益。

代码精简只是消除字符串解析、重复索引和中间转换后的附带收益，不是独立目标。禁止为了少写
模型代码而 clone `ModuleOp` 并运行原生 pass pipeline；那会退化成完整后缀编译的成本。

## 2. 当前基线与不可破坏原则

当前产品入口仍在 before-CVPipelining，BiSheng pass 将 `ModuleOp` 打印为 Generic MLIR 文本，
模型再解析为 `GenericModule`。模型内部约 4.9 万行 C++，主要性能热点和 2026-07-28 基线见
[validation.md](validation.md)。当前原模型的主要单轮基线为：

```text
160-input retry-only internal median aggregate    约 888 ms
process wall（三轮）                               1433 / 1392 / 1379 ms
```

所有阶段必须遵守以下原则：

- 只有 `Precision::Exact && overflow == true` 可以剪枝；blocker/incomplete 必须继续编译器。
- 未固定 seed 时保持原生 retry 合同：seed 0～19 任一成功即 non-overflow，全部失败才 overflow。
- 不修改 BiSheng 原生 AutoBlockify、CVPipelining、bufferization、PlanMemory 或 fallback 的核心
  逻辑。允许增加默认关闭的观察、计时、dump 和边界验证辅助。
- 不为 adapter、kernel 名、SSA 名、buffer 数量、配置名或 seed 写特例。
- 任何会影响 op、buffer、RNG、lifetime 或地址规划顺序的遍历必须来自显式有序容器。
- 新逻辑必须来自对应 BiSheng pass 源码；出现差异时先定位真实 pass 行为，禁止自行发明
  “合理逻辑”让测试通过。
- embedded correctness oracle 是同一真实 `bishengir-compile` attempt 中继续运行得到的原生
  local PlanMemory。
- 性能测量默认关闭 dump、validation、逐 pass artifact 和详细 stage timing。
- `Output/`、`ub_overflow_model_cpp/output/`、cache、dump、临时 TSV 和 profile 不提交。

## 3. 目标架构

```text
只读 mlir::ModuleOp
  -> MLIRModuleView
       Operation / Value / Type / Attribute / Region / Block
  -> stable-ID shadow overlay
       base Operation* + stable IDs + overrides + synthetic arena
  -> revisioned analysis manager
       def-use / CFG / parent / order / effects / typed metadata
  -> AutoBlockify-to-CV semantic stages
  -> CVPipelining and UB-affecting stages
  -> UBBufferProgram
  -> typed PlanProgram
  -> integer-ID PlanMemory
```

原始 `Operation*` 只在当前同步 pass 调用期间有效。模型不得修改输入 `ModuleOp`，也不得跨
RetriablePassManager attempt 保存 MLIR 指针。跨 fallback 复用只能保存无指针紧凑 snapshot。

## 4. 总体阶段顺序

| 阶段 | 任务 | 性质 | 进入下一阶段的核心条件 |
|---|---|---|---|
| 0 | 冻结基线和测量口径 | 准备 | 正确性、时间、内存基线可重复 |
| 1 | 直接 `ModuleOp` 输入 | P0 性能基础设施 | 生产路径不再打印/解析 MLIR 文本 |
| 2 | stable-ID shadow overlay | P0 性能基础设施 | 变换不复制完整原 IR，顺序合同通过 |
| 3 | 增量 analysis manager | P0 性能基础设施 | mutation 不再触发全量 def-use/CFG 重建 |
| 4 | 入口前移、改造已有 AutoBlockify 并补齐到 CV | P0 产品功能 | 新边界完整语义和 PlanMemory oracle 对齐 |
| 5 | 扩展后公共 fast path 优化 | P0 产品性能 | 新模型单轮总耗时明显下降 |
| 6 | 统一 `UBBufferProgram/PlanProgram` | P1 性能基础设施 | 正常路径无文本 PlanMemory bridge |
| 7 | 整数化和复用 PlanMemory | P1 overflow 性能 | 20-seed slow path 明显下降且语义不变 |
| 8 | fallback snapshot、LTO/PGO | P2 系统性能 | 新模型单轮耗时继续稳定下降 |

阶段 4 的边界要求必须从阶段 1 起参与接口设计，但实际 AutoBlockify→CV 功能实现必须等阶段
1～3 的最低可用基础设施完成后进行，避免在旧 `GenericModule` 上完成后再重写一次。

仓库中的轻量 AutoBlockify 已经实现，并有独立 runner、验证脚本和单元测试。后续任务不是
从零重新实现 AutoBlockify，而是先核对它与当前 BiSheng 源码是否仍一致，再将现有语义实现
适配到 direct-MLIR/shadow overlay 基础设施，最后接入新的 before-AutoBlockify 产品入口。
改造期间应尽量复用已经验证过的匹配条件、错误条件、顺序合同和测试用例。

## 5. 阶段 0：冻结基线和建立可观测性

### 目标

在结构性修改前保存可重复的正确性、性能和内存基线，明确优化前后的同口径比较方式。

### 执行方式

1. 确认工作树、分支、构建模式和模型 build identity；Release 必须为 `-O3 -DNDEBUG`。
2. 构建模型和真实 compiler：

   ```bash
   bash ub_overflow_model_cpp/build.sh
   cmake --build build --target bishengir-compile -j8
   ```

3. 运行完整模型测试：

   ```bash
   bash ub_overflow_model_cpp/tests/run_tests.sh
   ```

4. 保存代表性 embedded fixed-seed 结果。至少覆盖：simple AIV、MIX、attention overflow、
   late-seed success、auto multi-buffer、UB-saving、InjectBlockSync 和 AutoBlockify 输入。
5. 保存当前原模型二进制或可复现 commit，使用 production retry-only 对同一输入集合交替测量
   原模型和新模型，记录：
   - 每个输入的 `Result::totalTimeNs`；
   - 单轮 internal total；
   - 单轮 process wall；
   - median、mean、p95 和最慢输入；
   - 模型峰值 RSS。
6. 报告写入 `ub_overflow_model_cpp/output/performance/`，不提交。

### 完成条件

- 连续三轮关键数据没有无法解释的大幅漂移；
- 当前正确性基线可复现；
- 明确记录输入集合、配置、seed/retry 模式、并发度和 timing 开关。

### 提交边界

本阶段通常不产生产品提交；必要的纯计时修复单独提交，不与后续架构代码混合。

## 6. 阶段 1：删除生产文本边界，直接读取 ModuleOp

### 目标

生产 embedded 路径不再执行：

```text
ModuleOp -> generic text -> ParseGenericIR -> string-heavy GenericModule
```

standalone CLI 仍保留，但由 MLIR parser 解析文件后调用同一个 ModuleOp 入口。

### 执行方式

1. 在模型 API 中增加 MLIR-backed 同步入口。建议把“输入 IR”和“resolved options”拆开：
   - embedded API 接受 `mlir::ModuleOp` 或不持有所有权的等价 view；
   - standalone API 负责创建 `MLIRContext`、注册需要的 dialect、解析文件，再调用 embedded
     核心；
   - 旧文本 API 暂时只作为兼容 wrapper，不能继续作为生产入口。
2. 修改 `BiShengIRUBOverflowModel` 的 CMake 依赖，使其显式链接所需 MLIR IR、support、
   dialect/interface 库。只链接实际查询需要的库，不引入 PassManager 驱动整套原生 pipeline。
3. 新建只读 `MLIRModuleView`：
   - 以 IR 原始 region/block/op 顺序分配稳定 `OpId/BlockId/ValueId`；
   - `Type`、`Attribute`、`OperationName` 保存 MLIRContext uniqued 句柄；
   - 不复制完整 op 文本，不格式化 memref type，不解析 attribute dictionary 字符串；
   - 原始 `Operation*` 只在当前 evaluate 调用中使用。
4. 重构 input digest：
   - 普通生产请求不得为了生成 digest 重新打印完整 IR；
   - validation 模式可以由调用者提供已经计算的结构 digest，或使用单独的 opt-in 结构哈希。
5. 在迁移期间提供双入口差分：同一个 `ModuleOp` 分别走旧文本入口和新入口，比较完整模型
   结果和关键阶段摘要。

### 重点文件

```text
bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp
ub_overflow_model_cpp/include/ub_overflow_model/api.hpp
ub_overflow_model_cpp/src/api.cpp
ub_overflow_model_cpp/CMakeLists.txt
ub_overflow_model_cpp/src/ir/generic_ir.hpp
```

### 验证

- 新旧入口在代表 fixed seeds 上 status/required/peak/plan/lifetime/multi/inplace 一致；
- embedded model 对同进程原生 PlanMemory 一致；
- standalone CLI 仍能消费现有 before-CV 文件；
- ASan 或等价检查确认不保存失效 `Operation*`。

### 性能门槛

- 生产路径 `serialize_ns` 和 `ParseGenericIR` 基本消失；
- direct import 成本必须显著低于原 serialize+parse；
- product wall time不能因为 MLIR dialect 注册或重复 context 初始化而回退。

### 提交边界

一个独立提交：`perf(ub-model): read embedded input directly from MLIR`。

## 7. 阶段 2：stable-ID shadow overlay

### 目标

避免把完整 `ModuleOp` 复制为另一棵可变通用 IR，同时支持模型复刻 pass 所需的创建、删除、
移动和替换语义。

### 执行方式

1. 定义强类型 ID：`OpId`、`ValueId`、`BlockId`、`RegionId`、`BufferId`，禁止在新代码里以
   裸字符串或 SSA 打印名作为核心 identity。
2. 对原始节点只保存：
   - stable ID；
   - base `Operation*`/`Value`；
   - active bit；
   - 必要的 override bitmask；
   - projection/source identity。
3. synthetic node 存入 append-only arena；删除使用 tombstone/active `BitVector`，不要每个
   pass 后全量 compact 和重新编号。
4. block 顺序用显式 ordered ID vector；lookup 可以用 `DenseMap`，但其遍历不得产生语义
   顺序。
5. 提供统一 rewriter API：
   - create op/region/block；
   - insert/move/erase；
   - replace operand/all uses/uses except；
   - clone semantic node；
   - set typed attribute/type/effect override。
6. 迁移 pass 时保持旧实现并行可比较。优先迁移最能验证结构能力的操作：
   - canonicalization；
   - MarkRealCoreType projection；
   - CVPipelining；
   - AutoBlockify standalone 测试实现。
7. 所有序列化移动到 debug serializer，生产结构不得依赖 serializer 才能继续下一阶段。

### 验证

- 为每个 mutation primitive 增加顺序、use replacement、nested region 和 tombstone 单测；
- 对迁移 pass 比较旧 Generic 实现与 overlay 实现的阶段输出和最终 PlanMemory；
- 特别检查 MIX projection、scf iter_args/result/yield 的一致置换和 synthetic op 顺序。

### 性能门槛

- 不允许出现完整 `ModuleOp` clone；
- mutation-heavy kernel 的 module copy、compaction 和字符串 allocation 明显下降；
- 代表 160-input internal total 相对阶段 1 继续下降，否则先 profile 再扩展迁移。

### 提交边界

可拆为两个产品提交：基础 overlay primitives；首批 pass 迁移。不得把未验证的一半 pass 切换
为默认产品路径。

## 8. 阶段 3：revisioned 增量 analysis manager

### 目标

消除每个阶段或 mutation wave 后重复重建 definitions、users、value types、enclosing function、
descendants、CFG 和 metadata cache 的工作。

### 执行方式

1. 将分析划分为独立 revision：
   - topology/order；
   - def-use；
   - type/attribute/effect；
   - CFG/dominance；
   - enclosing function/descendants；
   - UB buffer feature summary。
2. 原始值的初始 users/definition 从 MLIR 读取；overlay mutation 维护 delta use-list，不能每次
   回退到 module scan。
3. rewriter 精确通知：operand replace、op create/erase/move、block create、type/attr override。
4. 只失效受影响分析；增加 debug assertion，禁止使用旧 revision 的 analysis。
5. 对只读 stage 复用同一 analysis；对 projection 使用 active `BitVector`，不要重新构造两份
   module 和两份索引。
6. 增加诊断计数器（默认关闭）：analysis build 次数、全量 scan 次数、updated uses 数、
   allocated synthetic nodes 数。

### 验证

- mutation 后增量结果与从头 rebuild 的 debug oracle 一致；
- randomized operation replacement/move 测试不出现悬空 ID；
- fixed-seed 完整 plan 结果保持一致。

### 性能门槛

- 生产代表路径不再在多个 stage 重复构造等价全量索引；
- 全量 rebuild 只允许出现在明确 stage boundary 或 debug cross-check；
- 复杂 MIX/attention kernel 得到可重复收益，而不是仅优化小 corpus 特例。

### 提交边界

一个独立基础设施提交；若需要迁移多个 pass，后续按 pass 分提交。

## 9. 阶段 4：入口前移、改造已有 AutoBlockify 并补齐到 CV

这是基础设施完成后的第一项功能扩展，也是后续所有性能数字的新产品边界。

### 9.1 真实 pass 序列

必须以 `HIVMPipelines.cpp` 当前源码为准。当前 AutoBlockify 到 CVPipelining 的顺序为：

```text
AutoBlockifyParallelLoop                         [conditional]
MarkMultiBuffer                                 [workspace manage enabled]
ExtendedCanonicalizer
canonicalizationHIVMPipeline:
  ArithToAffine
  CanonicalizeIterArg
  ExtendedCanonicalizer
  SCFForLoopCanonicalization
  CSE
  func ExtendedCanonicalizer
  HIVMOptSinglePoint
  func ExtendedCanonicalizer
  DeadStoreElimination
InlineOTFBroadcast
CVPipelining                                    [workspace manage enabled]
```

注意：这里的 CV 前 `MarkMultiBuffer` 与 PlanMemory 前只处理 local buffer 的
`MarkMultiBuffer` 是两次不同调用，配置和输入 IR 阶段不同，不能合并成一次。

### 9.2 新输入合同和参数

1. 将 input contract 升级为 before-AutoBlockify，并更新 pipeline fingerprint/build identity。
2. API 至少需要准确接收这一段真实分支使用的 resolved options：
   - `enableAutoBlockifyLoop`；
   - `enableTritonKernelCompile`（若产品 profile 固定为 true，也必须明确校验）；
   - `disableAutoCVWorkSpaceManage`；
   - `enableAutoMultiBuffer`；
   - `limitAutoMultiBufferOnlyForLocalBuffer`；
   - `limitAutoMultiBufferOfLocalBuffer`；
   - `limitMixAutoMultiBufferBuffer`；
   - `setWorkspaceMultibuffer`；
   - 后续 CVPipelining 已有参数。
3. 所有默认值来自同一个 `HIVMPipelineOptions` resolved instance，禁止模型侧重复推导默认值。

### 9.3 实现步骤

#### A. 建立开发边界和双 checkpoint oracle

- 在真实 pipeline 的 AutoBlockify 前增加默认关闭的 model/trace 接入点；
- 在真实 InlineOTFBroadcast 后、CVPipelining 前保留只用于验证的 checkpoint；
- validation 模式中，模型从 before-AutoBlockify 模拟到 before-CV，再让真实 pass 继续运行，
  比较模型边界摘要和最终原生 PlanMemory；
- 初期不得剪枝，避免未完成模型改变产品行为。

#### B. 改造并迁移已有 AutoBlockify

- 以真实 `AutoBlockifyParallelLoop.cpp` 为唯一语义来源；
- 现有 `src/passes/auto_blockify_parallel_loop.hpp` 已经实现了轻量 AutoBlockify，应作为本阶段
  的直接改造起点，不得无理由推倒重写；
- 先运行现有 `auto_blockify_model_runner`、`verify_auto_blockify.py` 和对应单元测试，确认当前
  实现与仓库内真实 pass 的语义基线；
- 将其中基于 `GenericModule/GenericRewriter` 的结构访问和 mutation 改接 shadow overlay，保留
  已经验证的匹配、physical block count、失败条件和操作顺序；
- 迁移期间允许旧 Generic 实现作为差分 oracle；overlay 实现完整通过后，生产只保留新路径，
  不长期维护两套 AutoBlockify；
- 精确复刻 physical block count、logical mark def-chain、op move、block-id replacement、
  synthetic op 顺序和失败条件；
- 与真实 AutoBlockify 单 pass checkpoint 做差分。

#### C. 实现 CV 前 MarkMultiBuffer

- 复用 typed option 和公共判定工具，但不能复用“PlanMemory 前 local-only state”作为结果；
- 模拟 workspace/tensor-level annotation 对后续 CVPipelining 的实际影响；
- 覆盖 `limitAutoMultiBufferOnlyForLocalBuffer` 和 `setWorkspaceMultibuffer`；
- 检查 CVPipelining 对 workspace multi-buffer mark 的清除/消费顺序。

#### D. 复用并校正 canonicalization 基础设施

- 按真实九个 pass 的顺序执行，不能把多个 canonicalization 简化成无序 fixpoint；
- 优先复用已经迁移到 overlay 的 ArithToAffine、CanonicalizeIterArg、CSE 和
  HIVMOptSinglePoint；
- 每个子 pass 可在 debug 下输出稳定结构摘要，默认不序列化 IR。

#### E. 补齐 InlineOTFBroadcast

- 读取真实 pass 的匹配、合法性、use replacement 和 erase 顺序；
- 只实现当前产品 profile 真实可达分支，但遇到未覆盖分支必须 blocker/fail open，不能默认
  当作 no-op；
- 与真实 pass checkpoint 做单 pass 和组合差分。

#### F. 切换产品入口

1. 先在 before-AutoBlockify 运行完整模型但 observe-only，继续真实 compiler；
2. 通过代表矩阵后，允许 exact overflow 剪枝；
3. 删除旧 before-CVPipelining 产品调用，避免同一次 attempt 运行两次模型；
4. 保留默认关闭的 before-CV checkpoint 作为后续诊断工具，而不是第二个产品模型入口。

### 9.4 正确性门槛

- 单 pass：AutoBlockify、pre-CV MarkMultiBuffer、canonicalization、InlineOTFBroadcast 分别与
  真实边界一致；
- 组合：模型模拟后的 before-CV 结构摘要与真实 pipeline 对齐；
- 端到端：same-process embedded model 与原生 local PlanMemory 的 status/required/peak/
  overflow 和 fixed-seed 详细 plan 对齐；
- 先跑代表输入 × 全有意义配置 × seeds `{0,13}`，再跑 20 seeds；
- 阶段完成前执行 160 × 27 × 20 embedded 全量；timeout、原生 abort、无模型观测必须单列，
  不能记为 matched；
- 已知原生 `SmallPtrSet<Value>` 合法置换只在已有证据覆盖的字段和算子上分类，不能用它掩盖
  新差异。

### 9.5 性能门槛

前移后必须分别报告：

```text
原模型 before-CV 单轮模型耗时（历史基线）
新模型 before-AutoBlockify 单轮模型耗时
新模型中 AutoBlockify→before-CV 模拟阶段耗时
两者输入边界和实际工作量差异
每输入 median / mean / p95 / maximum 与峰值 RSS
```

入口前移后模型增加了 AutoBlockify→CV 的实际工作，原模型和新模型的总耗时不是严格同工作量
A/B；报告必须明确标注这一点。功能正确但新模型单轮耗时明显变慢时，不得用“模型代码更少”
作为完成理由，应先保留 observe-only 或 feature gate，进入阶段 5 优化。

### 提交边界

建议至少拆为：

1. `refactor(ub-model): define before-autoblockify input contract`
2. `feat(ub-model): model autoblockify to cvpipelining prefix`
3. `test(ub-model): validate pre-autoblockify embedded boundary`
4. `feat(bishengir): move UB prediction before autoblockify`

不提交未完成的临时 dump 或 cache；需要保存现场时可以做不 push 的 `checkpoint(...)` 提交。

## 10. 阶段 5：优化扩展后的公共 fast path

### 目标

降低每次模型运行在 exact non-overflow 上界判定前都必须执行的公共工作，抵消入口前移新增
语义带来的模型内部成本。

### 执行方式

1. 重新 profile 新边界，不直接沿用 before-CV 时代的热点排序。
2. 将生产 request 明确标记为 decision-only：
   - 不构造 debug 名称、字符串 plan、stage artifact；
   - 不计算只供详细结果使用的字段；
   - 只维护 fast proof 和 slow path 真正需要的状态。
3. 合并重复 projection/canonicalization；只能消除语义等价的重复工作，不能改变 pass 顺序。
4. 对 immutable MLIR metadata 和 pre-AutoBlockify feature summary 只计算一次。
5. 对已经确定不包含 MIX/相关 op 的函数跳过无效 projection，但依据必须是通用 feature
   summary，而不是测试 kernel 名。
6. 保留 conservative upper-bound proof；debug/validation 观察证明但继续完整模型。

### 性能门槛

- 报告 fast-path 命中率和 fall-through 数；
- 新模型单轮 internal total 和每输入分布必须相对本阶段修改前明显下降；
- 同边界可比较阶段必须与原模型在同一输入上交替 A/B；入口不同的总耗时必须明确标注工作量
  差异，不能包装成严格加速比；
- 优化收益要在 simple、MIX、attention 多类输入上成立。

## 11. 阶段 6：统一 UBBufferProgram 和 typed PlanProgram

### 目标

消除以下多层状态和正常路径文本桥接：

```text
BufferizedSemanticIR
PostBufferizationRewriteState
AfterAllocExtraBufferState
AfterInlineLoadCopyState
AfterMarkMultiBufferState
PlanMemoryInputSemanticIR
OperationRecord::text
```

### 执行方式

1. 定义统一 `UBBufferProgram`，保存整数 identity、owner、allocation、alias、access、layout、
   address space、extra buffer、multi-buffer 和 preload 状态。
2. 每个 pass 对同一 program 写 typed delta，不再层层复制前一阶段完整对象。
3. 直接生成 `PlanProgram`：operation/event/value/buffer 均为整数 ID；control-flow、effects、
   iter_args、yield、inplace candidate 均为 typed 字段。
4. `plan_memory_input_builder.hpp` 中真实必要的特殊语义迁入 typed builder；文本格式、regex、
   memref 格式化和重复 value materialization 删除或移到兼容 parser/debug serializer。
5. 保留旧 bridge 作为短期 debug oracle，默认生产不调用；完整对齐后删除。

### 验证与性能门槛

- 新旧 bridge fixed-seed 全字段一致；
- `BuildPlanMemoryInput.EmitOperations/Normalize/MaterializeValueLists` 的主要成本消失；
- fast path 不得因为统一结构而被迫构造完整 `PlanProgram`；
- 代表和全量 embedded 回归通过后才能删除旧路径。

## 12. 阶段 7：PlanMemory 整数化和 seed-independent 复用

### 目标

降低真正 overflow、late-seed success 和 debug full-plan 路径的 liveness/address planning 成本，
不改变原生算法和 seed 结果。

### 执行方式

1. buffer/value/operation 名称查询改为整数 ID；名称只在最终诊断序列化时恢复。
2. retry 外一次构造 alias、CFG、event program、seed-independent liveness 基础。
3. 每个 seed 只重放 seed-sensitive kill/order 和地址规划状态。
4. conflict 集合使用 `BitVector`/紧凑矩阵；outline 改为 index arena，同时保持原 splice、
   rollback 和遍历顺序。
5. 一个 evaluation 内复用 scratch capacity，避免 20 seeds 反复分配。
6. 默认保持串行 retry；只有明确的单 kernel latency 实验证明收益后才考虑 seed 0 失败后的
   并行 retry，并显式控制外层调用并发以避免过度订阅。

### 验证与性能门槛

- 20 个 fixed seeds 的 selected seed、peak、required、offset、lifetime、inplace 一致；
- retry-only overflow/late-success wall time 明显下降；
- PlanMemory stage 目标收益 20%～40%，但以实测为准，不以代码改动量验收。

## 13. 阶段 8：跨 fallback snapshot 与构建优化

### 目标

消除 BiSheng code-motion fallback 对模型公共前缀的重复计算，并在架构稳定后评估链接和布局
优化。

### 执行方式

1. 找出 RetriablePassManager 中 UB fallback 的真实分叉参数；只缓存分叉前不受这些参数影响的
   无指针 snapshot。
2. cache key 覆盖输入结构、所有分叉前有效参数、target、pipeline fingerprint 和 model
   build identity。
3. 下一 attempt 的 `ModuleOp` 指针不可复用；从紧凑 snapshot 恢复 shadow/analysis。
4. 正确性和性能稳定后，分别 A/B 测试 ThinLTO/Full LTO 和代表模型输入 corpus PGO。
5. 不优先投入仅减少二进制或理论上更快但没有 profile 证据的编译选项。

### 完成条件

- fallback 前后结果合同不变；
- cache miss/stale 自动走安全现场路径；
- 多 attempt 总耗时下降；
- 新模型单轮总耗时在多轮交替测量中稳定下降。

## 14. 每阶段统一验证流程

### 阶段性结果汇报

每一个阶段完成实现和该阶段规定的测试后，执行者必须先向用户报告一次阶段性结果。除非用户
明确要求暂停，结果完全满足门槛时可以继续下一阶段；出现正确性差异、性能回退、范围扩张或
需要修改原生 BiSheng 逻辑时，必须暂停并等待用户决定。

每次阶段性汇报至少包含：

```text
阶段编号和目标
完成的代码修改与主要文件
当前 commit / 工作树状态
执行过的构建和测试命令
correctness matched/different/unavailable/timeout
修改前后单模型耗时
原模型与新模型的单轮 internal total / process wall
每输入 median / mean / p95 / maximum
入口或实际工作量是否发生变化
内存变化（该阶段适用时）
未解决问题和风险
下一阶段准备执行的内容
```

阶段性数字是临时结果，必须注明输入集合、配置、seed/retry、并发度和 timing 开关；后续更大
规模验证覆盖它们时，应更新 `.agent` 中的当前结论，不保留互相冲突的多代基线。

### 快速开发循环

1. 相关单元测试；
2. `bash ub_overflow_model_cpp/tests/run_tests.sh`；
3. representative operators × 相关 27 configs × seeds `0-19`；
4. 同机 baseline/new 交替性能测量至少三轮。

### 重大边界验证

以下修改必须执行 20-seed embedded 现场验证：

- 输入边界变化；
- ordering、stable ID、use-list 或 CFG 变化；
- AutoBlockify/pre-CV MarkMultiBuffer/CVPipelining 变化；
- PlanMemoryInput/PlanMemory 数据结构变化；
- fast proof 合同变化。

最终规模为当前有意义场景的 `160 × 27 × 20 = 86400`。若增加 AutoBlockify 参数场景，应新增
少量有意义配置，不做笛卡尔积；总数按实际配置文件重新报告。

### 发布前验证

- 现场运行真实 embedded oracle，确认脚本没有原生结果缓存或 prediction 后提前停止；
- 普通产品环境变量全部关闭；
- 使用真实 retry-only 重新测原模型和新模型的单轮时间与峰值 RSS；
- 检查模型 blocker 是否 fail open；
- 检查 exact overflow 是否仍进入 BiSheng 原 fallback；
- 检查非 overflow 是否继续完整真实 pipeline。

## 15. 性能报告固定格式

每个阶段必须同时报告以下内容，禁止只给某个 pass 的孤立加速比：

```text
日期 / commit / build mode / target
输入集合 / 配置 / seed 或 retry 模式 / jobs
原模型单轮 internal total / process wall
新模型单轮 internal total / process wall
每输入 totalTimeNs median/mean/p95/maximum
原模型/新模型输入边界与工作量说明
fast-path hit/fall-through/blocker
peak RSS
stage timing（仅诊断运行）
correctness matched/different/unavailable/timeout
```

性能结论只能来自预热后的交替 A/B 或 B/A 运行。不能先测完全部 baseline 再测全部 new，以免
温度、缓存和系统负载造成方向性偏差。

## 16. Git 与交接纪律

- 开始每阶段前记录 `git status --short` 和基线 commit，保留用户已有修改。
- 一次提交只包含一个可说明的基础设施或功能边界；测试和文档可独立提交。
- 半成品不必 push；需要跨会话保存时可在 `codex/` 分支建立清楚的本地
  `checkpoint(...)`，完成后整理成产品提交。
- 禁止 destructive reset/checkout；不得删除不属于当前任务的文件。
- 完成一个阶段后更新 `.agent/MEMORY.md`、`code_map.md`、`validation.md` 和本文状态，不追加
  多代流水账，直接替换失效结论。
- 任何 cache、dump、profile、runtime TSV 和临时 corpus 都不得进入产品提交。

## 17. 最终完成定义

只有同时满足以下条件，整个任务才算完成：

1. 产品 prediction pass 位于真实 AutoBlockify 之前；
2. 模型准确接受真实 pipeline 的 UB 相关 resolved options；
3. 模型完整复刻 AutoBlockify→CVPipelining 和后续到 local PlanMemory 的 UB 相关行为；
4. 生产路径直接读取 `ModuleOp`，不打印/解析完整 Generic MLIR；
5. exact overflow、exact non-overflow、blocker/fail-open 和 20-seed retry 合同正确；
6. embedded 现场 correctness matrix 达到预期，所有 unavailable/timeout 有明确分类；
7. 原模型与新模型在相同构建和 retry-only 口径下有同机单轮时间、每输入分布和峰值 RSS
   报告；
8. 新模型单轮速度达到阶段目标；入口前移导致的额外工作量已独立列出，不使用 autotune 或
   compiler 剪枝收益掩盖模型自身回退；
9. 默认运行不输出 debug 日志、不生成 dump、不读取测试 cache；
10. 文档、API、构建命令和真实产品行为一致。
