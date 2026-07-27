# UB 溢出判定模型

本模型在真实编译的 `CVPipelining` 前读取 Generic MLIR，复刻从
`CVPipelining` 到本地 `PlanMemory` 之间会影响 UB 规划的逻辑，并在真实编译前快速判断
kernel 是否发生 UB overflow。

生产方应直接使用 `precision`、`status` 和 `overflow`。`ub_peak_bits`、
`required_bits`、`capacity_bits` 与 `selected_seed` 用于解释结果；完整 buffer plan 主要用于
开发验证和可视化。

未固定 seed 时，模型按真实 PlanMemory 语义依次尝试 seed 0～19：任一 attempt 得到合法
plan 即为 `success`，20 次全部失败才是 `overflow`。无法精确建模时返回 `blocker`，调用方
必须继续执行真实编译，不能把它当作未溢出。

下面的命令均在仓库根目录执行。

## 1. 构建

构建独立可执行文件和进程内静态库：

```bash
bash ub_overflow_model_cpp/build.sh
```

输出为：

```text
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model
ub_overflow_model_cpp/output/lib/libub_overflow_model.a
```

构建带轻量模型的真实 BiSheng 编译器：

```bash
cmake --build build --target bishengir-compile -j8
```

## 2. 单独使用轻量模型

直接读取一个 before-CVPipelining MLIR：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  ub_overflow_model_cpp/data/before_cvpipelining/vector_add_2x_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --format=json
```

也可以通过标准输入传入 MLIR：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=- \
  --format=json < INPUT.mlir
```

默认执行 seed-retry。只有在定位单个 PlanMemory attempt 时才固定 seed：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --plan-memory-seed=5 \
  --format=json
```

常用参数如下；所有布尔参数均接受 `true/false` 或 `1/0`。

| 参数 | 含义 |
|---|---|
| `--before-cvpipelining-ir=PATH` | 输入 Generic MLIR，`-` 表示 stdin |
| `--format=json\|text` | 输出格式 |
| `--plan-memory-seed=-1\|0..19` | `-1` 为真实 retry，0～19 为固定 seed |
| `--disable-auto-cv-work-space-manage=BOOL` | 关闭自动 CV workspace 管理 |
| `--cv-pipeline-depth=N` | CV pipeline depth，`-1` 表示自动 |
| `--enable-preload=BOOL` | preload |
| `--enable-lazy-loading=BOOL` | CV lazy loading |
| `--enable-code-motion=BOOL` | code motion |
| `--enable-auto-bind-sub-block=BOOL` | 自动 sub-block bind |
| `--tile-mix-cube-loop=N` | MIX Cube loop tile factor |
| `--tile-mix-vector-loop=N` | MIX Vector loop tile factor |
| `--enable-ubuf-saving=BOOL` | UB-saving |
| `--enable-auto-multi-buffer=BOOL` | auto multi-buffer |
| `--enable-hivm-auto-storage-align=BOOL` | storage alignment |
| `--enable-hivm-cross-core-gss=BOOL` | cross-core GSS |
| `--enable-hivm-inject-block-all-sync=BOOL` | InjectBlockSync block-all 分支 |
| `--disable-auto-inject-block-sync=BOOL` | 关闭自动 block sync |
| `--limit-auto-multi-buffer-of-local-buffer=STRATEGY` | local buffer multi-buffer 策略 |
| `--limit-auto-multi-buffer-buffer=STRATEGY` | MIX buffer multi-buffer 策略 |
| `--show-runtime-timing` | 输出总耗时和逐阶段耗时 |
| `--verify-each` | 每个建模阶段后校验 IR |

multi-buffer strategy 可取 `no-limit`、`only-cube`、`only-vector` 或 `no-l0c`。
完整参数以以下命令为准：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model --help
```

### 进程内 C++ 接口

生产热路径应链接 `libub_overflow_model.a`，避免为每个 candidate 启动子进程。公共头文件为
`ub_overflow_model_cpp/include/ub_overflow_model/api.hpp`。

```cpp
#include "ub_overflow_model/api.hpp"

cvub::Request request;
request.compilerProfile = cvub::CompilerProfile::TritonMembaseA2A3;
request.compilerPipelineFingerprint = cvub::kA3MembasePipelineFingerprint;
request.target = "Ascend910_9382";
request.beforeCVPipeliningGenericMLIR = beforeCVPipeliningText;

// 必须传入真实 HIVMPipelineOptions 解析默认值和别名后的最终有效值。
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
    result.status == cvub::Status::Overflow) {
  // 淘汰当前 candidate。
}
// Blocker/InternalError 必须继续真实编译。
```

