# CVPipeline UB 模型开发交接

本文说明当前分支相对 `yy/cvpipeline-ub-model-product` 完成的全部开发，不只描述最后一轮
问题修复。

- 基线分支：`yy/cvpipeline-ub-model-product`
- 基线 commit：`1fb515dc58009343e99b28dcf08d4ab56fbcd897`
- 开发分支：`codex/cvpipeline-ub-post-model`
- 当前功能 commit：`05881a51`
- 更新时间：2026-07-16

## 1. 基线能力与本次开发目标

`yy/cvpipeline-ub-model-product` 已经提供一个独立的轻量 C++ UB 模型，支持：

```text
before-CVPipeline generic IR
  -> 轻量 CVPipelining 模型
  -> suffix 模型
  -> PlanMemory
```

它可以输出 UB peak，并与一个 suffix compiler 比较最终 buffer placement。但基线在
`RunCVPipeliningPass` 后直接进入 `BuildSuffixPlanMemoryInput`，没有建模真实编译流程中
位于 CVPipeline 与 OneShotBufferize 之间、会改变 UB buffer 集合、物理大小、alias 和
lifetime 的 post-CVPipeline passes。基线还存在以下边界：

- 只按原始 AIV 输入规划，不能把 MIX/CV 融合函数按真实 SplitMix 语义投影成 AIV。
- 没有 14 个 post-CVPipeline 阶段的覆盖清单和阶段级 fail-closed 诊断。
- 没有多个投影 AIV 函数的独立规划与模块 max 聚合。
- `precision` 主要是 action 级固定标签，未完整区分精确结果和非权威估算。
- oracle 主要比较最终 peak 和 `(extent, offset)`，没有 pipeline 漂移门禁和阶段快照。

本次开发的目标是在上述基线上补齐 post-CVPipeline 到 suffix/PlanMemory 的 UB 语义链，
并把不能证明精确的路径统一阻断，避免模型输出被误用于 UB 规划。

## 2. 当前整体数据流

当前 `plan-before-cvpipeline` 的数据流为：

```text
before-CVPipeline generic IR
  -> CVPipelining 模型
  -> 输入边界校验
  -> TileCubeVectorLoop
  -> Infer/SetBufferSize + early canonicalization
  -> Workspace/Sync UB invariant 检查
  -> SplitMixKernel AIV 投影
  -> InlineScope
  -> TileAndBindSubBlock
  -> Fold/CloneTensorEmpty + canonicalization
  -> LICM + SubsetHoisting
  -> InlineOTFLoadStore
  -> 每个 AIV 独立执行 suffix + PlanMemory
  -> 模块 peak = max(各 AIV peak)
```

只有整条链都返回 Exact，最终结果才允许作为权威 UB 规划。遇到未建模形态时保留合法
IR、记录首个 blocker；已经算出的数值只能进入 `debug_estimate`。

## 3. 相对 yy 基线新增的实现

### 3.1 Generic IR 安全改写基础设施

新增 `src/post_cvpipeline/ir_utils.hpp`，并增强 semantic IR/rewrite 层，为后续 pass 模型
提供事务性改写能力：

- value replace、operation erase/move、region/block 结构维护；
- DPS operand、block argument、branch forwarding 和嵌套 region 的引用更新；
- function isolation 和所需 declaration 提取；
- use-before-def、跨函数/跨 sibling region 引用、重复 parent、operand/type 不一致校验；
- 改写失败时不留下部分修改，转为 Incomplete。

这部分解决了在轻量 IR 上模拟真实 pass 时，错误删除、跨函数 value 泄漏和半改写 IR
继续进入 PlanMemory 的风险。

### 3.2 默认 Cube/Vector loop tiling

新增 `tile_cube_vector_loop.hpp`，建模默认 2/2 的 Cube/Vector loop tiling 对 UB 的影响：

- 从 store/fixpipe 和 producer dependency 证明切分轴，而不是仅按 shape 位置猜测；
- 使用 ceil-div 计算 tile，缩小 local allocation 和 GM boundary slice；
- 支持真实 `mmadL1` operand、M/N 轴、real dimensions、remainder 与 affine-min；
- 处理 vector local destination、Cube L0C/fixpipe、trip-count=1 和 rollback 后 global shrink；
- 动态 shape、轴歧义、多个 anchor、未知 body op、alignment/remainder 无法证明时阻断。

模型只规划 UB，但会识别 Cube/L1/L0 buffer，避免将非 UB buffer 误计入容量。

### 3.3 Buffer size、canonicalization、workspace 和 sync

新增 `buffer_size.hpp` 与 `canonicalization.hpp`：

- 推导并物化 UB buffer 的物理大小、alignment 和 multi-buffer 相关 annotation；
- 处理 value bounds、affine apply/division、静态 min 上界、alias view；
- 对动态 local size、未知 affine map、signed overflow 和冲突 annotation fail-closed；
- 建模会改变 allocation/owner/lifetime 的 early/post-split canonicalization；
- 修复不同 `tensor.empty` owner 被错误 CSE 的问题。

