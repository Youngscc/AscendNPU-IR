# CVPipelining UB 使用量模型

这是一个独立的轻量 C++ 工具，用于从 `CVPipelining` 前的 generic IR 建模到
PlanMemory 的 UB buffer、lifetime、offset 和 UB peak。Python 只负责命令封装、
JSON 转换和结果比较，不参与核心计算。

产品报告采用 fail-closed 合同：只有 `precision=exact` 的 peak、required 和 buffer
布局可用于 UB 规划；`precision=incomplete` 返回退出码 1，权威字段为空，已有计算值
仅放在 `debug_estimate`。模块中有多个 AIV 函数时会分别规划，模块容量取各函数最大值。

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
  --suffix-enable-triton-kernel-compile true|false
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
  --enable-triton-kernel-compile=false \
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
  --enable-triton-kernel-compile true|false
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

### 运行时统计

轻量模型和 suffix 编译器都提供独立的 `--show-runtime-timing` 开关。计时在各自
进程内部完成，不依赖 Python 的 subprocess 墙钟时间；关闭时不执行阶段计时，也不
改变原有输出。业务结果仍写 stdout，计时记录写 stderr：

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --before-cvpipelining-ir=INPUT.mlir \
  --random-seed=5 --format=json --show-runtime-timing

build/bin/bishengir-cvpipeline-suffix-compile INPUT.mlir \
  --plan-memory-seed=5 --show-runtime-timing -o /tmp/suffix.mlir
```

suffix 开启计时后默认使用 `--runtime-timing-exclude-dumps`：移除阶段快照/调试 IR
Pass，并关闭 PlanMemory TSV 和 `memory_info` 输出。需要测量包含 dump 的旧行为时显式
追加 `--runtime-timing-include-dumps`。最终 `-o` 输出发生在 `TOTAL` 计时结束之后，
本来就不计入。

记录格式固定为：

```text
CVPIPELINE_TIMING  1  model           TOTAL  -           0  <nanoseconds>
CVPIPELINE_TIMING  1  model           STAGE  CVPipelining 1  <nanoseconds>
CVPIPELINE_TIMING  1  suffix_compile  PASS   PlanMemory   1  <nanoseconds>
```

字段实际以 tab 分隔。同名阶段或 Pass 的 `occurrence` 从 1 递增。模型报告逻辑建模
阶段，suffix 编译器通过 MLIR PassInstrumentation 报告每一次真实 Pass；两者的
`TOTAL` 都包含输入解析和完整主链路，但不包含最终结果序列化/文件输出。

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
  --enable-triton-kernel-compile=false \
  --limit-auto-multi-buffer-of-local-buffer no-l0c \
  --limit-auto-multi-buffer-buffer only-cube \
  -o cvpipeline_ub_model_cpp/output/demo/suffix_after_planmemory.mlir
```

完整比较通常直接使用第 1 节的一键脚本，因为它会同时保存 model JSON、
suffix oracle 和比较结果。

## 测试数据

source adapter 和配对的 `CVPipelining` 前输入统一位于：

```text
cvpipeline_ub_model_cpp/data/adapter/
cvpipeline_ub_model_cpp/data/before_cvpipelining/
```

`data/before_cvpipelining/manifest.tsv` 记录每个 adapter 的生成状态、哈希和输出路径。

### Corpus 配置矩阵

常用差分配置保存在 `config/corpus_test_matrix.tsv`，由矩阵 runner 逐行读取。每组
配置固定使用一个 PlanMemory seed（默认 0），不会运行 retry：

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_matrix.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipelining \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seed 0 \
  --quiet
```

使用 `--list` 查看矩阵中的配置；使用一个或多个 `--config NAME` 选择配置；使用
`--dry-run` 检查展开后的命令。当前 25 组配置乘以 171 个输入，共 4275 个差分用例。
矩阵文件的 `required=0` 表示实验性配置：差分仍完整报告，但不阻塞必选矩阵退出码。
corpus runner 会为 suffix compiler 启用 `--ub-oracle-only`：前置流水线保持完整，最终
本地 PlanMemory 跳过 AIC，只收集 AIV/UB 结果。因此 AIC 的 CBUF overflow 不再阻止
UB 差分；若其他非 UB 失败仍使 oracle 不完整，结果才记为 `oracle_unavailable`。

测试时只需要让 runner 收集两个可执行文件自己输出的计时。它会在全部用例结束后
打印双方累计总耗时，并把每个输入的 TOTAL、STAGE 和 PASS 原始记录写入 TSV：

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipelining \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seeds 5 --runtime-timing-output /tmp/cvub-runtime.tsv --quiet
```

suffix 计时默认不包含 oracle/debug dump。runner 对 suffix 执行两次：第一次保留
dump 并用于差分校验，第二次关闭 dump、只采集计时；TSV 中只写第二次的 suffix
计时，因此不会为了取得 oracle 数据而污染编译耗时。若需要把 dump 开销也算进去，
显式追加 `--runtime-timing-include-dumps`，此时只执行一次 suffix。

需要只观察少数 kernel 时，可重复传入 `--input PATH`；相对路径按
`--corpus-root` 解析，TSV 中仍保留每个输入各自的 TOTAL 和明细。runner 会为每个
输入打印横向对照表：TOTAL 和双方同名步骤分别列出 model/suffix occurrence 数、
累计毫秒、差值和倍数。同名步骤的多次执行先求和；无法按名字安全对应的聚合阶段或
真实 Pass 不会强行配对，仍保留在 TSV 中。

需要同时测量 PlanMemory 的默认 seed retry 时追加 `--check-retry`。runner 会先执行
`--seeds` 指定的固定 seed，再执行一次 model 不传 `--random-seed`、suffix 使用
`--plan-memory-seed=-1` 的 retry 路径；计时表中这行的 seed 显示为 `retry`。

只需要 retry、不运行任何固定 seed 时使用 `--retry-only`。该模式忽略 `--seeds`
默认值，每个 kernel 只执行一组 retry，并且不能和 `--check-retry` 同时使用。

矩阵 runner 采用相同默认值，也接受 `--runtime-timing-include-dumps`。它给合并 TSV
增加 `configuration` 列，并且只在所有所选配置结束后打印一次累计耗时。

## 目录说明

```text
src/main.cpp           唯一生产主程序
src/pipeline/          从 CVPipelining 到 PlanMemory 的顺序组装和边界数据
src/passes/            按真实编译器 Pass 命名的轻量实现
src/passes/plan_memory PlanMemory lifetime、storage 和 planner
src/ir/                generic IR 解析和轻量语义表示
src/analysis/          Pass 使用的分析结果
src/support/           通用基础工具
scripts/               构建、JSON 处理和 suffix oracle 比较
demo/                  HTML 可视化模板
data/                  adapter 和 CVPipelining 前边界配对数据
examples/inputs/       两个便于快速运行的示例输入
```

生产主程序只调用
`src/pipeline/cvpipelining_ub_pipeline.hpp`。该文件按真实编译顺序显式调用各个
Pass；新增或排查某个 Pass 时，可以直接从这一调用序列跳转到 `src/passes/` 下的
同名实现。
