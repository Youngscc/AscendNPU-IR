# 当前代码与命令

当前正确性路线是 embedded model—原生 BiSheng PlanMemory；cv2pm 仅是历史诊断工具。
命令均在仓库根目录
`/Users/YokeLove/huawei/AscendNPU-IR` 执行。

## 核心实现

### 真实 BiSheng 与 cv2pm

```text
bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp
bishengir/lib/Tools/bishengir-compile/BiShengIRCompileMain.cpp
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
bishengir/lib/Dialect/HIVM/Transforms/          真实 pass 实现
bishengir/tools/bishengir-cvpipeline-suffix-compile/
                                                  cv2pm 驱动当前所在目录
build/bin/bishengir-compile
build/bin/cv2pm-bishengir-compile
```

cv2pm 虽与历史 suffix 共用部分驱动源码，但由
`BISHENGIR_CV2PM_FULL_PIPELINE=1` 构建为独立目标。判断模型语义时只看 cv2pm 分支及其
调用的生产 pass，不能因为文件名仍含 suffix 就把历史 suffix 当成 oracle。

重点真实 pass：

```text
CVPipelining                 bishengir/lib/Dialect/HIVM/Transforms/CVPipelining.cpp
TileCubeVectorLoop           bishengir/lib/Dialect/HIVM/Transforms/TileCubeVectorLoop.cpp
SplitMixKernel               bishengir/lib/Dialect/HIVM/Transforms/SplitMixKernel.cpp
TileAndBindSubBlock          bishengir/lib/Dialect/HIVM/Transforms/TileAndBindSubBlock.cpp
CloneTensorEmpty             bishengir/lib/Dialect/HIVM/Transforms/CloneTensorEmpty.cpp
HIVMDecomposeOp              bishengir/lib/Dialect/HIVM/Transforms/HIVMDecomposeOp.cpp
InferHIVMMemScope            bishengir/lib/Dialect/HIVM/Transforms/InferHIVMMemScope.cpp
AlignAllocSize               bishengir/lib/Dialect/HIVM/Transforms/AlignBuffer/HIVMAlignAllocSize.cpp
MarkStrideAlign              bishengir/lib/Dialect/HIVM/Transforms/AlignBuffer/MarkStrideAlign.cpp
EnableStrideAlign            bishengir/lib/Dialect/HIVM/Transforms/AlignBuffer/EnableStrideAlign.cpp
AllocExtraBuffer             bishengir/lib/Dialect/HIVM/Transforms/AllocExtraBuffer.cpp
InlineLoadCopy               bishengir/lib/Dialect/HIVM/Transforms/InlineLoadCopy.cpp
MarkMultiBuffer              bishengir/lib/Dialect/HIVM/Transforms/MarkMultiBuffer.cpp
PlanMemory                   bishengir/lib/Dialect/HIVM/Transforms/PlanMemory.cpp
OneShotBufferize             third-party/llvm-project/mlir/lib/Dialect/Bufferization/Transforms/
LoopInvariantCodeMotion      third-party/llvm-project/mlir/lib/Transforms/LoopInvariantCodeMotion.cpp
```

### 轻量模型

```text
ub_overflow_model_cpp/include/ub_overflow_model/api.hpp
ub_overflow_model_cpp/src/main.cpp
ub_overflow_model_cpp/src/pipeline/cvpipelining_ub_pipeline.hpp
ub_overflow_model_cpp/src/pipeline/plan_memory_input_semantic_ir.hpp
ub_overflow_model_cpp/src/pipeline/plan_memory_input_builder.hpp
ub_overflow_model_cpp/src/passes/cross_core_gss.hpp
ub_overflow_model_cpp/src/passes/inject_block_sync.hpp
ub_overflow_model_cpp/src/passes/
ub_overflow_model_cpp/src/passes/plan_memory/
ub_overflow_model_cpp/src/ir/
ub_overflow_model_cpp/src/analysis/
ub_overflow_model_cpp/src/support/
```

生产可执行文件是：

```text
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model
```

`cvpipeline_ub_model` 目前只是指向该文件的兼容符号链接，不是另一个模型。

cross-core 同步分支的三个逐请求有效参数是：

```text
--enable-hivm-cross-core-gss
--enable-hivm-inject-block-all-sync
--disable-auto-inject-block-sync
```

