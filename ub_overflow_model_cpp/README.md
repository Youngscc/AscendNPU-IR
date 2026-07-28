# UB Overflow 轻量模型使用说明

本目录提供一个在真实 `CVPipelining` 前预测本地 UB 是否溢出的轻量模型。模型复刻
`CVPipelining` 到本地 `PlanMemory` 之间会影响 UB 规划的语义，并以更低成本回答：当前
kernel 在真实 PlanMemory 的 seed-retry 规则下是否必然 overflow。

模型有三种使用方式：

1. 运行独立可执行文件，输入 before-CVPipelining Generic MLIR；
2. 通过 `libub_overflow_model.a` 在同一进程中调用 C++ API；
3. 使用已经接入模型的 `bishengir-compile`，在真实编译路径中自动预测和剪枝。

生产方只应根据 `precision`、`status` 和 `overflow` 作决策。`ub_peak_bits`、
`required_bits`、`selected_seed`、buffer plan 和 lifetime 主要用于解释结果与正确性验证。

## 1. 核心语义

### 1.1 seed-retry

生产模式不固定 seed。模型按照真实 PlanMemory 语义依次尝试 seed 0～19：

- 任一 seed 得到合法 plan：返回 exact non-overflow；
- 20 个 seed 全部失败：返回 exact overflow；
- 遇到无法精确建模的输入或配置：返回 blocker/incomplete，继续真实编译。

因此，性能测量使用 retry-only；正确性测量则必须分别固定并比较全部 20 个 seed。固定单个
seed 只适合定位差异，不能作为完整正确性结论。

### 1.2 可安全剪枝的结果

| `precision` | `status` | `overflow` | 调用方行为 |
|---|---|---:|---|
| `exact` | `overflow` | `true` | 可以终止当前 compile attempt |
| `exact` | `success` | `false` | 当前 attempt 不溢出，继续正常流程 |
| `incomplete` | `blocker` | `unknown` | 必须 fail-open，继续真实编译 |
| `incomplete` | `internal_error` | `unknown` | 必须 fail-open，继续真实编译 |

模型还包含一个安全的 exact non-overflow 快速路径：在 `MarkMultiBuffer` 后，把所有仍存活
的 UB buffer 按独立分配方式计算保守上界。若这个不使用 lifetime reuse 和 inplace 的上界
仍不超过 UB 容量，真实 PlanMemory 必然不会 overflow。该路径只给出 overflow 判定，不伪造
精确 peak、plan、lifetime 或 selected seed。

## 2. 环境与构建

以下命令均在仓库根目录执行：

```bash
cd /path/to/AscendNPU-IR
```

下文 Python 命令使用仓库虚拟环境中的 `.venv/bin/python3`。

### 2.1 构建独立模型和静态库

```bash
bash ub_overflow_model_cpp/build.sh
```

构建脚本使用 C++17 和 `-O3`，产物为：

```text
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model
ub_overflow_model_cpp/output/lib/libub_overflow_model.a
```

### 2.2 构建带模型的 BiSheng

```bash
cmake --build build --target bishengir-compile -j8
```

产物为：

```text
build/bin/bishengir-compile
```

`bishengir-compile` 的 CMake target 会在 build 目录中重新构建并静态链接模型；只要执行上面
的 `cmake --build` 命令，模型源码修改会被纳入编译器。`build.sh` 生成的是独立 CLI 和供外部
进程内调用的 `output/lib/libub_overflow_model.a`，不是 build 目录中同名 CMake target 的替代
品。需要同时使用独立模型和嵌入 BiSheng 时，两条构建命令都执行。

### 2.3 基础自测

```bash
bash ub_overflow_model_cpp/tests/run_tests.sh
```

该命令覆盖 C++ 进程内 API、文件/stdin 输入一致性、关键内部合同以及 Python 报告解析逻辑；
它不是模型与原生 PlanMemory 的全量正确性验证，后者见第 6 节。

## 3. 独立执行轻量模型

### 3.1 最小命令

