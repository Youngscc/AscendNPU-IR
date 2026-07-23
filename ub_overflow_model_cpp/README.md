# UB 溢出判定模型

这个工具在真实编译前读取 `CVPipelining` 前的 Generic MLIR，精确模拟所有会影响
PlanMemory 的变换和默认 seed-retry，判断当前 kernel 的 UB 规划是否会 overflow。
`CVPipelining` 和 `MultiBuffer` 是必须建模的内部阶段，不是产品的最终结果。

对外名称为 **UB Overflow Model（UB 溢出判定模型）**，主可执行文件为
`bishengir-ub-overflow-model`。源码目录名为 `ub_overflow_model_cpp`；构建
同时生成兼容入口 `cvpipeline_ub_model`，避免一次重命名破坏已有调用方。

生产调用最重要的结果是 `precision/status/overflow`。`ub_peak_bits`、
`required_bits` 和 `selected_seed` 用于解释判定；完整 buffer、offset、lifetime、inplace
结果主要服务于开发定位、可视化和 suffix 一致性验证。

## 结果语义

overflow 不是简单判断 `ub_peak_bits > capacity_bits`。当前精确语义是：

1. PlanMemory 按与 suffix 一致的规则计算 lifetime、inplace、multi-buffer 和地址；
2. 未固定 seed 时依次尝试 seed 0～19，并在第一次成功时停止；
3. 任一 attempt 能在 UB 容量内得到合法 plan，结果就是 `success`；
4. 20 个 attempt 全部失败，结果才是 `overflow`；
5. 模型无法精确覆盖输入时返回 `blocker`，不能把 blocker 当作未溢出。

| 字段 | 含义 |
|---|---|
| `precision` | `exact` 表示可以用于判定；`incomplete` 表示必须 fail closed |
| `status` | `success`、`overflow` 或 `blocker` |
| `overflow` | 精确规划是否在全部 retry 后仍失败 |
| `ub_peak_bits` | 最终选中 plan 中实际 buffer 末端的最大值，主要用于诊断 |
| `required_bits` | PlanMemory 按 256-bit 分配对齐得到的规划空间，是容量判定的重要依据 |
| `capacity_bits` | 当前目标 UB 容量，默认 192 KB |
| `selected_seed` | 默认 retry 最终选中的 seed；固定 seed 时就是指定值 |

由于分配对齐，`ub_peak_bits` 与 `required_bits` 不保证相等。因此生产方应直接消费
`status/overflow`，不要在外部重新实现 `ub_peak_bits > capacity_bits` 判定。

下面所有命令都在仓库根目录执行。

## 1. 构建

### 构建模型

```bash
bash ub_overflow_model_cpp/build.sh
```

生成文件：

```text
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model
```

### 构建 suffix 对照工具

```bash
cmake --build build --target bishengir-cvpipeline-suffix-compile -j8
```

生成文件：

```text
build/bin/bishengir-cvpipeline-suffix-compile
```

单个输入对比脚本也会增量构建模型和 suffix。如果两个程序已经构建好，可以给脚本
传入 `--skip-suffix-build`，避免再次调用 CMake。

## 2. 单独使用 UB 溢出判定模型

UB 溢出判定模型是本目录的主要产品。它不需要运行 suffix，也不依赖 Python 完成核心
计算。当前兼容接口仍会输出完整 plan；生产集成只需要读取顶层
`precision/status/overflow/ub_peak_bits/required_bits/capacity_bits`。

最简单的运行方式：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=ub_overflow_model_cpp/data/before_cvpipelining/vector_add_2x_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --format=json
```

结果写到 stdout。需要保存时直接重定向：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --format=json > /tmp/ub-plan.json
```

默认不固定 seed，PlanMemory 会执行正常的 seed-retry 并返回最终选择的 plan。只在定位
某个 attempt 时固定 seed：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --random-seed=5 \
  --format=json
