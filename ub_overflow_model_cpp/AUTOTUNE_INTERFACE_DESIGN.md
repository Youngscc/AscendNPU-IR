# UB overflow 模型接入合同

本文描述 `ub_overflow_model_cpp` 与真实 BiSheng 的当前产品接口。语义和默认值以
`ub_overflow_model_cpp/include/ub_overflow_model/api.hpp`、
`bishengir/lib/Dialect/HIVM/Pipelines/UBOverflowPrediction.cpp` 和
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp` 为准。

## 1. 产品边界

当前产品入口位于真实 `AutoBlockify` 前、`InferFuncCoreType` 后：

```text
adapter
  -> 原生 BiSheng HFusion/HIVM 前缀
  -> InferFuncCoreType
  -> UBOverflowPrediction(ModuleOp, resolved HIVMPipelineOptions)
       -> 轻量 AutoBlockify -> before-CV pass 序列
       -> 轻量 CVPipelining -> local PlanMemory
       -> Exact Overflow / Exact Success / fail-open
  -> 原生 AutoBlockify -> CVPipelining -> local PlanMemory（未剪枝时）
```

模型不消费原始 adapter，也不复刻 AutoBlockify 之前的约 61 次原生 pass。接入 pass 借用当前
`ModuleOp`，同步调用模型，不修改也不持有该 ModuleOp。模型核心仍使用项目自有表示，不把
LLVM/MLIR 对象作为跨 pass 的长期表示。

## 2. 版本化合同

公共常量：

| 常量 | 当前值 | 用途 |
|---|---:|---|
| `kInProcessAPIVersion` | 3 | C++ 进程内接口 |
| `kUBRelevantCompileOptionsVersion` | 5 | UB 相关有效参数集合 |
| `kBeforeAutoBlockifyInputContractVersion` | 2 | 当前产品输入边界 |
| `kBeforeCVPipeliningInputContractVersion` | 1 | 旧文本/迁移入口 |
| `kA3MembaseBeforeAutoBlockifyFingerprint` | `bishengir-a3-membase-before-autoblockify-v1` | 当前完整 pass manifest |

生产调用必须显式使用 contract v2 和 before-AutoBlockify fingerprint。`Request` 的默认 contract
仍为 v1，仅用于兼容既有 standalone 输入；不能依赖默认值构造 embedded 产品请求。版本、profile、
fingerprint 或输入边界不匹配时，模型返回 blocker 并 fail open。

## 3. 请求对象

```cpp
cvub::Request request;
request.inputContractVersion =
    cvub::kBeforeAutoBlockifyInputContractVersion;
request.compilerProfile = cvub::CompilerProfile::TritonMembaseA2A3;
request.compilerPipelineFingerprint =
    cvub::kA3MembaseBeforeAutoBlockifyFingerprint;
request.target = resolvedTarget;
request.requestId = requestId;

// 每个值都来自本次真实 HIVMPipelineOptions/CVPipeliningOptions。
request.options.enableTritonKernelCompile = ...;
request.options.enableAutoBlockifyLoop = ...;
request.options.limitAutoMultiBufferOnlyForLocalBuffer = ...;
request.options.workspaceMultiBufferNum = ...;
// 继续填写下表中的其余字段。

