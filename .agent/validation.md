# 当前验证体系

## 主正确性：进程内轻量模型对原生 BiSheng PlanMemory

后续正确性验证不再以 cv2pm 为 oracle。每个
`adapter × scenario × seed` 只启动一个真实 `bishengir-compile`：

1. 在 CVPipelining 前运行 embedded `UBOverflowPrediction`，使用本轮真实 resolved
   pipeline options；验证模式固定模型 PlanMemory seed 并输出详细 plan。
2. 不 prune，主 pipeline 继续执行到真实本地 PlanMemory；同一个固定 seed 传给原生
   PlanMemory，并输出其详细状态。
3. 在 PlanMemory 后停止，不执行后续 lowering/hivmc。
4. 比较 status、required、peak、buffer extent/offset、lifetime、multi-buffer 和 applied
   inplace。名字通过语义 identity 对齐，不要求 SSA printer 名字相同。

主入口：

```bash
python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 --jobs 12
```

测试开关 `BISHENGIR_UB_MODEL_VALIDATION`、`BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS`、
`BISHENGIR_PLAN_MEMORY_FORCE_SEED` 和 `BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY` 均由脚本
设置，默认关闭，不改变生产路径。AIV 因非 UB scope 先失败而没有 UB 观测时记为
`unavailable`，不能计作 matched；timeout 同理。

模型在 PlanMemory 前返回 incomplete 时，如果同一原生进程也失败，runner 使用
`config/failure_taxonomy.tsv` 比较双方稳定根因；通用 pipeline failure wrapper 属于
cascade，不得遮蔽后续具体 MLIR 诊断。原生 abort/timeout 没有稳定根因时仍记为
`unavailable`，不能仅凭“两边都没得到 plan”计作 matched。

### post-P0 代表回归（2026-07-27）

用户决定暂停预计约两小时的 85080 项全量；停止前落盘的 production-default 2400 项
全部 matched。随后选择 softmax、fused-attention、persistent-matmul、稳定 tiling 失败和
复杂 MIX/CV-balance 共 5 个输入，保留全部 27 场景及 seed 0～19。已知超时表排除 11 个
场景/输入 pair（220 seed 任务），实际运行 2480 项：

```text
matched=2246
different=14
unavailable=220
timeout=0
```

14 项均为 `ascend_tutorial_06-fused-attention` 在 10 个 auto-multi-buffer 场景中的
`plan/lifetime/inplace` 顺序置换，required、peak、buffer extent、multi-buffer 和 overflow
结论均一致；其中 `ubuf_saving_auto_mb` 只剩 seed 4 一项，说明具体变体 seed 会随原生
allocator/pointer-hash 历史变化。220 unavailable 包含两个 workspace-manage-off 场景对
全部 5 个输入的 200 项，以及 `preload_auto_mb × persistent-matmul` 的 20 个无稳定诊断
SIGABRT。`auto_bind_off × persistent-matmul` 和
`preload_auto_mb × recompute_w_u` 共 40 个稳定原生失败均按 taxonomy 与模型匹配。
报告为 `output/bisheng_embedded_validation_post_p0_subset5.tsv`，属于可再生成的本地产物。

该方式从 adapter 起点开始，因此前后参数天然来自同一真实 compiler config；不读取 cv2pm
缓存或 8 套 profile，也不会在模型和 PlanMemory 之间跨进程序列化 IR。

每个 seed 必须独立启动真实 BiSheng。不能在同一进程顺序执行 20 个 seed：前序
MemLiveness/MemPlan 和模型分配会改变后续 pointer-hash/allocator 历史，产生不属于严格
单-seed 合同的 plan/lifetime/inplace 结果。长测试逐行写报告，可用同命令加 `--resume`
继续；调度按 scenario/seed 交错 adapter，避免 12 个 worker 同时被同一个长尾 kernel 占满。
此前用户已决定不再验证的 66 个已知超时 pair 固化在
`config/known_timeout_pairs.tsv` 并默认排除；正式分母为 85080。显式
`--include-known-timeouts` 才恢复 86400 项。

