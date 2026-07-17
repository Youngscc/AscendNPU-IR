# CVPipeline UB Model Test Plan

## 1. 目标与范围

本测试方案用于验证轻量模型从 `CVPipelining` 前 IR 到 `PlanMemory` 后 UB plan 的
结果与真实编译器一致。验证对象包括：

- `precision`、`status` 和 blocker fail-closed 合同；
- UB `peak`、`required` 和 overflow 判定；
- buffer extent、offset、直接 lifetime 和 `multi_buffer_num`；
- 默认 inplace 与 `restrict-inplace-as-isa` 两条路径的 applied inplace pair；
- 固定 seed 和默认 retry 选择的 PlanMemory 结果；
- CVPipelining、multi-buffer 和 Triton 相关选项在模型与 suffix compiler 间的一致性。

`cvpipeline_ub_model_cpp/output/`、`Output/`、临时 MLIR、oracle TSV、日志和 HTML
均为测试产物，不属于提交内容。

## 2. 测试分层

### 2.1 快速门禁

快速门禁不依赖 MLIR/LLVM 构建，适合每次修改后执行：

```bash
bash cvpipeline_ub_model_cpp/tests/run_tests.sh
```

该入口包含：

- C++ 模型语义测试：解析、pass、lifetime、inplace、offset 和 planner；
- capability parity 与 pipeline manifest 一致性检查；
- Exact/Incomplete JSON 合同和多函数聚合测试；
- oracle 文本解析、函数名归一化和 applied inplace identity 比较；
- Triton 选项默认值及 model/demo/corpus/suffix 转发一致性检查。

通过标准：所有测试退出码为 0，并且 C++ 测试在
`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` 下编译通过。

### 2.2 模型构建与 smoke test

```bash
bash cvpipeline_ub_model_cpp/build.sh

cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --before-cvpipelining-ir=cvpipeline_ub_model_cpp/data/before_cvpipelining/vector_add_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --enable-auto-multi-buffer=false \
  --enable-triton-kernel-compile=false \
  --format=json
```

通过标准：构建成功；stdout 是合法 JSON；Exact 结果包含非空的 peak、required 和
buffer plan；Incomplete 结果满足 blocker 合同且不暴露权威 plan 字段。

### 2.3 单输入真实编译器差分

在已经构建 `bishengir-cvpipeline-suffix-compile` 的环境运行：

```bash
bash cvpipeline_ub_model_cpp/run_demo_ub_plan.sh \
  --before-cvpipelining-ir=cvpipeline_ub_model_cpp/data/before_cvpipelining/vector_add_bench.ttadapter/before_cvpipelining_func_func_add_kernel_32.mlir \
  --skip-suffix-build
```

通过标准：比较脚本退出码为 0；模型与 oracle 的 status、peak、required、buffer
placement、direct lifetime、multi-buffer 和 applied inplace pair 全部一致。

### 2.4 Corpus 差分门禁

先执行默认配置和两条 inplace 路径：

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipelining \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler /path/to/bishengir-cvpipeline-suffix-compile \
  --seeds 0-19 \
  --check-retry \
  --quiet

python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipelining \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler /path/to/bishengir-cvpipeline-suffix-compile \
  --seeds 0-19 \
  --restrict-inplace-as-isa \
  --check-retry \
  --quiet
```

对修改涉及的配置继续运行以下矩阵：

`run_corpus_oracle.py` 在交互终端默认显示按“输入 x seed（以及可选 retry）”计算的
进度、当前输入、seed、耗时和 ETA；需要保存纯净日志时可传 `--no-progress`。进度写入
stderr，最终 summary 保持在 stdout。

| 维度 | 配置 |
| --- | --- |
| inplace | 默认、`--restrict-inplace-as-isa` |
| PlanMemory | seed `0-19`、`--check-retry` |
| CVPipelining | 默认、`--disable-cv-pipelining`、不同 `--cv-pipeline-depth` |
| CV load scheduling | `--enable-preload`、`--enable-cv-lazy-loading` |
| code motion | `--enable-code-motion true/false` |
| auto multi-buffer | 关闭、`--enable-auto-multi-buffer` |
| Triton | 默认关闭、`--enable-triton-kernel-compile` |
| local strategy | `no-l0c`，必要时补 `no-limit/only-cube/only-vector` |
| mix strategy | `only-cube`，必要时补 `no-limit/only-vector/no-l0c` |

通过标准：

- 所有 Exact case 与真实编译器逐项一致；
- blocker 在不同 seed 间不发生无依据的 Exact/Incomplete 漂移；
- blocker 输入仍能被真实 suffix compiler 接受并完成安全性核对；
- 默认 retry 的 selected seed、peak、plan 和 lifetime 一致；
- 不出现 JSON 解析失败、真实编译器失败或非结构化模型失败。

只有在当前 corpus 被要求全部建模时才追加 `--require-all-exact`。已登记且符合
fail-closed 合同的未支持形态，不应为了提高 Exact 数量而放宽判定。

## 3. 重点回归场景

本轮与后续 PlanMemory 修改至少覆盖以下场景：

1. 默认 inplace 推断产生多个 applied pair，并验证 pair 的函数、extent、offset 和
   direct lifetime identity。
2. `restrict-inplace-as-isa` 路径产生或拒绝 pair，结果与真实编译器一致。
3. 多个 AIV 函数分别规划，模块容量取最大值而不是求和。
4. `_mix_aiv`/`_mix_aic` 名称在模型与 oracle 中归一化后仍对应同一函数。
5. buffer 复用、同地址 inplace、multi-buffer 和无复用布局均能正确比较。
6. overflow、非 overflow、Exact 和 blocker 四类结果保持报告合同。
7. Triton 开关默认关闭；显式开启后 model、demo、corpus 和 suffix 使用同一值。
8. 固定 seed 与默认 retry 都使用 direct alloc/free lifetime 输出最终 plan。

## 4. 提交前执行顺序

1. 运行 `tests/run_tests.sh`。
2. 运行模型构建和一个 smoke case。
3. 在完整 BiShengIR 工具链环境运行两条 inplace corpus 门禁。
4. 修改 multi-buffer 或 Triton 逻辑时，补跑对应配置矩阵。
5. 检查 `git status --short`，确保提交中没有 `Output/`、`output/`、日志、临时 IR、
   oracle dump、HTML 或无关 submodule 指针变化。

若受本机工具链限制无法运行真实 compiler oracle，提交说明必须明确列出未执行项，
不能用模型侧单测代替真实编译器差分结论。
