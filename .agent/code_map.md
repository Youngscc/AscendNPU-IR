# 当前代码与命令

本文件只记录当前产品和新的性能目标。历史 suffix/cv2pm 路线不再展开；需要追溯时使用
Git 历史。

## 产品调用路径

```text
adapter
  -> bishengir-compile
       -> HIVM prefix
       -> UBOverflowPrediction module pass
            -> cvub::evaluate()
       -> real CVPipelining and HIVM pipeline when not pruned
       -> real local PlanMemory
```

生产接入：

```text
bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp
bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.h
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
bishengir/lib/Tools/RetriablePassManager/RetriablePassManager.cpp
bishengir/include/bishengir/Tools/RetriablePassManager/UbOverflowRetryPolicy.h
```

当前文本边界位于 `UBOverflowPrediction.cpp`：`ModuleOp` 以 generic form 打印到
`std::string`，随后传给模型 API。新的 direct-MLIR/overlay 方案应替换这条生产路径，但保留
文本 API 供 standalone CLI 和兼容测试使用。

## 模型入口和公共接口

```text
ub_overflow_model_cpp/include/ub_overflow_model/api.hpp
ub_overflow_model_cpp/src/api.cpp
ub_overflow_model_cpp/src/main.cpp
ub_overflow_model_cpp/CMakeLists.txt
ub_overflow_model_cpp/build.sh
```

生产 CMake target 是 `BiShengIRUBOverflowModel`。当前 build 为 Release，模型和接入 pass
均使用 `-O3 -DNDEBUG`；后续不能把普通 O3 切换当作主要优化收益。

standalone 可执行文件：

```text
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model
```

## 当前核心流水

```text
ub_overflow_model_cpp/src/pipeline/cvpipelining_ub_pipeline.hpp
ub_overflow_model_cpp/src/pipeline/bufferized_semantic_ir.hpp
ub_overflow_model_cpp/src/passes/post_bufferization_rewrites.hpp
ub_overflow_model_cpp/src/pipeline/after_alloc_extra_buffer.hpp
ub_overflow_model_cpp/src/pipeline/after_inline_load_copy.hpp
ub_overflow_model_cpp/src/pipeline/after_mark_multi_buffer.hpp
ub_overflow_model_cpp/src/pipeline/plan_memory_input_semantic_ir.hpp
ub_overflow_model_cpp/src/pipeline/plan_memory_input_builder.hpp
ub_overflow_model_cpp/src/passes/plan_memory/
```

生产 decision-only fast path 位于
`src/pipeline/cvpipelining_ub_pipeline.hpp::ProveConservativeNonOverflow`。
它在 MarkMultiBuffer 后按 AIV 函数累加所有存活 UB buffer 的独立对齐物理大小；生产命中后
跳过 PlanMemoryInput/PlanMemory，debug 路径只观察证明并继续完整计划。

当前高耗时和基础设施：

```text
src/ir/generic_ir.hpp                         string-heavy GenericOperation
src/ir/generic_rewriter.hpp                   mutation + CompactGenericModule
src/ir/generic_analysis.hpp                   dense read-only indexes
src/analysis/pipeline_metadata_cache.hpp      stage-local type/attr parsing cache
src/passes/mark_real_core_type.hpp            MIX combined projection
src/passes/tile_and_bind_sub_block.hpp        large transformation stage
src/pipeline/after_alloc_extra_buffer.hpp      typed/layout reconstruction hotspot
src/pipeline/plan_memory_input_builder.hpp     normal-path bridge hotspot
src/passes/plan_memory/mem_liveness_analysis.hpp
src/passes/plan_memory/mem_plan.hpp
```

## 目标基础设施位置

后续命名可以在实现时调整，但职责应保持清楚：

```text
MLIR adapter（BiSheng 侧）
  读取 ModuleOp/Operation/Value/Type/Attribute，不修改原 IR

shadow IR（模型侧）
  stable OpId/ValueId、base Operation*、局部 override、synthetic node arena

analysis manager（模型侧）
  revisioned def-use/CFG/enclosing-function/block-order/feature summary

UBBufferProgram（模型侧）
  统一 allocation/access/alias/layout/multi-buffer 状态

Typed PlanProgram（模型侧）
  PlanMemory 正常路径不生成也不读取 OperationRecord::text
```