```

### 模型参数

| 参数 | 说明 |
|---|---|
| `--before-cvpipelining-ir=PATH` | 输入的 CVPipelining 前 Generic MLIR |
| `--format=json\|text` | 输出 JSON 或文本结果 |
| `--random-seed=0..19` | 固定 seed；省略时运行默认 retry |
| `--disable-cv-pipelining=BOOL` | CVPipelining 开关 |
| `--cv-pipeline-depth=N` | pipeline depth；`-1` 表示自动选择 |
| `--enable-preload=BOOL` | preload 开关 |
| `--enable-cv-lazy-loading=BOOL` | lazy loading 开关 |
| `--enable-code-motion=BOOL` | code motion 开关 |
| `--tile-mix-cube-loop=N` | MIX CUBE loop tile factor |
| `--tile-mix-vector-loop=N` | MIX VECTOR loop tile factor |
| `--enable-ubuf-saving=BOOL` | UB-saving 开关 |
| `--enable-auto-multi-buffer=BOOL` | auto multi-buffer 开关 |
| `--enable-triton-kernel-compile=BOOL` | Triton kernel 路径开关 |
| `--disable-align-alloc-size=BOOL` | 是否关闭 AlignAllocSize |
| `--disable-enable-stride-align=BOOL` | 是否关闭 EnableStrideAlign |
| `--disable-infer-hivm-data-layout=BOOL` | 是否关闭 data-layout 推断 |
| `--limit-auto-multi-buffer-of-local-buffer=STRATEGY` | local buffer 策略 |
| `--limit-auto-multi-buffer-buffer=STRATEGY` | MIX buffer 策略 |
| `--restrict-inplace-as-isa` | 使用严格 inplace 规则 |
| `--verify-each` | 每个建模 pass 后校验 Generic IR |
| `--show-runtime-timing` | 将总时间和各阶段时间写到 stderr |

multi-buffer strategy 可取 `no-limit`、`only-cube`、`only-vector` 或 `no-l0c`。

例如开启 auto multi-buffer：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --enable-auto-multi-buffer=true \
  --limit-auto-multi-buffer-of-local-buffer=no-limit \
  --limit-auto-multi-buffer-buffer=only-cube \
  --format=json
```

需要保存逐阶段调试快照时：

```bash
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --format=json \
  --debug --debug-dir=/tmp/cvub-debug
```

## 3. 使用可视化 demo

demo 使用轻量模型生成 UB plan JSON 和可视化 HTML。只运行模型、不执行 suffix：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=ub_overflow_model_cpp/data/before_cvpipelining/vector_add_2x_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --skip-oracle --skip-suffix-build
```

默认输出到：

```text
ub_overflow_model_cpp/output/demo/<kernel>/ub_plan.json
ub_overflow_model_cpp/output/demo/<kernel>/ub_plan_visualizer.html
ub_overflow_model_cpp/output/demo/<kernel>/model_stdout.log
```

使用 `--output-root DIR` 修改输出根目录，也可以分别使用 `--json PATH` 和
`--html PATH` 指定文件。打开生成的 HTML 即可查看 buffer lifetime、offset 和 UB peak。

## 4. 单个输入与 suffix 对比

去掉 `--skip-oracle` 后，demo 脚本会同时运行轻量模型和 suffix，并比较两边的 UB
结果：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=ub_overflow_model_cpp/data/before_cvpipelining/vector_add_2x_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --skip-suffix-build
```

默认比较正常 seed-retry 的最终结果。固定 seed 对比：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=INPUT.mlir \
  --random-seed=5 \
  --skip-suffix-build
```

脚本参数与模型参数一一对应，但为避免与 demo 自身选项冲突，CVPipelining 参数使用
`--cv-*` 前缀，其余编译参数使用 `--suffix-*` 前缀。例如：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=INPUT.mlir \
  --cv-pipeline-depth=4 \
  --suffix-enable-auto-multi-buffer=true \
  --suffix-local-multi-buffer-strategy=no-limit \
  --suffix-mix-multi-buffer-strategy=only-cube \
  --skip-suffix-build
```

