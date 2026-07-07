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

当前 dump/阅读任务以 `createDumpIRBeforePlanMemoryPass` 前后的 local PlanMemory
输入/输出为主。`createMarkMultiBufferPass` 后的 IR 最接近 local PlanMemory 输入。
`createDumpIRBeforePlanMemoryPass` 现在受 `--enable-dump-ir-before-plan-memory` 控制；
普通编译默认不会插入这个 debug pass。

从输入到第二次 PlanMemory 的展开 pass 顺序见
[pass_sequence_to_second_planmemory.md](pass_sequence_to_second_planmemory.md)。
细粒度 pass 顺序和每个 pass 的 pipeline 代码位置见
[pass_order_code_locations.md](pass_order_code_locations.md)。
已暂停的逐 pass estimator 方案见
[pass_memory_modeling_guide.md](pass_memory_modeling_guide.md)，不要作为当前默认入口。

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

修改 compile option 或 pipeline 后，用户自行 build：

```bash
cmake --build build --target bishengir-compile
```

本机 shell 中曾出现 `ninja: command not found`，因此优先使用 `cmake --build`。

如果需要完整 pipeline 中 local PlanMemory 前 IR，当前代码支持：

```bash
BISHENGIR_DUMP_BEFORE_PLAN_MEMORY=/tmp/before_plan_memory.mlir \
build/bin/bishengir-compile input.ttadapter \
  --enable-hfusion-compile \
  --enable-hivm-compile \
  --enable-triton-kernel-compile \
  --enable-dump-ir-before-plan-memory
```

`--enable-dump-ir-before-plan-memory` 决定是否插入 debug pass；环境变量决定输出路径。
两者都存在时才写出 before dump。该 pass 位于 `MarkMultiBuffer` 和 local `PlanMemory`
之间。

批量观察 `data/` 下每个 IR 的 local PlanMemory before、memory info 和日志可用：

```bash
bash tools/dump_planmemory_ir.sh \
  --data-dir data \
  --output-root Output/planmemory_ir_dumps \
  --allow-failures
```

默认每个 case 输出 `before_local_plan_memory.mlir`、对应 `.raw.mlir` 原始打印、
`memory_info*.json`、编译 stdout/stderr 和顶层 `summary.tsv`。脚本在每个 case 输出目录中运行
`bishengir-compile`，因此 `MemoryDisplay.cpp` 固定写出的 `./memory_info.json` /
`./memory_info_aic.json` / `./memory_info_aiv.json` 会被保留在对应 case 目录；顶层
`summary.tsv` 的 `memory_info_files` 列列出这些文件。默认不再创建 full pass
`ir_tree/`；需要全量 pass IR tree 时加 `--dump-full-ir-tree`，需要只把它作为临时
提取源并在结束后删除时加 `--drop-full-tree`。

`after_local_plan_memory.mlir`、`after_local_plan_memory_<entry_func>.mlir` 和
`plan_memory_after_dumps/` 依赖 full IR tree；只有使用 `--dump-full-ir-tree` 或
`--drop-full-tree` 时才会生成。主 after 快照优先选择 local entry 函数目录下最后一个
`*_hivm-plan-memory.mlir`；如果 before IR 中有多个 `hacc.entry` 函数，例如 mix AIC/AIV，
会额外输出每个 entry 对应的
`after_local_plan_memory_<entry_func>.mlir`，避免误选 `infer_task_type_function`
或较早的 workspace/global PlanMemory dump。默认会做轻量 IR pretty formatting（拆 module/function attributes、函数参数、HIVM
`ins/outs` 和长 operand 列表），原始输出保存在 `.raw.mlir`；如需完全原样输出可加
`--raw`。`--allow-failures` 适用于缺少 `hivmc` 或 PlanMemory 后续阶段失败但 dump
已生成的本地调试环境。

常用 dump 命令速查：

```bash
# 单个输入：只保留 before + memory_info + logs。
bash tools/dump_planmemory_ir.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/planmemory_ir_dumps \
  --allow-failures

# 单个输入：额外抽取 after_local_plan_memory*.mlir，但删除 full ir_tree。
bash tools/dump_planmemory_ir.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/planmemory_ir_dumps \
  --drop-full-tree \
  --allow-failures

# 单个输入：保留 full ir_tree，用于逐 pass IR 观察。
bash tools/dump_planmemory_ir.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/planmemory_ir_dumps \
  --dump-full-ir-tree \
  --allow-failures

# 扫描整个 data 目录：默认只保留 before + memory_info + logs。
bash tools/dump_planmemory_ir.sh \
  --data-dir data \
  --output-root Output/planmemory_ir_dumps \
  --allow-failures

# 只处理排序后的前 N 个输入。
bash tools/dump_planmemory_ir.sh \
  --data-dir data \
  --max-files 3 \
  --output-root Output/planmemory_ir_dumps_smoke \
  --allow-failures

# 检查脚本语法。
bash -n tools/dump_planmemory_ir.sh
```

旧根目录脚本已整理到 `tools/`：

```bash
# 旧 main.sh：运行 Python memory_modeling adapter 验证；当前主线默认不用。
bash tools/run_memory_modeling_adapters.sh

# 旧 run.sh：编译单个 IR，并从 full pass dump 中整理粗粒度阶段快照。
bash tools/dump_coarse_compile_ir.sh \
  --input data/attn_fwd.ttadapter

# 旧 run_triton.sh：跑保存的 Triton Python stage，让 Triton 把 IR dump 到 data/。
bash tools/dump_triton_stage_ir.sh

# 手写 fake/attn_fwd bishengir-compile smoke 命令的脚本化入口。
bash tools/compile_ttadapter_memory_display.sh \
  --input data/attn_fwd.ttadapter

bash tools/compile_ttadapter_memory_display.sh --fake
```

第一阶段 UB peak pass-group 消融可用：

```bash
bash tools/run_ub_peak_pass_ablation.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/ub_peak_pass_ablation
```

该 runner 调用 `tools/dump_planmemory_ir.sh`，按实验配置分别输出 local PlanMemory
before、`memory_info*.json`、stdout/stderr，并在 run root 下生成 `summary.tsv` 和
`memory_metrics.tsv`。默认 target 是 `Ascend910_9382`，默认不创建完整 IR tree。若需要
同时保留 after PlanMemory 快照和 full pass IR tree，可给 runner 加 `--keep-full-tree`。
后续可把 `--input` 换成 `--data-dir data` 批量扫目录。

常用消融实验命令速查：

```bash
# 对 attn_fwd 跑第一阶段 pass-group 消融。
bash tools/run_ub_peak_pass_ablation.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/ub_peak_pass_ablation

# 对 data/ 下所有输入跑同一套消融矩阵。
bash tools/run_ub_peak_pass_ablation.sh \
  --data-dir data \
  --output-root Output/ub_peak_pass_ablation

# 消融实验同时保留 full ir_tree。
bash tools/run_ub_peak_pass_ablation.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/ub_peak_pass_ablation \
  --keep-full-tree

# 固定 run 名称，便于覆盖/对比特定实验目录。
bash tools/run_ub_peak_pass_ablation.sh \
  --input data/attn_fwd.ttadapter \
  --output-root Output/ub_peak_pass_ablation \
  --run-name manual_check

# 检查脚本语法。
bash -n tools/run_ub_peak_pass_ablation.sh
```