## 历史验证资产（非当前 oracle）

下面两层 cv2pm 验证及缓存数字是此前开发检查点，保留用于定位 cv2pm 本身或追溯历史，
不得用作后续轻量模型正确性结论。

### 第一层：cv2pm 对真实 BiSheng

目的：证明 `cv2pm-bishengir-compile` 没有遗漏或增加从 CVPipelining 边界到本地
PlanMemory 边界之间的生产语义。

验证方法：

1. 从同一个 adapter、相同编译器参数出发，由真实 `bishengir-compile` 生成对应
   before-CVPipelining bytecode。
2. 真实编译器继续运行到本地 PlanMemory 前；cv2pm 从该 bytecode 开始运行到同一边界。
3. 比较边界 IR、退出类型和稳定诊断。PlanMemory 本身不参与这一层完整性验证。
4. 参数不一致时以真实编译器的有效参数语义为准，不给真实编译器补实验开关。

注意：生产 CVPipelining/workspace 中存在基于对象地址哈希的无序遍历；跨进程可能出现
GM workspace、`iter_args/result/yield` 的一致置换和 SSA 编号变化。不能为了消除文本
排列差异修改生产 pass 的遍历顺序。应结合 pass 序列审计、规范化语义比较、相同失败
信号和真实编译器自身重复运行基线判断。

当前已知验证缺陷：`BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY` 会在
`BiShengIRCompileMain.cpp` 中跳过 HIVMC version detection。版本可能影响边界之前的
pass，因此修复前该停止模式不是严格的生产前缀等价证据。正确做法是照常解析和设置
HIVMC version，只在目标边界之后提前返回。

### 第二层：轻量模型对 cv2pm

目的：模型在同一 before-CVPipelining IR、同一有效配置和同一 seed 下复刻 cv2pm 的
PlanMemory 结果。

当前比较合同：

```text
precision / status / overflow
selected seed and retry outcome
peak_bits / required_bits / capacity_bits
buffer extent and every offset
direct alloc/free lifetime
multi_buffer_num
applied inplace pair identity
deterministic pre-PlanMemory failure presence and stable failure class
```

buffer 通过语义 identity 对齐，不要求 `%alloc_N` 等 printer 临时名字逐字一致。结构
组织可以不同，但影响 UB 的 allocation、顺序、lifetime、offset 和配置语义必须一致。

### 矩阵和缓存

```text
160 inputs × 27 meaningful scenarios × 20 seeds = 86400 seed results
8 before-CVPipelining profiles
4320 compressed cache records
```

2026-07-27 新接入的三项 cross-core 分支参数已进入统一矩阵。原 22 场景显式使用生产
默认值 `true/false/false`，另加 5 个非笛卡尔积 InjectBlockSync 场景。旧 22 场景缓存只在
cv2pm 二进制和实际参数语义等价时做带审计信息的身份迁移；新增 5 场景仍由 cv2pm 实跑。

当前 cv2pm snapshot 为 `077458d14e64`。4320 条记录逐文件审计为 27 个场景各 160 条、
schema 2、同一二进制哈希、完整参数身份，0 条坏记录。缓存包含 84840 个 PlanMemory
seed 结果和 78 个前缀失败配置/输入；后者分为 66 个 timeout（无 oracle）和 12 个
确定性失败。最终 10 进程分场景只读验证为 `matched=84852`、`failures=0`：84840 个
PlanMemory seed 全部 exact，12 个确定性失败全部对齐，命中非 primary 合同 21 次。
`ascend_tutorial_06-fused-attention` 的 `inject_block_auto_mb` seed 10 在分片进程分配历史下
触发一次 plan/lifetime/inplace 置换；同一 cv2pm snapshot 实跑观测到完全相同的第二
合同并持久化，随后 27 场景全量只读复跑通过。