原生条件只在 `cross-core-gss=true && block-all=false && disable-auto=false`
时走 `CrossCoreGSS`，其余组合走 `InjectBlockSync`。禁用自动注入时仍必须插入
`SetFFTSBaseAddr`。进程内 options 版本为 4，prediction pass 从真实
`HIVMPipelineOptions` 逐字段传入这三个值。

生产集成 pass 位于：

```text
bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp
bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.h
```

它紧邻真实 `createCVPipeliningPass` 之前执行，并与真实 pass 共用一个 resolved
`CVPipeliningOptions`。A2/A3 Triton membase 的 prediction 与 prune 默认开启；exact overflow
终止当前 attempt，并保留 BiSheng 原有 UB fallback 可识别的 `ub overflow` 诊断文本。普通
运行不输出模型中间记录；`BISHENGIR_UB_MODEL_EMIT_RESULT=1` 显式输出版本化机器记录，
`BISHENGIR_UB_FLOW_TRACE=1` 显式输出输入、effective options 和两个阶段标识。同进程
validation 自动输出机器记录。

同进程正确性验证入口：

```text
ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py
ub_overflow_model_cpp/tests/test_bisheng_embedded_matrix.py
```

它从 adapter 启动真实 `bishengir-compile`，让 embedded model 与主 pipeline 使用同一
固定 seed，并在真实本地 PlanMemory 后比较完整 UB 合同。

## 数据、场景与结果

```text
ub_overflow_model_cpp/data/adapter/                  163 个 adapter 源文件
ub_overflow_model_cpp/data/before_cvpipelining/      矩阵使用的 160 个基础输入
Output/before_cvpipelining_profiles/                 8 套生成 profile
ub_overflow_model_cpp/config/ub_relevant_parameter_scenarios.tsv
                                                     27 个场景定义（含 5 个 InjectBlockSync 场景）
ub_overflow_model_cpp/config/failure_taxonomy.tsv    稳定失败分类
ub_overflow_model_cpp/output/cv2pm_oracle_cache/     cv2pm schema 2 20-seed 缓存；当前为
                                                     4320/4320 条单一快照完整记录，
                                                     包含 23 条已缓存非 primary 合同
ub_overflow_model_cpp/output/cv2pm_model_validation.tsv
                                                     84852/84852 matched 模型对比报告
```

`Output/` 和 `ub_overflow_model_cpp/output/` 是可重新生成的本地产物，不应提交。
`data/adapter/` 中另有 3 个未进入当前 160-input 矩阵的定向输入：
`gather_out_to_ub`、`index_put`、`scatter_ub_to_out`。

## 常用命令

构建轻量模型：

```bash
bash ub_overflow_model_cpp/build.sh
```

构建 cv2pm：

```bash
cmake --build build --target cv2pm-bishengir-compile -j8
```

快速单元/集成测试：

```bash
bash ub_overflow_model_cpp/tests/run_tests.sh
```

单独运行模型，默认执行真实 retry：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --format=json
```

固定一个 seed 只用于定位某次 attempt：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --plan-memory-seed=5 \
  --format=json
```

单个原生 BiSheng 同进程对比：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --input ADAPTER.ttadapter \
  --seeds 0 \
  --jobs 1
```

全量 20-seed 正确性验证：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 \
  --jobs 12
```

历史 cv2pm 缓存生成（不再用于当前正确性测试）：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/build_cv2pm_oracle_cache.py \
  --jobs 12 \
  --pipeline-timeout 360 \
  --plan-timeout 120
```

只有需要重新计算现有 timeout/旧记录时才加 `--refresh`。

验证 cv2pm 与真实 BiSheng 前缀：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/compare_bisheng_cv2pm_matrix.py \
  --jobs 12 \
  --timeout 180 \
  --output-root /tmp/bisheng-cv2pm-compare
```

## 遗留命名

以下内容仍可能因兼容性或复用 parser 而存在，但不是当前验证入口：

```text
bishengir-cvpipeline-suffix-compile
scripts/run_corpus_oracle.py
scripts/compare_ub_plan_with_suffix_oracle.py
run_demo_ub_plan.sh 中的 suffix 路径
```

`run_corpus_matrix.py` 当前仍从 legacy-named comparator 导入解析函数。这只是代码复用，
它自身属于旧 cv2pm 缓存流程。当前入口 `run_bisheng_embedded_matrix.py` 也暂时复用同一
parser，但 oracle 是同进程真实 PlanMemory；后续可以重命名公共 parser 来消除歧义。
