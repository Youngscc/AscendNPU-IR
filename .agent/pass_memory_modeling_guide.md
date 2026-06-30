# Pass Memory Modeling Guide

日期：2026-06-30

## 输出精度

每个 pass/阶段都可以有一条记录，但精度要分级：

```text
exact_plan
  能调用 PlanMemory 或后续 dry-run oracle，输出真实规划级
  required/capacity/headroom/offset。

concrete_structural
  已有 memref/workspace alloc、静态 shape 或部分 scope 信息；可统计 raw/aligned size，
  但不能断言 PlanMemory 峰值。

symbolic
  仍在 tensor/HFusion/tiling 层；只记录 tile、fusion、logical buffer、潜在内存压力。

not_applicable
  pass 不直接影响内存行为。
```

## Pass 阶段划分

### S0 输入与配置快照

记录：

- IR 层级：tensor/TTIR、HFusion、HIVM tensor、HIVM memref、PlanMemory-ready。
- compile config：HFusion/HIVM/Triton、auto multi-buffer、code motion、workspace reuse、
  tile mix loop、block dim、tiling tuning 等。

精度：`symbolic` 或 `concrete_structural`。

### S1 HFusion 前处理和 outline

代表流程：

```text
canonicalize module
append device spec
HFusion preProcess / preFlatten / flattenAndFold / inferAndOutline
```

关注：

- tensor.empty、reshape、slice、flatten、fold。
- fusion/outline 改变 kernel 边界和未来 buffer ownership。

精度：`symbolic`。

### S2 HFusion autoschedule / tiling

代表流程：

```text
createHFusionAutoSchedulePass
hfusionTilingOptimizationPipeline
```

关注：

- tile shape、blockDim、cube tiling tuning、max buffer count tuning。
- 上游 schedule 如何造成后续 local buffer shape/并发变化。

精度：`symbolic`。

### S3 ConvertToHIVM

代表流程：

```text
createHFusionToHIVMConversionPass
createTritonGlobalKernelArgsToHIVMOpPass
createTensorToHIVMConversionPass
createConvertToHIVMOpPass
```

关注：

- HFusion/Tensor 到 HIVM op 的映射。
- Triton MacroInstr/CoreOp 路径差异。
- 如果已经出现 memref，开始建立 `BufferRecord`，但通常还不是 final scope。

精度：`symbolic` 或 `concrete_structural`。

### S4 HIVM pre-bufferization 和 global workspace plan

代表流程：

```text
normalize matmul/conv/fixpipe
insert load/store/workspace for mix CV
mark multi-buffer candidates
CV pipelining
tile cube/vector loop
inferAndSetBufferSizePipeline
createPlanMemoryPass(memMode = GLOBAL_WORKSPACE_PLAN)
code motion
```

关注：

- workspace 插入、workspace reuse、workspace multi-buffer。
- code motion 是否改变后续 lifetime。
- 这里的 PlanMemory 是 global workspace，不是 UB/L1/L0C local plan。

精度：workspace 可 `exact_plan`；local 通常仍是 `symbolic/concrete_structural`。

### S5 Bufferization

代表流程：

```text
OneShotBufferize
canonicalizationHIVMPipeline
ConvertToHIVMOp
DropEquivalentBufferResults
```

关注：

- tensor SSA 转 memref。
- `memref.alloc`、`memref.copy`、等价 buffer 删除。
- 建立核心 `BufferRecord` 和 value lineage。

精度：`concrete_structural`。

### S6 Post-bufferization 早期：copy/decompose/scope

代表流程：

```text
NonContiguousReshapeToCopy
InferHIVMMemScope
HIVMDecomposeOp
HIVMAggregatedDecomposeOp
RecognizeDeinterleave
```

关注：

- non-contiguous reshape 转 copy。
- 第一次 mem scope 推断。
- decompose 继续引入 copy/buffer。

精度：`concrete_structural`。

### S7 Layout/shape/size 稳定

代表流程：

```text
alignStoragePipeline
InferHIVMDataLayout
RemoveHIVMDataLayoutAnnotation
inferAndSetBufferSizePipeline
FlattenOps
ReduceRankSubview
LiftLowestStride
```

关注：

- align、stride、layout、rank、static size。
- 动态 shape 必须显式标记 unknown/upper-bound/symbolic，不能假装 exact。

精度：`concrete_structural`。

### S8 Local PlanMemory-ready

代表流程：

```text
AllocExtraBuffer
InferHIVMMemScope
canonicalizationHIVMPipeline
InlineLoadCopy
MarkMultiBuffer
<dry-run oracle / checkpoint>
PlanMemory(local)
```

关注：

- extra buffer 创建。
- 新 buffer 的 scope 推断。
- canonicalization/DSE 对 buffer 集合和 lifetime 的影响。
- InlineLoadCopy 改变 copy/load 形态。
- MarkMultiBuffer 决定 local multi-buffer 倍数。

这是第一优先落点。`MarkMultiBuffer` 后应使用真实 PlanMemory，或后续重新实现的
dry-run oracle，对模型做 exact 校验。

精度：`MarkMultiBuffer` 后为 `exact_plan`；更早子点多为 `concrete_structural`。

### S9 Local PlanMemory 后

记录真实 PlanMemory result：

- 20 seed retry 的结果。
- liveness、inplace、multi-buffer expansion、scope merge、spec allocation。
- 成功 offset 或失败 required bits。

精度：`exact_plan`。

## 每个 pass 的抽取模板

建模一个 pass 时回答：

```text
1. 是否创建/删除 tensor.empty、memref.alloc、memref_ext.alloc_workspace？
2. 是否改变 shape、element type、rank、stride、layout、align？
3. 是否改变 address space/scope？
4. 是否插入 copy、load、store、extra buffer、temporary buffer？
5. 是否移动 op，改变 alloc/use/free time？
6. 是否改变 function/kernel 边界？
7. 是否添加/移除 MultiBufferAttr？
8. 是否改变 inplace/reuse 条件？
9. 是否影响 workspace reuse 或 local reuse？
10. 当前点是否满足 PlanMemory 或 dry-run oracle 输入前提？
```

建议 effect 字段：

```text
created_buffers
deleted_buffers
resized_buffers
scope_changes
layout_changes
lifetime_changes
copy_insertions
extra_buffer_insertions
multi_buffer_changes
inplace_reuse_candidate_changes
workspace_changes
precision
blocking_dependency
```

## 推荐实施顺序

1. 先做 S8：`AllocExtraBuffer -> InferHIVMMemScope -> InlineLoadCopy -> MarkMultiBuffer`。
2. 用真实 PlanMemory 或后续 dry-run oracle 对齐 local UB/L1/L0C
   required/capacity/headroom。
3. 再向前覆盖 S5-S7，追踪 concrete memref 如何变成 PlanMemory-ready 输入。
4. 最后覆盖 S1-S4，解释 HFusion/tiling/config 如何导致后续内存压力。
5. 全链路 trace 中，每个 pass 都输出 effect；只有满足前提的点标为 `exact_plan`。
