# 当前代码与命令

本文件记录当前产品、before-AutoBlockify 扩展位置和后续性能目标；已删除的旧后缀工具只通过
Git 历史追溯。

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

当前生产边界由 `UBOverflowPrediction.cpp` 同步调用 `evaluateModule(ModuleOp, Request)`。这是
提交 `1dfddd59` 的源码事实，但不再作为继续 MLIR-backed 基础设施迁移的方向。新的依赖目标是：
BiSheng adapter 可以读取 `ModuleOp`，轻量模型 core 只接收项目自有输入。当前
`MLIRModuleView::materializeLegacyGenericModule()` 暂时保留为兼容边界，不继续向更多 pass
传播 MLIR 类型；core/adapter 拆分排在现有热点优化之后。

下一产品边界位于 `HIVMPipelines.cpp` 的 `createAutoBlockifyParallelLoopPass()` 之前。典型
Triton adapter 到该位置已经由原生 BiSheng 执行约 61 次 pass；模型不复刻这段前缀。切换前
先使用默认关闭的测试 hook observe-only，当前 before-CV 产品入口继续保留。

## before-AutoBlockify 到 CV 的原生序列

```text
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp

AutoBlockifyParallelLoop                         [conditional]
MarkMultiBuffer                                  [workspace manage enabled]
ExtendedCanonicalizer
canonicalizationHIVMPipeline:
  ArithToAffine
  CanonicalizeIterArg
  ExtendedCanonicalizer
  SCFForLoopCanonicalization
  CSE
  func ExtendedCanonicalizer
  HIVMOptSinglePoint
  func ExtendedCanonicalizer
  MemrefDeadStoreElimination
InlineOTFBroadcast
CVPipelining                                     [existing model begins here]
```

原生实现位置：

```text
bishengir/lib/Dialect/HIVM/Transforms/AutoBlockifyParallelLoop.cpp
bishengir/lib/Dialect/HIVM/Transforms/MarkMultiBuffer.cpp
bishengir/lib/Transforms/ExtendedCanonicalizer.cpp
bishengir/lib/Conversion/ArithToAffine/ArithToAffine.cpp
bishengir/lib/Dialect/SCF/Transforms/CanonicalizeIterArg.cpp
third-party/llvm-project/mlir/lib/Dialect/SCF/Transforms/LoopCanonicalization.cpp
third-party/llvm-project/mlir/lib/Transforms/CSE.cpp
bishengir/lib/Dialect/HIVM/Transforms/HIVMOptSinglePoint.cpp
bishengir/lib/Dialect/MemRef/Transforms/DeadStoreElimination.cpp
bishengir/lib/Dialect/HIVM/Transforms/InlineOTFBroadcast.cpp
```

`ub_overflow_model_cpp/src/passes/pre_cv_cse.hpp`

阶段 4.5 的 standalone module CSE 投影。实现与上游 `CSEDriver` 相同的 region scope、
isolated-from-above、nested-region-first traversal、commutative/region operation equivalence、只读
memory effect barrier 和 deferred erase。`verify_cse_pipeline.py` 同时验证 checkpoint `07 -> 08`
和累计 `00 -> 08`；multi-block region 当前不在 160 输入支持域内并显式 blocker。

`ub_overflow_model_cpp/src/passes/func_extended_canonicalizer.hpp`

阶段 4.6 第一次 func-scoped ExtendedCanonicalizer 的独立边界。它复用已验证的 module fixed-point
实现，因为后者的 OperationFolder 与全部 modeled rewrite 已按 enclosing function 分区；双函数
fixture 和 `verify_first_func_extended_canonicalizer_pipeline.py` 分别验证 scope 及 checkpoint
`08 -> 09`/累计 `00 -> 09`。同文件的 `RunSecondFuncExtendedCanonicalizer` 是阶段 4.8 的独立
入口；`verify_second_func_extended_canonicalizer_pipeline.py` 验证 `10 -> 11`/累计 `00 -> 11`，
当前同样为 `1280/1280 PASS`。