首个增量缓存定向运行（python fused-attention，5 场景 × 20 seeds）曾为 85/100 primary
matched：差异全部集中在 `inject_block_auto_mb`，其中 2 个可由重复运行观测到的真实
cv2pm 第二合同解释，13 个仍为 `plan/lifetime/inplace` 差异。2026-07-27 已定位并修复：
轻量模型原先只生成最近冲突的静态 set/wait，遗漏了原生 BLOCKSYNC 的完整前向/反向
调度；同时 CV-unroll 动态 selector 必须像
`SyncCodegen::CreateSetWaitBlockOpForMultiBuffer` 一样在同步端点创建，而不能沿用 GSS 的
loop-body-start 放置。修复后同一缓存只读验证为 `100/100` 全字段 matched（含 2 次已存
真实 variant 命中），原 22 场景同一输入的 20-seed 快速回归为 `440/440`。

场景定义：`ub_overflow_model_cpp/config/ub_relevant_parameter_scenarios.tsv`。
每个场景通过 `pre_cv_profile` 读取对应 profile，不允许所有后缀配置都错误复用同一份
before-CVPipelining IR。

schema 2 中每个 seed 都从原始 before-CVPipelining bytecode 开始，在同一个 cv2pm 进程
中依次执行完整后缀和 PlanMemory。不得用“后缀进程 dump before-PlanMemory IR，再启动
local-PlanMemory 进程”的结果作为 oracle：PlanMemory liveness 仍会观察真实 MLIR
`Value*` 的哈希桶顺序，跨进程序列化会改变这种顺序。

缓存身份包含：

```text
cv2pm executable SHA-256
input bytecode SHA-256
effective cv2pm arguments
seed list
execution mode = full_cv2pm_per_seed
```

`read-only` 验证只执行模型，不调用当前 cv2pm；它验证所选记录都来自 manifest 指定的
同一个 cv2pm snapshot。更新 cv2pm、输入、矩阵或参数后必须重建相应缓存，不能继续使用
旧结果。

### 超时和确定性失败

- timeout 没有确定性输出，不是 oracle；必须单列并从正确率分母中排除，不能算通过或
  blocker。
- PlanMemory 前稳定失败是有效 oracle。模型不能返回 exact plan，并须对齐失败存在性和
  `failure_taxonomy.tsv` 中的稳定分类。
- SIGABRT 等诊断中的未初始化数值可能不稳定，只比较稳定的首条 error 和退出类别。

## 最新保留证据

旧缓存 manifest（2026-07-24 15:13，schema 1，已确认无效）：

```text
scenarios=22
inputs_per_scenario=160
records=3520
seed_results=70400
pipeline_failures=72
```

这些 timeout、确定性失败和 mismatch 数均可能受到拆进程方式影响，不再作为当前证据。

旧报告（2026-07-25 10:51，schema 1，已确认无效）：

```text
plan rows=68960: matched=65617, different=3343
pipeline-failure rows=12: matched=0, different=12
timeout pairs=60
```

不能再将其中的差异视为真实 parity 问题。修改后的真实状态必须靠 schema 2 全量缓存和
报告确定。

schema 2 早期定向证据（2026-07-25，已被后续检查点覆盖）：

```text
auto_mb_default × ascend fused attention: 18/20 matched
  seed 7,10: plan/lifetime/inplace differ; required/peak equal
auto_mb_unrestricted × KDA:              20/20 matched
```

旧 schema 1 对同一 KDA 报出的 4 个 peak mismatch，在完整单进程 cv2pm 下全部消失。

### 代码检查点 `7d65e7ab1` 的 schema 2 代表性验证（2026-07-25）

本检查点按真实 BiSheng/MLIR 语义补齐了以下关键差异：

- CrossCoreGSS 的 disjoint outer-loop event-id 复用、preload repeat flag-id 和动态
  CV-pipeline weighted event coloring 顺序；
- TileAndBindSubBlock/BubbleUpSubviewFromTiling 的父子 tile 重算，以及父 view 有其他用户
  时创建新 view、保留原 view 类型和 offset 的 PatternRewriter 行为；