Workspace offset 和 cross-core sync 当前按“UB 集合、大小、alias、lifetime 不变”的结构
不变量验证；检测到 allocation、shape 或 alias 变化即阻断，而不是直接假定无影响。

### 3.4 MIX/CV 融合函数投影到 AIV

新增 `core_type.hpp` 和 `split_mix_aiv.hpp`：

- 按真实 core registry、operation 属性和上下文规则区分 Cube、Vector、Neutral；
- 模拟 SplitMix 的 AIV 投影、scope/call/DPS result 替换与 vector cleanup；
- 保留 workspace/fixpipe 数据边界，不把 Cube 的 L1/L0A/L0B/L0C 纳入 UB；
- MIX 函数生成 `<source>_mix_aiv`，后续只规划投影后的 AIV；
- 多个 MIX/AIV 函数分别隔离，未知 core op、递归或无法证明的控制流返回 blocker。

这部分补上了 yy 基线只保留原始 AIV、会漏掉 CV 融合算子 Vector 侧 UB buffer 的问题。

### 3.5 Scope、sub-block、tensor-empty、code motion 和 OTF

新增以下 pass 语义模型：

- `inline_scope.hpp`：inline scope/private call，维护多 result、DPS 和嵌套 region 映射。
- `tile_bind_subblock.hpp`：处理 sub-block store limit、回滚条件，并支持一个严格限定的
  静态成功路径：外部 tensor 输入、两个 destination `tensor.empty`、单
  load/vadd/GM-store。该路径会缩小 tile 物理大小并维护 slice alias/lifetime。
- `tensor_empty.hpp`：FoldTensorEmpty、CloneTensorEmpty 和 destination ownership，保证
  每个真实 owner 获得独立 storage。
- `code_motion.hpp`：支持可投机纯 op LICM，以及单 iter-arg、全静态、extract/insert
  descriptor 完全一致的 SubsetHoisting。
- `inline_otf_load_store.hpp`：支持已验证的 unaligned 静态 concat/store 重建；aligned
  和 sub-byte 情形按 no-op 处理。

Task7/Task8 仍是部分覆盖：复杂成功 tiling、动态 store/subset/OTF、多 iter-arg 和复杂
嵌套 loop 会明确返回 blocker，不会静默按 Exact 处理。

### 3.6 每个 AIV 独立 suffix/PlanMemory 规划

新增 `planning.hpp` 并调整 suffix bridge：

- 每个投影 AIV 函数单独构造 suffix 输入和执行 PlanMemory；
- 模块 `ub_peak_bits` 和 `required_bits` 按各函数最大值聚合，不相加；
- 每个函数保留自己的 selected seed、buffer、offset、lifetime 和 overflow 状态；
- 报告增加 `multi_buffer_num` 和 `inplace_pairs`；
- Exact overflow 保持 `precision=exact,status=overflow`，退出码为 2；
- 多函数误送入单函数 suffix bridge 时 fail-closed。

当前已发现模型默认 inplace 推断会在部分输入上生成真实编译器没有的 pair。为防止假
Exact，所有尚未证明的非空 inplace pair 会使整链返回 blocker。打开
`restrictInplaceAsISA` 后可精确放行更多已验证输入。

### 3.7 CLI、报告与可视化合同

`plan-before-cvpipeline` 现在输出逐阶段 coverage 和结构化 diagnostics：

- Exact success：权威 peak、required 和函数 buffer plan；
- Exact overflow：权威 required、overflow 状态，退出码 2；
- Incomplete：`status=blocker`，退出码 1，权威 peak/required/functions 置空；
- 非权威计算结果仅位于 `debug_estimate`。

Python wrapper、summary、demo 和 HTML visualizer 已适配多函数和 blocker 合同，避免把
调试估算显示为正式规划。新增 MIX demo 用于验证 SplitMix 到 AIV 的整链行为。

### 3.8 真实编译器 oracle 与覆盖门禁

扩展 `bishengir-cvpipeline-suffix-compile`：

- 可在 14 个 post-CVPipeline 阶段和 11 个 suffix pass group 后输出快照；
- 可导出阶段 manifest 和 pipeline 文本 SHA-256；
- 固定关闭 MLIR threading，保证快照和 verifier 顺序稳定。

新增：

- `config/post_cvpipeline_manifest.tsv`：14 阶段顺序、UB role、证据等级和输入合同；
- `config/real_pipeline.sha256`：当前真实 pipeline 文本 hash；
- `scripts/validate_pipeline_manifest.py`：阶段新增、删除、调序和 hash 漂移检查；
- `scripts/run_corpus_oracle.py`：全 corpus、逐 seed 的最终 PlanMemory/Blocker 合同验证；
- `scripts/compare_ub_plan_with_suffix_oracle.py`：比较 status、required、peak、buffer、
  offset、lifetime、multi-buffer 和 inplace 信息。

注意：真实编译器已经能生成 25 个阶段快照，但模型侧尚未生成完全同构的逐阶段语义
快照，因此“逐阶段模型 vs 真实 pass”的自动差分还没有闭环。当前 pipeline builder 也
仍在 oracle 工具内手工组装，而不是直接复用生产 builder。这两项属于后续最高优先级。