```bash
MODEL="$PWD/ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model"
INPUT="$PWD/ub_overflow_model_cpp/data/before_cvpipelining/ascend_tutorial_01-vector-add.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir"

"$MODEL" "$INPUT" --format=json
```

也可以显式写输入参数：

```bash
"$MODEL" \
  --before-cvpipelining-ir="$INPUT" \
  --format=json
```

通过 stdin 输入时使用 `-`：

```bash
"$MODEL" \
  --before-cvpipelining-ir=- \
  --format=json < "$INPUT"
```

默认就是生产 seed-retry，不需要额外传 seed 参数。

### 3.2 输出和退出码

JSON 中最重要的字段为：

| 字段 | 含义 |
|---|---|
| `precision` | `exact` 表示结果可用于剪枝；`incomplete` 表示必须继续真实编译 |
| `status` | `success`、`overflow` 或 `blocker` |
| `overflow` | exact 结果中的布尔值；`precision=incomplete` 时不得使用该字段作判断 |
| `ub_peak_bits` | 成功 plan 的 UB peak；非 exact 时为 `null` |
| `required_bits` | overflow 时的需求量，或成功时的实际需求 |
| `capacity_bits` | 当前 UB 容量 |
| `functions[].selected_seed` | retry 中每个函数选中的 seed |
| `functions` | 按函数给出的 buffer plan、lifetime 和 inplace 结果 |
| `diagnostics` | blocker 或错误的解释信息 |

独立程序退出码：

| 退出码 | 含义 |
|---:|---|
| `0` | exact success，即 non-overflow |
| `2` | exact overflow |
| `1` | blocker、参数错误或内部错误 |

脚本调用方不要把非零退出码统一当成 overflow；必须同时解析 `precision/status/overflow`。

### 3.3 固定 seed 定位问题

```bash
"$MODEL" \
  --before-cvpipelining-ir="$INPUT" \
  --plan-memory-seed=5 \
  --format=json
```

`--plan-memory-seed=-1` 与不传该参数等价，表示真实 retry。`0..19` 表示只执行指定 seed。

### 3.4 参数分类

所有布尔参数均接受 `true/false` 或 `1/0`。完整列表始终以 `--help` 为准：

```bash
"$MODEL" --help
```

输入与结果参数：

| 参数 | 含义 |
|---|---|
| `--before-cvpipelining-ir=PATH` | before-CVPipelining Generic MLIR；`-` 表示 stdin |
| `--format=json\|text` | 输出 JSON 或 TSV 风格文本 |
| `--plan-memory-seed=-1\|0..19` | retry-only 或固定 seed |

与真实 CV/UB 逻辑映射的参数：

| 参数 | 含义 |
|---|---|
| `--disable-auto-cv-work-space-manage=BOOL` | 关闭自动 CV workspace 管理 |
| `--cv-pipeline-depth=N` | CV pipeline depth；`-1` 表示自动 |
| `--enable-preload=BOOL` | preload/skew 分支 |
| `--enable-lazy-loading=BOOL` | CV lazy loading |
| `--enable-code-motion=BOOL` | code motion |
| `--enable-auto-bind-sub-block=BOOL` | 自动 sub-block bind |
| `--tile-mix-cube-loop=N` | MIX Cube loop tile factor |
| `--tile-mix-vector-loop=N` | MIX Vector loop tile factor |
| `--enable-ubuf-saving=BOOL` | UB-saving |
| `--enable-auto-multi-buffer=BOOL` | auto multi-buffer |
| `--enable-hivm-auto-storage-align=BOOL` | HIVM storage alignment |
| `--enable-hivm-cross-core-gss=BOOL` | cross-core GSS |
| `--enable-hivm-inject-block-all-sync=BOOL` | InjectBlockSync block-all 分支 |
| `--disable-auto-inject-block-sync=BOOL` | 关闭自动 block sync |
| `--limit-auto-multi-buffer-of-local-buffer=STRATEGY` | local buffer multi-buffer 策略 |
| `--limit-auto-multi-buffer-buffer=STRATEGY` | MIX buffer multi-buffer 策略 |