- affine 常量 operand composition、InferHIVMDataLayout 和若干后缀 pass 的缺失语义；
- schema 2 oracle 生成、稳定性审计和已记录合法 cv2pm 非确定性变体合同。

代表矩阵使用 cv2pm snapshot `3747f2790b14`：

```text
12 selected auto-MB/preload/tile/ubuf scenarios
× 2 fused-attention inputs
× 20 explicit seeds
= 480 tasks

matched=480
different=0
exact=480/480
variant_matches=34
blockers=0
timeouts=0
```

`variant_matches` 只接受同一输入、配置、seed 和 cv2pm snapshot 经 stability audit 实际
观察并写入 schema 2 缓存的完整结果；不能用模糊比较或临时重跑结果掩盖 mismatch。

阶段定向验证 `tile_balanced_high_auto_mb × ascend fused-attention × seed 3` 中，从
`TileCubeVectorLoop` 到 `InlineOTFLoadStore` 的 operation multiset 全部一致。完整
`bash ub_overflow_model_cpp/tests/run_tests.sh` 通过，并包含
`64 -> 32 -> 4` nested subview 多用户回归。

这仍是代表性子集，不得宣称 160×22×20 全量通过；下一阶段需在项目正式 cache 目录重建
schema 2 全量 oracle 后再执行 70400 项 read-only 模型验证。

### 检查点 `9b86d0f25` 的局部验证（2026-07-25）

当前检查点包含四项生产语义对齐：按 `CVPipelining.cpp` 修正 preload workspace 的两阶段
迁移和依赖驱动删除；按 `TileCubeVectorLoop.cpp` 补齐 preload vector scope 的 collection
diagnostic；按 `TileAndBindSubBlock.cpp::modifyOpToSliced` 与
`HIVMBubbleUpExtractSlicePass::verifyMarkedExtractSlicesAreBubbledUp`，补齐 writable
`bufferization.to_tensor` 目的切片在 extract-of-extract 事务中遗失的 marker/rollback
状态，并保留 store/copy marked slice 源仍被 `tensor.insert_slice` 使用时的生产 use gate。
后者明确排除 CVPipelining 预先生成的 `hivm.preload_workspace` 切片。没有修改 BiSheng/
cv2pm 或 oracle 缓存。

代表性矩阵：

```text
5 kernels × 22 scenarios × 20 seeds, plus one seed-independent failure row
tasks=2181
matched=1703
different=478
exact PlanMemory rows=2180/2180
failure parity=1/1
```

为避免增量构建未完成时误用旧二进制，先显式等待父检查点 `bf331f978` 重建结束，再用
完全相同的输入、参数、seed 和 oracle snapshot `5f2d3de326ee` 生成 paired report：

```text
parent:  matched=943,  different=1238
current: matched=1323, different=858
different -> matched: 380（均为 ascend_tutorial_03-matrix-multiplication）
matched -> different: 0
exact -> blocker:     0
```

在此基础上，`9b86d0f25` 相对 `d281c0ad1` 的同一报告又产生：

```text
different -> matched: 380（均为 merge_16x16_to_64x64_inverse_kernel）
matched -> different: 0
exact -> blocker:     0
```

完整 `preload_auto_mb` 场景：

```text
comparable PlanMemory cases=150 × 20 seeds = 3000
seed-independent failure cases=4
tasks=3004
matched=2952
different=52
exact PlanMemory rows=3000/3000
failure parity=4/4
skipped timeout inputs=6
```

此外 `bash ub_overflow_model_cpp/tests/run_tests.sh` 全部通过。以上是局部回归证据，不得
替代 160×22×20 全量结论。

### 检查点 `0d44562f0` 的 schema 2 代表验证（2026-07-25）

正式 schema 2 oracle cache 使用 cv2pm snapshot `3747f2790b14`，包含 3520 个记录和
70400 个逻辑 seed 结果；其中 60 个 timeout 场景/输入对不作为 oracle，12 个稳定失败
场景/输入对按 failure taxonomy 比较。此次只读缓存，不重跑 cv2pm。

