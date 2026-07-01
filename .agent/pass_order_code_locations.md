# Pass Order Code Locations

日期：2026-07-01

## 范围

本文件按从输入到第二次 `PlanMemory` 的细粒度顺序，记录每个 pass 及其 pipeline
代码位置。

代码位置指 **pass 被加入 `OpPassManager` 的位置**，不是 pass 实现文件位置。需要查
具体实现时，再从 `create*Pass` 名字跳到对应 transform 源文件。

默认路径同 [pass_sequence_to_second_planmemory.md](pass_sequence_to_second_planmemory.md)：

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
```

如果启用 Triton 或关闭 workspace 管理，条件 pass 会变化。

## Helper 展开位置

### HFusion canonicalizationPipeline

位置：
`bishengir/lib/Dialect/HFusion/Pipelines/HFusionPipelines.cpp`

```text
createCSEPass                                      line 72
createExtendedCanonicalizerPass                   line 75
tensor::createNormalizeTensorOpsPass              line 76
```

### HIVM canonicalizationHIVMPipeline

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
createArithToAffineConversionPass                 line 79
scf::createCanonicalizeIterArgPass                line 80
createExtendedCanonicalizerPass                   line 81
createSCFForLoopCanonicalizationPass              line 82
createCSEPass                                     line 83
createExtendedCanonicalizerPass                   line 84
createHIVMOptSinglePointPass                      line 85
createExtendedCanonicalizerPass                   line 86
memref::createDeadStoreEliminationPass            line 87
```

### inferAndSetBufferSizePipeline

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
createAutoInferBufferSizePass                     line 144
createArithToAffineConversionPass                 line 147
createConstantizeBufferSizePass                   line 148
createSetBufferSizePass                           line 149
```

### hivmAutoInsertLdStForMixCVPipeline

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
createInsertLoadStoreForMixCVPass                 line 196
createInsertLoadStoreForScalarPass                line 198
```

如果 `enableLegacyInsertLoadStoreForMixCV=true`，只走
`createInsertLoadStoreForMixCVPass`，位置是 line 193。

### hivmCrossCoreSyncPipeline

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

默认展开：

```text
HIVM canonicalizationHIVMPipeline                  line 118
createMarkRealCoreTypePass                        line 119
createCrossCoreGSSPass                            line 123
createMarkRealCoreTypePass(removeCoreTypeAttrs)   line 140
```

如果不走 CrossCoreGSS，中间改为 `createInjectBlockSyncPass`，位置是 line 132。

### syncBlockLockPipeline(Prepare)

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
createSyncBlockHoistingPass                       line 324
createBindSyncBlockLockArgPass                    line 325
createInsertInferSyncBlockLockNumAndInitFuncPass  line 327
createSyncBlockLockLoweringPass                   line 328
```

### alignStoragePipeline

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
createAlignAllocSizePass                          line 313
createMarkStrideAlignPass                         line 315
memref::createFoldAllocReshapePass                line 317
createEnableStrideAlignPass                       line 318
```

`createMarkStrideAlignPass` 受 `enableHIVMAutoStorageAlign` 控制。

## Ordered Small Passes

### S0 / Top-level

位置：
`bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp`

```text
001 createCanonicalizeModulePass                  line 54
002 hacc::createAppendDeviceSpecPass              line 56
```

### S1 / HFusion

位置：
`bishengir/lib/Dialect/HFusion/Pipelines/HFusionPipelines.cpp`

