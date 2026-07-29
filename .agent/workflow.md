# 当前工作流：逐 pass 扩展到 before-AutoBlockify

## 当前任务

把模型输入从 before-CVPipelining 前移到 before-AutoBlockify，逐个复刻两者之间的 13 个原生
pass，再接入现有 CVPipelining→PlanMemory 模型。AutoBlockify 前的 61 次原生 pass 继续由
BiSheng 执行，不复刻。

当前不做全局 IR/analysis 基础设施替换，不在模型核心新增 LLVM/MLIR 依赖，不同时进行纯性能
重构。

阶段状态：checkpoint infrastructure、AutoBlockify、pre-CV MarkMultiBuffer、outer
module-level ExtendedCanonicalizer、阶段 4.1 ArithToAffine、阶段 4.2 CanonicalizeIterArg 和阶段
4.3 module-level ExtendedCanonicalizer、阶段 4.4 SCFForLoopCanonicalization、阶段 4.5 CSE
和阶段 4.6 func-scoped ExtendedCanonicalizer 已于 2026-07-30 完成；当前只执行阶段 4.7
HIVMOptSinglePoint，其 after-pass checkpoint 未对齐前不得开始第二次 func-scoped
ExtendedCanonicalizer。

## 每个 pass 的固定开发循环

1. 完整阅读对应原生源码和其直接调用的 pattern/helper。
2. 写出匹配条件、修改动作、失败条件、worklist/use-order 和参数来源清单。
3. 建立该 pass 的默认关闭原生 checkpoint；checkpoint 代码不得修改原生语义。
4. 在现有模型自有结构中实现最小等价逻辑；优先复用已验证代码，但先证明阶段和条件相同。
5. 增加成功、失败、边界和未知分支 fixture。
6. 比较单 pass 前后结构：op/block/region 顺序、def-use、type/layout/effect 和全部相关属性。
7. 从 before-AutoBlockify 开始运行累计前缀 checkpoint。
8. 做代表输入的最终 PlanMemory seeds `{0,13}` 小回归。
9. 全部通过后做一次本地提交并报告，再进入下一个 pass。

禁止先把所有 pass 串起来，然后只根据最终 peak/plan 差异补规则。

## 实现顺序

```text
checkpoint infrastructure
AutoBlockify
pre-CV MarkMultiBuffer
outer ExtendedCanonicalizer
ArithToAffine
CanonicalizeIterArg
ExtendedCanonicalizer(module)
SCFForLoopCanonicalization
CSE
ExtendedCanonicalizer(func)
HIVMOptSinglePoint
ExtendedCanonicalizer(func)
MemrefDeadStoreElimination
InlineOTFBroadcast
combined before-CV checkpoint
embedded observe-only
20-seed full validation
production entry switch
```

任何一步未通过，后续 pass 不开始。

## 不得编造逻辑

- 原生源码是唯一语义来源；测试数据只用于证明和定位。
- 不以“对当前 160 输入无变化”替代源码中的匹配条件。
- 不认识的 operation、pattern 或 effect 必须 blocker/fail open。
- 不增加 kernel、SSA、buffer 数量、配置或 seed 特例。
- 不改变 greedy rewrite、CSE dominance、use-list、operation order 或 RNG 顺序。
- 不修改原生 BiSheng pass 使其迁就模型。
- 如果现有模型 helper 与新阶段的 IR 语义不同，不能因名称相同直接复用。

## 验证层级

1. pattern/helper 单元测试；
2. 单 pass native/model checkpoint；
3. 从 AutoBlockify 开始的累计 checkpoint；
4. before-CV 完整结构差分；
5. 代表算子 × 相关配置 × seeds 0～19；
6. 160 × 有意义配置 × 20 seeds embedded 全量；
7. production-default retry-only 性能；
8. 160 × 有意义配置、full-plan、无提前证明的结构性能。

正确性 oracle 始终是同一真实 `bishengir-compile` attempt 中的原生 local PlanMemory。缓存不能
替代原生结果。提前 non-overflow 在 validation 中只作为信号，必须继续完整模型和原生计划。

## 参数纪律

所有新增选项从同一个真实 `HIVMPipelineOptions` resolved instance 传递，不能在模型侧重复推导
默认值。至少覆盖 AutoBlockify enable、Triton enable、workspace manage、auto-MB、local-only、
local/mix strategy 和 workspace multi-buffer 数量。

## 依赖纪律

- BiSheng adapter 可以读取 `ModuleOp`；模型核心新增 pass 不使用 LLVM/MLIR 类型。
- 使用现有 `GenericModule` 和公共工具，不启动 ShadowIR 全迁移。
- standalone 与 embedded 使用相同模型语义；兼容 before-CV 入口只作短期测试。
- 当前已有 overlay 只在 AutoBlockify 已验证实现内使用，不把它扩散到后续 pass。

## Git 与临时产物

- 每个通过 checkpoint 和小回归的 pass 单独提交或形成清晰的逻辑提交。
- 未完成代码可做不 push 的 `checkpoint(...)`，未经用户要求不 push。
- 保留用户已有修改，禁止 destructive reset/checkout。
- `Output/`、`ub_overflow_model_cpp/output/`、cache、dump、runtime TSV 和临时 profile 不提交。
- 每个 pass 完成后更新 `.agent` 当前状态；过时计划直接替换，不堆叠流水账。