代表集包含 12 个输入、全部 22 个场景和显式 seed 0～19：

```text
tasks=4509
matched=4473
different=36
exact precision=4506/4509
failure parity cases=9
skipped timeout seed rows=30
```

同一代表集在修改前为 44 个 difference。本检查点补齐 OneShotBufferize 对
`tensor.insert_slice` 生成 destination subview + copy 后的 stride-align 投影，并在
PlanMemoryInput bridge 中按 HIVMOptSinglePoint survivor map 删除 canonicalization 不会
保留的死 `annotation.mark`。因此
`attn_fwd × {cv_workspace_manage_off, cv_workspace_manage_off_auto_mb}` 的 8 个
plan/lifetime/inplace 差异全部消失，未引入代表集回退。

验证命令还包括完整 `bash ub_overflow_model_cpp/tests/run_tests.sh`，全部通过；新增回归覆盖
SinglePoint 删除的 local allocation 与重编号 survivor ordinal 碰撞的判定。此证据仍是
代表子集，不能替代最终 160×22×20 全量 read-only 结果。

`config/validation_baseline.tsv` 是“已知差异 ratchet”，用于阻止新增 mismatch 和
`exact -> blocker` 回退；它不是正确答案，也不能把已有差异视为通过。

### 检查点 `3c40ef509` 的 schema 2 代表验证（2026-07-25）

沿用 `0d44562f0` 完全相同的 12 个输入、全部 22 个场景和显式 seed 0～19；正式 cache
仍来自 cv2pm snapshot `3747f2790b14`，此次为 read-only，只运行轻量模型：

```text
tasks=4509
matched=4479
different=30
exact precision=4500/4509
failure parity cases=9
skipped timeout seed rows=30
```

相对上一检查点，3 个 matmul 输入在
`cv_workspace_manage_off`/`cv_workspace_manage_off_auto_mb` 下的 6 个稳定
failure-presence 差异全部消失。修复严格复刻生产逻辑：SplitMixKernel Cube 投影执行
`FoldEmptyInsertSlice`；被轻量主流程丢弃的 AIC 投影复用同一 post-SplitMix 后缀直到
OneShotBufferize/InferHIVMMemScope，并执行生产 CopyOp 支持矩阵中的 L1-to-L1 verifier。
没有 adapter、seed 或 buffer-name 特例，也没有修改 BiSheng/cv2pm 核心逻辑。

剩余 30 个差异全部属于两个 fused-attention 输入的 seed-sensitive
`plan,lifetime,inplace`；没有 all-20-seed failure cluster。完整
`bash ub_overflow_model_cpp/tests/run_tests.sh` 通过，新增回归直接覆盖 Cube
`tensor.empty -> tensor.insert_slice` 折叠和 destination 转发。代表报告为
`/tmp/cvub-known-after-aic-helper-regression.tsv`；它是本地诊断产物，不提交。

### Read-only variant audit 与 151-input 扩大验证（2026-07-25）

`0952d2a38` 允许 `--cache-mode read-only --oracle-variant-runs=N`：只对 primary mismatch
重跑当前 cv2pm 观察完整 PlanMemory 合同，更新本次内存报告但绝不写缓存。回归测试会比较
审计前后的 cache record 原始字节，防止只读模式偷偷追加 variant。

检查点 `3c40ef509` 的代表集 30 个 fused-attention primary mismatch 全部在最多 10 次观察
内精确匹配另一种真实 cv2pm 结果：

```text
9 scenarios × 2 attention inputs × 20 seeds = 360
primary mismatches=30
new observed contracts=30
resolved by exact variant=30
final matched=360/360
```

随后对未受缓存缺失影响的 151 个输入运行全部 22 场景和显式 seed 0～19：

