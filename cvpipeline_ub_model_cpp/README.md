# CVPipeline UB 使用量模型

这是一个独立的轻量 C++ 工具，用于从 `before-CVPipeline` generic IR 建模到
PlanMemory 的 UB buffer、lifetime、offset 和 UB peak。Python 只负责命令封装、
JSON 转换和结果比较，不参与核心计算。

默认示例输入：

```text
cvpipeline_ub_model_cpp/examples/inputs/demo_vector_add_before_cvpipeline.mlir
cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir
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
CVPipeline 参数：
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

例如，只调整 CVPipeline 的 pipeline 深度：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  --pre-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir \
  --cv-pipeline-depth=4
```

例如，只调整 suffix/PlanMemory 的 multi-buffer 策略：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  --pre-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_vector_add_before_cvpipeline.mlir \
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

脚本中可以编辑的关键变量位于文件开头，包括默认输入、CVPipeline 参数、
suffix/PlanMemory 参数、suffix 编译器路径和输出目录。

## 2. 可视化 demo

运行一键 demo 后，HTML 和 JSON 位于：

```text
cvpipeline_ub_model_cpp/output/demo/ub_plan_visualizer.html
cvpipeline_ub_model_cpp/output/demo/ub_plan.json
```

直接运行：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh
```

也可以只生成 JSON，再替换可视化页面中的数据：

```bash
python3 cvpipeline_ub_model_cpp/scripts/generate_ub_demo_json.py \
  --pre-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir \
  --output=cvpipeline_ub_model_cpp/output/demo/ub_plan.json
```

页面模板是：

```text
cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html
```

打开页面后使用 `Open JSON` 载入新的 JSON。也可以把任意输入路径传给
`run_demo_ub_plan.sh --pre-cvpipeline-ir PATH`，脚本会重新生成 JSON 和 HTML。

## 3. 单独运行轻量模型

先增量构建模型：

```bash
bash cvpipeline_ub_model_cpp/build.sh
```

直接运行 before-CVPipeline 到 UB plan：

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=plan-before-cvpipeline \
  --before-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir \
  --cv-pipeline-depth=-1 \
  --disable-cv-pipelining=false \
  --enable-preload=false \
  --enable-cv-lazy-loading=false \
  --enable-auto-multi-buffer=false \
  --limit-auto-multi-buffer-of-local-buffer no-l0c \
  --limit-auto-multi-buffer-buffer only-cube \
  --format=json
```

上面的命令故意没有设置 seed，保持 PlanMemory 的默认 retry 行为。

主要参数对应关系如下：

```text
输入：
  --before-cvpipeline-ir PATH

CVPipeline：
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

只有在复现固定 PlanMemory attempt 时才使用：

```text
--random-seed N
```

## 4. 单独构建和运行 suffix 编译器

suffix 编译器属于原编译器工程，不是轻量模型的核心实现。使用原编译器的
构建目录进行增量构建：

```bash
cmake --build build --target bishengir-cvpipeline-suffix-compile -j8
```

运行 suffix 编译器时同样不设置 seed：

```bash
build/bin/bishengir-cvpipeline-suffix-compile \
  cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir \
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

suffix 编译器的 PlanMemory dump 会写入终端重定向指定的日志文件。完整比较
通常直接使用第 1 节的一键脚本，因为它会同时保存 model JSON、suffix oracle
和比较结果。

## 目录说明

```text
src/semantic_ir/       generic IR 解析和语义表示
src/cvpipeline/        CVPipelining 相关 UB 语义
src/suffix/             suffix pass 的 UB 相关模拟
src/planmemory/         PlanMemory lifetime、storage 和 planner
src/cli/                生产入口
scripts/                构建封装、JSON 处理和 oracle 比较
demo/                   HTML 可视化模板
examples/inputs/        产品自带示例 IR
```