```text
003 symbol::createEraseSymbolPass                 line 82
004 createArithToHFusionConversionPass            line 84
005 createMathToHFusionConversionPass             line 85
006 createLinalgToHFusionConversionPass           line 86
007 createTensorToHFusionConversionPass           line 92
008 tensor::createCanonicalizeTensorReshapePass   line 94
009 createCSEPass                                 line 72
010 createExtendedCanonicalizerPass               line 75
011 tensor::createNormalizeTensorOpsPass          line 76
012 createArithToHFusionConversionPass            line 99
013 createConvertGenericToNamedOpPass             line 100
014 createLegalizeBF16Pass                        line 101
015 createDecomposePass(NO_CONSTRAINT)            line 105
016 createHFusionNormalizeSliceOpsPass            line 106
017 createHFusionNormalizeOpsPass                 line 108
018 createLegalizeBoolPass                        line 109
019 createSimplifyOpsPass                         line 110
020 createHFusionInlineBrcPass                    line 111
021 createHFusionNormalizeOpsPass                 line 114
022 createCSEPass                                 line 72
023 createExtendedCanonicalizerPass               line 75
024 tensor::createNormalizeTensorOpsPass          line 76
025 tensor::createBubbleUpExtractSlicePass        line 119
026 createCSEPass                                 line 72
027 createExtendedCanonicalizerPass               line 75
028 tensor::createNormalizeTensorOpsPass          line 76
029 createLinalgFoldUnitExtentDimsPass            line 125
030 createCSEPass                                 line 72
031 createExtendedCanonicalizerPass               line 75
032 tensor::createNormalizeTensorOpsPass          line 76
033 createArithToHFusionConversionPass            line 128
034 createComposeMultiReduce                      line 131
035 tensor::createPropagateReshapePass            line 138
036 createSimplifyOpsPass                         line 139
037 createCSEPass                                 line 72
038 createExtendedCanonicalizerPass               line 75
039 tensor::createNormalizeTensorOpsPass          line 76
040 tensor::createFoldTensorEmptyPass             line 162
041 createFlattenOpsPass                          line 167
042 tensor::createCanonicalizeTensorReshapePass   line 169
043 createCSEPass                                 line 72
044 createExtendedCanonicalizerPass               line 75
045 tensor::createNormalizeTensorOpsPass          line 76
046 createCacheIOForReturnArg                     line 171
047 tensor::createFoldTensorEmptyPass             line 173
048 createCSEPass                                 line 72
049 createExtendedCanonicalizerPass               line 75
050 tensor::createNormalizeTensorOpsPass          line 76
051 createFoldSymbolicDimPass                     line 179
052 createInferFuncFusionKind                     line 180
053 createHFusionOpFusionPass                     line 188
054 createCSEPass                                 line 72
055 createExtendedCanonicalizerPass               line 75
056 tensor::createNormalizeTensorOpsPass          line 76
057 createOutlineSingleOpPass                     line 193
058 createCSEPass                                 line 72
059 createExtendedCanonicalizerPass               line 75
060 tensor::createNormalizeTensorOpsPass          line 76
061 createUnfoldSymbolicDimPass                   line 195
062 createDropSymbolsPass                         line 196
063 createEliminateDuplicateFuncsPass             line 197
064 createDowngradeFP64CstOpPass                  line 144
065 tensor::createTrickleConcatDownPass           line 145
066 tensor::createBubblePadUpPass                 line 146
067 createLegalizeBoolPass                        line 147
068 tensor::createFoldTensorEmptyPass             line 148
069 tensor::createNormalizeLastDimUnalignedTensorOpPass line 150
070 createReorderOpsByBFS                         line 219
071 createCSEPass                                 line 72
072 createExtendedCanonicalizerPass               line 75
073 tensor::createNormalizeTensorOpsPass          line 76
074 createDecomposePass(AFTER_HFUSION_FLATTEN)    line 225
075 createHFusionAutoSchedulePass                 line 240
076 createDecomposeMulti                          line 242
077 createConvertGenericToNamedOpPass             line 244
078 createCSEPass                                 line 72
079 createExtendedCanonicalizerPass               line 75
080 tensor::createNormalizeTensorOpsPass          line 76
081 createReorderOpsByBFS                         line 247
082 createConstantizeTilingDataPass                line 203
083 createCSEPass                                 line 72
084 createExtendedCanonicalizerPass               line 75
085 tensor::createNormalizeTensorOpsPass          line 76
086 createPackTilingDataPass                      line 208
087 createArithToAffineConversionPass             line 210
088 createCSEPass                                 line 72
089 createExtendedCanonicalizerPass               line 75
090 tensor::createNormalizeTensorOpsPass          line 76
091 createSCFForLoopCanonicalizationPass          line 212
092 createCSEPass                                 line 72
093 createExtendedCanonicalizerPass               line 75
094 tensor::createNormalizeTensorOpsPass          line 76
095 createWrapHostFuncPass                        line 252
096 createCSEPass                                 line 72
097 createExtendedCanonicalizerPass               line 75
098 tensor::createNormalizeTensorOpsPass          line 76
099 createHFusionInlineBrcPass                    line 258
100 createHFusionNormalizeOpsPass                 line 261
101 createAddFFTSAddrPass                         line 268
102 createHoistTensorEmptyPass                    line 269
103 createDecomposePass(AFTER_HFUSION_FLATTEN)    line 276
```