```text
comparable cases=3289
tasks=65783
primary matched=65766
primary different=17
exact precision=65780/65783
failure parity cases=3
skipped timeout seed rows=30
```

17 个 difference 全部是上述已知 `python_tutorial_06-fused-attention` 的
seed-sensitive `plan,lifetime,inplace` 置换；针对本次清单的独立只读审计再次得到
`17/17` exact variant matches，因此这部分扩大验证的语义结果是 `65783/65783`，没有新
adapter、scenario 或 difference cluster。报告：
`/tmp/cvub-full-unaffected-151x22x20.tsv` 和
`/tmp/cvub-full-unaffected-known-variant-audit.tsv`。

缓存完整性：一次把 variant audit 误用为 writable mode 的命令先触发当前 cv2pm 哈希
`6d4d9d20e373` 的 cache rebuild；在 17/3520 条后停止。17 条不同 snapshot 记录已完整
移动到 `/private/tmp/cvub-accidental-cache-EJntYM`，正式 cache 目录剩余 3503 条记录并已
逐条确认全属于 manifest snapshot `3747f2790b14`。这避免了混合 snapshot，但正式 cache
当前不完整；不得宣称 160×22×20 全量通过，须在独立新目录用一个 cv2pm snapshot 重建
全部记录后再做最终门禁。

隔离目录随后补齐同一当前 cv2pm snapshot `6d4d9d20e373` 下的
`production_default/preload × 9 adapters`，形成 18 条独立 schema 2 记录；其中原 17 条
全部命中，只新运行 20 个 seed 进程补齐缺少的一条。轻量模型对这 18 × 20 = 360 项结果
为 `360/360 exact`，报告 `/tmp/cvub-current-snapshot-missing18.tsv`。这证明被隔离组合没有
模型回退，但不同 snapshot 的局部证据不能与旧 3503 条记录拼成单快照全量结论。

### 当前 snapshot 的 160×22×20 全量验证（2026-07-26）

已将旧正式缓存和上述隔离小缓存移出工作区，然后使用当前
`cv2pm-bishengir-compile` 二进制哈希
`6d4d9d20e37336c8e4a38177ca672acf3fa3db300015e03649bbf7725c1c61db` 重建 schema 2
正式缓存。完整性审计结果：

```text
records=3520
logical seed results=70400
PlanMemory records=3448
timeout records=60
deterministic pre-PlanMemory failure records=12
stored PlanMemory seed results=68960
mixed snapshots=0
```

随后用显式 seed 0～19、24 workers 只读运行轻量模型：

```text
comparable tasks=68972
primary matched=68954
primary different=18
exact PlanMemory rows=68960/68972
failure parity cases=12/12
skipped timeout records=60
elapsed=921.754s
```

18 个 primary 差异均为两个 fused-attention 输入在 9 个 auto-multi-buffer 场景中的
seed-sensitive `plan/lifetime/inplace` 置换；peak 和 required 不变。对这两个输入、9 个场景
和全部 20 seeds 执行最多 10 次的 read-only variant audit：

```text
tasks=360
primary mismatches=18
new observed contracts=18
resolved by exact variant=18
final matched=360/360
```

因此当前单一 cv2pm snapshot 上的最终语义结果是 `68972/68972`。这个结论包含
12 条确定性失败的失败对齐；60 条 timeout 因没有 oracle 而排除，不能计为通过。
主报告为 `ub_overflow_model_cpp/output/cv2pm_model_validation.tsv`，变体审计报告为
`ub_overflow_model_cpp/output/cv2pm_model_variant_audit.tsv`；两者都是可再生成的本地产物，不提交。

完整 `bash ub_overflow_model_cpp/tests/run_tests.sh` 通过。

#### 变体持久化后的普通只读复测