`ub_overflow_model_cpp/src/passes/pre_cv_hivm_opt_single_point.hpp`

阶段 4.7 的 pre-CV pure-buffer scalarization。实现直接对应原生
`HIVMOptSinglePoint.cpp` 和 `Dialect/Utils/Util.cpp` 的 scalar load/store、memref traceback 与
memory-user closure；与 post-bufferization 的 analysis-only `hivm_opt_single_point.hpp` 是不同
阶段，不能互换。`verify_hivm_opt_single_point_pipeline.py` 验证 checkpoint `09 -> 10` 和累计
`00 -> 10`，当前 8 profiles × 160 inputs 为 `1280/1280 PASS`。

`ub_overflow_model_cpp/src/passes/pre_cv_memref_dead_store_elimination.hpp`

阶段 4.9 的 pre-CV MemRef DSE。实现直接对应原生
`Dialect/MemRef/Transforms/DeadStoreElimination.cpp`、`ValueDependencyAnalyzer.cpp` 与 MLIR
`MemRefUtils.cpp::eraseDeadAllocAndStores`，包括 ViewLike alias root、同层 forwarding barrier、
reg-based 尾部 HIVM Load 清理和递归 subview 死链删除。对应 runner 开关为
`--apply-memref-dse`，验证脚本 `verify_memref_dse_pipeline.py` 同时比较 checkpoint `11 -> 12`
和累计 `00 -> 12`；当前 8 profiles × 160 inputs 为 `1280/1280 PASS`。

`ub_overflow_model_cpp/src/passes/pre_cv_inline_otf_broadcast.hpp`

阶段 5 的 pre-CV VBrc inline。实现直接对应原生 `InlineOTFBroadcast.cpp` 的单一
`VBrcInlinePattern`，包括 LAST-axis 白名单、Ascend950 shift、非 LAST trait/interface 条件、
element-type 拒绝、DPS input replacement 和 broadcast dims 合并。runner 开关为
`--apply-inline-otf-broadcast`；`verify_inline_otf_broadcast_pipeline.py` 验证 checkpoint
`12 -> 13` 与累计 `00 -> 13`，当前 8 profiles × 160 inputs 为 `1280/1280 PASS`。

`ub_overflow_model_cpp/src/pipeline/pre_cv_prefix_pipeline.hpp`

阶段 6 唯一组合入口，严格按原生 checkpoint 01～13 顺序串联全部 pre-CV pass，并集中映射
AutoBlockify/MarkMultiBuffer 的 resolved options。`pre_cv_prefix_model_runner` 的
`--apply-combined-prefix` 与 `verify_combined_pre_cv_prefix.py` 直接比较 checkpoint 00→13；当前
8 profiles × 160 inputs 为 `1280/1280 PASS`。API 的 before-AutoBlockify contract v2 才调用
该入口，legacy v1 不调用。

`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

阶段 7 的 embedded 调用点现位于 checkpoint 00 后、原生 AutoBlockify 前。它与后续真实
CVPipelining 共用同一 resolved options，并在全量验证期间强制 `pruneOnOverflow=false`；完成
全量 gate 后才允许删除该覆盖。`UBOverflowPrediction.cpp` 构造 before-AutoBlockify contract v2
并将相同 ModuleOp 交给模型。

现有可复用模型代码：

```text
ub_overflow_model_cpp/src/passes/auto_blockify_parallel_loop.hpp
ub_overflow_model_cpp/tests/test_auto_blockify_parallel_loop.cpp
ub_overflow_model_cpp/scripts/verify_auto_blockify.py
ub_overflow_model_cpp/src/passes/canonicalization_hivm_pipeline.hpp
ub_overflow_model_cpp/src/passes/convert_arith_to_affine.hpp
```

`src/passes/mark_multi_buffer.hpp` 是 PlanMemory 前、post-bufferization 阶段的实现，不等价于
CV 前 MarkMultiBuffer；只能复用经源码证明相同的 enum/helper。

## 原生逐 pass checkpoint

默认关闭的 checkpoint hook 位于：

```text
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
bishengir/lib/Tools/bishengir-compile/BiShengIRCompileMain.cpp
ub_overflow_model_cpp/scripts/dump_ub_prefix_checkpoints.py
ub_overflow_model_cpp/tests/test_ub_prefix_checkpoints.py
```

生成全部 14 个原生 checkpoint：

```bash
python3 ub_overflow_model_cpp/scripts/dump_ub_prefix_checkpoints.py \
  ub_overflow_model_cpp/data/adapter/ascend_tutorial_01-vector-add.ttadapter \
  --compiler build/bin/bishengir-compile \
  --output-dir /tmp/cvub-prefix-checkpoints
