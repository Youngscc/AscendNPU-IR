# Code Map

这里只列当前工作树中实际存在的文件和命令。

## 真实编译器

```text
bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
bishengir/tools/bishengir-cvpipeline-suffix-compile/
build/bin/bishengir-cvpipeline-suffix-compile
```

关键真实 pass：

```text
CVPipelining                 bishengir/lib/Dialect/HIVM/Transforms/CVPipelining.cpp
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

需要 pass 顺序或规则时直接读取以上源码。不要根据 oracle 数值猜实现。

## 轻量模型

```text
cvpipeline_ub_model_cpp/src/main.cpp
cvpipeline_ub_model_cpp/src/pipeline/cvpipelining_ub_pipeline.hpp
cvpipeline_ub_model_cpp/src/pipeline/plan_memory_input_semantic_ir.hpp
cvpipeline_ub_model_cpp/src/pipeline/plan_memory_input_builder.hpp
cvpipeline_ub_model_cpp/src/passes/
cvpipeline_ub_model_cpp/src/passes/plan_memory/
cvpipeline_ub_model_cpp/src/ir/
cvpipeline_ub_model_cpp/src/analysis/
cvpipeline_ub_model_cpp/src/support/
```

职责：

```text
src/main.cpp                         生产 CLI、fail-fast、结果格式
src/pipeline/cvpipelining_ub_pipeline.hpp
                                     真实顺序组装、边界状态、debug trace
src/passes/cvpipelining/             CVPipelining 分析和重写
src/passes/<pass>.hpp                对应真实 pass 的 UB 相关轻量语义
src/passes/plan_memory/              alias、liveness、StorageEntry、MemPlan、retry
src/pipeline/plan_memory_input_*     从后缀状态投影 PlanMemory 输入
src/ir/                              generic MLIR parser、SSA/region/block 表示和重写器
```

## 数据与产物

```text
cvpipeline_ub_model_cpp/data/adapter/                 原始 adapter
cvpipeline_ub_model_cpp/data/before_cvpipelining/     171 个当前 corpus MLIR
cvpipeline_ub_model_cpp/data/before_cvpipelining/manifest.tsv
cvpipeline_ub_model_cpp/examples/inputs/              小型演示输入
cvpipeline_ub_model_cpp/output/                       可重新生成，禁止提交
Output/                                               oracle/debug 产物，禁止提交
```

manifest 有 174 条 adapter 记录，但当前发现 171 个 `.mlir` 输入；测试规模按实际
discover 的 171 个 MLIR 计算。

## 当前可执行命令

构建模型：

```bash
bash cvpipeline_ub_model_cpp/build.sh
```

快速门禁：

```bash
bash cvpipeline_ub_model_cpp/tests/run_tests.sh
```

构建真实 suffix compiler：

```bash
cmake --build build --target bishengir-cvpipeline-suffix-compile -j8
```

默认全 corpus 差分：

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipelining \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seeds 0 --require-all-exact --quiet
```

增加 PlanMemory 维度：

```bash
# 20 个固定 seed + 默认 retry
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py <公共参数> \
  --seeds 0-19 --check-retry --quiet

# restrict inplace 路径
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py <公共参数> \
  --seeds 0-19 --restrict-inplace-as-isa --check-retry --quiet
```

增加当前已暴露的 pass 配置：

```text
--disable-cv-pipelining
--cv-pipeline-depth N
--enable-preload
--enable-cv-lazy-loading
--enable-code-motion true|false
--enable-triton-kernel-compile
--enable-auto-multi-buffer
--local-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
--mix-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
```

单输入 model + suffix + HTML：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=<input.mlir> \
  --skip-suffix-build
```

单独模型：

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --before-cvpipelining-ir=<input.mlir> \
  --format=json
```

pass 级定位：

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --before-cvpipelining-ir=<input.mlir> \
  --random-seed=0 --format=json \
  --debug --debug-dir=/tmp/cvub-debug

python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py <公共参数> \
  --seeds 0 --stage-snapshots-dir=/tmp/cvub-oracle-stages
```

## 比较合同

`scripts/compare_ub_plan_with_suffix_oracle.py` 和 `run_corpus_oracle.py` 比较：

```text
status / success / overflow
selected seed and retry result
peak_bits / required_bits / capacity
buffer extent and every offset
direct alloc/free lifetime
multi_buffer_num
applied inplace pair identity
```

函数名只做 `_mix_aiv/_mix_aic` 规范化；buffer 名通过语义 identity 对齐，不使用
printer 临时名字作为唯一正确性依据。