上述 18 个第二合同已写入同一 snapshot 的 schema 2 记录。这些合同都是当前
cv2pm 对完全相同的输入、配置和 seed 在独立进程中产生的完整精确观测，不是
模糊比较规则。根因定位于生产 `PlanMemory.cpp::currentlyLiveValuesOrdered()`：它把结果
收集到 `SetVector`，但仍按 `LivenessBlockInfo::in()` 的 `SmallPtrSet<Value, 16>` 迭代顺序
插入 live-in；跨进程 `Value*` 地址哈希可导致两个等价 multi-buffer allocation 互换。
轻量模型不持有 MLIR `Value*`，不能也不应预测某次 cv2pm 进程的 ASLR/地址哈希。
不修改 BiSheng 核心逻辑，不在模型中选择 kernel/seed 特例。

修复缓存 manifest 为全矩阵范围后，完整性审计和不启用在线 variant audit 的普通
read-only 复测结果为：

```text
cache records=3520
compiler snapshots=1
variant seeds=18
variant contracts=18
comparable tasks=68972
matched=68972
failures=0
failure parity cases=12/12
variant matches=18
skipped timeout records=60
elapsed=691.436s
```

因此当前报告中已没有 unresolved difference。`exact=68960/68972` 的分母还包含 12 条
确定性 failure oracle，它们按失败合同匹配，不是 PlanMemory exact plan；不能把该指标
误解为仍有 12 个差异。

## 性能测量

- 只评估产品真实场景：不固定 seed，执行 retry-only。
- 默认不启用 dump、stage snapshot、memory display 或逐 pass IR 序列化。
- 总耗时和模块/pass 耗时应在同一 kernel、同一有效配置下横向比较。
- 性能结果不能替代 20-seed 正确性验证；优化后仍须运行正确性矩阵。

### production-default retry-only 全输入基准（2026-07-26）

同机串行测量 160 个去重输入，每输入成对运行 cv2pm 和轻量模型，执行 5
个完整重复（共800对）。双方都使用 `production_default`、`plan-memory-seed=-1`；cv2pm
还使用 `--mlir-disable-threading --ub-oracle-only --runtime-timing-exclude-dumps`，关闭
memory display 并输出到 `/dev/null`。测量前各预热一次，工具启动顺序交替，避免固定
先后带来的系统偏差。

5 轮聚合耗时的中位数：

```text
                         cv2pm       model       speedup / reduction
process wall total       4.790 s     1.518 s     3.16x / 68.3%
instrumented TOTAL       3.309 s     0.886 s     3.74x / 73.2%
```

160 个 kernel 的逐 kernel 中位数上，模型在墙钟和内部 `TOTAL` 两个口径下都全部
更快。墙钟加速分布为 5 个 `1–2x`、115 个 `2–3x`、39 个 `3–5x`、1 个 `>10x`。
最极端的 `chunk_kda_bwd_kernel_wy_dqkg_fused_opt_v2` 为 `1141.489 ms vs 73.808 ms`；
排除它后墙钟仍为 `2.53x`，内部 `TOTAL` 仍为 `2.69x`。

`attn_fwd.ttadapter` 在5轮中均非零退出，但这是成功对齐的 retry-only UB overflow：
cv2pm 和模型都得到 `required=1716224 bits`、`capacity=1572864 bits`，不是性能测试失败。
本地可再生成报告：

```text
ub_overflow_model_cpp/output/runtime/cv2pm_vs_model_retry_production_default.tsv
ub_overflow_model_cpp/output/runtime/cv2pm_vs_model_retry_production_default_detail.tsv
ub_overflow_model_cpp/output/runtime/cv2pm_vs_model_retry_production_default_repeats.tsv
```

### cv2pm timeout 输入的模型可解性（2026-07-26）

对全矩阵中 60 个 cv2pm timeout 场景/输入对单独运行模型真实 retry-only，12 workers、
每件 120 秒上限：`60/60` 全部 model timeout，没有产生 success、overflow 或 blocker
合同。这 60 件是 6 个长尾输入在 10 个 auto-multi-buffer 场景中的组合。

