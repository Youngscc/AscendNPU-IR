# CVPipelining UB Model Memory

只保存当前可执行事实。源码与命令见 `code_map.md`，PlanMemory 输入验收记录见
`plan_memory_input_validation.md`。不要保存未授权的私有 IR。

## 目标与约束

```text
generic IR before CVPipelining + pass/PlanMemory config
  -> lightweight C++ model
  -> exact absolute local UB peak or exact overflow required_bits
```

- Oracle 是 `build/bin/bishengir-cvpipeline-suffix-compile` 的真实 minimal suffix。
- 核心模型使用独立 C++；Python 只生成数据、执行矩阵和比较结果。
- 模块、类、action、脚本和产物只按真实 pass 或 pass 边界命名。
- 生产代码固定为
  `src/main.cpp -> src/pipeline/cvpipelining_ub_pipeline.hpp -> src/passes/<real-pass-name>`；
  开发验证不得混入这条调用链。
- 只复刻影响 UB 的语义，但规则必须来自真实 pass/PlanMemory 源码；禁止按 adapter
  名称、SSA 编号、buffer 数量或最终数值拟合。
- compiler 改动只允许只读 dump，不得修改真实 pass 逻辑。
- 无法证明 exact 时返回 blocker，不得用估计值冒充 exact。

## Pass 通路

当前主线按真实编译顺序组织：

```text
CVPipelining
  -> OneShotBufferize and post-bufferization normalization
  -> HIVMDecomposeOp
  -> InferHIVMMemScope / AlignStorage
  -> AllocExtraBuffer
  -> InlineLoadCopy
  -> MarkMultiBuffer
  -> PlanMemory
```

模型还包含该通路中会改变下游 UB 的前置变换，包括
`LoopInvariantCodeMotion`、`GlobalWorkspacePlan`、`CrossCoreGSS`、
`SplitMixKernel`、`TileAndBindSubBlock` 和必要 canonicalization。

## 当前验证结果

- 代码结构已收口为单一生产入口、显式 Pass pipeline 和按真实 Pass 命名的实现目录；
  重组后核心/开发工具可构建，4 组单测、HIVM registry 检查和 2565 tuple 固定 seed
  完整文本 plan 回归通过。
- `CVPipelining` 输出：171 inputs x 15 configs，共 2565/2565 boundary dump 和
  SemanticIR exact；按内容去重为 166 个输出对象。
- `OneShotBufferize` 输出：171 inputs x 15 CVPipelining configs，共 2565/2565
  operation correspondence、allocation 和 value/access root exact。
- `HIVMDecomposeOp`：166 个去重的 `CVPipelining` 输出，378 function pairs exact。
- `AllocExtraBuffer`：2732 个 UB buffer 投影 exact。
- `InlineLoadCopy`：388 function pairs 和对应 buffer rewrite exact。
- `MarkMultiBuffer`：166 objects x 7 configs，共 1162 tuples exact。
- `PlanMemory` 输入投影：1162 tuples，17836 buffers、23303 accesses exact。
- 完整 `PlanMemory` 输入笛卡尔积：171 inputs、15 CVPipelining configs、17 unique
  multi-buffer configs，共 43605/43605 canonical byte exact，0 mismatch、0 blocker；
  其中 2250 次真实 compiler 在该边界后按预期报告 UB/CBUF overflow。
- `PlanMemory`：1692 个可复用 input/config/seed tuples 的 lifetime 与 plan canonical
  TSV byte exact；包含 20 seeds、两种 `restrict-inplace-as-isa` 和真实 multi-buffer。
- `CVPipelining` 前入口到同一 suffix 入口的 seed 0、restrict=false 内部端到端比较
  为 2565/2565 exact。

## 关键实现事实

- 生产 CLI 只保留 `before CVPipelining -> PlanMemory -> overflow/plan` 主路径；旧的
  lifetime dump、liveness dump 和 action 分发只保留在开发验证器中。替代输入边界
  必须显式启用 `--debug --debug-entry=...`，普通模式不会执行或接受它们。
- debug 摘要按真实 Pass 名写入 stderr；只有指定 `--debug-dir` 才惰性序列化并保存
  完整阶段快照。非 debug 模式不创建 trace 对象、不执行快照序列化，stderr 保持静默，
  开启 debug 前后的 stdout 结果必须逐字节一致。
- 三个输入边界都必须是实际普通文件；缺失或目录输入在解析前 fail-fast，不能把空
  module/plan 当成 exact。聚合测试对缺少的生成式 compiler oracle 明确报告 `SKIP`，
  不允许以 0 个对象的 PASS 代替验证。
- softmax 的首个真实差异来自 `LoopInvariantCodeMotion`；模型按 def-use 与 effect
  条件提升循环不变量。
- sparse-attention 的最后一批差异来自
  `InsertSliceBubbleUpStrategy::handleExtractInsertExtractCase` 和后续
  `ConvertToHIVMOpPass::applyPatternsGreedily`。模型按真实 slice 关系重写，从
  `scf.for` 静态 lb/ub/step 构造 domain，只在 domain 上恒定时折叠 affine 表达式。
- overflow 仍完成 fallback placement。`peakBits` 使用 byte-aligned 原始 extent；
  `requiredBits` 使用 `StorageEntry` 对齐后的容量需求，overflow 退出码为 2。
- 未建模 generic operation 必须 fail-fast；语义初始化必须幂等。

## 剩余边界

- canonical PlanMemory 输入使用 `b0/b1/...` 规范化 allocation identity，因此验证
  身份对应关系、operation order 和全部 UB 信息，但不验证 MLIR printer 的展示名。
- `LoopInvariantSubsetHoisting` 尚无独立轻量实现；当前矩阵未观察到它造成 UB 差异。
- `scf.while` 只有结构单测；动态 stride-aligned allocation 的最终 rank-reduced
  subview 仍应返回 blocker。
- exact 声明只适用于已记录的数据和配置域。新增差异必须先定位第一个真实 pass
  边界，再对照 BiSheng 源码实现。
- `all_configs.tsv` 展开为每个输入 5760 次 PlanMemory 运行，171 个输入共 984960
  次；该近百万次最终 lifetime/offset/peak 矩阵尚未全部保存和验证，不能用 43605
  个 PlanMemory 输入 exact 替代这一结论。

## 验收规则

- pass 输入/输出：规范化后信息逐字节一致，不丢 buffer identity 或 operation order。
- PlanMemory：固定 input/config/seed 后，lifetime 与 plan canonical TSV 逐字节一致。
- 端到端：比较 status、selected seed、peak、required/capacity，以及每个 buffer 的
  extent、lifetime 和 offset。
- 测试默认只报告总量；失败时再按 adapter、config 和真实 pass 分类。
