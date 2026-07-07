# Pass Sequence To Second PlanMemory

日期：2026-06-30

## 范围和假设

本文件记录从 `bishengir-compile` 输入进入 `buildBiShengHIRPipeline`，直到第二次
`createPlanMemoryPass` 的 pass 序列。

这里的“第二次 PlanMemory”指：

```text
HIVM pre-bufferization: createPlanMemoryPass(GLOBAL_WORKSPACE_PLAN)
HIVM post-bufferization: createPlanMemoryPass(LOCAL_MEM_PLAN)
```

默认按以下常见完整路径展开：

```text
enableHfusionCompile=true
enableHIVMCompile=true
disableHIVMTensorCompile=false
enableTritonKernelCompile=false
disableAutoCVWorkSpaceManage=false
enableMultiKernel=false
enableOpsReorder=true
enableAutoBindSubBlock=true
enableCodeMotion=true
enableHIVMAutoStorageAlign=true
enableHIVMCrossCoreGSS=true
disableAutoInjectBlockSync=false
```

如果 `enableHfusionCompile=false`，整段 HFusion pipeline 跳过。如果
`disableAutoCVWorkSpaceManage=true`，第一次 global workspace PlanMemory 跳过，
此时 local PlanMemory 不再是“第二次”。如果 `enableTritonKernelCompile=true`，
HFusion 和 HIVM 前半段会走 Triton 分支，序列需要单独展开。

## 代码来源

```text
bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp
bishengir/lib/Dialect/HFusion/Pipelines/HFusionPipelines.cpp
bishengir/lib/Dialect/HIVM/Pipelines/ConvertToHIVMPipeline.cpp
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
```

## Helper pipeline 展开

### HFusion canonicalizationPipeline

每次出现 `HFusion canonicalizationPipeline`，等价于：

```text
createCSEPass
createExtendedCanonicalizerPass
tensor::createNormalizeTensorOpsPass
```

### HIVM canonicalizationHIVMPipeline

每次出现 `HIVM canonicalizationHIVMPipeline`，等价于：

```text
createArithToAffineConversionPass
scf::createCanonicalizeIterArgPass
createExtendedCanonicalizerPass
createSCFForLoopCanonicalizationPass
createCSEPass
createExtendedCanonicalizerPass
createHIVMOptSinglePointPass
createExtendedCanonicalizerPass
memref::createDeadStoreEliminationPass
```

### inferAndSetBufferSizePipeline

每次出现 `inferAndSetBufferSizePipeline`，等价于：

```text
createAutoInferBufferSizePass
createArithToAffineConversionPass
createConstantizeBufferSizePass
createSetBufferSizePass
```

### hivmCrossCoreSyncPipeline

在默认条件下展开为：

```text
HIVM canonicalizationHIVMPipeline
createMarkRealCoreTypePass
createCrossCoreGSSPass
createMarkRealCoreTypePass(removeCoreTypeAttrs=true)
```

如果 `enableHIVMCrossCoreGSS=false` 或强制 block-all-sync，则中间分支改为
`createInjectBlockSyncPass`。

### syncBlockLockPipeline(Prepare)

```text
createSyncBlockHoistingPass
createBindSyncBlockLockArgPass
createInsertInferSyncBlockLockNumAndInitFuncPass
createSyncBlockLockLoweringPass
```

### alignStoragePipeline

```text
createAlignAllocSizePass
createMarkStrideAlignPass
memref::createFoldAllocReshapePass
createEnableStrideAlignPass
```

如果 `enableHIVMAutoStorageAlign=false`，跳过 `createMarkStrideAlignPass`。

## 完整顺序

### S0 buildBiShengHIRPipeline 入口

```text
createCanonicalizeModulePass
hacc::createAppendDeviceSpecPass
```

### S1 HFusion pipeline

