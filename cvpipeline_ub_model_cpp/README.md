# CVPipelining UB 使用量模型

这是一个独立的轻量 C++ 工具，用于从 `CVPipelining` 前的 generic IR 建模到
PlanMemory 的 UB buffer、lifetime、offset 和 UB peak。Python 只负责命令封装、
JSON 转换和结果比较，不参与核心计算。

默认演示输入来自统一测试数据集：

```text
cvpipeline_ub_model_cpp/data/before_cvpipelining/triton.language.randn.ttadapter/before_cvpipelining_func_func_kernel_randn_32.mlir
cvpipeline_ub_model_cpp/data/before_cvpipelining/vector_add_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir
```

## 1. 增量构建并比较

一键脚本会依次完成以下工作：

1. 增量构建轻量模型。
2. 增量构建 `bishengir-cvpipeline-suffix-compile`。
3. 分别运行轻量模型和 suffix 编译器。
4. 比较 UB peak，以及 `(extent_bits, offset_bytes)` buffer placement。

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh
```

脚本默认不传 PlanMemory seed。这样 PlanMemory 会保持与原本编译器相同的
retry/attempt 行为。只有需要固定某个 attempt 做定位时，才显式追加
`--random-seed N`。

脚本中的参数分为两组：

```text
CVPipelining 参数：
  --cv-disable-pipelining true|false
  --cv-pipeline-depth N
  --cv-enable-preload true|false
  --cv-enable-lazy-loading true|false

suffix / PlanMemory 参数：
  --suffix-enable-auto-multi-buffer true|false
  --suffix-local-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --suffix-mix-multi-buffer-strategy no-limit|only-cube|only-vector|no-l0c
  --restrict-inplace-as-isa true|false
```

例如，只调整 CVPipelining 的 pipeline 深度：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=cvpipeline_ub_model_cpp/data/before_cvpipelining/triton.language.randn.ttadapter/before_cvpipelining_func_func_kernel_randn_32.mlir \
  --cv-pipeline-depth=4
```

例如，只调整 suffix/PlanMemory 的 multi-buffer 策略：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=cvpipeline_ub_model_cpp/data/before_cvpipelining/vector_add_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --suffix-enable-auto-multi-buffer=true \
  --suffix-local-multi-buffer-strategy=no-limit \
  --suffix-mix-multi-buffer-strategy=only-cube
```

如果本地已经有可用的 suffix 编译器，但不希望脚本再次调用 CMake，可以使用：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh --skip-suffix-build
```

完整参数说明：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh --help
```

脚本中可以编辑的关键变量位于文件开头，包括默认输入、CVPipelining 参数、
suffix/PlanMemory 参数、suffix 编译器路径和输出目录。

## 2. 可视化 demo

运行一键 demo 后，输出按 kernel 名称分目录保存。例如输入文件为
`before_cvpipelining_func_func_flip_kernel_32.mlir` 时，结果位于：

```text
cvpipeline_ub_model_cpp/output/demo/flip_kernel_32/ub_plan_visualizer.html
cvpipeline_ub_model_cpp/output/demo/flip_kernel_32/ub_plan.json
cvpipeline_ub_model_cpp/output/demo/flip_kernel_32/model_stdout.log
cvpipeline_ub_model_cpp/output/demo/flip_kernel_32/suffix_oracle/
```

可以用 `--output-root DIR` 修改所有 kernel 子目录的根路径；显式传入的
`--json`、`--html` 和 `--oracle-dir` 会分别覆盖对应默认路径。

也可以只生成 JSON：

```bash
python3 cvpipeline_ub_model_cpp/scripts/generate_ub_demo_json.py \
  --before-cvpipelining-ir=cvpipeline_ub_model_cpp/data/before_cvpipelining/triton.language.randn.ttadapter/before_cvpipelining_func_func_kernel_randn_32.mlir \
  --output=cvpipeline_ub_model_cpp/output/demo/ub_plan.json
```

页面模板是 `cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html`。打开页面后使用
`Open JSON` 载入新的 JSON；也可以通过 `run_demo_ub_plan.sh
--before-cvpipelining-ir PATH` 重新生成 JSON 和 HTML。

## 3. 单独运行轻量模型

先增量构建模型：

```bash
bash cvpipeline_ub_model_cpp/build.sh
```

直接运行 `CVPipelining` 前边界到 UB plan：

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --before-cvpipelining-ir=cvpipeline_ub_model_cpp/data/before_cvpipelining/triton.language.randn.ttadapter/before_cvpipelining_func_func_kernel_randn_32.mlir \
  --cv-pipeline-depth=-1 \
  --disable-cv-pipelining=false \
  --enable-preload=false \
  --enable-cv-lazy-loading=false \
  --enable-auto-multi-buffer=false \
  --limit-auto-multi-buffer-of-local-buffer no-l0c \
  --limit-auto-multi-buffer-buffer only-cube \
  --format=json
```

上面的命令故意没有设置 seed，保持 PlanMemory 的默认 retry 行为。只有在复现
固定 PlanMemory attempt 时才使用 `--random-seed N`。

主要参数对应关系如下：

```text
输入：
  --before-cvpipelining-ir PATH

CVPipelining：
  --cv-pipeline-depth N
  --disable-cv-pipelining true|false
  --enable-preload true|false
  --enable-cv-lazy-loading true|false

suffix / PlanMemory：
  --enable-auto-multi-buffer true|false
  --limit-auto-multi-buffer-of-local-buffer STRATEGY
  --limit-auto-multi-buffer-buffer STRATEGY
  --restrict-inplace-as-isa

输出：
  --format=text|json
  --output PATH
```