multi-buffer strategy 可取 `no-limit`、`only-cube`、`only-vector` 或 `no-l0c`。

仅用于开发和诊断的参数：

| 参数 | 用途 |
|---|---|
| `--show-runtime-timing` | 在 stderr 输出总耗时和逐阶段耗时 |
| `--verify-each` | 每个建模阶段后校验中间 IR |
| `--restrict-inplace-as-isa` | 定位 inplace 合同差异 |
| `--disable-cv-pipelining` | 诊断性跳过 CVPipelining |
| `--disable-align-alloc-size` | 诊断 alignment 分支 |
| `--disable-enable-stride-align` | 诊断 stride-align 分支 |
| `--disable-infer-hivm-data-layout` | 诊断 data-layout 分支 |

开发参数不能代替生产参数，也不应出现在正式性能数字中。

## 4. 进程内 C++ API

公共头文件为
`ub_overflow_model_cpp/include/ub_overflow_model/api.hpp`，链接
`ub_overflow_model_cpp/output/lib/libub_overflow_model.a`。

```cpp
#include "ub_overflow_model/api.hpp"

cvub::Request request;
request.compilerProfile = cvub::CompilerProfile::TritonMembaseA2A3;
request.compilerPipelineFingerprint = cvub::kA3MembasePipelineFingerprint;
request.target = "Ascend910_9382";
request.beforeCVPipeliningGenericMLIR = beforeCVPipeliningText;

// 这里必须传真实 HIVMPipelineOptions 在默认值、别名和 disable_* 解析后的有效值。
request.options.disableAutoCVWorkSpaceManage = disableAutoCVWorkspaceManage;
request.options.cvPipelineDepth = cvPipelineDepth;
request.options.enableCVLazyLoading = enableCVLazyLoading;
request.options.enablePreload = enablePreload;
request.options.enableCodeMotion = enableCodeMotion;
request.options.enableAutoBindSubBlock = enableAutoBindSubBlock;
request.options.enableUbufSaving = enableUbufSaving;
request.options.enableAutoMultiBuffer = enableAutoMultiBuffer;
request.options.enableHIVMAutoStorageAlign = enableHIVMAutoStorageAlign;
request.options.enableHIVMCrossCoreGSS = enableHIVMCrossCoreGSS;
request.options.enableHIVMInjectBlockAllSync = enableHIVMInjectBlockAllSync;
request.options.disableAutoInjectBlockSync = disableAutoInjectBlockSync;
request.options.tileMixVectorLoop = tileMixVectorLoop;
request.options.tileMixCubeLoop = tileMixCubeLoop;
request.options.localMultiBufferStrategy =
    cvub::MultiBufferStrategy::CubeNoL0C;
request.options.mixMultiBufferStrategy =
    cvub::MultiBufferStrategy::OnlyCube;

const cvub::Result result = cvub::evaluate(request);
if (result.precision == cvub::Precision::Exact &&
    result.status == cvub::Status::Overflow &&
    result.overflow.value_or(false)) {
  // 可以淘汰当前 candidate。
} else {
  // Success、Blocker 和 InternalError 都继续真实编译。
}
```

`evaluate()` 同步执行、不向库边界外抛异常、不写临时文件，也不修改调用方的真实
`ModuleOp`。`std::string_view` 形式的输入只需在本次同步调用期间有效。固定 seed、容量覆盖和
逐阶段 timing 位于 `evaluateForDebug()`，生产 prediction pass 不应调用 debug 接口。

完整接口合同见 [AUTOTUNE_INTERFACE_DESIGN.md](AUTOTUNE_INTERFACE_DESIGN.md)。

## 5. 在真实 BiSheng 中执行

A2/A3 Triton membase 路径会在真实 `CVPipelining` 前运行模型。普通编译默认不打印轻量
模型调试日志。

```bash
COMPILER="$PWD/build/bin/bishengir-compile"
ADAPTER="$PWD/ub_overflow_model_cpp/data/adapter/ascend_tutorial_01-vector-add.ttadapter"

"$COMPILER" "$ADAPTER" \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  -o /tmp/vector-add.o
```