```text
symbol::createEraseSymbolPass
createArithToHFusionConversionPass
createMathToHFusionConversionPass
createLinalgToHFusionConversionPass
createTensorToHFusionConversionPass
tensor::createCanonicalizeTensorReshapePass
HFusion canonicalizationPipeline
createArithToHFusionConversionPass
createConvertGenericToNamedOpPass
createLegalizeBF16Pass
createDecomposePass(NO_CONSTRAINT)
createHFusionNormalizeSliceOpsPass
createHFusionNormalizeOpsPass
createLegalizeBoolPass
createSimplifyOpsPass
createHFusionInlineBrcPass
createHFusionNormalizeOpsPass
HFusion canonicalizationPipeline
tensor::createBubbleUpExtractSlicePass
HFusion canonicalizationPipeline
createLinalgFoldUnitExtentDimsPass
HFusion canonicalizationPipeline
createArithToHFusionConversionPass
createComposeMultiReduce
tensor::createPropagateReshapePass
createSimplifyOpsPass
HFusion canonicalizationPipeline
tensor::createFoldTensorEmptyPass
createFlattenOpsPass
tensor::createCanonicalizeTensorReshapePass
HFusion canonicalizationPipeline
createCacheIOForReturnArg
tensor::createFoldTensorEmptyPass
HFusion canonicalizationPipeline
createFoldSymbolicDimPass
createInferFuncFusionKind
createHFusionOpFusionPass
HFusion canonicalizationPipeline
createOutlineSingleOpPass
HFusion canonicalizationPipeline
createUnfoldSymbolicDimPass
createDropSymbolsPass
createEliminateDuplicateFuncsPass
createDowngradeFP64CstOpPass
tensor::createTrickleConcatDownPass
tensor::createBubblePadUpPass
createLegalizeBoolPass
tensor::createFoldTensorEmptyPass
tensor::createNormalizeLastDimUnalignedTensorOpPass
createReorderOpsByBFS
HFusion canonicalizationPipeline
createDecomposePass(AFTER_HFUSION_FLATTEN)
createHFusionAutoSchedulePass
createDecomposeMulti
createConvertGenericToNamedOpPass
HFusion canonicalizationPipeline
createReorderOpsByBFS
createConstantizeTilingDataPass
HFusion canonicalizationPipeline
createPackTilingDataPass
createArithToAffineConversionPass
HFusion canonicalizationPipeline
createSCFForLoopCanonicalizationPass
HFusion canonicalizationPipeline
createWrapHostFuncPass
HFusion canonicalizationPipeline
createHFusionInlineBrcPass
createHFusionNormalizeOpsPass
createAddFFTSAddrPass
createHoistTensorEmptyPass
createDecomposePass(AFTER_HFUSION_FLATTEN)
```

### S2 ConvertToHIVM pipeline

```text
createHFusionToHIVMConversionPass
createTensorToHIVMConversionPass
createConvertToHIVMOpPass
```

If `enableTritonKernelCompile=true`, insert
`createTritonGlobalKernelArgsToHIVMOpPass` after
`createHFusionToHIVMConversionPass`.

### S3 HIVM optimize pipeline before first PlanMemory

```text
createInitEntryKernelPass
tensor::createPropagateReshapePass(forHIVM)
scf::createRemoveRedundantLoopInitPass
createNormalizeMatmulPass
createNormalizeConvOpsPass
createNormalizeBitwiseSelectPass
createInlineFixpipePass
createInsertLoadStoreForMixCVPass
createInsertLoadStoreForScalarPass
createTileBatchMMIntoLoopPass
createNormalizeMatmulPass
createInsertNZ2NDForDebugPass
createInlineFixpipePass
createInsertLoadStoreForMixCVPass
createInsertLoadStoreForScalarPass
createInsertWorkSpaceForMixCVPass
createBindWorkSpaceArgPass
createInferFuncCoreTypePass
createMarkMultiBufferPass
createExtendedCanonicalizerPass
createInlineOTFBroadcastPass
createCVPipeliningPass
inferAndSetBufferSizePipeline
createPlanMemoryPass(GLOBAL_WORKSPACE_PLAN)
```

上面的 `createPlanMemoryPass(GLOBAL_WORKSPACE_PLAN)` 是第一次 PlanMemory，规划
global workspace，不是 local UB/L1/L0C plan。

### S4 Between first PlanMemory and bufferization

