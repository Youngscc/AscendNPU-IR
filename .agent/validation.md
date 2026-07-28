# 当前验证与性能基线

## 正确性 oracle

当前唯一主 oracle 是同一真实 `bishengir-compile` attempt 中的原生本地 PlanMemory：

1. 从 adapter 运行真实 prefix。
2. CVPipelining 前运行 embedded model，传入本轮真实 resolved options。
3. 每次验证独立固定一个 seed，完整正确性结论必须覆盖 seeds 0～19；提前 non-overflow 判定
   照常执行并记录，但只作为 observe-only，不得提前返回，模型继续输出完整 plan。
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

2026-07-28 对 `ascend_tutorial_01-vector-add.ttadapter`、`production_default` 做了重构后的
现场 20-seed smoke：`20 matched / 0 different / 0 unavailable / 0 timeout`；20 行均为
`non_overflow_upper_bound_proven=true`、
`decision_paths=full_plan_after_non_overflow_upper_bound`、
`native_plan_memory_observed=true`、
`non_overflow_proof_verified=true`。这证明提前判定命中后仍完成两侧方案验证，但不替代后续
更大现场矩阵。

### embedded 现场验证

一个 `bishengir-compile` attempt 内先运行当前 embedded model，再继续真实 CVPipelining 到
本地 PlanMemory，并比较完整合同。脚本不提供原生结果 cache 或 prediction 后提前停止模式；
这样每一个报告行都同时验证提前判定信号、完整轻量模型计划和真实原生计划。

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
前仍必须重新执行 embedded 正确性矩阵。

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

## 性能测量规则

- 性能只比较轻量原模型和新模型自身的单轮速度，使用未固定 seed 的真实 retry-only；不评估
  autotune 总收益，也不计算原生 compiler 被剪枝后节省的时间。
- 默认关闭 dump、validation、stage artifact、memory display 和逐 pass IR 序列化。
- stage timing 只用于拆分热点；它本身有开销，不能替代 production wall/API timing。
- standalone 进程墙钟包含每个 kernel 的启动成本；主指标是同一输入集合的每输入
  `Result::totalTimeNs`、单轮 internal total、process wall 和峰值 RSS。
- 比较优化前后时使用同一 build、同一输入、同一参数、预热后交替运行，并取多轮中位数。
- 性能测试使用 O3 Release；正确性依旧使用明确的 20 seeds，二者不能互相替代。
- 入口从 before-CV 前移到 before-AutoBlockify 后，新模型增加了实际语义工作；报告必须同时
  给出新增前缀的阶段耗时，并说明原模型/新模型并非严格同工作量。

## 2026-07-28 当前基线

阶段 0 新增了直接测量 embedded production `evaluate()` 的同口径基线。输入为 160 个与
before-CV corpus 配对的 adapter，production-default，retry-only，O3，validation/dump/stage
timing 全关闭；真实 prefix 后在 prediction pass 边界停止。三轮为：

```text
round       prediction total    model total    serialize total    process wall    peak RSS
1                741.444 ms      715.850 ms         25.595 ms       4719.278 ms     44.35 MB
2                725.845 ms      700.471 ms         25.373 ms       4580.319 ms     44.40 MB
3                722.965 ms      697.682 ms         25.283 ms       4625.515 ms     44.25 MB
```

480 个样本的 prediction 每输入 median/mean/p95/max 为
`1.598/4.563/15.415/90.005 ms`，每轮 147/160 命中 exact non-overflow fast path。报告在本地
`output/performance/stage0/`，不提交。

阶段 0 代表正确性共 140 个独立 fixed-seed attempt：production-default 四类输入 80、
auto-MB 20、UB-saving 20、InjectBlockSync 20，结果全部 matched；AutoBlockify 独立 160-input
验证也全部通过。

## 2026-07-28 阶段 1 direct-ModuleOp 结果

阶段 1 使用同一 160-input production-default retry-only 集合，将阶段 0 冻结二进制与当前
direct-ModuleOp 编译器交错运行三轮。480 个样本结果为：

```text
variant       prediction total    model/import total    serialize    process wall    peak RSS
baseline           2047.247 ms           1979.956 ms     67.291 ms    12569.785 ms     44.29 MB
direct             1968.624 ms           1968.624 ms      0.000 ms    12493.642 ms     43.89 MB
change                 -3.84%                -0.57%       -100.0%         -0.61%
```

每输入 prediction median/mean/p95/max 从
`1.448/4.265/14.711/85.453 ms` 下降到 `1.342/4.101/14.197/81.951 ms`。两侧每轮均有
147/160 个 exact non-overflow fast-path 命中。报告位于本地
`output/performance/stage1-direct-final-160x3.*`，不提交。

完整 `tests/run_tests.sh` 通过；阶段 0 同一组 140 个独立 fixed-seed embedded attempt 再次为
`140 matched / 0 different / 0 unavailable / 0 timeout`。直接入口与兼容文本入口还在代表配置
上逐字段对比通过。新的 `build/bin/bishengir-ub-overflow-model` 用 MLIR parser 读取文件后
调用同一个 `evaluateModule()`，并在销毁输入 ModuleOp 后使用拥有所有权的 Result，作为同步
借用生命周期检查。