const cvub::Result result = cvub::evaluateModule(module, request);
```

`evaluateModule()` 是 embedded 产品入口。`evaluate()` 接受调用方拥有的 Generic MLIR 文本，
用于 LLVM/MLIR 无关的兼容 CLI。两个入口同步返回，均不跨 ABI 抛异常。

### 3.1 必须传递的有效参数

参数必须来自真实编译器已经解析完默认值、别名、正反开关后的同一个 options 对象。不得在模型
一侧重新推导默认值。

| 字段 | 对应编译器参数/来源 | 首次影响阶段 |
|---|---|---|
| `enableTritonKernelCompile` | `enable-triton-kernel-compile` | AutoBlockify gate |
| `enableAutoBlockifyLoop` | `enable-auto-blockify-loop` | AutoBlockify gate |
| `limitAutoMultiBufferOnlyForLocalBuffer` | `limit-auto-multi-buffer-only-for-local-buffer` | pre-CV MarkMultiBuffer |
| `workspaceMultiBufferNum` | `set-workspace-multibuffer` | pre-CV MarkMultiBuffer |
| `cvPipelineDepth` | resolved CV depth | CVPipelining |
| `enableCVLazyLoading` | resolved CV lazy-loading | CVPipelining |
| `enablePreload` | `enable-preload` / skew mode | CVPipelining |
| `enableCodeMotion` | `enable-code-motion` | CVPipelining |
| `enableAutoBindSubBlock` | `enable-auto-bind-sub-block` | Tile/Bind |
| `enableUbufSaving` | `enable-ubuf-saving` | UB-saving |
| `enableAutoMultiBuffer` | `enable-auto-multi-buffer` | multi-buffer |
| `enableHIVMAutoStorageAlign` | `enable-hivm-auto-storage-align` | storage align |
| `tileMixVectorLoop` | `tile-mix-vector-loop` | MIX tiling |
| `tileMixCubeLoop` | `tile-mix-cube-loop` | MIX tiling |
| `localMultiBufferStrategy` | `limit-auto-multi-buffer-of-local-buffer` | local multi-buffer |
| `mixMultiBufferStrategy` | `limit-auto-multi-buffer-buffer` | MIX multi-buffer |
| `disableAutoCVWorkSpaceManage` | `disable-auto-cv-work-space-manage` | pipeline gate/workspace |
| `enableHIVMCrossCoreGSS` | `enable-hivm-cross-core-gss` | sync injection |
| `enableHIVMInjectBlockAllSync` | `enable-hivm-inject-block-all-sync` | sync injection |
| `disableAutoInjectBlockSync` | `disable-auto-inject-block-sync` | sync injection |

`predictionConfig()` 从真实 `HIVMPipelineOptions` 和即将交给原生 CVPipelining 的
`CVPipeliningOptions` 逐字段构造这些值。新增会影响该边界内 UB 行为的原生参数时，必须同时：

1. 增加版本化 API 字段；
2. 从真实 resolved options 映射；
3. 在模型对应 pass 中复刻原生分支；
4. 增加有意义的参数场景和 fixed-seed 差分；
5. 更新 options version 和必要的 pipeline fingerprint。

## 4. 结果合同

调用方首先读取 `precision`、`status` 和 `overflow`：

| 结果 | 含义 | 调用方行为 |
|---|---|---|
| `Exact + Overflow + overflow=true` | 20 个 seed 全部无法得到合法计划 | 可以剪枝当前 compile attempt |
| `Exact + Success + overflow=false` | 至少一个 seed 成功，或严格上界证明不溢出 | 继续候选流程/真实编译 |
| `Incomplete + Blocker` | 输入、语义或原生已知失败无法精确建模 | 必须 fail open |
| `Incomplete + InternalError` | 模型内部错误 | 必须 fail open |

未固定 seed 的生产模式严格复刻原生 retry：依次尝试 seed 0～19，任一成功立即返回 success，
20 次全部失败才返回 overflow。固定 seed 只属于 debug/oracle，不进入生产请求。

`ubPeakBits`、`requiredBits`、`selectedSeed`、`functions`、buffer plan、lifetime、multi-buffer 和
inplace 记录是完整 PlanMemory 路径的解释及验证合同。调用方不得只比较 peak 而忽略其他字段。

### 4.1 提前 non-overflow

生产 `evaluateModule()` 可以在 MarkMultiBuffer 后使用保守上界证明 non-overflow。命中时：

- `precision=Exact`、`status=Success`、`overflow=false`；
- `decisionOnlyNonOverflow=true`；
- `conservativeUpperBoundBits` 有值；
- 不伪造 exact peak、required、selected seed 或完整 plan。

debug/validation 必须继续执行到模型 PlanMemory 和原生 PlanMemory，验证该信号确实正确。结构性能
测量同时关闭提前返回和上界证明计算，不能把 fast-path 收益写成模型基础流水速度。

## 5. BiSheng 中的控制流

产品开关：

```text
--enable-ub-overflow-prediction=true|false
--prune-predicted-ub-overflow=true|false
```

模型仅在 A2/A3 Triton membase 且自动 CV/workspace 管理启用的产品路径插入。启用 prediction、
禁用 prune 时为 observe-only：模型运行，但真实 AutoBlockify、CVPipelining 和 PlanMemory 始终
继续。启用 prune 时，只有 `Exact + Overflow` 会令当前 attempt 失败，并交给 BiSheng 原有
`RetriablePassManager` fallback；模型不实现也不修改 fallback。

普通运行不打印模型日志。手工控制流诊断使用：

```bash
BISHENGIR_UB_FLOW_TRACE=1 \
build/bin/bishengir-compile INPUT.ttadapter \
  --enable-hfusion-compile=true \
  --enable-triton-kernel-compile=true \
  -o /tmp/output.o
