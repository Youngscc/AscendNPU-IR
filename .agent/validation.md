# 当前验证与性能基线

## 正确性 oracle

当前唯一主 oracle 是同一真实 `bishengir-compile` attempt 中的原生本地 PlanMemory：

1. 从 adapter 运行真实 prefix。
2. CVPipelining 前运行 embedded model，传入本轮真实 resolved options。
3. 验证模式固定一个 seed；提前 non-overflow 判定照常执行并记录，但只作为 observe-only，
   不得提前返回，模型继续输出完整 plan。
4. 主 pipeline 继续到原生本地 PlanMemory，并在该边界后停止。
5. 比较 status、required、peak、buffer extent/offset、lifetime、multi-buffer 和 applied
   inplace；稳定失败按 taxonomy 比较。

入口：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 --jobs 12
```

每个 seed 必须独立启动真实 compiler。原生 MLIR 中仍存在基于对象地址哈希的无序结构；
把 20 seeds 顺序塞进同一进程会改变 allocator/pointer 历史，不能代表严格的单-seed
生产合同。

### 两级 embedded 验证

权威路线仍是现场模式：一个 `bishengir-compile` attempt 内先运行当前 embedded model，再
继续真实 CVPipelining 到本地 PlanMemory，并比较完整合同。

重复开发循环可以启用 read-through cache：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 --jobs 12 \
  --oracle-cache-dir ub_overflow_model_cpp/output/bisheng_embedded_oracle_cache
```

- miss：现场执行 model + native PlanMemory，比较并写入原生合同；
- hit：仍从 adapter 运行真实 BiSheng prefix 和当前 embedded model，在 prediction 后停止，
  再与缓存原生合同对比；
- stale：实际 pre-CV IR digest、resolved-options digest、seed、完整参数或 pipeline fingerprint
  任一不一致，自动回到现场并刷新；
- fallback/multi-attempt：不 replay 单 attempt 缓存，始终现场运行；
- cached mismatch/unavailable：自动再跑现场 oracle 并刷新，最终差异必须来自现场确认；
- 发布前或原生后缀语义变化后：去掉 cache 参数现场全量，或先使用
  `--refresh-oracle-cache` 重建，不能把 cache hit 数当作现场 oracle 数。

2026-07-28 warm smoke：production-default/vector-add/seed 0 首次现场约 `2.311 s`，缓存命中
约 `21.9 ms`，两次均 matched；重链接后的首次冷启动曾约 `1.50 s`，随后回到 `23.6 ms`。
该比值仅证明机制有效，不能外推为全矩阵加速比。

以下开关只用于验证，默认必须关闭：

```text
BISHENGIR_UB_MODEL_VALIDATION
BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS
BISHENGIR_PLAN_MEMORY_FORCE_SEED
BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY
BISHENGIR_STOP_AFTER_UB_OVERFLOW_PREDICTION
BISHENGIR_UB_MODEL_EMIT_RESULT
BISHENGIR_UB_FLOW_TRACE
```

## 当前正确性状态

产品基线提交 `4e1053359` 上，完整 `bash ub_overflow_model_cpp/tests/run_tests.sh` 已通过。
embedded 路线已经验证生产默认、auto-multi-buffer、InjectBlockSync、UB-saving、preload 和
MIX 代表输入；历史 post-P0 代表矩阵中 required/peak/overflow 差异已清零，剩余差异是
原生 `SmallPtrSet<Value>` 指针哈希导致的 fused-attention plan/lifetime/inplace 合法置换，
以及没有稳定 UB 观测的 unavailable 项。

此前停止的 embedded 全量不能被记为“86400 全通过”。当前新的大规模基础设施修改在合入
前仍必须重新执行 embedded 正确性矩阵，不能引用旧 cv2pm cache 代替。

2026-07-28 代表子集现场验证覆盖 8 个算子、27 场景、20 seeds，共 4320 组：

```text
matched                                      3960
strict plan/lifetime/inplace difference        20
unavailable                                   340
timeout                                         0
```

