# Code Map

## Pipeline 入口

```text
bishengir/tools/bishengir-compile/bishengir-compile.cpp
bishengir/lib/Tools/bishengir-compile/BiShengIRCompileMain.cpp
bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp
```

主流程：

```text
buildBiShengHIRPipeline
  CanonicalizeModule
  AppendDeviceSpec
  HFusion pipeline
  ConvertToHIVM pipeline
  OptimizeHIVM pipeline
```

## HFusion / Convert / HIVM pipeline

```text
bishengir/lib/Dialect/HFusion/Pipelines/HFusionPipelines.cpp
bishengir/lib/Dialect/HIVM/Pipelines/ConvertToHIVMPipeline.cpp
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
```

local PlanMemory 前最关键片段在 `HIVMPipelines.cpp`：

```text
createAllocExtraBufferPass
createInferHIVMMemScopePass
canonicalizationHIVMPipeline
createInlineLoadCopyPass
createMarkMultiBufferPass
createPlanMemoryPass(default LOCAL_MEM_PLAN)
```

完整阶段划分见 [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md)。
从输入到第二次 PlanMemory 的展开 pass 顺序见
[pass_sequence_to_second_planmemory.md](pass_sequence_to_second_planmemory.md)。

## PlanMemory

```text
bishengir/lib/Dialect/HIVM/Transforms/PlanMemory.cpp
bishengir/include/bishengir/Dialect/HIVM/Transforms/PlanMemory.h
bishengir/include/bishengir/Dialect/HIVM/Transforms/Passes.td
```

关键类/函数：

```text
MemLivenessAnalysis::build
MemLivenessAnalysis::GetBufferInfo
MemPlan::plan
MemPlan::PlanLocalMemAddress
MemPlan::PlanMemAddressOfWholeLocalBuffer
MemPlan::EmitPlanMemoryFailureInfo
PlanMemoryPass::runOnOperation
```

PlanMemory local 容量常量：

```text
UB:  32 bytes align, 192 KiB
L1:  32 bytes align, 512 KiB
L0C: 512 bytes align, 128 KiB
```

`requires N bits` 来自失败后按 PlanMemory 规则重新规划失败 scope 得到的
`max(bitsOffset + alignedConstBits)`，不是裸 buffer size 求和。

## Dry-run estimator / oracle

当前代码已回退 `hivm-estimate-memory` 实现；后续如果重新实现，应放在
PlanMemory-ready checkpoint，复用 `MemLivenessAnalysis` 和 `MemPlan`，但不做最终
IR rewrite，也不要输出会触发 fallback 的 overflow 文本。

## MemoryDisplay

```text
bishengir/lib/Dialect/HIVM/Transforms/MemoryDisplay.cpp
bishengir/include/bishengir/Dialect/HIVM/Transforms/MemoryDisplay.h
```

可用于交叉验证 buffer、offset、extent、lifetime、source location、tmpbuf 标记。
注意它仍在 PlanMemory 内部执行，不是前置模型本身。

## Debug / 复现命令

只验证 PlanMemory，不需要 CANN/hivmc/真实卡：

```bash
build/bin/bishengir-opt \
  bishengir/test/Dialect/HIVM/plan-memory.mlir \
  --split-input-file \
  --hivm-plan-memory='mem-plan-mode=local-mem-plan'
```

对 PlanMemory-ready IR 跑真实 local plan 和 MemoryDisplay：

```bash
build/bin/bishengir-opt input.mlir \
  --hivm-plan-memory='mem-plan-mode=local-mem-plan enable-memory-display=true'
```

如果需要完整 pipeline 中 local PlanMemory 前 IR，当前代码支持：

```text
BISHENGIR_DUMP_BEFORE_PLAN_MEMORY=/tmp/before_plan_memory.mlir
```

该环境变量控制的 debug pass 位于 `MarkMultiBuffer` 和 local `PlanMemory` 之间。

根目录本地辅助脚本 `run.sh` 支持 `bash run.sh [options] [input.mlir]`，
也支持 `--input FILE` / `--output FILE`。`--dump` / `--no-dump` 或环境变量
`PRINT_INTERMEDIATE_DUMPS=0|1` 用于控制是否开启 `--mlir-print-ir-after-all`、
IR tree dump、local PlanMemory 前 dump 和 coarse dump 汇总。
