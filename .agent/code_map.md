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
createHIVMAggregatedDecomposeOpPass(AFTER_LIFT_LOWEST_STRIDE)
createAllocExtraBufferPass
createInferHIVMMemScopePass
canonicalizationHIVMPipeline
createInlineLoadCopyPass
createMarkMultiBufferPass
createDumpIRBeforePlanMemoryPass
createPlanMemoryPass(default LOCAL_MEM_PLAN)
```

第一版 exact oracle 以 `createMarkMultiBufferPass` 后、
`createDumpIRBeforePlanMemoryPass`/`createPlanMemoryPass` 前的 IR 为起点。继续往前
移动 checkpoint 时，上面这段 suffix 中的 pass 都要按新增建模对象处理，不能假设
它们不影响峰值。

完整阶段划分见 [pass_memory_modeling_guide.md](pass_memory_modeling_guide.md)。
从输入到第二次 PlanMemory 的展开 pass 顺序见
[pass_sequence_to_second_planmemory.md](pass_sequence_to_second_planmemory.md)。
细粒度 pass 顺序和每个 pass 的 pipeline 代码位置见
[pass_order_code_locations.md](pass_order_code_locations.md)。

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

## Lightweight Python memory modeling

```text
memory_modeling/
memory_modeling/core.py
memory_modeling/checkpoints.py
memory_modeling/scanner.py
memory_modeling/run_adapters.py
memory_modeling/cli.py
memory_modeling/s0_snapshot.py
```

当前轻量模块先用 Python 搭建逐 pass 建模框架：

```text
core.py         Precision/MemoryState/BufferRecord/PassEffect/PlanPeak/Checkpoint
checkpoints.py 反向推进 checkpoint 注册表，当前覆盖 S0 和 S8.5 -> S8.0
scanner.py     text-based MLIR structural scan，先去掉行注释再统计
s8_local_model.py
                S8.5 第一版文本模型：解析 MarkMultiBuffer 后 IR 的 local
                memref.alloc、scope、静态 size、MultiBufferAttr、简化 lifetime，
                计算 per-function/per-scope 保守 interval-reuse peak
run_adapters.py
                批量验证入口：扫描 data/ 下所有非隐藏普通文件，逐个运行 S8.5
                Python 模型和 bishengir-opt local PlanMemory + MemoryDisplay oracle，
                输出 UB 对比优先的 comparison.json/comparison.csv；每个 case 保留
                memory.json、model_memory_info.json、suffix_input.mlir、
                after_plan.mlir、memory_info*.json 和必要时的 oracle.log
cli.py         通用 CLI，可列 checkpoint 或扫描输入
s0_snapshot.py 兼容旧 S0 入口，内部转调 scanner/create_report
```

它只输出 `symbolic` / `concrete_structural` 信息，或承载外部 PlanMemory oracle
填入的 `exact_plan` 结果；不替代 MLIR parser 或 PlanMemory。
后续需要 MLIR Operation walk、`MemLivenessAnalysis`、`MemPlan` 或 pass instrumentation
时，再迁移或补充 C++ 实现。

运行示例：

```bash
python3 -m memory_modeling.s0_snapshot input.mlir --pretty
python3 -m memory_modeling.cli --reverse-plan --pretty
python3 -m memory_modeling.cli input.mlir --checkpoint S8.5 --pretty
python3 -m memory_modeling.run_adapters --data-dir data --pretty
```

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

批量跑 `data/` 下 adapter 并比较 Python 模型与真实 PlanMemory oracle：

```bash
python3 -m memory_modeling.run_adapters \
  --data-dir data \
  --output-root Output/memory_modeling_adapters \
  --pretty
```

默认 run name 是时间戳。顶层 `comparison.csv` 是主要查看入口，字段集中在
`ub_model_peak_bits`、`ub_ascendir_peak_bits_from_max_addr`、
`ub_delta_model_minus_ascendir_bits`、`ub_overflow_required_bits` 和
`ub_capacity_bits`。每个输入文件一个子目录，包含 `memory.json`、
`model_memory_info.json`、`suffix_input.mlir`、成功时的 `after_plan.mlir`、原始
`memory_info*.json`，以及 oracle 有输出时的 `oracle.log`。
`model_memory_info.json` 是 Python 模型侧逐 buffer 视图，包含 model offset、
extent、end address、lifetime、size/aligned size 和 multi-buffer copy index。

如果需要完整 pipeline 中 local PlanMemory 前 IR，当前代码支持：

```text
BISHENGIR_DUMP_BEFORE_PLAN_MEMORY=/tmp/before_plan_memory.mlir
```

该环境变量控制的 debug pass 位于 `MarkMultiBuffer` 和 local `PlanMemory` 之间。

根目录本地辅助脚本 `run.sh` 支持 `bash run.sh [options] [input.mlir]`，
也支持 `--input FILE` / `--output FILE`。`--dump` / `--no-dump` 或环境变量
`PRINT_INTERMEDIATE_DUMPS=0|1` 用于控制是否开启 `--mlir-print-ir-after-all`、
IR tree dump、local PlanMemory 前 dump 和 coarse dump 汇总。