### S2 / ConvertToHIVM

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/ConvertToHIVMPipeline.cpp`

```text
104 createHFusionToHIVMConversionPass             line 38
105 createTensorToHIVMConversionPass              line 42
106 createConvertToHIVMOpPass                     line 43
```

Triton 路径会在 104 后插入：

```text
createTritonGlobalKernelArgsToHIVMOpPass          line 40
```

### S3 / HIVM pre-bufferization to first PlanMemory

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
107 createInitEntryKernelPass                     line 438
108 tensor::createPropagateReshapePass(forHIVM)   line 210
109 scf::createRemoveRedundantLoopInitPass        line 211
110 createNormalizeMatmulPass                     line 212
111 createNormalizeConvOpsPass                    line 213
112 createNormalizeBitwiseSelectPass              line 214
113 createInlineFixpipePass                       line 215
114 createInsertLoadStoreForMixCVPass             line 196
115 createInsertLoadStoreForScalarPass            line 198
116 createTileBatchMMIntoLoopPass                 line 219
117 createNormalizeMatmulPass                     line 221
118 createInsertNZ2NDForDebugPass                 line 222
119 createInlineFixpipePass                       line 223
120 createInsertLoadStoreForMixCVPass             line 196
121 createInsertLoadStoreForScalarPass            line 198
122 createInsertWorkSpaceForMixCVPass             line 227
123 createBindWorkSpaceArgPass                    line 228
124 createInferFuncCoreTypePass                   line 231
125 createMarkMultiBufferPass                     line 251
126 createExtendedCanonicalizerPass               line 255
127 createInlineOTFBroadcastPass                  line 256
128 createCVPipeliningPass                        line 262
129 createAutoInferBufferSizePass                 line 144
130 createArithToAffineConversionPass             line 147
131 createConstantizeBufferSizePass               line 148
132 createSetBufferSizePass                       line 149
133 createPlanMemoryPass(GLOBAL_WORKSPACE_PLAN)   line 278
```

### S4 / Between first PlanMemory and bufferization

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
134 createArithToAffineConversionPass             line 79
135 scf::createCanonicalizeIterArgPass            line 80
136 createExtendedCanonicalizerPass               line 81
137 createSCFForLoopCanonicalizationPass          line 82
138 createCSEPass                                 line 83
139 createExtendedCanonicalizerPass               line 84
140 createHIVMOptSinglePointPass                  line 85
141 createExtendedCanonicalizerPass               line 86
142 memref::createDeadStoreEliminationPass        line 87
143 createMarkRealCoreTypePass                    line 119
144 createCrossCoreGSSPass                        line 123
145 createMarkRealCoreTypePass(removeCoreTypeAttrs) line 140
146 createSplitMixKernelPass                      line 293
147 scope::createInlineScopePass                  line 294
148 createTileAndBindSubBlockPass                 line 296
149 tensor::createFoldTensorEmptyPass             line 297
150 createArithToAffineConversionPass             line 79
151 scf::createCanonicalizeIterArgPass            line 80
152 createExtendedCanonicalizerPass               line 81
153 createSCFForLoopCanonicalizationPass          line 82
154 createCSEPass                                 line 83
155 createExtendedCanonicalizerPass               line 84
156 createHIVMOptSinglePointPass                  line 85
157 createExtendedCanonicalizerPass               line 86
158 memref::createDeadStoreEliminationPass        line 87
159 createLoopInvariantCodeMotionPass             line 302
160 createLoopInvariantSubsetHoistingPass         line 303
161 createCloneTensorEmptyPass                    line 306
162 createHIVMInlineOTFLoadStorePass              line 307
```

### S5 / Bufferization

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
163 bufferization::createOneShotBufferizePass     line 169
164 createArithToAffineConversionPass             line 79
165 scf::createCanonicalizeIterArgPass            line 80
166 createExtendedCanonicalizerPass               line 81
167 createSCFForLoopCanonicalizationPass          line 82
168 createCSEPass                                 line 83
169 createExtendedCanonicalizerPass               line 84
170 createHIVMOptSinglePointPass                  line 85
171 createExtendedCanonicalizerPass               line 86
172 memref::createDeadStoreEliminationPass        line 87
173 bufferization::createDropEquivalentBufferResultsPass line 176
174 createArithToAffineConversionPass             line 79
175 scf::createCanonicalizeIterArgPass            line 80
176 createExtendedCanonicalizerPass               line 81
177 createSCFForLoopCanonicalizationPass          line 82
178 createCSEPass                                 line 83
179 createExtendedCanonicalizerPass               line 84
180 createHIVMOptSinglePointPass                  line 85
181 createExtendedCanonicalizerPass               line 86
182 memref::createDeadStoreEliminationPass        line 87
183 bufferization::createDropEquivalentBufferResultsPass line 178
184 createConvertToHIVMOpPass                     line 183
```

### S6 / Post-bufferization to local PlanMemory

位置：
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