## 2026-07-28 阶段 2 stable-ID/COW 结果

阶段 2 完整测试通过；与阶段 0 相同的 140 个独立 fixed-seed embedded attempt 为
`140 matched / 0 different / 0 unavailable / 0 timeout`，另有 direct/text 入口 20-seed
逐字段检查通过。

160-input production retry-only 三轮交错结果：

```text
variant       prediction total    process wall    median/input    peak RSS
stage 0            1987.189 ms     12026.759 ms        1.404 ms     44.27 MB
stage 2            1890.174 ms     12188.317 ms        1.285 ms     44.43 MB
internal ratio        0.95118
```

同一台机器上阶段 1 的 direct/stage-0 internal ratio 为 `0.96160`，因此消除阶段间机器漂移后，
阶段 2 相对阶段 1 再下降约 `1.08%`。process wall 约 `+1.34%`，主要被每输入真实 prefix/进程
启动噪声支配；该轮没有模型 unavailable，fast-path 命中仍为每轮 147/160。报告位于本地
`output/performance/stage2-final-160x3.*`，不提交。

## 2026-07-28 阶段 3 revisioned analysis 结果

完整测试通过，包含 operand replacement 后 full-index build 计数不增加、增量 use-list 与从头
扫描一致的断言；140 个代表 embedded fixed-seed attempt 再次全部 matched。

160-input × 3 production retry-only 中，阶段 3 与同轮阶段 0 的 internal total 分别为
`1893.580 ms` 与 `1992.655 ms`，ratio `0.95028`；阶段 2 ratio 为 `0.95118`。全 corpus
增益约 `0.09%`，主要价值是 mutation wave 不再触发 definitions/type/hierarchy 的等价重建；
process wall 为 `12211.104 ms`，峰值 RSS `44.35 MB`。报告位于本地
`output/performance/stage3-final-160x3.*`，不提交。

## 2026-07-28 阶段 2 后续 overlay 迁移

- stable-ID AutoBlockify 与保留的 legacy 实现在单元 fixture 上逐字节结构一致；
- `verify_auto_blockify.py` 对 160 个 before-CV 输入与原生 pass 差分为 `160 PASS / 0 FAIL`；
- 完整 `tests/run_tests.sh` 通过；
- rejected 通用 COW 实验在修复悬空 definition 指针后正确性恢复，但 160-input × 3 相对迁移前
  internal total 从约 `615～619 ms/round` 回退到 `640～651 ms/round`，RSS 从约
  `44.2 MB` 增至约 `45.6 MB`，因此未保留；
- 撤销 COW 后的 160-input × 3 现场 A/B 两侧均为 `480/480 observed`、fast-path
  `441/480`，internal total 分别为 `1854.3 ms` 与 `1856.6 ms`（约 `+0.12%`，视为噪声），
  RSS 回到约 `44.3 MB`。本轮报告仅在 `/tmp`，不提交。
- 只借用整数 stable-ID 列表、显式 `mutate()` 的缩小实验同样被否决：同口径 internal total
  从 `1838.2 ms` 增至 `1922.5 ms`（约 `+4.6%`），峰值 RSS 约增加 `0.4 MB`；代码已完整
  撤销。它证明即使避免误触发 COW，全流水 vector facade 的读取分支仍会吞掉投影复制收益。

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

此前的 compile-only autotune 和 compiler baseline/shadow/prune 数据只保留历史诊断价值，
不再用于当前性能验收。后续报告不得用这些外部节省抵消模型自身的单轮回退。

## 每个性能阶段的验证门槛

1. 修改后先运行相关单测和 `tests/run_tests.sh`。
2. 选择 simple、MIX、attention overflow、late-seed success、auto-MB、InjectBlockSync、
   UB-saving 和稳定失败输入，执行 embedded 固定 seed 全字段对比。
3. 数据结构、ordering、PlanMemory 或 fast-path 合同发生变化时，运行 27 场景 × 160 输入 ×
   20 seeds 的现场 embedded 矩阵；known timeout/unavailable 必须单列，不能算通过。
4. 性能回归使用原模型/新模型交替的 production retry-only；至少 3 轮，报告每输入分布、
   单轮 total、峰值 RSS、stage 分布、最慢 kernel 和 overflow kernel。
5. fast path 除完整 plan 验证外，还要单独统计命中率、fall-through 数和 decision parity。

## 禁止的验证捷径

- 用任何缓存替代当前 embedded 原生 PlanMemory；
- 把 exact mismatch 改成 blocker/incomplete；
- 只比较 peak/overflow，忽略 fixed-seed 完整 plan 回归；
- 将 timeout、SIGABRT 或无稳定 UB 观测计为 matched；
- 为通过矩阵修改原生 BiSheng 遍历或增加 kernel/seed 特例；
- 用同进程连续 20 seeds 的结果冒充独立 attempt 结果。