3980 个可比较结果的 status/required/peak/overflow 全部一致。20 个严格差异全部来自
`python_tutorial_06-fused-attention.ttadapter` 的 auto-multi-buffer 相关场景，表现为两个
等尺寸 UB buffer 的 offset 互换以及随之变化的 lifetime/inplace 身份。对涉及场景和 seed
现场复跑 60 组后，严格差异数由 20 变为 28，22 个 tuple 的 matched/different 状态翻转，
而差异字段始终只有 plan/lifetime/inplace；这再次证明它是原生无序 value 集合的进程相关
合法置换，不影响 UB peak 或 overflow 决策。

340 个 unavailable 中，320 个来自 `cv_workspace_manage_off` 及
`cv_workspace_manage_off_auto_mb`：产品条件下 prediction pass 不插入，因此没有模型观测可
比较；剩余 20 个来自 `preload_auto_mb` + matrix-multiplication，原生 compiler 以 SIGABRT
结束且没有稳定可比较诊断。报告位于本地可再生成的
`output/bisheng_embedded_representative_8x27x20.tsv`，不提交仓库。

历史 cv2pm schema-2 曾得到 27 场景下 `84852/84852` 可比较结果匹配；它证明模型开发阶段
达到过完整后缀等价，但从 2026-07-27 起不再是当前 oracle。suffix 更早退出当前验证体系。

## 性能测量规则

- 产品性能只测未固定 seed 的真实 retry-only。
- 默认关闭 dump、validation、stage artifact、memory display 和逐 pass IR 序列化。
- stage timing 只用于拆分热点；它本身有开销，不能替代 production wall/API timing。
- standalone 进程墙钟包含每个 kernel 的启动成本；embedded 产品判断优先看
  `Result::totalTimeNs`/`model_ns`，同时报告 `serialize_ns` 或 direct-import 成本。
- 比较优化前后时使用同一 build、同一输入、同一参数、预热后交替运行，并取多轮中位数。
- 性能测试使用 O3 Release；正确性依旧使用明确的 20 seeds，二者不能互相替代。

## 2026-07-28 当前基线

160 个去重 before-CVPipelining 输入，production-default，retry-only，O3，开启 stage
timing，三轮结果：

```text
round                     process wall       internal total
1                         1433.009 ms         912.580 ms
2                         1392.238 ms         886.105 ms
3                         1379.322 ms         878.362 ms
per-input median sum                           888.429 ms
return codes                                   159 success / 1 overflow
```

top-level 聚合：

```text
PlanMemory                              178.249 ms   20.06%
BuildPlanMemoryInput                    141.355 ms   15.91%
AlignStorageAndAllocExtraBuffer         126.228 ms   14.21%
MarkRealCoreType                         61.923 ms    6.97%
ParseGenericIR                           46.501 ms    5.23%
PostBufferizationRewrites                42.587 ms    4.79%
TileAndBindSubBlock                      33.474 ms    3.77%
InferHIVMDataLayout.AICProjection        30.137 ms    3.39%
Canonicalization source-aligned          28.416 ms    3.20%
Canonicalization HIVM                    28.369 ms    3.19%
OneShotBufferize                         26.629 ms    3.00%
CopyOpVerifier.AICProjection             25.167 ms    2.83%
```

主要 nested 热点：

```text
BuildPlanMemoryInput.EmitOperations      69.348 ms
PlanMemory.AnalyzeLifetimes              66.457 ms
PlanMemory.PlanAddress                   53.066 ms
PlanMemory.PrepareLiveness               42.823 ms
BuildPlanMemoryInput.Normalize           18.741 ms
```

代表输入：

```text
attn_fwd                                 81.973 ms
  PlanMemory                             66.511 ms
  PlanMemory.PlanAddress                 51.532 ms

chunk_kda_bwd...                         74.760 ms
  MarkRealCoreType                       13.051 ms
  AlignStorageAndAllocExtraBuffer        10.318 ms
  BuildPlanMemoryInput                    9.919 ms
  PlanMemory                              9.244 ms

ascend vector-add                         2.037 ms
  BuildPlanMemoryInput                    0.485 ms
  AlignStorageAndAllocExtraBuffer         0.327 ms
  ParseGenericIR                          0.192 ms
```

