# CVPipelining UB Model Memory

本目录只记录当前工作树可以由源码或现存测试证明的事实。源码位置和命令见
`code_map.md`；历史大矩阵见 `plan_memory_input_validation.md`。遇到冲突时，以当前
源码和最新真实 suffix 差分结果为准。

## 目标与原则

```text
generic IR before CVPipelining + supported config
  -> lightweight C++ pass pipeline
  -> exact absolute local UB peak / exact overflow required_bits
  -> exact buffer lifetime and offset plan
```

- Oracle 是真实 `bishengir-cvpipeline-suffix-compile`，不是旧模型输出。
- 核心逻辑全部使用轻量 C++；Python 只负责命令封装、数据生成、比较和可视化。
- 模块和函数按真实 pass 命名；规划阶段名不得进入生产 API。
- 规则必须来自 BiSheng/MLIR 源码。禁止按 adapter 名、SSA 编号、buffer 数量或最终
  peak 拟合。
- 生产主线不得运行验证或修改 oracle。suffix compiler 只允许增加只读 dump。
- 不能证明 exact 时必须 fail closed，返回 blocker/incomplete，不得返回估计值冒充
  exact。

## 当前生产入口

生产调用链只有一条：

```text
src/main.cpp
  -> RunCVPipeliningUBModulePipeline
  -> src/pipeline/cvpipelining_ub_pipeline.hpp
  -> src/passes/<real-pass-name>
  -> PlanMemory
```

输入是 `before CVPipelining` generic MLIR。普通模式不接受其他边界；
`after-cvpipelining` 和 `before-plan-memory` 只在 `--debug --debug-entry=...` 下开放。

当前生产 CLI 暴露：

```text
CVPipelining:
  disable-cv-pipelining
  cv-pipeline-depth
  enable-preload
  enable-cv-lazy-loading

UB suffix:
  enable-code-motion
  enable-triton-kernel-compile
  enable-auto-multi-buffer
  limit-auto-multi-buffer-of-local-buffer
  limit-auto-multi-buffer-buffer

PlanMemory:
  random-seed (0..19；省略时执行真实 retry 行为)
  restrict-inplace-as-isa
```

## 已实现主链路

当前 pipeline 按以下顺序实现会影响 UB 的语义：

```text
CVPipelining
  -> TileCubeVectorLoop (当前生产值固定 vector=2, cube=2)
  -> InferAndSetBufferSize
  -> GlobalWorkspacePlan
  -> FoldTensorEmpty
  -> CanonicalizationHIVMPipeline
  -> MarkRealCoreType / CrossCoreGSS / MarkRealCoreType
  -> SplitMixKernel
  -> InlineScope (strict, transactional, fail closed)
  -> TileAndBindSubBlock
  -> FoldTensorEmpty / source-aligned canonicalization
  -> LoopInvariantCodeMotion
  -> LoopInvariantSubsetHoisting
  -> CloneTensorEmpty
  -> HIVMInlineOTFLoadStore
  -> optional OptimizeDpsOpWithYieldedInsertSlice + CloneTensorEmpty (Triton)
  -> OneShotBufferize
  -> HIVMOptSinglePoint / HIVMDecomposeOp
  -> ConvertNonContiguousReshapeToCopy
  -> InferHIVMMemScope / AlignStorage
  -> AllocExtraBuffer
  -> InlineLoadCopy
  -> MarkMultiBuffer
  -> PlanMemory input projection
  -> MemLivenessAnalysis / inplace / storage planning / retry selection
```

重要实现事实：

- 多个 AIV 函数分别规划，模块 peak/required 取函数最大值，不求和。
- PlanMemory overflow 仍完成 fallback placement；`peakBits` 使用 byte-aligned 原始
  extent，`requiredBits` 使用 StorageEntry 对齐后的容量需求，overflow 退出码为 2。
- `LoopInvariantSubsetHoisting` 已有独立轻量实现；支持已证明的静态 `scf.for`
  subset 关系，未支持描述符或 loop 形态事务式返回 incomplete。
- `InlineLoadCopy` 按真实 SSA Value 判定，不能用同一底层 allocation 代替；独立
  `tensor.extract_slice` subview 会阻止错误内联。
- debug 模式按真实 pass 名向 stderr 输出关键计数；只有指定 `--debug-dir` 才生成
  规范化阶段快照。普通模式不构造快照，stdout 业务结果应与 debug 开关无关。
- generic operation 语义初始化必须幂等；未建模且接触 local memory 的 operation
  必须 fail fast。

## 当前验证基线

最新复核日期：2026-07-17，基于当前工作树重新构建后的二进制。

1. 快速门禁全部通过：

```bash
bash ub_overflow_model_cpp/tests/run_tests.sh
```

覆盖 C++ pass/PlanMemory 单测、严格告警编译、166 项 capability 映射、14 个真实
suffix 阶段清单、多 AIV 聚合、inplace、overflow、Python wrapper 与 oracle parser。

2. 默认配置、seed=0 的真实 corpus 差分：

```text
inputs=171
reported_exact=171
matched=171
blockers=0
invalid=0
failures=0
```

复现命令：

```bash
python3 ub_overflow_model_cpp/scripts/run_corpus_oracle.py \
  --corpus-root ub_overflow_model_cpp/data/before_cvpipelining \
  --model ub_overflow_model_cpp/output/bin/cvpipeline_ub_model \
  --compiler build/bin/bishengir-cvpipeline-suffix-compile \
  --seeds 0 --require-all-exact --quiet --no-progress
```

比较不是只看 peak，还包含 status、required、buffer `(extent, offset)` multiset、
direct lifetime、multi-buffer 数量和 applied inplace pair。