```text
185 createLiftZeroRankPass                        line 337
186 scf::createMapForToForallPass                 line 338
187 createHIVMMapForallToBlocksPass               line 339
188 createHIVMDecomposeOpPass                     line 341
189 createSyncBlockHoistingPass                   line 324
190 createBindSyncBlockLockArgPass                line 325
191 createInsertInferSyncBlockLockNumAndInitFuncPass line 327
192 createSyncBlockLockLoweringPass               line 328
193 createNonContiguousReshapeToCopyPass          line 346
194 createInferHIVMMemScopePass                   line 347
195 createHIVMDecomposeOpPass                     line 349
196 createHIVMAggregatedDecomposeOpPass(BEFORE_HIVM_STRIDE_ALIGNMENT) line 355
197 createHIVMRecognizeDeinterleaveOpPass         line 358
198 createHIVMAggregatedDecomposeOpPass(AFTER_RECOGNIZE_DEINTERLEAVE) line 362
199 createHIVMAggregatedDecomposeOpPass(AFTER_RECOGNIZE_BROADCAST) line 366
200 createAlignAllocSizePass                      line 313
201 createMarkStrideAlignPass                     line 315
202 memref::createFoldAllocReshapePass            line 317
203 createEnableStrideAlignPass                   line 318
204 createHIVMAggregatedDecomposeOpPass(AFTER_HIVM_STRIDE_ALIGNMENT) line 373
205 createInferHIVMDataLayoutPass                 line 375
206 createRemoveHIVMDataLayoutAnnotationPass      line 376
207 createHIVMAggregatedDecomposeOpPass(AFTER_INFER_HIVM_DATA_LAYOUT) line 380
208 createExtendedCanonicalizerPass               line 386
209 createAutoInferBufferSizePass                 line 144
210 createArithToAffineConversionPass             line 147
211 createConstantizeBufferSizePass               line 148
212 createSetBufferSizePass                       line 149
213 createFlattenOpsPass                          line 388
214 createHIVMAggregatedDecomposeOpPass(AFTER_HIVM_FLATTEN_OPS) line 392
215 createReduceRankSubviewPass                   line 393
216 createLiftLowestStridePass                    line 394
217 createHIVMAggregatedDecomposeOpPass(AFTER_LIFT_LOWEST_STRIDE) line 398
218 createAllocExtraBufferPass                     line 399
219 createInferHIVMMemScopePass                   line 401
220 createArithToAffineConversionPass             line 79
221 scf::createCanonicalizeIterArgPass            line 80
222 createExtendedCanonicalizerPass               line 81
223 createSCFForLoopCanonicalizationPass          line 82
224 createCSEPass                                 line 83
225 createExtendedCanonicalizerPass               line 84
226 createHIVMOptSinglePointPass                  line 85
227 createExtendedCanonicalizerPass               line 86
228 memref::createDeadStoreEliminationPass        line 87
229 createInlineLoadCopyPass                      line 403
230 createMarkMultiBufferPass                     line 414
231 createDumpIRBeforePlanMemoryPass              line 415
232 createPlanMemoryPass(LOCAL_MEM_PLAN)          line 419
```

`createPlanMemoryPass(LOCAL_MEM_PLAN)` 是第二次 PlanMemory，也是 local UB/L1/L0C
精确规划点。

## 条件插入但不在默认顺序中的 pass

```text
createSymbolDCEPass                              HFusionPipelines.cpp:88
createGPUToHFusionConversionPass                 HFusionPipelines.cpp:89
createAdaptTritonKernelPass                      HFusionPipelines.cpp:90
tensor::createCanonicalizeTensorReshapePass      HFusionPipelines.cpp:297
tensor::createPropagateReshapePass(forHIVM)      HFusionPipelines.cpp:303
tensor::createFoldTensorEmptyPass                HFusionPipelines.cpp:304
tensor::createNormalizeLastDimUnalignedTensorOpPass HFusionPipelines.cpp:306
createTritonGlobalKernelArgsToHIVMOpPass         ConvertToHIVMPipeline.cpp:40
tensor::createOptimizeDpsOpWithYieldedInsertSlicePass HIVMPipelines.cpp:157
createCloneTensorEmptyPass                       HIVMPipelines.cpp:158
createSinkOpToConsumerInLoopPass                 HIVMPipelines.cpp:161
createConvertToHIVMOpPass                        HIVMPipelines.cpp:174
createAutoBlockifyParallelLoopPass               HIVMPipelines.cpp:236
createTileCubeVectorLoopPass                     HIVMPipelines.cpp:267
createInsertInferWorkSpaceSizeFuncPass           HIVMPipelines.cpp:285
createInsertInferTaskTypeFuncPass                HIVMPipelines.cpp:289
createInjectBlockSyncPass                        HIVMPipelines.cpp:132
createCreatePreloadPass                          HIVMPipelines.cpp:426
createGraphSyncSolverPass                        HIVMPipelines.cpp:98
createInjectSyncPass                             HIVMPipelines.cpp:107
```