模型的真实控制流如下：

- `Exact + Overflow`：当前 attempt 在真实 CVPipelining 前失败；BiSheng 原有 fallback 决定
  是否关闭 code motion、auto multi-buffer 后重试；
- exact non-overflow：继续真实 CVPipelining 和后续 pipeline；
- blocker/internal error：fail-open，继续真实 pipeline；
- `--disable-auto-cv-work-space-manage=true`：当前产品路径不会插入 prediction pass。

### 5.1 手工观察执行过程

```bash
BISHENGIR_UB_FLOW_TRACE=1 \
"$COMPILER" "$ADAPTER" \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  -o /tmp/vector-add.o
```

日志层级为：

```text
[UB-FLOW][ATTEMPT N][LIGHTWEIGHT_MODEL][RESULT]
[UB-FLOW][ATTEMPT N][LIGHTWEIGHT_MODEL][OPTIONS]
[UB-FLOW][ATTEMPT N][LIGHTWEIGHT_MODEL][DECISION]
[UB-FLOW][ATTEMPT N][BISHENG_CVPIPELINE][DONE]
[BISHENG][FALLBACK][RETRY]
[BISHENG][FALLBACK][SUMMARY]
```

这只是人工诊断开关，正式性能测试必须关闭。

### 5.2 机器可读结果

```bash
BISHENGIR_UB_MODEL_EMIT_RESULT=1 \
"$COMPILER" "$ADAPTER" \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  --prune-predicted-ub-overflow=false \
  -o /tmp/vector-add.o 2>/tmp/ub-model-result.log

grep '^BISHENGIR_UB_MODEL_RESULT ' /tmp/ub-model-result.log
```

摘要中常用字段：

| 字段 | 含义 |
|---|---|
| `status/precision/overflow` | 产品判定 |
| `decision_path` | `full_plan`、`non_overflow_upper_bound` 等决策路径 |
| `serialize_ns` | 当前 `ModuleOp` 打印为 Generic MLIR 的时间 |
| `model_ns` | `evaluate()` 内部时间 |
| `input_digest/options_digest` | 输入和有效参数指纹，只用于关联结果 |
| `diagnostic_category` | 稳定的 blocker/error 分类 |

当前集成下，一次 prediction 的内部成本定义为：

```text
prediction_ns = serialize_ns + model_ns
```

### 5.3 控制模型行为

完全关闭模型：

```bash
"$COMPILER" "$ADAPTER" \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  --enable-ub-overflow-prediction=false \
  -o /tmp/vector-add-without-model.o
```

只观察模型、不因 predicted overflow 中止真实 pipeline：

```bash
"$COMPILER" "$ADAPTER" \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  --enable-ub-overflow-prediction=true \
  --prune-predicted-ub-overflow=false \
  -o /tmp/vector-add-shadow.o
```

默认产品行为是模型开启且允许 exact overflow 剪枝。不要在普通使用中设置
`BISHENGIR_UB_MODEL_VALIDATION`、PlanMemory dump 或强制 seed 环境变量。

## 6. 正确性测试

正确性标准不是缓存文件，而是同一个真实 `bishengir-compile` 进程中：

```text
同一 adapter + 同一有效参数 + 同一固定 seed
  -> embedded 轻量模型完整结果
  -> 同一次执行继续到原生本地 PlanMemory
  -> 比较 status / required / peak / plan / lifetime / multi / inplace
```

验证模式会关闭 predicted-overflow 剪枝。即使轻量模型提前证明 non-overflow，也会继续构造
完整模型结果，并继续执行原生 PlanMemory，验证“提前判定”与最终完整结果同时正确。