完整 wrapper 参数使用：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh --help
```

suffix 输出和比较现场保存在：

```text
ub_overflow_model_cpp/output/demo/<kernel>/suffix_oracle/
```

## 5. 矩阵对比

测试输入位于：

```text
ub_overflow_model_cpp/data/before_cvpipelining/
```

当前 corpus 包含 160 个去重输入。25 组常用参数配置保存在：

```text
ub_overflow_model_cpp/config/corpus_test_matrix.tsv
```

查看配置名称：

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py --list
```

### 固定一个 seed

下面的命令运行 `25 × 160 = 4000` 项对比，不运行 retry：

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seed 0 --quiet
```

### 固定 20 个 seeds

完整正确性对比使用 seed 0～19，共运行 `25 × 160 × 20 = 80,000` 项：

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seeds 0-19 --quiet
```

### 只运行部分配置或输入

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --config default \
  --config auto_multi_buffer \
  --seeds 0-19 \
  --case-start 0 \
  --max-cases 32 --quiet
```

常用选择参数：

| 参数 | 说明 |
|---|---|
| `--config NAME` | 只运行指定配置，可重复 |
| `--seed N` | 固定一个 seed |
| `--seeds RANGE` | 固定多个 seeds，例如 `0-19` |
| `--retry-only` | 不运行固定 seed，只运行默认 retry |
| `--case-start N` | 跳过前 N 个输入 |
| `--max-cases N` | 最多运行 N 个输入 |
| `--dry-run` | 只打印展开后的命令 |
| `--stop-on-failure` | 第一次失败后停止 |
| `--no-progress` | 不显示进度条 |
| `--quiet` | 只打印汇总和失败 |

### 使用 suffix 缓存

第一次运行使用 `read-write`，未命中时执行 suffix 并写入缓存：

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seeds 0-19 \
  --suffix-cache-dir ub_overflow_model_cpp/output/suffix_oracle_cache \
  --suffix-cache-mode read-write --quiet
```

缓存填满后使用 `read-only`，只运行模型并与缓存对比：

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seeds 0-19 \
  --suffix-cache-dir ub_overflow_model_cpp/output/suffix_oracle_cache \
  --suffix-cache-mode read-only --quiet
```

重新编译 suffix、修改输入或参数后会自动使用新的缓存键。需要强制重新生成时使用
`--suffix-cache-mode refresh`。

## 6. 时间对比

时间对比使用真实应用场景的 PlanMemory retry，不固定 seed。默认不把 suffix 的
oracle/debug dump 开销计入编译时间。

### 测量全部 160 个输入

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --retry-only \
  --runtime-timing-output /tmp/cvub-runtime.tsv --quiet
```

### 只测指定 kernel

`--input` 可以重复传入。相对路径按 `--corpus-root` 解析：

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --input python_tutorial_06-fused-attention.ttadapter/before_cvpipelining_func_func_attn_fwd_32.mlir \
  --input python_tutorial_09-persistent-matmul.ttadapter/before_cvpipelining_func_func_matmul_kernel_32.mlir \
  --retry-only \
  --runtime-timing-output /tmp/cvub-selected-runtime.tsv
```

### 测量全部 25 组配置

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --retry-only \
  --runtime-timing-output /tmp/cvub-matrix-runtime.tsv --quiet
```

runner 会在终端输出每个 kernel 的横向对照表，并在 TSV 中保存：

- model 和 suffix 的总时间；
- model 每个 stage 的时间；
- suffix 每个 pass 的时间；
- 同名 stage/pass 的耗时差和倍数；
- 矩阵运行时对应的配置名称。

需要把 suffix dump 开销也计入时，追加：

```text
--runtime-timing-include-dumps
```

查看所有参数：

```bash
bash ub_overflow_model_cpp/run_demo_ub_plan.sh --help
ub_overflow_model_cpp/output/bin/bishengir-ub-overflow-model --help
python3 ub_overflow_model_cpp/scripts/run_corpus_oracle.py --help
python3 ub_overflow_model_cpp/scripts/run_corpus_matrix.py --help
```