当前 embedded 样例中 generic MLIR 序列化约 `0.2~0.9 ms/kernel`；它不在 standalone
`internal total` 中。删除文本边界时必须同时测量 serialize 和 ParseGenericIR 两部分。

## exact non-overflow 上界实验

2026-07-28 使用当前完整模型结果，对每个函数按独立 buffer 计算：

```text
sum(AlignUp(extentBits, 256) * multiBufferNum)
```

不利用 lifetime reuse 或 inplace，因此它是保守上界。default 160 输入结果：

```text
safe exact non-overflow: 145
needs full PlanMemory:     15
single-buffer > capacity:   0
safe 输入当前 bridge + PlanMemory: 约 266 ms
```

实现门槛：

- 必须在 extra-buffer、InlineLoadCopy 和 MarkMultiBuffer 完成之后计算；
- 只统计 UB address space，并按函数处理容量；
- 不确定动态大小、归属或 multi-buffer 数时直接 fall through 到完整 PlanMemory；
- production 可以返回 exact overflow decision，但不伪造精确 peak/offset/lifetime；
- debug/full-plan validation 必须执行 fast predicate 并记录 observe-only 结论，但禁用提前
  return，继续轻量模型完整计划和原生 PlanMemory；
- 新增专门测试证明上界结果从不把真实 overflow 判成 non-overflow。

2026-07-28 已按上述门槛实现：生产命中后仅返回 exact non-overflow decision，debug 和
embedded validation 计算同一证明但继续完整模型；若完整 PlanMemory 返回 overflow，模型
会报告证明矛盾而不是接受 fast-path。8 个代表算子 × 27 场景 × seeds `{0,13}` 的回归为
`398 matched / 0 different / 34 unavailable / 0 timeout`；34 项仍是两套 workspace-manage-off
无模型观测（32）和 preload+matrix-multiplication 原生 abort（2）。

同日 compile-only autotune 性能实验使用 5 个代表 adapter、每个 4 个配置、5 次交替重复、
单线程、O3、真实 retry-only、停止在本地 PlanMemory 后：

```text
paired non-overflow runs                 80
average added cost per safe candidate    14.366 ms / 20.42%
baseline per-repeat median               2.4216 s
prune per-repeat median                  2.3349 s
five-repeat cumulative saving            438.35 ms / 3.61%
speedup                                  1.037x
```

单独对 python fused-attention 交替测量 21 轮，baseline/fast-path 中位数为
`125.174/151.485 ms`，paired overhead 中位数 `26.490 ms`；机器摘要中模型中位
`25.640 ms`、Generic MLIR 序列化 `0.424 ms`。本地可再生成报告为
`output/performance/autotune_fastpath_20260728/summary.json`，不提交。

## 每个性能阶段的验证门槛

1. 修改后先运行相关单测和 `tests/run_tests.sh`。
2. 选择 simple、MIX、attention overflow、late-seed success、auto-MB、InjectBlockSync、
   UB-saving 和稳定失败输入，执行 embedded 固定 seed 全字段对比。
3. 数据结构、ordering、PlanMemory 或 fast-path 合同发生变化时，日常迭代可先跑缓存矩阵，
   随后运行 27 场景 × 160 输入 × 20 seeds 的无缓存 embedded 矩阵；known
   timeout/unavailable 必须单列，不能算通过。
4. 性能回归使用 production retry-only；至少 3 轮，报告中位数、stage 分布、最慢 kernel
   和 overflow kernel。
5. fast path 除完整 plan 验证外，还要单独统计命中率、fall-through 数和 decision parity。

## 禁止的验证捷径

- 用 suffix 或 cv2pm cache 替代当前 embedded oracle；
- 把 embedded cache hit 报告冒充发布前现场原生 PlanMemory 全量；
- 把 exact mismatch 改成 blocker/incomplete；
- 只比较 peak/overflow，忽略 fixed-seed 完整 plan 回归；
- 将 timeout、SIGABRT 或无稳定 UB 观测计为 matched；
- 为通过矩阵修改原生 BiSheng 遍历或增加 kernel/seed 特例；
- 用同进程连续 20 seeds 的结果冒充独立 attempt 结果。