### 6.1 查看 27 组参数场景

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py --list
```

参数定义保存在
`ub_overflow_model_cpp/config/ub_relevant_parameter_scenarios.tsv`。这些是有意义的组合，不是所有
参数的笛卡尔积，覆盖 production default、preload、code motion、sub-block bind、storage
alignment、UB-saving、MIX tiling、auto multi-buffer、workspace 策略和 block sync 分支。

### 6.2 单个输入完整验证

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --input python_tutorial_06-fused-attention.ttadapter \
  --seeds 0-19 \
  --jobs 1 \
  --timeout 360 \
  --report /tmp/ub-model-single.tsv
```

如果只想定位某个 seed，可临时改成 `--seeds 13`；脚本会提示该结果只用于诊断。

### 6.3 约十分钟的代表子集

下面的命令覆盖默认配置、preload、auto multi-buffer、UB-saving 和 block sync，并选择 vector、
matrix、attention 与 MIX 类输入。实际耗时取决于机器和并发度。

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --config preload \
  --config auto_mb_default \
  --config ubuf_saving_auto_mb \
  --config inject_block_normal \
  --input ascend_tutorial_01-vector-add.ttadapter \
  --input ascend_tutorial_03-matrix-multiplication.ttadapter \
  --input python_tutorial_06-fused-attention.ttadapter \
  --input attn_fwd.ttadapter \
  --seeds 0-19 \
  --jobs 8 \
  --timeout 360 \
  --report /tmp/ub-model-representative.tsv
```

### 6.4 单配置、160 输入

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --seeds 0-19 \
  --jobs 8 \
  --timeout 360 \
  --report /tmp/ub-model-production-default.tsv
```

规模为 `160 × 20 = 3200` 行。

### 6.5 完整 27 × 160 × 20

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 \
  --jobs 8 \
  --timeout 360 \
  --report /tmp/ub-model-full.tsv
```

理论规模为 `27 × 160 × 20 = 86400` 行。默认会跳过
`ub_overflow_model_cpp/config/known_timeout_pairs.tsv` 中已确认的原生慢组合，所以启动摘要中的
实际 `total` 可能小于 86400。当前清单排除了 66 个 `(scenario, adapter)` 组合，因此默认
实际执行 `85080` 行。若明确要重新执行这些组合，可加 `--include-known-timeouts`。

任务中断后从已有 TSV 继续：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 \
  --jobs 8 \
  --timeout 360 \
  --report /tmp/ub-model-full.tsv \
  --resume
```

`--resume` 以 `(scenario, adapter, seed)` 为键跳过已经写入报告的行。报告逐行落盘，因此进程
被中断时已完成部分仍可保留。

### 6.6 如何读取正确性报告

| `status` | 含义 | 处理方式 |
|---|---|---|
| `matched` | 模型与原生 PlanMemory 合同一致 | 通过 |
| `different` | 至少一个可比较字段不同 | 必须分析和修复 |
| `unavailable` | 当前配置没有形成可比较的两侧结果 | 单列原因，不能算 matched |
| `timeout` | 单次真实编译超过 `--timeout` | 调大超时或作为已知慢组合单列 |

`differences` 指明差异字段，`evidence` 给出模型/原生的简要证据。提前 non-overflow 证明相关
字段为：

- `non_overflow_upper_bound_proven=true`：模型观察到了安全上界；
- `decision_paths=full_plan_after_non_overflow_upper_bound`：验证模式在证明后仍完成了模型 plan；
- `native_plan_memory_observed=true`：同次编译确实执行了原生 PlanMemory；
- `non_overflow_proof_verified=true`：两侧完整结果验证该快速判定没有误报。

脚本只在所有行都是 `matched` 时返回 0。`different`、`unavailable` 或 `timeout` 都会返回 1，
避免它们被静默计入通过数。

其中 `cv_workspace_manage_off` 和 `cv_workspace_manage_off_auto_mb` 是特殊的激活边界：真实产品
在 `disable-auto-cv-work-space-manage=true` 时不会插入 prediction pass，所以 embedded 对比
没有模型侧结果，报告为 `unavailable` 是预期行为。完整矩阵的判断标准应写成“所有可比较行
matched、没有未解释的 different；unavailable/timeout 按场景和原生诊断单列”，不能把预期
unavailable 改写成 matched，也不能因为脚本返回 1 就直接判断模型逻辑错误。

