# CVPipeline UB 使用量模型

这是一个独立的轻量 C++ 工具，用于从 **before-CVPipeline** generic IR 出发，
建模到 PlanMemory 的 UB buffer、lifetime、offset 和 UB peak。Python 只负责命令
封装、JSON 转换和可视化，不参与核心计算。

工具直接建模整条路径：

```text
before-CVPipeline generic IR
  -> CVPipelining 模型 (createCVPipeliningPass 的 UB 语义)
  -> post-CVPipeline AIV 投影 (14 个阶段，见 config/post_cvpipeline_manifest.tsv)
  -> 每个 AIV 函数独立的 suffix/PlanMemory 规划
  -> 模块级 UB peak（按函数取最大，绝不求和）
```

## 输入边界

输入是编译器在 **`createCVPipeliningPass` 之前**立即产出的 generic MLIR：

- `func.func` 的 `hivm.func_core_type` 为 `MIX` / `AIV` / `AIC`，`hacc.function_kind = DEVICE`；
- 模块带有 `dlti.target_system_spec`（至少含 `L0C_SIZE` 与 `UB_ALIGN_SIZE`）；
- op 形态与编译器在该边界产出的一致（`hivm.hir.*` 结构化算子、`tensor.empty`、
  `tensor.extract_slice`/`insert_slice`、`memref.reinterpret_cast`、`scf.for` 等）。

`MIX` 函数会被 `SplitMixKernelAIVProjection` 投影为 `<name>_mix_aiv`；只有 AIV 侧
到达后续阶段，Cube 侧的 L1/L0A/L0B/L0C 不进入 PlanMemory。

## 假设的 post-CVPipeline 默认配置

工具假设编译器 post-CVPipeline 的默认开关（报告为
`assumed_post_cvpipeline_options`，不随 CLI 选项改变）：

```text
tile_mix_vector_loop   = 2     # TileCubeVectorLoop 默认 2/2 切分
tile_mix_cube_loop     = 2
enable_auto_bind_sub_block = true   # TileAndBindSubBlock 默认开启
enable_code_motion     = true       # LICM + SubsetHoisting 默认开启
enable_ubuf_saving     = false      # UB 节省默认关闭（不支持）
```

不支持的非默认配置：`enable_ubuf_saving = true`、以及任何会改变 UB buffer 集合的
非默认 post-pipeline 选项，都会被报告为 `incomplete` 或 `blocker`，绝不静默按 exact
处理。

## precision 与 status 的含义

- `precision = exact`：所有 post-CVPipeline 阶段都是可复现的已建模路径，UB 计划可
  复现编译器行为。
- `precision = incomplete`：某个阶段遇到已知的硬情形或未建模形态，按 fail-closed 报
  告。该结果的 `status` 固定为 `blocker`，正式 peak、required 和函数 buffer 计划不
  输出；如底层规划曾完成，只会放在明确不可用于规划的 `debug_estimate` 中。已记录的
  覆盖缺口：
  - `TileAndBindSubBlock` 已支持单 load/vadd/store 的静态成功切片 UB 语义；其它成功
    tiling 形态、slice 包装的动态 store 和不能证明回滚等价的形态仍会阻断；
  - `LoopInvariantSubsetHoisting` 已支持单 iter-arg 的静态
    extract_slice/insert_slice/yield 对；transfer、动态或更复杂循环体仍会阻断；
  - `InlineOTFLoadStore` 的动态 OTF 拼接 store（未建模）。
- 只有 `precision = exact` 才可能返回 `success` 或 `overflow`；三种 status 的退出码
  分别是 `0`/`2`/`1`。

## 模块 peak 语义

模块 UB peak 是**所有 AIV 函数原始同时期 peak 的最大值**，不是求和：投影后的 AIV
函数独立执行，模块永远不会看到它们的 peak 叠加。`required_bits` 是模块有效需求
（各函数有效需求的最大值）。多个 AIV 函数会被独立规划后按最大值聚合。

## 默认示例输入

```text
cvpipeline_ub_model_cpp/examples/inputs/demo_vector_add_before_cvpipeline.mlir   # 默认配置 blocker；debug peak=65536（inplace 尚未证明）
cvpipeline_ub_model_cpp/examples/inputs/demo_randn_before_cvpipeline.mlir        # blocker；仅保留调试估算
cvpipeline_ub_model_cpp/examples/inputs/demo_mix_before_cvpipeline.mlir          # 端到端 MIX, peak=4096 exact
```

## 1. 一键 demo

依次构建轻量模型，运行三个示例（vector_add / randn / MIX），打印每个示例的
precision、status、UB peak，并为 MIX 示例生成可视化 JSON/HTML：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh
```

只调整 CVPipeline 的 pipeline 深度：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh --cv-pipeline-depth=4
```

只调整 suffix/PlanMemory 的 multi-buffer 策略：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  --suffix-enable-auto-multi-buffer=true \
  --suffix-local-multi-buffer-strategy=no-limit \
  --suffix-mix-multi-buffer-strategy=only-cube
```

完整参数说明：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh --help
```