### 调试模式

普通模式只执行 `CVPipelining` 前输入到 `PlanMemory` 的 overflow 主链路，stdout
只包含结果，stderr 不打印阶段日志。需要定位与真实编译器的首个差异时，显式开启：

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --before-cvpipelining-ir=cvpipeline_ub_model_cpp/data/before_cvpipelining/python_tutorial_02-fused-softmax.ttadapter/before_cvpipelining_func_func_softmax_kernel_32.mlir \
  --random-seed=0 \
  --format=json \
  --debug \
  --debug-dir=/tmp/cvub-pass-trace
```

`--debug` 在 stderr 按真实 Pass 名输出关键计数；`--debug-dir` 进一步保存按执行顺序
编号的规范化阶段快照。删掉 `--debug-dir` 时不会构造或写入完整快照。调试日志不改变
stdout 的业务结果，可直接与非 debug 输出逐字节比较。

为缩短逐 Pass 定位时间，开发边界入口也只在 debug 模式开放：

```text
--debug --debug-entry=after-cvpipelining --after-cvpipelining-ir=PATH
--debug --debug-entry=before-plan-memory --before-plan-memory-ir=PATH
```

这些入口用于验证后缀 Pass 或 PlanMemory，不属于产品默认输入契约。

## 4. 单独构建和运行 suffix 编译器

suffix 编译器属于原编译器工程，不是轻量模型的核心实现。使用原编译器的
构建目录进行增量构建：

```bash
cmake --build build --target bishengir-cvpipeline-suffix-compile -j8
```

运行 suffix 编译器时同样不设置 seed：

```bash
build/bin/bishengir-cvpipeline-suffix-compile \
  cvpipeline_ub_model_cpp/data/before_cvpipelining/triton.language.randn.ttadapter/before_cvpipelining_func_func_kernel_randn_32.mlir \
  --mlir-disable-threading \
  --cv-pipeline-depth=-1 \
  --disable-cv-pipelining=false \
  --enable-preload=false \
  --enable-cv-lazy-loading=false \
  --enable-auto-multi-buffer=false \
  --limit-auto-multi-buffer-of-local-buffer no-l0c \
  --limit-auto-multi-buffer-buffer only-cube \
  -o cvpipeline_ub_model_cpp/output/demo/suffix_after_planmemory.mlir
```

完整比较通常直接使用第 1 节的一键脚本，因为它会同时保存 model JSON、
suffix oracle 和比较结果。

## 开发验证

开发工具和单元测试仍保留在 `scripts/` 下：

```bash
bash cvpipeline_ub_model_cpp/scripts/build_dev_tools.sh
bash cvpipeline_ub_model_cpp/scripts/run_unit_tests.sh
bash cvpipeline_ub_model_cpp/scripts/validate_all.sh
python3 cvpipeline_ub_model_cpp/scripts/validate_one_shot_bufferize_output.py \
  --max-inputs=0 --jobs=8
```

`validate_all.sh` 中依赖历史 compiler dump 的测试只有在对应 oracle 存在时运行；
缺失时会明确打印 `SKIP`，不会把 0 个样例记成 PASS。完整 oracle 矩阵仍由后面的
专项脚本负责生成和比较。

最后一条命令用全部 `CVPipelining` 前输入和 15 组 CVPipelining config 检查
比较 OneShotBufferize 交给 HIVMDecomposeOp 的 operation correspondence、
allocation 以及 value/access buffer root。产物默认写入
`Output/experiments/one_shot_bufferize/`；任何 mismatch 或 blocker
都会返回非零状态。省略 `--max-inputs=0` 时，固定均匀抽取 12 个输入用于快速回归。

最终验收固定真实 PlanMemory seed，并分别比较 PlanMemory 输入、lifetime 和 plan
的规范化语义。比较不得丢弃 buffer identity、operation order、offset 或 peak。

## 测试数据

source adapter 和配对的 `CVPipelining` 前输入统一位于：

```text
cvpipeline_ub_model_cpp/data/adapter/
cvpipeline_ub_model_cpp/data/before_cvpipelining/
```

重新生成边界数据：

```bash
bash cvpipeline_ub_model_cpp/scripts/dump_before_cvpipelining_dataset.sh
```

`data/before_cvpipelining/manifest.tsv` 记录每个 adapter 的生成状态、哈希和输出路径。

## 目录说明

```text
src/main.cpp           唯一生产主程序
src/pipeline/          从 CVPipelining 到 PlanMemory 的顺序组装和边界数据
src/passes/            按真实编译器 Pass 命名的轻量实现
src/passes/plan_memory PlanMemory lifetime、storage 和 planner
src/ir/                generic IR 解析和轻量语义表示
src/analysis/          Pass 使用的分析结果
src/support/           通用基础工具
src/tools/             开发期 dump/oracle 入口，不进入生产主线
src/validation/        开发期比较逻辑，不进入生产主线
scripts/               构建、JSON 处理和 oracle 验证
demo/                  HTML 可视化模板
data/                  adapter 和 CVPipelining 前边界配对数据
```

生产主程序只调用
`src/pipeline/cvpipelining_ub_pipeline.hpp`。该文件按真实编译顺序显式调用各个
Pass；新增或排查某个 Pass 时，可以直接从这一调用序列跳转到 `src/passes/` 下的
同名实现。