并发度不是越大越好。`--jobs 8` 是常用起点；若内存压力明显或系统开始 kill 子进程，先降到
4，而不是把 kill 当成模型差异。

## 7. 时间成本与优化收益测试

当前性能口径只比较轻量模型自身的单轮速度，不使用 autotune 总时间，也不把模型剪枝后省掉
的原生 compiler 时间算作模型实现的加速。统一条件为：

- 同一台机器、同一输入集合、同一组有效参数；
- 模型和 `bishengir-compile` 都使用优化构建；
- 生产 retry-only，不固定 seed；
- 关闭 validation、dump、flow trace、逐 pass artifact；
- 预热后交错执行 A/B，至少 3 轮；
- 正确性仍单独执行 20 个固定 seed，不能用性能运行代替。

### 7.1 指标定义

| 指标 | 定义 | 用途 |
|---|---|---|
| `serialize_ns` | `ModuleOp` 到 Generic MLIR 的序列化时间 | 当前集成边界成本 |
| `model_ns` | `evaluate()` 内部执行时间 | 模型核心成本 |
| `prediction_ns` | `serialize_ns + model_ns` | 当前产品路径的模型总内部成本 |
| `process_wall_ns` | 进程启动、真实前缀、模型、原生后缀到本地 PlanMemory | 集成观察值，不等于模型时间 |
| `max_rss_bytes` | 当前编译子进程的峰值 RSS | 内存成本参考 |

正式比较模型优化收益时使用 `prediction_ns`。`process_wall_ns` 包含大量共同的真实 BiSheng
工作，只用于确认集成后没有异常系统回退。

### 7.2 单个 kernel 的逐阶段耗时

独立模型可以输出真实 retry 的总时间和每个建模阶段时间：

```bash
MODEL="$PWD/ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model"
INPUT="$PWD/ub_overflow_model_cpp/data/before_cvpipelining/python_tutorial_06-fused-attention.ttadapter/before_cvpipelining_func_func_attn_fwd_32.mlir"

"$MODEL" \
  --before-cvpipelining-ir="$INPUT" \
  --plan-memory-seed=-1 \
  --show-runtime-timing \
  --format=json \
  >/tmp/ub-model-result.json \
  2>/tmp/ub-model-stages.tsv

grep '^CVPIPELINE_TIMING' /tmp/ub-model-stages.tsv
```

stderr 格式为：

```text
CVPIPELINE_TIMING  1  model  TOTAL  -          0  <nanoseconds>
CVPIPELINE_TIMING  1  model  STAGE  <pass>     N  <nanoseconds>
```

同名 stage 可能执行多次，`N` 是 occurrence。按 pass 比较时要先对同一输入、同一轮中的同名
stage 求和，再横向比较两个版本。`--show-runtime-timing` 本身会增加少量记录成本，因此它用于
热点分析；正式总时间使用下一节的 production machine result。

### 7.3 单个 kernel 的生产路径成本

```bash
COMPILER="$PWD/build/bin/bishengir-compile"
ADAPTER="$PWD/ub_overflow_model_cpp/data/adapter/python_tutorial_06-fused-attention.ttadapter"

BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY=1 \
BISHENGIR_UB_MODEL_EMIT_RESULT=1 \
/usr/bin/time -p "$COMPILER" "$ADAPTER" \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  --mlir-disable-threading \
  --enable-ub-overflow-prediction=true \
  --prune-predicted-ub-overflow=false \
  -o /dev/null \
  2>/tmp/ub-model-one-kernel.log

grep '^BISHENGIR_UB_MODEL_RESULT ' /tmp/ub-model-one-kernel.log
grep '^real ' /tmp/ub-model-one-kernel.log
```

这里使用 shadow 模式，是为了让 overflow 和 non-overflow 样本都继续到相同的原生本地
PlanMemory 边界。机器摘要中的 `serialize_ns + model_ns` 是模型成本；`real` 是更宽的编译
区间，二者不能混用。

### 7.4 160 输入的当前版本测量

