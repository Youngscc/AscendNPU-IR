# 当前工作流：嵌入式 UB 判定性能重构

## 工作原则

当前唯一开发目标是降低 embedded lightweight model 的产品耗时。允许依赖 LLVM/MLIR，
但优化对象仍是轻量判定器，不能把它替换成克隆 Module 后运行完整原生 pass pipeline。

用户要求分析时只做只读调研；用户要求实现时，按下面阶段推进。每个阶段可以建立本地
checkpoint，但只有语义和验证完整后才整理为产品提交。

## 阶段 0：保持可测基线

- 构建必须确认 Release `-O3 -DNDEBUG`，并等待进程真实退出。
- 保存当前 160-input retry-only 基线和代表 kernel 分阶段数据。
- 正确性使用 embedded 原生 PlanMemory；性能使用 production retry-only。
- 正确性验证始终现场执行 seeds 0～19；即使提前证明 non-overflow，也继续完整模型和原生
  PlanMemory，禁止用缓存替代最终方案对比。
- 新计时默认不加入 dump、validation 或 artifact 成本。

## 阶段 1：exact decision fast path

状态：2026-07-28 已实现并完成代表正确性与性能测量。后续优化必须保留这里的证明合同和
observe-only 验证路径。

在 `AfterMarkMultiBuffer` 后增加保守的独立分配上界：

```text
sum(AlignUp(buffer.constBits, 256) * multiBufferNum) <= capacity
```

满足时只能证明 `overflow=false`；不要伪造完整 buffer plan。实现时：

- production request 使用 decision-only 路径，命中后可提前结束轻量模型自身的后续计算；
- debug/fixed-seed/full-plan validation 同样执行提前判定并输出 observe-only 结果，但强制
  fall through 到轻量模型完整 plan，再继续原生 PlanMemory；
- incomplete size/owner/address-space 信息一律 fall through；
- 保持当前完整 PlanMemory 实现不变，作为 slow path 和 oracle。

阶段完成条件：单元证明、代表 embedded parity、全场景 decision parity、命中率和 A/B
性能报告。

## 阶段 2：删除生产文本边界

新增 BiSheng/MLIR adapter，直接从当前 `ModuleOp` 构造模型输入：

- 读取 `Operation`、`Value`、`Type`、`Attribute`、Region/Block 顺序；
- 不修改原始 IR，不额外 clone 完整 Module；
- standalone Generic MLIR 文本 API 保留为兼容入口；
- 两种入口必须生成同一模型语义和确定顺序。

不要只把 MLIR 转成旧的全字符串 `GenericOperation` 后就结束；adapter API 应能承接后续
stable-ID/typed IR，避免二次重写。

## 阶段 3：stable-ID shadow IR

目标结构：

```text
base Operation* + stable OpId/ValueId
override bitmask + synthetic node arena
explicit ordered block lists
projection active BitVector
revisioned analysis manager
```

要求：

- 所有 mutation 经过统一 rewriter，精确失效 topology/uses/types/attrs 分析；
- 原始 Type/Attribute/OperationName 使用 MLIRContext 句柄；
- `DenseMap`/`SmallPtrSet` 不得提供语义遍历顺序；
- 删除操作使用 active bit/tombstone，避免每个 pass 全量 compaction；
- 只在明确边界生成一次确定性序号。

优先迁移 ParseGenericIR、canonicalization、MarkRealCoreType 和 AIC/AIV projection，因为它们
能同时验证 direct input、analysis invalidation 和 projection overlay。

## 阶段 4：统一 Buffer/Plan IR

把以下嵌套状态合并为单一 `UBBufferProgram`：

```text
BufferizedSemanticIR
PostBufferizationRewriteState
AfterAllocExtraBufferState
AfterInlineLoadCopyState
AfterMarkMultiBufferState
PlanMemoryInputSemanticIR
```

正常路径直接产生 typed `PlanProgram`：

- BufferId/ValueId/OperationId 均为整数；
- op kind、effects、pipe、broadcast/transpose/reduce/cum dims、stride align、multi-buffer 等均为
  typed 字段；
- 不生成或读取 `OperationRecord::text`；
- 文本只在 debug serializer 和兼容 file parser 中存在。

`AlignStorageAndAllocExtraBuffer` 应改为 typed query，避免逐 operation 复制完整
`GenericOperation` 并重新解析/格式化类型。

## 阶段 5：PlanMemory 实现优化

保持原生算法、顺序和布尔结论，优化基础设施：

- string buffer/value identity 改为整数 ID；
- seed-independent alias/CFG/event program 在 retry 外构建；
- 每个 seed 只重放 seed-sensitive kill 顺序和计划状态；
- storage conflict 使用紧凑矩阵/BitVector；
- outline 从分散 list node 改为 index-based arena，保持 splice/rollback 顺序；
- scratch capacity 在一个 evaluation 的 seed 间复用。

默认优先单轮模型吞吐量，保持串行 retry。只有明确需要单 kernel 低延迟时，才评估 seed 0
失败后并行 seed 1～19；并行模式必须选择与串行相同的 overflow 布尔值，并显式控制外层
调用并发以避免过度订阅。

## 阶段 6：跨 fallback 和构建优化

- RetriablePassManager 的 UB fallback 通常只改变 code motion；可缓存分叉前的无指针紧凑
  snapshot。
- cache key 必须覆盖输入结构、所有分叉前有效参数和模型 build identity；不能保存上一个
  attempt 的 `Operation*`。
- 架构稳定后再测试 ThinLTO/Full LTO 和代表模型输入 corpus PGO。
- 不优先投入 `-fno-exceptions`、目标 CPU 特化或完整原生 pass clone。

## 每轮修改流程

1. 先用计时和源码确认热点，不为测试 corpus 写特例。
2. 明确哪些数据和顺序必须保持，写针对性回归。
3. 实现一项基础设施变化，避免同时混入 UB 逻辑调整。
4. 运行相关单测和代表 embedded 20-seed 现场对比；单 seed 只用于定位。
5. 比较同机 A/B 性能；收益不稳定或回退时先定位，不用更大矩阵掩盖。
6. 重大边界完成后运行 embedded 20-seed 全量。
7. 更新 `.agent` 的当前基线和已实现状态；删除被新事实覆盖的旧任务描述。

## Git 和临时产物

- 半成品不必进入产品历史；需要保存现场时，在 `codex/` 分支做明确的本地
  `checkpoint(...)` 提交，不必 push。
- 产品提交按基础设施/功能/验证拆分，不能混入 cache、dump、Output、runtime TSV 或备份。
- 保留用户已有修改，禁止 destructive reset/checkout。
- 调试使用独立 `mktemp -d`；长验证结束前可保留本任务 tmp，收尾时只删除本任务产物。
- `.agent` 只保存当前目标、基线和长期约束；过时流水账使用 Git 历史追溯，不再长期保留。
