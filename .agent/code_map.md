# Code Map

## PlanMemory 主实现

文件：

```text
bishengir/lib/Dialect/HIVM/Transforms/PlanMemory.cpp
bishengir/include/bishengir/Dialect/HIVM/Transforms/PlanMemory.h
```

关键点：

- `MemLivenessAnalysis::build()` 收集 liveness、buffer 信息、gen/kill 等。
- `MemPlan::plan()` 执行具体规划。
- `MemPlan::EmitPlanMemoryFailureInfo()` 发出 overflow 诊断。
- `PlanMemoryPass::runOnOperation()` 是 pass 入口。
- 当前实现会做最多 20 次 deterministic retry，尝试不同 buffer 顺序。

overflow 诊断大致格式：

```text
<scope> overflow, requires <bits> bits while <bits> bits available!
```

## PlanMemory 容量常量

在 `PlanMemory.cpp` 中硬编码：

```text
UB align:   32 bytes
UB size:    192 KiB
L1 align:   32 bytes
L1 size:    512 KiB
L0C align:  512 bytes
L0C size:   128 KiB
```

对应变量：

```text
ubAlignSize
ubSpaceSize
l1AlignSize
l1SpaceSize
l0cAlignSize
l0cSpaceSize
```

## Pass 定义

文件：

```text
bishengir/include/bishengir/Dialect/HIVM/Transforms/Passes.td
```

PlanMemory pass:

```text
hivm-plan-memory
```

重要选项：

```text
mem-plan-mode=local-mem-plan
mem-plan-mode=global-work-space-plan
enable-memory-display=true
enable-global-workspace-reuse=true
restrict-inplace-as-isa=true
```

## HIVM Pipeline 中的位置

文件：

```text
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
```

PlanMemory 跑两次：

1. Global workspace plan:

```text
inferAndSetBufferSizePipeline
createPlanMemoryPass(memMode = GLOBAL_WORKSPACE_PLAN)
```

2. Local memory plan:

```text
createAllocExtraBufferPass
createInferHIVMMemScopePass
canonicalizationHIVMPipeline
createInlineLoadCopyPass
createMarkMultiBufferPass
createPlanMemoryPass(default LOCAL_MEM_PLAN)
```

用户当前任务最重要的是第二次 local memory plan 前的状态。

## 编译选项和 fallback

文件：

```text
bishengir/include/bishengir/Tools/bishengir-compile/Options.td
bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp
bishengir/lib/Tools/bishengir-compile/BiShengIRCompileMain.cpp
bishengir/lib/Tools/RetriablePassManager/RetriablePassManager.cpp
bishengir/lib/Tools/RetriablePassManager/OverFlowPolicyBase.cpp
```

关键事实：

- `bishengir-compile` 会用 `RetriablePassManager` 包住 pipeline。
- 某些模式下遇到 overflow 会 retry。
- 现有 overflow retry policy 可能会关闭 `enable-code-motion` 或 `enable-auto-multi-buffer` 后重试。
- 用户想要在这个真实 overflow 发生前预测，避免进入 retry。

## MemoryDisplay

文件：

```text
bishengir/lib/Dialect/HIVM/Transforms/MemoryDisplay.cpp
bishengir/include/bishengir/Dialect/HIVM/Transforms/MemoryDisplay.h
```

用途：

- PlanMemory 成功或失败时，可以收集 buffer、offset、extent、lifetime、source location。
- `enable-memory-display=true` 时会生成 JSON。
- 默认输出名可能是 `memory_info.json`、`memory_info_aic.json`、`memory_info_aiv.json`。

这个机制适合借鉴，但不完全等同于用户要的前置估算器，因为它仍然在 PlanMemory 内部执行。