原始 `Operation*` 只在一次同步 pass 调用期间有效。RetriablePassManager 的下一个 attempt 会
clone 新 module；跨 fallback 缓存必须保存无指针的紧凑 snapshot，不能保存旧 attempt 的
MLIR 指针。

## 数据和验证入口

```text
ub_overflow_model_cpp/data/adapter/                  真实 compiler 起点
ub_overflow_model_cpp/data/before_cvpipelining/      160 个去重性能/开发输入
ub_overflow_model_cpp/config/ub_relevant_parameter_scenarios.tsv
                                                      27 个有意义场景
ub_overflow_model_cpp/config/known_timeout_pairs.tsv  已知原生长尾
ub_overflow_model_cpp/config/failure_taxonomy.tsv     稳定失败分类
ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py
ub_overflow_model_cpp/tests/test_bisheng_embedded_matrix.py
ub_overflow_model_cpp/tests/run_tests.sh
ub_overflow_model_cpp/output/bisheng_embedded_oracle_cache/  本地可再生成缓存
```

当前正确性入口从 adapter 启动真实 `bishengir-compile`，在同一个 attempt 中比较 embedded
model 与原生 PlanMemory。`data/before_cvpipelining` 适合 standalone 性能分析，不替代
embedded 正确性 oracle。

`run_bisheng_embedded_matrix.py --oracle-cache-dir` 是 read-through 模式：miss 在同进程执行
model + native PlanMemory 并写缓存；hit 仍运行真实 prefix 和 embedded pass，但通过
`BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION` 在 prediction 后停止并读取原生合同。只缓存
单 attempt 参考；fallback/multi-attempt 始终现场运行，cache mismatch 也自动现场确认。

## 常用命令

构建模型和静态库：

```bash
bash ub_overflow_model_cpp/build.sh
```

构建带 embedded pass 的真实 compiler：

```bash
cmake --build build --target bishengir-compile -j8
```

完整模型单元/集成测试：

```bash
bash ub_overflow_model_cpp/tests/run_tests.sh
```

standalone 真实 retry-only：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --format=json
```

单输入 stage timing（只用于性能诊断）：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --show-runtime-timing \
  --format=json
```

单个 embedded 固定 seed 对比：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --input ADAPTER.ttadapter \
  --seeds 0 \
  --jobs 1
```

主要阶段完成后的 20-seed 正确性矩阵：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 \
  --jobs 12
```

重复开发验证使用 embedded native cache：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 \
  --jobs 12 \
  --oracle-cache-dir ub_overflow_model_cpp/output/bisheng_embedded_oracle_cache
```

强制现场刷新同一缓存增加 `--refresh-oracle-cache`。重大边界和发布前最终验证应去掉
`--oracle-cache-dir`，不能把 cache hit 报告当作现场全量。

手工查看 embedded 模型结果和耗时：

```bash
BISHENGIR_UB_MODEL_EMIT_RESULT=1 \
BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY=1 \
build/bin/bishengir-compile ADAPTER.ttadapter \
  --enable-hfusion-compile=true \
  --enable-triton-kernel-compile=true \
  -o /tmp/ub-model-probe.o
```

机器摘要中的 `decision_path=non_overflow_upper_bound` 表示模型使用了 exact non-overflow
fast path；此时 peak/required/selected_seed 为 unknown 是有意的窄合同，不是信息丢失。

普通产品运行默认不应设置 validation、dump、flow trace 或 machine-result 环境变量。

## 本地生成物

以下路径不提交：

```text
Output/
ub_overflow_model_cpp/output/
__pycache__/
临时 dump、cache、runtime TSV 和 profile 报告
```

历史兼容文件可能仍包含 suffix/cv2pm 名称，但它们不属于当前优化任务。不要为了清理命名
顺手修改工作正常的历史工具；只有当前实现依赖造成歧义或性能成本时才处理。