为排除并发争用，另将 bytecode 最小的
`s8_exp28_hoist_Q_loads_3589us.ttadapter × auto_mb_default` 在无其他模型任务时单独运行，
它在 480 秒内仍未完成。因此当前不能认为轻量模型可以补出这些 cv2pm timeout
的 UB 结果；这些输入对两侧都是算法长尾，仍必须作为无 oracle 的 timeout 排除。
本地报告：

```text
ub_overflow_model_cpp/output/runtime/cv2pm_timeout_model_retry.tsv
```

### P0 实现优化后的完整验证与长尾复测（2026-07-26）

轻量模型完成三类不改变 UB 语义的实现优化：生产 API 默认关闭逐 stage timing；
`CompactGenericModule` 使用稠密映射、move 和 identity fast path；CVPipelining 复用稠密
definition/user/type 索引，并缓存属性、direct contained parent 和 descendant 列表。依赖
展开额外按 work item 去重已处理的 operation，避免同一 core op 在依赖环中重复执行
memref subnet DFS；第一次展开和工作栈顺序保持不变。

修改后重新执行完整单元/集成测试和当前 schema 2 缓存的 20-seed 只读验证：

```text
comparable tasks=68972
matched=68972
failures=0
exact PlanMemory rows=68960/68972
failure parity cases=12/12
variant matches=18
skipped cv2pm timeout records=60
elapsed=750.145s
```

随后重新运行上述 60 个原 cv2pm/model 双侧 timeout 组合，模型得到
`60/60 exact success`、`model timeout=0`。代表性的
`s8_exp28_hoist_Q_loads_3589us.ttadapter × auto_mb_default` 从单独运行 480 秒仍超时降到
约 0.45 秒。因为 cv2pm 对这些组合仍没有完成结果，这 60 项只能证明模型可解性，不能
加入 parity 分母或宣称结果与 cv2pm 一致。

同机 160 个 production-default 输入、真实 retry-only、各 5 轮的模型 A/B 墙钟中位数：

```text
detailed stage timing enabled   1.300s
detailed stage timing disabled  1.261s
timing recording overhead       3.07%
```

相对修改前同规模、开启 timing 的约 1.518 秒聚合基线，P0 数据结构和重复工作优化约降低
14% 墙钟；生产默认关闭详细 timing 后合计约降低 17%。不同测量批次存在系统噪声，后续
长期性能门禁应继续使用同机交替 A/B，而不是只比较绝对时间。

### P1 重复投影与 bridge 优化后的完整验证（2026-07-26）

P1 仅优化实现方式：canonicalization 复用稠密 active/block-argument/ordinal 索引和增量
use-list；AIC InferHIVMDataLayout 与 Copy verifier 共享同一份 Cube projection；
PlanMemoryInput bridge 缓存 operation placement，成员索引使用预留容量的无序容器，normalize
直接消费已经物化的 SSA operand/result 列表。没有修改 PlanMemory、buffer plan、seed/retry
或 AIC/AIV pass 语义。

完整单元/集成测试通过；随后读取同一 schema 2 cv2pm snapshot
`6d4d9d20e373` 执行 22 × 160 × 20 全量模型验证：

```text
comparable tasks=68972
matched=68972
failures=0
exact PlanMemory rows=68960/68972
failure parity cases=12/12
variant matches=18
new variant contracts=0
skipped cv2pm timeout records=60
elapsed=768.994s
```

同机 160 个 production-default 输入、真实 retry-only 的性能复测：

```text
detailed stage timing enabled, 5-run median     805.904 ms
P0 detailed stage aggregate baseline            855.831 ms
aggregate reduction                               5.83%

production timing disabled, 5-run median           1.20 s
P0 production timing-disabled median                1.261 s
wall-clock reduction                                4.84%
```

P1 最显著的单 stage 变化是 `CopyOpVerifier.AICProjection` 从约 52 ms 降到约 25 ms；
`BuildPlanMemoryInput` 的 5-run 典型值约 139 ms，其中 normalize 约 18.3 ms。绝对时间包含
进程启动和系统噪声，长期比较仍应使用同机交替 A/B。