```

只生成一个 stage 时增加 `--stage 01_after_auto_blockify`。自定义真实编译参数必须放在 `--`
后；未提供时脚本使用 Triton/AutoBlockify 开发默认参数。checkpoint 位于
`<output-dir>/attempt-N/`；停止开关在 before-CV 边界生效，不运行 CVPipelining。

AutoBlockify 的同 attempt 全量差分：

```bash
python3 ub_overflow_model_cpp/scripts/verify_auto_blockify_pipeline.py \
  --jobs 8 \
  --failure-dir ub_overflow_model_cpp/output/auto_blockify_pipeline_verification/failures \
  --json-report ub_overflow_model_cpp/output/auto_blockify_pipeline_verification/full.json
```

脚本用 `data/before_cvpipelining` 的 160 个目录名选择对应 adapter；每个 adapter 只运行一次
真实 compiler，并直接比较该 attempt 的 `00_before_auto_blockify` 经模型变换后的结构与
`01_after_auto_blockify` 原生结构。

pre-CV MarkMultiBuffer 实现与验证：

```text
ub_overflow_model_cpp/src/passes/pre_cv_mark_multi_buffer.hpp
ub_overflow_model_cpp/tools/pre_cv_prefix_model_runner.cpp
ub_overflow_model_cpp/tests/fixtures/pre_cv_mark_multi_buffer.mlir
ub_overflow_model_cpp/tests/test_pre_cv_mark_multi_buffer.cpp
ub_overflow_model_cpp/scripts/verify_pre_cv_mark_multi_buffer_pipeline.py
```

```bash
python3 ub_overflow_model_cpp/scripts/verify_pre_cv_mark_multi_buffer_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/pre_cv_mark_mb_full.json
```

脚本读取 `config/pre_cv_profiles/*.args` 的 8 个相关 profile；每个 case 同时验证单 pass 与从
before-AutoBlockify 开始的累计前缀，oracle 是同一 compiler attempt 的
`02_after_pre_cv_mark_multi_buffer`。

outer module-level ExtendedCanonicalizer 实现与验证：

```text
ub_overflow_model_cpp/src/passes/outer_extended_canonicalizer.hpp
ub_overflow_model_cpp/src/ir/operation_folder.hpp
ub_overflow_model_cpp/tests/fixtures/outer_extended_canonicalizer.mlir
ub_overflow_model_cpp/tests/test_outer_extended_canonicalizer.cpp
ub_overflow_model_cpp/scripts/verify_outer_extended_canonicalizer_pipeline.py
```

```bash
python3 ub_overflow_model_cpp/scripts/verify_outer_extended_canonicalizer_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/outer_extended_canonicalizer_phase3.json
```

脚本逐项比较单 pass `02_after_pre_cv_mark_multi_buffer -> outer` 和累计
`00_before_auto_blockify -> AutoBlockify -> MarkMultiBuffer -> outer` 与同一真实 compiler attempt
的 `03_after_outer_extended_canonicalizer`。当前 8 profiles × 160 inputs 为 `1280/1280 PASS`。

ArithToAffine 独立阶段实现与验证：

```text
ub_overflow_model_cpp/src/passes/convert_arith_to_affine.hpp
ub_overflow_model_cpp/src/passes/affine_min_max_canonicalization.hpp
ub_overflow_model_cpp/tests/fixtures/arith_to_affine.mlir
ub_overflow_model_cpp/tests/test_arith_to_affine.cpp
ub_overflow_model_cpp/scripts/verify_arith_to_affine_pipeline.py
```

`RunArithToAffineConversionPass` 是 checkpoint `03 -> 04` 的严格 pass 入口；不得替换成会附带
后续 canonicalization/CSE 的组合入口。验证脚本同时比较单 pass 和从 checkpoint `00` 开始的
累计前缀，当前 8 profiles × 160 inputs 为 `1280/1280 PASS`。

CanonicalizeIterArg 独立阶段实现与验证：

```text
ub_overflow_model_cpp/src/passes/canonicalization_hivm_pipeline.hpp
ub_overflow_model_cpp/tests/fixtures/canonicalize_iter_arg.mlir
ub_overflow_model_cpp/tests/test_canonicalize_iter_arg.cpp
ub_overflow_model_cpp/scripts/verify_canonicalize_iter_arg_pipeline.py
```

`RunCanonicalizationHIVMAfterArithToAffine` 当前是 checkpoint `04 -> 05` 的严格阶段入口；其 CSE
来自原生 `CanonicalizeIterArgPattern` 内部调用，不是无条件 standalone pass。验证脚本比较单 pass
与从 checkpoint `00` 开始的累计前缀，当前 8 profiles × 160 inputs 为 `1280/1280 PASS`。

canonicalization pipeline 内的 module-level ExtendedCanonicalizer：

```text
ub_overflow_model_cpp/src/passes/module_extended_canonicalizer.hpp
ub_overflow_model_cpp/src/passes/affine_min_max_canonicalization.hpp
ub_overflow_model_cpp/src/passes/convert_arith_to_affine.hpp
ub_overflow_model_cpp/tests/fixtures/module_extended_canonicalizer.mlir
ub_overflow_model_cpp/tests/test_module_extended_canonicalizer.cpp
ub_overflow_model_cpp/scripts/verify_module_extended_canonicalizer_pipeline.py
```

`RunModuleExtendedCanonicalizer` 是 checkpoint `05 -> 06` 的严格阶段入口，复用公共 dialect
patterns，但额外执行此边界由 ArithToAffine/CanonicalizeIterArg 暴露的 affine fixed point。
验证脚本比较单 pass 与从 checkpoint `00` 开始的累计前缀，当前 8 profiles × 160 inputs 为
`1280/1280 PASS`。

SCFForLoopCanonicalization：

```text
ub_overflow_model_cpp/src/passes/scf_for_loop_canonicalization.hpp
ub_overflow_model_cpp/tests/fixtures/scf_for_loop_canonicalization.mlir
ub_overflow_model_cpp/tests/test_scf_for_loop_canonicalization.cpp
ub_overflow_model_cpp/scripts/verify_scf_for_loop_canonicalization_pipeline.py
```

`RunSCFForLoopCanonicalization` 是 checkpoint `06 -> 07` 的严格阶段入口；实现来自上游
`LoopCanonicalization.cpp` 与 `AffineCanonicalizationUtils.cpp`，不能替换成普通 canonicalizer。
验证脚本比较单 pass 与从 checkpoint `00` 开始的累计前缀，当前 8 profiles × 160 inputs 为
`1280/1280 PASS`。

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

## 当前依赖边界与暂停项

```text
BiSheng adapter（允许 MLIR）
  ModuleOp + resolved options
  -> 一次性转换

模型 core（不得新增 LLVM/MLIR）
  GenericModule / model-owned states
  -> current lightweight passes
  -> PlanMemory
```

当前已有的 direct-MLIR 文件：

```text
ub_overflow_model_cpp/src/ir/mlir_module_view.hpp
ub_overflow_model_cpp/src/mlir_main.cpp
ub_overflow_model_cpp/include/ub_overflow_model/api.hpp
```

它们属于现有集成技术债和边界代码，不是新模型 core 的基础设施。`GenericShadowOverlay`、stable
IDs 和最低可用 revisioned analysis 当前冻结，不继续迁移 CVPipelining、canonicalization、
MarkRealCoreType 或后 bufferization 全流水。全局 `UBBufferProgram/PlanProgram` 替换同样暂停。

## 数据和验证入口

```text
ub_overflow_model_cpp/data/adapter/                  真实 compiler 起点
ub_overflow_model_cpp/data/before_cvpipelining/      160 个去重性能/开发输入
ub_overflow_model_cpp/config/ub_relevant_parameter_scenarios.tsv
                                                      27 个有意义场景
ub_overflow_model_cpp/config/known_timeout_pairs.tsv  已知原生长尾
ub_overflow_model_cpp/config/failure_taxonomy.tsv     稳定失败分类
ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py
ub_overflow_model_cpp/scripts/plan_memory_contract.py  两侧 PlanMemory 合同解析
ub_overflow_model_cpp/scripts/measure_embedded_model.py  production 单轮 A/B 与 RSS
ub_overflow_model_cpp/tests/test_bisheng_embedded_matrix.py
ub_overflow_model_cpp/tests/run_tests.sh
```

现有 direct-MLIR/overlay 边界（冻结，不扩张）：

```text
ub_overflow_model_cpp/src/ir/mlir_module_view.hpp        同步只读 ModuleOp view/import
ub_overflow_model_cpp/src/ir/stable_id.hpp               强类型稳定 ID
ub_overflow_model_cpp/src/ir/shadow_overlay.hpp          base/synthetic shadow 与 rewriter
ub_overflow_model_cpp/src/ir/cow_string.hpp              MLIR 属性延迟格式化/COW override
ub_overflow_model_cpp/src/mlir_main.cpp                  MLIR parser standalone 边界
ub_overflow_model_cpp/include/ub_overflow_model/api.hpp  evaluateModule 同步 API
```

当前正确性入口从 adapter 启动真实 `bishengir-compile`，在同一个 attempt 中比较 embedded
model 与原生 PlanMemory。`data/before_cvpipelining` 适合 standalone 性能分析，不替代
embedded 正确性 oracle。

`run_bisheng_embedded_matrix.py` 不读取原生结果缓存：每个选中的 seed 都从 adapter 启动真实
编译，在模型之后继续到原生本地 PlanMemory，再比较完整合同。

## 常用命令

构建兼容模型和静态库：

```bash
bash ub_overflow_model_cpp/build.sh
```

构建与 embedded 路径共用 ModuleOp API 的 standalone：

```bash
cmake --build build --target bishengir-ub-overflow-model -j8
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
build/bin/bishengir-ub-overflow-model \
  INPUT.mlir \
  --format=json
```

兼容文本入口的单输入 stage timing（只用于性能诊断）：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --show-runtime-timing \
  --format=json
```

单个 embedded 输入的完整 20-seed 对比：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --input ADAPTER.ttadapter \
  --seeds 0-19 \
  --jobs 1
```

主要阶段完成后的 20-seed 正确性矩阵：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 \
  --jobs 12
```

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

160-input production retry-only 单轮性能测量：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/measure_embedded_model.py \
  --variant current=build/bin/bishengir-compile \
  --rounds 3 \
  --report ub_overflow_model_cpp/output/performance/current.tsv \
  --summary ub_overflow_model_cpp/output/performance/current.json
```

工具只为测量显式打开 machine result 和 prediction 后停止边界；不会打开 validation、dump 或
原生 PlanMemory。

## 本地生成物

以下路径不提交：

```text
Output/
ub_overflow_model_cpp/output/
__pycache__/
临时 dump、cache、runtime TSV 和 profile 报告
```
