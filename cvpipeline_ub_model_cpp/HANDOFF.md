# CVPipeline UB 模型交接说明

更新时间：2026-07-16

开发分支：`codex/cvpipeline-ub-post-model`

合入基线：`yy/cvpipeline-ub-model-product`

## 1. 当前结论

本分支已经把 UB 模型收紧为 fail-closed：只有 `precision=exact` 的结果能够用于 UB
规划；任何阶段不能证明与真实编译器 UB 语义等价时，命令返回 `status=blocker`、退出码
1，并清空权威 peak、required 和 buffer plan。底层已经算出的非权威结果只放在
`debug_estimate` 中。

这是一版安全的阶段性实现，不等于原计划的全部路径都已经精确建模。默认配置下，全
corpus 的 171 个输入中 47 个 Exact、124 个 Blocker、0 个错误；打开
`restrictInplaceAsISA` 后是 69 个 Exact、102 个 Blocker、0 个错误。Blocker 比错误的
UB 规划更符合当前验收合同。

## 2. 已完成内容

- 扩展 `bishengir-cvpipeline-suffix-compile`，可输出 14 个 post-CVPipeline 阶段和
  11 个 suffix pass group 的快照、阶段 manifest 与 pipeline 文本哈希。
- 增加真实 pipeline manifest/hash 门禁以及全 corpus oracle runner。
- CLI、Python report 和 demo 已统一执行 blocker 合同；调试估算不会再显示为正式规划。
- `TileAndBindSubBlock` 已支持一个经过严格约束的静态成功路径：外部 tensor 输入、两个
  `tensor.empty`、单 load/vadd/store，并建模 tile 物理大小、slice alias 和 lifetime。
- `LoopInvariantSubsetHoisting` 已支持单 iter-arg、静态
  extract/insert/yield 的 subset 路径。
- 修复不同 `tensor.empty` 被错误 CSE 的问题。
- 最终计划与报告已携带 `multi_buffer_num` 和 `inplace_pairs`，corpus runner 会比较
  buffer multiset、offset、peak、required、lifetime、multi-buffer 与 inplace。
- overflow 保持 Exact/overflow，不再被错误降级成 Incomplete。

主要入口：

- `src/post_cvpipeline/pipeline.hpp`：14 阶段建模与 precision 汇总。
- `src/post_cvpipeline/tile_bind_subblock.hpp`：Task7 的窄成功路径和阻断逻辑。
- `src/post_cvpipeline/code_motion.hpp`：Task8 subset/LICM 建模。
- `src/post_cvpipeline/planning.hpp`：权威计划、调试估算与模块聚合。
- `src/suffix/suffix_pipeline.hpp`、`src/planmemory/mem_plan.hpp`：suffix 与最终规划。
- `scripts/run_corpus_oracle.py`：全 corpus 最终计划差分。
- `scripts/validate_pipeline_manifest.py`：真实 pipeline 阶段与 hash 门禁。

## 3. 已知未完成项

### P0：默认 inplace 推断尚未证明等价

轻量模型会在部分输入上生成真实编译器不存在的 inplace pair，例如 vector-add 的两个
UB buffer。当前所有非空的模型 inplace pair 都会将整体结果降为 Blocker。因此默认
vector-add，以及 Task7 成功 tiling 的整链结果，暂时不能作为权威规划使用。

Task7 本身的调试估算已与真实编译器一致：两个 2048-bit 物理 buffer，peak 为
2048 bit；但 suffix inplace 尚未证明，所以整链仍然是 Blocker。

下一步应先对照真实 `InitializeInplacePairList`、OneShotBufferize alias 决策和
`restrictInplaceAsISA` 行为，修正或证明轻量模型的 inplace 生成规则。修复后再解除对应
Blocker。

### P1：逐阶段自动语义差分尚未闭环

真实编译器已经能生成 25 个阶段快照，但目前自动 runner 主要比较最终 PlanMemory
结果；模型侧还没有为每个阶段生成同构快照并逐阶段比较 UB buffer、size、alias 和
lifetime。因此不能把“最终跑通”解释成每个阶段都已被 oracle 证明。

另外，oracle 工具当前复制了生产 pipeline 的组装顺序，还没有直接复用生产 pipeline
builder。manifest/hash 能检测已知顺序变化，但 builder 改动仍需要人工同步 oracle
组装代码。

### P1/P2：仍被阻断的输入路径

- Task7 的其它成功 tiling 形态、动态 store、复杂 slice 包装和无法证明 rollback
  等价的路径。
- Task8 的 transfer、动态 subset、多 iter-arg 和复杂嵌套循环。
- 动态 OTF concat/store。
- 部分 suffix 临时 buffer、alias/inplace 和默认 corpus 成功路径。
- 11 个 suffix pass group 尚未全部具备独立的正向与阻断 fixture。

## 4. 验证基线

本次实现完成时的验证结果：

- C++：166 个测试全部通过。
- Python report 测试全部通过。
- ASan + UBSan C++ 测试全部通过。
- corpus seeds `0..19`：`inputs=171 exact=47 blockers=124 failures=0`。
- retry、auto-multi-buffer seed 0：`47/124/0`。
- `restrictInplaceAsISA` seed 0：`69/102/0`。
- demo 通过：vector-add/randn 为明确 Blocker，MIX 为 Exact。
- `git diff --check` 通过。

常用复验命令：

```bash
bash cvpipeline_ub_model_cpp/tests/run_tests.sh
python3 cvpipeline_ub_model_cpp/tests/test_report.py

python3 cvpipeline_ub_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root cvpipeline_ub_model_cpp/data/before_cvpipeline \
  --model cvpipeline_ub_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler /path/to/bishengir-cvpipeline-suffix-compile \
  --seeds 0-19
```

真实 oracle 的构建和快照命令见 `README.md` 的“真实编译器 oracle 门禁”章节。

## 5. 建议后续顺序

1. 用最小 vector-add 和 Task7 fixture 定位真实/模型 inplace pair 差异，先解决 P0。
2. 给模型增加 25 个阶段的规范化 UB 语义快照，并与真实快照逐阶段自动差分。
3. 让 oracle 阶段清单直接来自生产 pipeline builder，消除双份组装逻辑。
4. 按 corpus 命中率依次补齐动态 OTF、复杂 Task7/Task8 和 suffix 路径。
5. 为每个 suffix pass group 补齐至少一个 Exact fixture 和一个 Blocker fixture，再扩大
   Exact 合同。

## 6. 工作区约定

`cvpipeline_ub_model_cpp/output/` 是本地构建、demo 和报告产物，保持未跟踪，不应提交。
新增 Exact 路径时必须同时添加真实编译器差分证据；不能证明的路径继续返回 Blocker，
不要仅凭模型自洽测试改成 Exact。
