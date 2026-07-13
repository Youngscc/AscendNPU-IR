# Code Map

## Oracle 与 Pipeline

```text
bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp
bishengir/lib/Dialect/HFusion/Pipelines/
bishengir/lib/Dialect/HIVM/Pipelines/
bishengir/tools/bishengir-cvpipeline-suffix-compile/
build/bin/bishengir-cvpipeline-suffix-compile
```

需要 pass 顺序时直接读 pipeline 源码，不在记忆中复制。PlanMemory 前关键尾部为
`AllocExtraBuffer -> InferHIVMMemScope -> canonicalization -> InlineLoadCopy ->
MarkMultiBuffer -> PlanMemory(LOCAL)`。

## 真实语义

```text
bishengir/lib/Dialect/HIVM/Transforms/CVPipelining.cpp
bishengir/lib/Dialect/HIVM/Transforms/HIVMDecomposeOp.cpp
bishengir/lib/Dialect/HIVM/Transforms/FlattenOps.cpp
bishengir/lib/Dialect/HIVM/Transforms/ComposeCollapseExpand.cpp
bishengir/lib/Dialect/HIVM/Transforms/ConvertNonContiguousReshapeToCopy.cpp
bishengir/lib/Dialect/HIVM/Transforms/LiftLowestStride.cpp
bishengir/lib/Dialect/HIVM/Transforms/AllocExtraBuffer.cpp
bishengir/lib/Dialect/HIVM/Transforms/InlineLoadCopy.cpp
bishengir/lib/Dialect/HIVM/Transforms/MarkMultiBuffer.cpp
bishengir/lib/Dialect/HIVM/Transforms/PlanMemory.cpp
bishengir/include/bishengir/Dialect/HIVM/Transforms/PlanMemory.h
```

## 模型入口

```text
cvpipeline_ub_model_cpp/src/semantic_ir/
cvpipeline_ub_model_cpp/src/cvpipeline/cvpipelining.hpp
cvpipeline_ub_model_cpp/src/cvpipeline/cvpipelining_analysis.hpp
cvpipeline_ub_model_cpp/src/cvpipeline/cvpipelining_rewrite.hpp
cvpipeline_ub_model_cpp/src/cvpipeline/cvpipelining_preload_rewrite.hpp
cvpipeline_ub_model_cpp/src/cvpipeline/cvpipelining_pass.hpp
cvpipeline_ub_model_cpp/src/semantic_ir/generic_rewriter.hpp
cvpipeline_ub_model_cpp/src/suffix/planmemory_bridge.hpp
cvpipeline_ub_model_cpp/src/planmemory/{mem_liveness_analysis,storage_entry,mem_plan}.hpp
cvpipeline_ub_model_cpp/src/planmemory/planmemory_model.hpp
cvpipeline_ub_model_cpp/src/cli/cvpipeline_ub_model.cpp
cvpipeline_ub_model_cpp/src/cli/dev_validate.cpp
cvpipeline_ub_model_cpp/tests/
```

生产入口是 `cvpipeline_ub_model.cpp`；`dev_validate.cpp` 仅用于开发验证。

## 数据

```text
Output/adapters/<adapter>/<stage>/
Output/index/
Output/experiments/post_bufferization_pass_oracles/
Output/index/c6_pass_oracles/summary.tsv
```

## 常用命令

```bash
cd cvpipeline_ub_model_cpp
./scripts/build_dev_tools.sh
./scripts/run_unit_tests.sh
python3 scripts/validate_c7_planmemory_input.py --with-accesses
python3 scripts/validate_planmemory_bridge_input.py --one-config-per-ir --max-failures=100
python3 scripts/validate_d1_cvpipelining.py
python3 scripts/validate_d3_end_to_end.py --seeds 0 --restrict-modes 0 --compare-text-plan
```

指定 PlanMemory-input bridge case：追加 `--only-hash=<full-sha256>`。构建真实 suffix：

```bash
cmake --build build --target bishengir-cvpipeline-suffix-compile -j8
```

完整脚本说明以 `cvpipeline_ub_model_cpp/README.md` 为准。

生产端到端入口：

```bash
python3 scripts/plan_precvpipeline_ub.py \
  --pre-cvpipeline-ir=<generic-before-cvpipeline.mlir> \
  --cv-pipeline-depth=-1 \
  --random-seed=0 \
  --format=text

./output/bin/cvpipeline_ub_model \
  --action=plan-before-cvpipeline \
  --before-cvpipeline-ir=<generic-before-cvpipeline.mlir> \
  --cv-pipeline-depth=-1 \
  --random-seed=0 \
  --format=json
```