`evaluate()` 不抛异常、不写临时文件，也不修改真实编译器的 `ModuleOp`。接口和参数合同详见
[AUTOTUNE_INTERFACE_DESIGN.md](AUTOTUNE_INTERFACE_DESIGN.md)。

## 3. 在真实 BiSheng 中使用

A2/A3 Triton membase 路径默认在 `CVPipelining` 前运行轻量模型。模型返回
`Exact + Overflow` 时，当前 compile attempt 不再执行真实 CVPipelining；BiSheng 自己的
fallback 会先关闭 code motion，仍失败时再关闭 auto multi-buffer。模型返回 success、
blocker 或 internal error 时，真实 pipeline 继续运行。

普通编译不打印模型中间日志。需要手工观察控制流时设置：

```bash
BISHENGIR_UB_FLOW_TRACE=1 \
build/bin/bishengir-compile INPUT.ttadapter \
  --enable-hfusion-compile=true \
  --enable-triton-kernel-compile=true \
  -o /tmp/output.o
```

输出按以下层次区分轻量模型、真实 CVPipelining 和 BiSheng fallback：

```text
[UB-FLOW][ATTEMPT N][LIGHTWEIGHT_MODEL][RESULT]
[UB-FLOW][ATTEMPT N][LIGHTWEIGHT_MODEL][OPTIONS]
[UB-FLOW][ATTEMPT N][LIGHTWEIGHT_MODEL][DECISION]
[UB-FLOW][ATTEMPT N][BISHENG_CVPIPELINE][DONE]
[BISHENG][FALLBACK][RETRY]
[BISHENG][FALLBACK][SUMMARY]
```

机器可读摘要使用 `BISHENGIR_UB_MODEL_EMIT_RESULT=1` 显式开启。完全关闭模型可传入
`--enable-ub-overflow-prediction=false`；只运行 shadow、但不提前结束 overflow attempt，
可传入 `--prune-predicted-ub-overflow=false`。

## 4. 可视化 demo

只运行轻量模型并生成 plan JSON 与 HTML：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=ub_overflow_model_cpp/data/before_cvpipelining/vector_add_2x_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --skip-oracle --skip-suffix-build
```

默认输出到 `ub_overflow_model_cpp/output/demo/<kernel>/`。打开其中的
`ub_plan_visualizer.html` 可查看 buffer lifetime、offset 和 UB peak。

## 5. 单个输入对比

当前正确性标准是同一个真实 `bishengir-compile` 进程中的轻量模型与真实本地
PlanMemory，不是历史 suffix 或 cv2pm。单输入、单配置、固定 seed 的命令为：

```bash
python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --input python_tutorial_06-fused-attention.ttadapter \
  --seeds 13 \
  --jobs 1 \
  --report /tmp/ub-model-single.tsv
```

报告比较 status、required、peak、buffer plan、lifetime、multi-buffer 和 inplace。

## 6. 矩阵对比

查看经过选择的参数配置：

```bash
python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py --list
```

运行部分输入和全部 20 个固定 seed：

```bash
python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --config production_default \
  --max-inputs 10 \
  --seeds 0-19 \
  --jobs 8 \
  --report /tmp/ub-model-subset.tsv
```

完整矩阵去掉 `--config` 和 `--max-inputs`。长任务可以加 `--resume` 继续已有报告。

## 7. 时间测量

模型的真实 retry 总时间和各阶段时间：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --plan-memory-seed=-1 \
  --show-runtime-timing \
  --format=json >/tmp/model.json 2>/tmp/model-timing.tsv
```

测量集成后的真实编译时，不要开启 validation、PlanMemory dump 或阶段快照；模型自身耗时
可通过机器摘要中的 `model_ns` 获取：

```bash
BISHENGIR_UB_MODEL_EMIT_RESULT=1 \
/usr/bin/time -p build/bin/bishengir-compile INPUT.ttadapter \
  --enable-hfusion-compile=true \
  --enable-triton-kernel-compile=true \
  -o /tmp/output.o 2>/tmp/bisheng-timing.log
```

开发期的 cv2pm、缓存、差异定位和历史验证方法保留在 `.agent/validation.md`；它们不是产品
使用入口。