```text
hivmCrossCoreSyncPipeline
createSplitMixKernelPass
scope::createInlineScopePass
createTileAndBindSubBlockPass
tensor::createFoldTensorEmptyPass
HIVM canonicalizationHIVMPipeline
createLoopInvariantCodeMotionPass
createLoopInvariantSubsetHoistingPass
createCloneTensorEmptyPass
createHIVMInlineOTFLoadStorePass
```

### S5 Bufferization pipeline

```text
bufferization::createOneShotBufferizePass
HIVM canonicalizationHIVMPipeline
bufferization::createDropEquivalentBufferResultsPass
HIVM canonicalizationHIVMPipeline
bufferization::createDropEquivalentBufferResultsPass
createConvertToHIVMOpPass
```

这里是第一个适合做 concrete structural buffer/memref 统计的阶段。

### S6 Post-bufferization to local PlanMemory

```text
createLiftZeroRankPass
scf::createMapForToForallPass
createHIVMMapForallToBlocksPass
createHIVMDecomposeOpPass
syncBlockLockPipeline(Prepare)
createNonContiguousReshapeToCopyPass
createInferHIVMMemScopePass
createHIVMDecomposeOpPass
createHIVMAggregatedDecomposeOpPass(BEFORE_HIVM_STRIDE_ALIGNMENT)
createHIVMRecognizeDeinterleaveOpPass
createHIVMAggregatedDecomposeOpPass(AFTER_RECOGNIZE_DEINTERLEAVE)
createHIVMAggregatedDecomposeOpPass(AFTER_RECOGNIZE_BROADCAST)
alignStoragePipeline
createHIVMAggregatedDecomposeOpPass(AFTER_HIVM_STRIDE_ALIGNMENT)
createInferHIVMDataLayoutPass
createRemoveHIVMDataLayoutAnnotationPass
createHIVMAggregatedDecomposeOpPass(AFTER_INFER_HIVM_DATA_LAYOUT)
createExtendedCanonicalizerPass
inferAndSetBufferSizePipeline
createFlattenOpsPass
createHIVMAggregatedDecomposeOpPass(AFTER_HIVM_FLATTEN_OPS)
createReduceRankSubviewPass
createLiftLowestStridePass
createHIVMAggregatedDecomposeOpPass(AFTER_LIFT_LOWEST_STRIDE)
createAllocExtraBufferPass
createInferHIVMMemScopePass
HIVM canonicalizationHIVMPipeline
createInlineLoadCopyPass
createMarkMultiBufferPass
createDumpIRBeforePlanMemoryPass
createPlanMemoryPass(LOCAL_MEM_PLAN)
```

上面的 `createPlanMemoryPass(LOCAL_MEM_PLAN)` 是第二次 PlanMemory，也是 local
UB/L1/L0C 精确规划点。`createDumpIRBeforePlanMemoryPass` 位于 `MarkMultiBuffer` 和
local `PlanMemory` 之间；现在需要 `--enable-dump-ir-before-plan-memory` 才会插入该
debug pass，并通过环境变量 `BISHENGIR_DUMP_BEFORE_PLAN_MEMORY` 指定输出路径。

## 条件 pass 摘要

- `enableHfusionCompile=false`：跳过 S1。
- `enableTritonKernelCompile=true`：HFusion 走 Triton 分支；ConvertToHIVM 额外插入
  `createTritonGlobalKernelArgsToHIVMOpPass`；HIVM bufferization 前后还有 Triton
  专属 pass。
- `disableAutoCVWorkSpaceManage=true`：跳过 mix CV load/store/workspace、pre-buffer
  MarkMultiBuffer、CVPipelining、global workspace PlanMemory 等；此时 local
  PlanMemory 不是第二次。
- `enableOpsReorder=false`：跳过 HFusion autoschedule 前后的 `createReorderOpsByBFS`。
- `enableAutoBindSubBlock=false`：跳过 `createTileAndBindSubBlockPass`。
- `enableCodeMotion=false`：跳过 `createLoopInvariantCodeMotionPass` 和
  `createLoopInvariantSubsetHoistingPass`。
- `enableHIVMAutoStorageAlign=false`：跳过 `createMarkStrideAlignPass`。
- `enablePreload=true`：第二次 PlanMemory 之后会走 `createCreatePreloadPass`，不在本文件
  主序列范围内。