该脚本**不调用**历史 suffix 编译器/oracle；post-CVPipeline 流水线由轻量工具直接建模。

## 2. 可视化 demo

一键 demo 后，HTML 和 JSON 位于：

```text
cvpipeline_ub_model_cpp/output/demo/ub_plan_visualizer.html
cvpipeline_ub_model_cpp/output/demo/ub_plan.json
```

页面模板是 `cvpipeline_ub_model_cpp/demo/ub_plan_visualizer.html`，打开后用
`Open JSON` 载入新的 JSON。任意输入路径都可经 `--json`/`--html` 重新生成。

## 3. 单独运行轻量模型

先增量构建：

```bash
bash cvpipeline_ub_model_cpp/build.sh
```

直接运行 before-CVPipeline 到 UB plan（以 MIX 示例为例）：

```bash
cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --action=plan-before-cvpipeline \
  --before-cvpipeline-ir=cvpipeline_ub_model_cpp/examples/inputs/demo_mix_before_cvpipeline.mlir \
  --cv-pipeline-depth=-1 \
  --disable-cv-pipelining=false \
  --enable-preload=false \
  --enable-cv-lazy-loading=false \
  --enable-auto-multi-buffer=false \
  --limit-auto-multi-buffer-of-local-buffer no-l0c \
  --limit-auto-multi-buffer-buffer only-cube \
  --format=json
```

上面的命令故意没有设置 seed，保持 PlanMemory 的默认 retry 行为；只有复现固定
PlanMemory attempt 时才使用 `--random-seed N`。

主要参数对应关系如下：

```text
输入：
  --before-cvpipeline-ir PATH

CVPipeline（只影响 createCVPipeliningPass 建模路径）：
  --cv-pipeline-depth N
  --disable-cv-pipelining true|false
  --enable-preload true|false
  --enable-cv-lazy-loading true|false

suffix / PlanMemory（只影响 CVPipeline 之后的 UB buffer）：
  --enable-auto-multi-buffer true|false
  --limit-auto-multi-buffer-of-local-buffer STRATEGY
  --limit-auto-multi-buffer-buffer STRATEGY
  --restrict-inplace-as-isa

输出：
  --format=text|json
  --output PATH
```

## 4. post-CVPipeline 阶段覆盖

`config/post_cvpipeline_manifest.tsv` 记录 14 个 post-CVPipeline 阶段的覆盖判定
（`oracle-exact` / `partial` / `ub-invariant` / `unsupported`），并在报告的
`stage_coverage` 中逐阶段输出。`partial` 表示存在已精确支持的输入路径，也存在明确
阻断的未覆盖路径；只有经过真实编译器逐阶段差分证明的完整输入合同才能标成
`oracle-exact`。`config/initial_suffix_manifest.tsv` 记录 11 个 suffix 阶段的 `ub_role` 与
`input_contract`（每个阶段在 post-CVPipeline 流水线之后假设的 op 形态；未支持形态
按 fail-closed 抛错并转为 blocker 诊断，绝不静默丢弃 buffer）。

## 5. 真实编译器 oracle 门禁

`bishengir-cvpipeline-suffix-compile` 支持在 14 个 post-CVPipeline 阶段和 11 个
suffix pass group 后输出稳定语义快照，并导出真实 pipeline 的阶段顺序和文本哈希：

```bash
bishengir-cvpipeline-suffix-compile INPUT.mlir -o /tmp/result.mlir \
  --plan-memory-seed=0 --mlir-disable-threading \
  --dump-stage-oracle-dir=/tmp/cvub-stages \
  --dump-pipeline-stage-manifest=/tmp/cvub-pipeline.tsv

python3 cvpipeline_ub_model_cpp/scripts/validate_pipeline_manifest.py \
  --compiler-manifest /tmp/cvub-pipeline.tsv \
  --post-manifest cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv \
  --suffix-manifest cvpipeline_ub_model_cpp/config/initial_suffix_manifest.tsv \
  --expected-pipeline-sha256 cvpipeline_ub_model_cpp/config/real_pipeline.sha256
```

全 corpus 最终 PlanMemory 差分（Exact 校验 peak、buffer/offset 和规范化 gen/kill
关系；Incomplete 校验 blocker 合同）：

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipeline \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler /path/to/bishengir-cvpipeline-suffix-compile \
  --seeds 0-19
```

oracle 固定关闭 MLIR 多线程，避免 pass verifier 和逐阶段快照并行时产生非确定性；
这不改变 pass 顺序或 UB 语义。

## 目录说明

```text
src/semantic_ir/       generic IR 解析和语义表示
src/cvpipeline/        CVPipelining 相关 UB 语义
src/post_cvpipeline/   post-CVPipeline 14 阶段 AIV 投影建模
src/suffix/            suffix pass 的 UB 相关模拟
src/planmemory/        PlanMemory lifetime、storage 和 planner
src/cli/               生产入口
scripts/               构建封装、JSON 处理和可视化
demo/                  HTML 可视化模板
examples/inputs/       产品自带示例 IR
config/                阶段覆盖与 suffix 输入契约清单
```