仓库提供 `measure_embedded_model.py`，它会：

1. 运行真实 BiSheng prefix 并在真实 pre-CVPipelining pass 中调用生产 `evaluate()`；
2. 使用 retry-only，关闭 validation、dump、trace 和 fixed seed；
3. 使用 shadow 模式继续到原生本地 PlanMemory，保证所有样本终点一致；
4. 预热后按输入执行多轮；
5. 输出每个算子的 TSV 和聚合 JSON。

完整 160 输入、3 轮：

```bash
mkdir -p ub_overflow_model_cpp/output/performance

.venv/bin/python3 ub_overflow_model_cpp/scripts/measure_embedded_model.py \
  --variant current="$PWD/build/bin/bishengir-compile" \
  --rounds 3 \
  --warmup-inputs 8 \
  --timeout 120 \
  --report ub_overflow_model_cpp/output/performance/current.tsv \
  --summary ub_overflow_model_cpp/output/performance/current.json
```

先快速测 10 个输入：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/measure_embedded_model.py \
  --max-inputs 10 \
  --rounds 3 \
  --warmup-inputs 3 \
  --report /tmp/ub-model-perf-10.tsv \
  --summary /tmp/ub-model-perf-10.json
```

只测指定算子，可重复传 `--input`：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/measure_embedded_model.py \
  --input python_tutorial_06-fused-attention.ttadapter \
  --input attn_fwd.ttadapter \
  --input ascend_tutorial_09-persistent-matmul.ttadapter \
  --rounds 5 \
  --warmup-inputs 3 \
  --report /tmp/ub-model-perf-selected.tsv \
  --summary /tmp/ub-model-perf-selected.json
```

TSV 一行对应 `(variant, round, adapter)`。常用分析方式：

- 按 `adapter` 排序 `prediction_ns`，找最慢 kernel；
- 比较 `serialize_ns` 和 `model_ns`，判断时间在集成边界还是模型内部；
- 按 `decision_path` 分组，检查 non-overflow 快速路径的命中与耗时；
- 先确认 `measurement_status=observed`，再统计时间；
- `unavailable/timeout` 不参与性能均值，必须单独报告数量和原因。

### 7.5 两个模型版本交错 A/B

需要两个分别链接了旧/新模型的 `bishengir-compile`。推荐使用两个 worktree 和独立 build
目录；也可以在每个版本构建后把完整 `bishengir-compile` 保存到不同绝对路径。两个二进制
必须采用相同构建类型和编译选项。

```bash
BASELINE_COMPILER=/absolute/path/to/baseline/build/bin/bishengir-compile
CURRENT_COMPILER=/absolute/path/to/current/build/bin/bishengir-compile

.venv/bin/python3 ub_overflow_model_cpp/scripts/measure_embedded_model.py \
  --variant baseline="$BASELINE_COMPILER" \
  --variant current="$CURRENT_COMPILER" \
  --rounds 5 \
  --warmup-inputs 8 \
  --timeout 120 \
  --report /tmp/ub-model-ab.tsv \
  --summary /tmp/ub-model-ab.json
```

脚本会按 round 和 input 交替变体顺序，避免“先测完旧版本、再测新版本”造成温度、频率和系统
负载偏差。建议报告：

- 每个版本各轮 `prediction_total_ns`；
- 各轮总时间的中位数；
- 每输入 `prediction_ns` 的 median、p95、maximum；
- `serialize_total_ns` 与 `model_total_ns`；
- 最慢 kernel 的逐输入对比；
- unavailable/timeout 数量；
- 峰值 RSS。

聚合收益计算为：

```text
speedup       = median(baseline round total) / median(current round total)
saved_time    = median(baseline round total) - median(current round total)
reduction_pct = saved_time / median(baseline round total) * 100%
```

可直接从 summary JSON 计算：