```

`BISHENGIR_UB_MODEL_EMIT_RESULT=1` 只打开一行机器摘要。validation、checkpoint、PlanMemory dump、
stage timing 和 native range timing 都是默认关闭的开发功能，不能在普通产品调用中设置。

## 6. Debug 控制

`DebugModelControls` 与生产请求严格分离：

| 字段 | 用途 |
|---|---|
| `fixedPlanMemorySeed` | fixed seeds 0～19 的 oracle 对比 |
| `capacityOverrideBits` | 定向容量边界测试 |
| `collectStageTimings` | 细粒度 stage 诊断，有额外开销 |
| `disableConservativeNonOverflowProof` | 强制执行完整 PlanMemory 的结构性能测量 |
| 其余 disable/restrict 字段 | 兼容实验与定向验证 |

生产 prediction pass 正常调用 `evaluateModule()`。只有 validation 或明确的性能诊断环境才调用
`evaluateModuleForDebug()`。

## 7. 正确性验证

唯一主 oracle 是同一个真实 `bishengir-compile` attempt 中、使用相同 resolved options 的原生
local PlanMemory：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 \
  --jobs 12 \
  --report ub_overflow_model_cpp/output/before_auto_full_correctness.tsv
```

验证模式不会剪枝。每个 seed 独立启动编译器，比较 status、overflow、required、peak、plan、
lifetime、multi-buffer 和 inplace。等尺寸 buffer identity 置换只有在去掉 physical offset 后的
完整 extent、lifetime 与 inplace 图也相等时才单独分类；它不是普通 matched。known timeout、
原生 abort、pass 未插入和 unavailable 必须单列，不能计作通过。

AutoBlockify→before-CV 的每个新增 pass 还必须通过同一原生 attempt 的单 pass checkpoint、累计
checkpoint、定向 fixture 和代表输入最终 PlanMemory。不能只凭最终 peak 相等宣布 pass 对齐。

## 8. 同边界性能测量

正式结构性能比较使用相同的 before-AutoBlockify→local PlanMemory 边界、Release/O3、真实
retry-only，并关闭提前 non-overflow 返回及上界证明：

```bash
.venv/bin/python3 \
  ub_overflow_model_cpp/scripts/measure_before_auto_boundary.py \
  --compiler build/bin/bishengir-compile \
  --rounds 3 \
  --report ub_overflow_model_cpp/output/performance/before_auto_full.tsv \
  --summary ub_overflow_model_cpp/output/performance/before_auto_full.json
```

脚本在同一个 compiler process 中先运行模型，再计时原生 AutoBlockify 到 local PlanMemory，避免
把 adapter 前缀计入任一边界。正式倍率只使用 model/native attempt 数完整配对的样本；fallback、
原生失败、长超时跳过和部分观测均单列。主要结论为：

```text
BiSheng/model = paired native boundary total / paired model internal total
```

同时报告 total、median、mean、p95、max、process wall 和峰值 RSS。细粒度诊断可在小子集上加
`--collect-stage-timings`，并查看 pre-CV prefix 与 CV→PlanMemory 分布；因为计时本身有开销，
该模式不能替代正式倍率。

## 9. 兼容入口

- `build/bin/bishengir-ub-overflow-model --input-stage=before-autoblockify` 使用当前产品合同；
- 不传 `--input-stage` 时仍按 before-CVPipelining v1 解释输入，便于旧 corpus 和 demo；
- `ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model` 是不依赖 MLIR 的文本兼容 CLI；
- 旧 suffix/cv2pm 工具已经退出产品与正确性路线，不应恢复为 oracle。

兼容入口可以保留，但任何产品正确性和速度结论都必须使用真实 embedded before-AutoBlockify
边界。