3. 历史定向回归：KDA、attention、cumsum、gated-delta 四个代表输入各 20 seeds，
共 80 组曾全部 exact。该结果用于风险判断；修改 PlanMemory/liveness/inplace 后应重新
执行，不能用当前 seed=0 corpus 代替。

## 当前缺陷与未完成项

以下内容不得描述为“已经完整支持”：

1. **额外 suffix 参数尚未暴露/建模完成。** 当前生产 CLI 没有
   `tile-mix-cube-loop`、`tile-mix-vector-loop`、`enable-ubuf-saving`、
   `disable-align-alloc-size`、`disable-enable-stride-align`、
   `disable-infer-hivm-data-layout`。其中 tile 内部结构存在但生产值固定为 2/2；
   `SinkOpToConsumerInLoop` 当前没有进入模型主链路。
2. **Ascend950 tightly-coupled 两个 pass 未进入主线。**
   `MarkTightlyCoupledBuffer` / `HoistTightlyCoupledAlloc` 在模型和当前 suffix 组装中被
   暂时关闭；现有单测只验证遇到相关 buffer 时 fail closed，corpus 没有证明 950
   完整行为。
3. **动态 stride-aligned allocation 仍是显式 blocker。** 已知静态上界的部分动态
   allocation 可以精确处理，但最终需要动态 stride-aligned rank-reduced subview 的
   形态不能宣称支持。
4. **控制流覆盖不是任意 MLIR。** `scf.while` 有 liveness 结构处理，但没有与
   171-input corpus 等价的完整真实 oracle 覆盖；scope/cf、未知同步模式、未知 local
   operation 和未证明 inline callee 会 fail closed。
5. **身份规范化不验证 printer 名。** canonical PlanMemory 输入使用 `b0/b1/...`
   对齐 allocation identity，验证 operation order 和全部 UB 信息，但不要求真实
   `%alloc_N` 展示名逐字相同。
6. **配置笛卡尔积的最终 plan 未跑完。** 历史 PlanMemory 输入边界有 43605 组 exact
   证据，但 171 输入乘全部配置、20 seeds 和两种 restrict 的近百万次最终
   lifetime/offset/peak 矩阵没有完整保存和重跑。

## 调试与修复工作流

1. 用同一 input/config/seed 复现 final mismatch。
2. 开启 model `--debug --debug-dir=...`，suffix 使用 stage snapshot/MLIR pass dump。
3. 从前向后比较真实 pass 边界，找到第一个差异；不要从最终 peak 倒推规则。
4. 阅读对应 BiSheng/MLIR 源码，只实现影响 UB 的同等语义。
5. 依次执行：定向样例、相关单测、20-seed 定向回归、seed=0 全 corpus、受影响配置
   矩阵。
6. 只有规范化信息完全一致才算通过；结构组织可不同，但 buffer identity、顺序、
   lifetime、offset 和配置语义必须一致。

## 本地临时备份

`ub_overflow_model_cpp/local_suffix_option_exposure_backup.md` 是未跟踪的本地说明，
记录了一次已撤销的参数暴露尝试。它不是当前实现、不是验收证据，也不应被误提交。

## 测试资产审计待办（2026-07-17）

本节只记录审计结论；对应清理和测试接线修改已按要求撤销，当前工作树尚未实施：

1. `scripts/validate_pipeline_manifest.py` 没有接入 `tests/run_tests.sh` 或 corpus
   门禁。真实 suffix 生成的阶段名称与两个 manifest 一致，但当前 textual pipeline
   hash 为 `F407B8759D7F188AD54C5FBF6D978448F4D289971D26C0CE1E52E98F4A986902`，
   `config/real_pipeline.sha256` 仍保存
   `9C942281D31E559F26033E4BED84584FB1ED97ED20487CBE8ACE9DC2BF45A52F`。
   后续应先审核 pipeline 变化，再更新 hash，并增加独立的 compiler-backed manifest
   门禁；不要让不依赖 LLVM/MLIR 的快速单测强制构建 suffix compiler。
2. 三个 tracked fixture 当前没有测试消费者：
   `code_motion_nonspeculatable.mlir`、`subblock_bind_slice_wrapped.mlir`、
   `subblock_bind_unmodeled_static.mlir`。应为它们补定向回归，或在确认合同失效后删除，
   不能把孤立 fixture 当作已有覆盖。
3. 对 `subblock_bind_slice_wrapped.mlir` 的一次试接表明：真实
   `TileAndBindSubBlock.cpp::tileAndSliceOp` 与模型都实现了从动态
   `tensor.extract_slice` 回溯静态 parent 的预检查；但该最小 fixture 本身没有提供
   足够的 tiling-dimension 证据，不能以“最终必须生成 `scf.for`”作为断言。后续测试
   应直接验证 unsupported-store 判定，或补全能够触发真实 tiling 的 fixture。
4. `scripts/__pycache__/` 和 `tests/__pycache__/` 是忽略的本地缓存，其中包含已删除旧
   验证脚本留下的 `.pyc`，可安全清理，不应上传。
5. `HANDOFF.md` 与 `.agent`/README 重复且状态过期（仍写 168 Exact、3 blocker）；
   后续应删除或明确归档为历史文档。本轮已恢复原文件，避免把清理动作混入当前修改。

完整性检查没有发现漏 track 的生产/测试源码：当前唯一项目内未 track 的说明文件是
`local_suffix_option_exposure_backup.md`，它按上节约定保持本地；171 个 corpus MLIR、
examples 输入和主程序可达的 C++ 头文件均已 track。