```bash
SUMMARY=/tmp/ub-model-ab.json .venv/bin/python3 - <<'PY'
import json
import os
import statistics

with open(os.environ["SUMMARY"], encoding="utf-8") as stream:
    data = json.load(stream)

def median_total(name):
    rounds = data["variants"][name]["rounds"]
    return statistics.median(row["prediction_total_ns"] for row in rounds)

baseline = median_total("baseline")
current = median_total("current")
print(f"baseline_median_total_ms={baseline / 1e6:.3f}")
print(f"current_median_total_ms={current / 1e6:.3f}")
print(f"speedup={baseline / current:.3f}x")
print(f"reduction_pct={(baseline - current) / baseline * 100:.2f}%")
PY
```

不要只报告某一个 pass 的加速比，也不要用少数快速 kernel 掩盖 attention/MIX/overflow slow
path 的回退。总量、分布和最慢输入应一起给出。

### 7.6 关于“剪枝收益”

模型自身的优化收益与产品剪枝收益是两个不同问题：

- 模型自身收益：旧模型与新模型的 `prediction_ns` A/B，是本项目当前性能验收口径；
- 产品剪枝收益：exact overflow 发生后少执行了多少真实 compiler 工作，受候选分布、fallback
  次数和外部编译环境影响。

若需要观察产品行为，可以在同一完整编译 workload 上比较
`--enable-ub-overflow-prediction=false` 与默认 prune 模式的总 wall time，但必须单独报告，不能
用这部分节省抵消模型自身变慢。完整编译还要求 `hivmc` 等真实工具链在 `PATH` 中；第 7.4、
7.5 节的模型性能测量在本地 PlanMemory 后停止，不依赖后端 codegen。

## 8. 可视化 plan

只运行轻量模型并生成 plan JSON 与 HTML：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir="$PWD/ub_overflow_model_cpp/data/before_cvpipelining/ascend_tutorial_01-vector-add.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir"
```

默认输出到 `ub_overflow_model_cpp/output/demo/<kernel>/`。打开其中的
`ub_plan_visualizer.html` 可查看 buffer lifetime、offset 和 UB peak。demo、dump、缓存、性能
报告和整个 `ub_overflow_model_cpp/output/` 都是可再生成产物，不应提交到 Git。

## 9. 常见问题

### 为什么性能测试不用 seeds 0～19？

生产行为是 retry-only：找到第一个合法 seed 就停止，20 个都失败才报告 overflow。依次固定
20 个 seed 会强制完成生产中本来不会执行的 attempt，测到的是正确性验证成本，不是产品成本。

### 为什么正确性测试不能只用 retry-only？

retry-only 只验证最终布尔结果，可能掩盖某些 seed 的 plan、lifetime、multi-buffer 或 inplace
差异。完整复刻需要 20 个固定 seed 分别与原生 PlanMemory 对比。

### 为什么 `model_ns` 和 `/usr/bin/time` 差很多？

`model_ns` 只包含 `evaluate()`；`serialize_ns` 是进入模型前的 Generic MLIR 序列化；进程 wall
还包含进程启动、adapter 解析、真实 BiSheng 前缀和后续本地 PlanMemory。比较模型速度时用
`serialize_ns + model_ns`，不要用整个 compiler wall 代替。

### 为什么没有 `BISHENGIR_UB_MODEL_RESULT`？

先检查：

1. 是否设置了 `BISHENGIR_UB_MODEL_EMIT_RESULT=1`；
2. 是否启用了 Triton kernel compile；
3. 是否误传 `--enable-ub-overflow-prediction=false`；
4. 是否启用了 `--disable-auto-cv-work-space-manage=true`，该分支不会插入 prediction pass；
5. 当前 `bishengir-compile` 是否在模型静态库更新后重新链接。

### blocker 是否等于 non-overflow？

不等于。blocker 表示模型无法给出 exact 结论。独立工具会以退出码 1 返回；嵌入编译器时必须
fail-open，继续执行真实 pipeline。

### 测试时需要打开 dump 吗？

不需要。正确性脚本会打开它所需的同进程 PlanMemory 观测；性能脚本会主动关闭 validation、
dump 和 trace。手工性能测试也不应设置 `BISHENGIR_DUMP_*`。
