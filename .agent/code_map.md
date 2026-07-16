# Code Map

## 真实 Pipeline

```text
bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp
bishengir/lib/Dialect/HFusion/Pipelines/
bishengir/lib/Dialect/HIVM/Pipelines/
bishengir/tools/bishengir-cvpipeline-suffix-compile/
build/bin/bishengir-cvpipeline-suffix-compile
```

需要 pass 顺序时直接读取 pipeline 源码。compiler 中的改动只用于 dump。

## 真实 Pass 与模型

```text
BiSheng source                                            Lightweight model
bishengir/.../Transforms/CVPipelining.cpp                  src/passes/cvpipelining/
MLIR OneShotBufferize + post-bufferization passes          src/passes/one_shot_bufferize.hpp
bishengir/.../Transforms/HIVMDecomposeOp.cpp                src/passes/hivm_decompose_op.hpp
bishengir/.../Transforms/InferHIVMMemScope.cpp              src/passes/infer_hivm_mem_scope.hpp
bishengir/.../Transforms/AlignStorage.cpp                   src/passes/align_storage.hpp
bishengir/.../Transforms/AllocExtraBuffer.cpp               src/passes/alloc_extra_buffer.hpp
bishengir/.../Transforms/InlineLoadCopy.cpp                 src/passes/inline_load_copy.hpp
bishengir/.../Transforms/MarkMultiBuffer.cpp                src/passes/mark_multi_buffer.hpp
bishengir/.../Transforms/PlanMemory.cpp                     src/passes/plan_memory/
```

所有 UB 相关变换位于 `src/passes/`，文件名直接使用真实 Pass 名。生产主线组装在
`src/pipeline/cvpipelining_ub_pipeline.hpp`；各 Pass 边界状态和 `PlanMemory` 输入构建
也位于 `src/pipeline/`，不会伪装成 Pass。

## 入口

```text
src/main.cpp                           唯一生产入口
src/tools/dev_validate.cpp             开发 oracle/dump 入口
src/pipeline/cvpipelining_ub_pipeline.hpp 真实 Pass 调用顺序
scripts/plan_before_cvpipelining_ub.py
run_demo_ub_plan.sh
```

生产入口：

```bash
./cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --before-cvpipelining-ir=<generic-before-cvpipelining.mlir> \
  --cv-pipeline-depth=-1 \
  --format=json
```

生产模式只有 `before CVPipelining -> PlanMemory` 一条入口。开发时可加 `--debug`
在 stderr 输出真实 Pass 名和关键计数；再加 `--debug-dir=<dir>` 保存规范化阶段快照。
`after-cvpipelining` 与 `before-plan-memory` 输入边界只允许和 `--debug-entry`、
`--debug` 一起使用，不进入普通产品路径。

## 数据

```text
cvpipeline_ub_model_cpp/data/adapter/
cvpipeline_ub_model_cpp/data/before_cvpipelining/
cvpipeline_ub_model_cpp/testdata/plan_memory/
Output/adapters/<adapter>/<pass>/
Output/index/<pass-or-boundary>/
```

`data/` 和 `testdata/` 是稳定输入；`Output/` 是可重新生成的 oracle、日志和 pass dump。

## 验证命令

```bash
bash cvpipeline_ub_model_cpp/scripts/build_dev_tools.sh
bash cvpipeline_ub_model_cpp/scripts/run_unit_tests.sh
python3 cvpipeline_ub_model_cpp/scripts/validate_config_test_matrix.py
python3 cvpipeline_ub_model_cpp/scripts/validate_one_shot_bufferize_output.py --max-inputs=0 --jobs=8
python3 cvpipeline_ub_model_cpp/scripts/validate_hivm_decompose_op.py
python3 cvpipeline_ub_model_cpp/scripts/validate_alloc_extra_buffer.py
python3 cvpipeline_ub_model_cpp/scripts/validate_inline_load_copy_buffer_projection.py
python3 cvpipeline_ub_model_cpp/scripts/validate_mark_multi_buffer.py
python3 cvpipeline_ub_model_cpp/scripts/validate_plan_memory_input.py --with-accesses
python3 cvpipeline_ub_model_cpp/scripts/validate_plan_memory_input_matrix.py --axis-product --jobs=8
python3 cvpipeline_ub_model_cpp/scripts/validate_before_cvpipelining_end_to_end.py \
  --seeds 0 --restrict-modes 0 --compare-text-plan
```

构建真实 suffix compiler：

```bash
cmake --build build --target bishengir-cvpipeline-suffix-compile -j8
```