## 4. Task1–Task11 对照

| Task | 内容 | 当前状态 |
|---|---|---|
| 1 | 测试框架、coverage 类型、14 阶段 manifest | 已实现；生产 builder 自动导出未闭环 |
| 2 | 安全 Generic IR 改写与函数隔离 | 已实现并有结构校验测试 |
| 3 | core 分类、SplitMix、MIX → AIV | 已实现受支持合同；未知形态 blocker |
| 4 | 默认 Cube/Vector loop tiling | 已实现静态默认路径；动态/歧义路径 blocker |
| 5 | buffer size、canonicalization、workspace、sync | 已实现受支持路径和不变量门禁 |
| 6 | InlineScope、tensor-empty、post-split canonicalization | 已实现受支持路径 |
| 7 | TileAndBindSubBlock | 部分实现；窄静态成功路径已建模，其余 blocker |
| 8 | LICM、SubsetHoisting、InlineOTFLoadStore | 部分实现；静态已验证路径可用，动态路径 blocker |
| 9 | 每 AIV suffix/PlanMemory 与模块 max 聚合 | 已实现；默认 inplace 等价性仍需解决 |
| 10 | CLI、diagnostics、report、visualizer | 已实现 fail-closed 合同 |
| 11 | MIX E2E、suffix audit、oracle、文档 | 部分实现；逐阶段差分和 suffix 全 fixture 未闭环 |

## 5. 验证结果

当前功能 commit `05881a51` 的验证基线：

- C++：166 个测试全部通过。
- Python report 合同测试全部通过。
- ASan + UBSan C++ 测试全部通过。
- corpus seeds `0..19`：`inputs=171 exact=47 blockers=124 failures=0`。
- 默认 retry、auto-multi-buffer seed 0：`47/124/0`。
- `restrictInplaceAsISA` seed 0：`69/102/0`。
- demo：vector-add/randn 为明确 Blocker，MIX 为 Exact。
- `git diff --check` 通过。

这里的 `failures=0` 表示 Exact 最终计划差分或 Blocker 输出合同没有失败，不表示 171 个
输入都已被精确建模。默认配置下仍有 124 个输入被安全阻断。

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

## 6. 当前未闭环问题与建议接手顺序

### P0：修正 suffix inplace 语义

先用最小 vector-add 和 Task7 fixture 对照真实 `InitializeInplacePairList`、
OneShotBufferize alias 决策与 `restrictInplaceAsISA`。当前 Task7 的 debug estimate 已与
真实编译器一致：两个 2048-bit buffer、peak 2048 bit，但整链因未证明的 inplace pair
仍是 blocker。

### P0：把 Exact 与 oracle 证据真正绑定

当前 `Partial` stage 的窄 matcher 可以自行返回 Exact，但还没有按输入路径绑定
checked-in oracle 证据。严格按“只有 oracle-proven Exact 可规划”的验收标准，下一步应
为模型生成 25 个同构语义快照，并在门禁中逐阶段比较 buffer/size/alias/lifetime；未有
证据的 Partial 路径继续 blocker。

### P1：消除 oracle 与生产 pipeline 的双份组装

让生产 pipeline builder 暴露 stage hook 或直接导出 textual pipeline。当前 manifest
和 hash 能检查 oracle 副本自身的漂移，但生产 builder 改动后仍需要人工同步。

### P1/P2：扩大受支持输入合同

按 corpus 命中率依次补齐：

1. Task7 复杂成功 tiling、动态 store 和 rollback。
2. 多 iter-arg/dynamic subset、动态 OTF concat/store。
3. suffix decomposition temporary、alias/inplace 和 multi-buffer 边界。
4. 为 11 个 suffix pass group 分别补至少一个 Exact fixture 和一个 Blocker fixture。

## 7. 关键文件

```text
src/post_cvpipeline/pipeline.hpp               14 阶段调度和 precision 汇总
src/post_cvpipeline/tile_cube_vector_loop.hpp  Cube/Vector loop tiling
src/post_cvpipeline/buffer_size.hpp            物理 UB 大小推导
src/post_cvpipeline/split_mix_aiv.hpp           MIX → AIV 投影
src/post_cvpipeline/tile_bind_subblock.hpp      Task7
src/post_cvpipeline/code_motion.hpp             LICM/SubsetHoisting
src/post_cvpipeline/inline_otf_load_store.hpp   OTF rewrite
src/post_cvpipeline/planning.hpp                每函数规划和模块聚合
src/suffix/                                    suffix UB 模型
scripts/run_corpus_oracle.py                    全 corpus oracle
config/post_cvpipeline_manifest.tsv             阶段覆盖合同
tests/test_post_cvpipeline.cpp                  C++ 阶段与回归测试
tests/test_report.py                            CLI/report/oracle 合同测试
```

`cvpipeline_ub_model_cpp/output/` 是本地构建、demo 和 oracle 产物，保持未跟踪，不应提交。
新增 Exact 路径时必须增加真实编译器差分证据；不能证明等价的路径应继续返回 blocker。
