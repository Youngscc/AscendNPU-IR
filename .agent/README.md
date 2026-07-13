# CVPipeline UB Model Memory

只保存当前可执行事实，不记录开发流水账。接手任务先读本文件；定位源码再读
`code_map.md`。不要保存未授权的私有 IR。

## 目标与约束

```text
before-CVPipeline generic IR + CVPipeline/suffix/PlanMemory config
  -> lightweight C++ model
  -> exact absolute local UB peak / exact overflow required_bits
```

- Oracle：`build/bin/bishengir-cvpipeline-suffix-compile` 的 minimal suffix。
- 核心实现使用独立 C++；Python 仅用于生成数据和比较。
- 只保留 UB 相关语义，但规则必须来自真实 pass/PlanMemory，禁止按 oracle
  数值拟合。
- 真实 suffix pass 逻辑不可为测试修改；compiler 改动只允许只读 dump。
- 无法证明 exact 时返回 blocker，不能输出估计值冒充 exact。

## 当前状态

- PlanMemory-local model 完成：PlanMemory-before IR -> lifetime -> plan/offset/peak。
  8240 tuples 的 lifetime 与 plan canonical TSV byte exact；比较器 mutation test
  有效。
- Modeled suffix 已连接：before-OneShotBufferize generic IR ->
  PlanMemory-before structured input，覆盖 OneShotBufferize、
  post-bufferization rewrite、InferHIVMMemScope、AlignStorage、
  AllocExtraBuffer、InlineLoadCopy、MarkMultiBuffer 和 PlanMemoryInputBridge。
- PlanMemory-input bridge：1162 strategy tuples，17836 AIV/UB buffers、
  23303 accesses byte exact；166 unique IR 中 165 byte exact。唯一动态
  stride-aligned allocation 在生产入口显式 blocker。
- CVPipelining model 已接入生产 CLI：`--action=plan-before-cvpipeline` 从
  before-CVPipeline generic IR 直接输出 UB plan/peak。
- Demo 入口 `cvpipeline_ub_model_cpp/scripts/run_demo_ub_plan.sh` 已同时运行
  轻量模型和 `bishengir-cvpipeline-suffix-compile` oracle，并比较 UB peak 与
  `(extent_bits, offset_bytes)` buffer placement；`--help` 可编辑 CVPipeline、
  suffix、PlanMemory 参数。
- 端到端当前证据：171 snapshots x 15 configs = 2565 tuples，生产
  CVPipelining+suffix+PlanMemory 与保存的 after-CVPipeline suffix oracle 在
  seed=0、restrict=false 上 JSON 摘要和文本 plan buffer 表一致；额外
  20 cases x 20 seeds x 2 restrict modes = 800 tuples 也一致。

raw SemanticIR diff 仍可能包含 effects/properties 和 SSA 编号顺序差异；验收应使用
规范化信息等价比较。

## 已建模的关键 pass

- `CVPipelining.cpp`：普通路径和 preload 路径的 workspace expansion、work item
  拆分、local tensor/memref output、workspace subview/collapse/to_tensor、
  preload scope/extract_slice、DPS init remap、workspace yield 用户和 atomic
  set/none/trailing 语义。
- Suffix：OneShotAnalysis/OneShotBufferize、HIVMOptSinglePoint、
  HIVMDecomposeOp、ConvertNonContiguousReshapeToCopy、InferHIVMMemScope、
  AlignAllocSize/MarkStrideAlign/EnableStrideAlign、FlattenOps/
  ReduceRankSubview/LiftLowestStride、AllocExtraBuffer、InlineLoadCopy、
  MarkMultiBuffer、PlanMemoryInputBridge。
- PlanMemory：gen/kill、ordered live set、ignore-inplace、multi-buffer、
  shuffled planning attempts、offset/peak/overflow required bits。

## 剩余风险

- 动态 stride-aligned allocation 的最终 rank-reduced subview 物化仍是 blocker。
- 冻结 suffix 的正式逐 pass 编译器消融证据尚未全部保存；当前以
  suffix-compile oracle 和 normalized byte comparison 作为主证据。
- 开发验证脚本仍保留少量历史文件名/action 名称；生产入口和核心 C++ 类型/函数按
  pass/边界命名。

## 验收

- PlanMemory-input bridge 直接比较 model 与真实 PlanMemory-before canonical
  input；不含 seed。
- PlanMemory-local 再比较 lifetime 与 plan canonical TSV；固定
  input/config/seed 必须逐字节一致。
- 生产端到端比较 JSON 摘要与文本 plan buffer 表；blocker 不得输出
  `exact_plan`。
- 最终输出含 `ub_peak_bits`、`capacity_bits`、`overflow`、overflow 时的
  `required_bits`。

## 维护

- 本文件只更新当前数字、剩余问题和决策；历史过程不写入。
- `code_map.md` 只放入口与命令。两文件总量尽量低于 150 行。
- 测试默认报告总量；仅失败时分类。
