# CVPipeline UB 模型交接说明

## 基线与合并关系

本分支以 `yy/cvpipeline-ub-model-product` 为产品基线，并已合并其 2026-07-16 的
`5528b27c` 更新。该更新重构了轻量模型：产品入口统一为 `src/main.cpp`，IR、pass 和
pipeline 分别位于 `src/ir/`、`src/passes/`、`src/pipeline/`。原分支中并行存在的
`src/post_cvpipeline/`、旧 CLI 和旧 suffix pipeline 已删除，避免两套实现继续漂移。

当前开发分支为 `codex/cvpipeline-ub-post-model`，PR 推送目标为
`jskhub/cvpipeline-ub-post-model`。

## 本次冲突解决保留的功能

在 yy 新架构上保留并适配了以下产品合同：

- 模型逐个选择并规划模块中的 AIV 函数，模块 `ub_peak_bits` 和 `required_bits` 取各
  函数最大值，不把互不并发的函数容量相加。
- JSON 报告区分 `precision=exact` 和 `precision=incomplete`。只有 Exact 输出权威
  peak、required 和 buffer plan；Incomplete 返回 `status=blocker`、退出码 1，权威
  字段置空。
- 已计算但尚未证明与真实编译器等价的结果只放在 `debug_estimate`，不能用于 UB 规划。
- buffer 报告保留 offset、lifetime、`multi_buffer_num` 和 `inplace_pairs`。
- 默认 inplace 推断出现非空 pair、但尚未证明与真实编译器一致时，整条链 fail-closed。
- pass 抛出的未支持形态与模型主动判定的 Incomplete 使用同一 blocker 报告合同。
- oracle 比较继续检查 status、peak、required、buffer extent/offset/lifetime、
  multi-buffer 和 inplace，不因合并而降级为只比较 peak。

`memref.store` 已纳入 generic operation 的内存写语义，用于正确构造多 AIV 测试和
PlanMemory 生存期。

## 验证入口

构建和测试：

```bash
bash cvpipeline_ub_model_cpp/build.sh
bash cvpipeline_ub_model_cpp/tests/run_tests.sh
```

真实编译器 corpus 门禁：

```bash
python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipelining \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler /path/to/bishengir-cvpipeline-suffix-compile \
  --seeds 0
```

本次合并后的 seed 0 结果为：171 个输入，58 个 Exact，113 个安全 blocker，0 个
差分或报告合同失败。这里的 blocker 是有意的保守结果，不代表已精确覆盖该输入。

## 仍需继续补齐的范围

- 113 个 blocker 中的真实产品路径需要按诊断逐类补模；没有 oracle 证据前不得放宽
  Exact。
- 默认 inplace pair 与真实 `InitializeInplacePairList`/OneShotBufferize 决策仍需精确
  对齐。目前采用 blocker 防止假 Exact。
- 动态 stride-aligned allocation 等 yy pass 明确不支持的路径仍会阻断。
- 当前 corpus 门禁已比较最终 PlanMemory 语义；若要把 Exact 证据推进到每个阶段，仍需
  将模型和真实编译器的阶段快照做同构语义差分。

## 工作区约束

`cvpipeline_ub_model_cpp/output/` 是本地构建和 demo 产物，保持未跟踪，不纳入提交。
