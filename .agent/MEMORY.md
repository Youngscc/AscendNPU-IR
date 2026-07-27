# AscendNPU-IR 项目记忆

最后核实：2026-07-27，产品分支 `codex/ub-overflow-model-product` 已从历史 WIP
提交中整理出最终源码状态；此前 P0/P1 性能状态见下文。本文件是 `.agent` 的唯一入口；事实若与当前源码或新测试冲突，应先重新
核实并更新这里，不能继续沿用旧数字。

## 当前系统关系

```text
adapter
  -> 单个真实 bishengir-compile 进程
       |-> CVPipelining 前：进程内轻量模型预测并输出详细 plan
       `-> 主 pipeline：继续运行到真实本地 PlanMemory 并输出详细 plan
                    -> 同进程、同参数、同 seed 逐项比较
```

- `ub_overflow_model_cpp` 是轻量 UB overflow 模型。
- 模型的复刻对象和参数合同是原生 `bishengir-compile` 中从 CVPipelining 前边界到
  本地 PlanMemory 的真实生产 pipeline；正确性标准是真实主 pipeline 的 PlanMemory。
- `cv2pm-bishengir-compile` 应完整复用真实 BiSheng 从 CVPipelining 边界到本地
  PlanMemory 结果的生产 pass 语义。
- `bishengir-cvpipeline-suffix-compile` 是历史实验工具。除非用户明确要求处理历史
  suffix，否则不得再把它用于模型架构、参数、正确性或性能对比。
- `cv2pm-bishengir-compile` 与已有缓存只保留作历史诊断资产，不再作为后续模型正确性
  测试的 oracle，也不能替代同进程原生 BiSheng 验证。

## 产品目标

模型在真实编译前消费 before-CVPipelining Generic MLIR 和与原生 BiSheng 对齐的有效参数，
执行影响本地 PlanMemory 的轻量语义，输出精确的 UB 规划和 overflow 结论。生产方主要
消费：

```text
precision / status / overflow / ub_peak_bits / required_bits /
capacity_bits / selected_seed
```

未固定 seed 时执行真实的 seed 0～19 retry；任何 attempt 成功即为 non-overflow，20
次全部失败才是 overflow。模型不能精确处理时必须 fail closed，不能将 blocker 当作
non-overflow，也不能为了减少 mismatch 把原本 exact 的输入降级成 blocker。

## 当前测试资产

- 2026-07-27 起，主正确性入口为
  `scripts/run_bisheng_embedded_matrix.py`。每个 adapter/scenario/seed 只启动一个真实
  `bishengir-compile`：模型和主 pipeline 共享本轮有效参数与固定 seed，比较 status、
  required、peak、buffer plan、lifetime、multi-buffer 和 applied inplace。
- 验证模式通过 `BISHENGIR_UB_MODEL_VALIDATION=1` 输出模型明细，通过
  `BISHENGIR_DUMP_PLAN_MEMORY_ATTEMPTS=1` 输出真实 PlanMemory 明细，并使用
  `BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY=1` 在目标边界后结束。三者默认关闭，不改变
  生产编译；`STOP_AFTER` 与旧 `STOP_BEFORE` 不同，仍执行正常 HIVMC version detection。
- 同进程方案不读取 cv2pm cache，不依赖 8 套 before-CVPipelining profile，也没有跨进程
  bytecode/text 重建造成的对象地址历史差异。cv2pm schema-2 数字仅是历史检查点。
- 新入口已实测默认向量 seed 0、python fused-attention seed 13，以及 fused-attention 上
  `production_default/auto_mb_unrestricted/inject_block_all × seeds {0,13}`，共 8 项全部
  完整字段匹配；这只是快速验证，不能宣称全量通过。
- 2026-07-27 全量首轮发现并修复两个验证/API 缺口：Cube-only module 没有 AIV 时应为
  exact success、UB=0，而不是 blocker；真实 PlanMemory overflow 没有 applied plan/peak，
  只比较其权威 status/required。随后尝试把 20 seeds 合并到同一进程加速，但
  `auto_mb_unrestricted × python fused-attention` 的 5 个 seed 因 allocator/pointer-hash
  历史变化产生假差异；单 seed 独立进程均匹配。因此正确性必须保持一 seed 一真实
  BiSheng 进程。runner 已改为跨 adapter 交错调度，并逐行 checkpoint，支持 `--resume`。
- 2026-07-27 同进程全量首轮的两个 P0 已修复：模型的 AllocExtraBuffer 临时接口函数现
  继承原函数 `hivm.enable_saving_ub` 语义，真实 softmax seed 0 从错误的 68352 bit 对齐
  原生 58112 bit；原 680 个 UB-saving 差异定向复测为 678 matched，仅剩 2 个同峰值的
  fused-attention pointer-hash plan 变体。runner 现在将模型 incomplete 与原生稳定失败按
  failure taxonomy 比较，并忽略通用 `Failed to run BiSheng(HIR|IR) pipeline` 级联包装；
  `preload_auto_mb` 原 80 个假差异复测为 20 matched、60 unavailable、0 different，后者
  均为原生 abort 且没有稳定可比较诊断。完整 `tests/run_tests.sh` 通过。
- 2026-07-27 post-P0 同进程代表回归选择 5 个高信息密度输入、全部 27 场景和 seed
  0～19；排除 11 个已知超时 pair 的 220 个 seed 任务后，实际 2480 项结果为
  `2246 matched / 14 different / 220 unavailable / 0 timeout`。14 项全部是
  `ascend_tutorial_06-fused-attention` 的 auto-multi-buffer 场景，仅
  `plan/lifetime/inplace` 置换，required/peak/overflow 无差异；220 unavailable 由两个
  workspace-manage-off 场景的非 UB 可比性缺失（200）和 persistent-matmul preload abort
  无稳定诊断（20）组成。另有 40 个原生稳定失败按 taxonomy 与模型匹配。此前启动的
  85080 项全量按用户要求停止，已落盘的 production-default 2400 项全部 matched。

- 当前验证矩阵选择 160 个去重后的 before-CVPipelining 输入。`data/adapter/` 还有 3 个
  未进入该矩阵的定向 adapter，不能把目录文件总数误写成矩阵规模。
- 8 套生成的 before-CVPipelining profile。
- 27 个经过选择的有意义参数场景（含 5 个 InjectBlockSync 场景），不使用无意义笛卡尔积。
- 完整目标是每个“场景 × 输入”缓存 seed 0～19，共 `27 × 160 = 4320` 个压缩记录、
  `86400` 个 seed 结果。
- oracle 缓存 schema 2 必须让每个 seed 从原始 before-CVPipelining bytecode 开始，在
  单个 cv2pm 进程中执行完整后缀和 PlanMemory。身份包含 cv2pm 二进制哈希、输入
  bytecode 哈希、参数、seed 列表和 `full_cv2pm_per_seed` 模式。
- 2026-07-25 发现旧 schema 1 先 dump before-PlanMemory，再在新进程运行 local
  PlanMemory；这会破坏真实 MLIR `Value*` 哈希顺序与后缀对象分配之间的相关性。旧
  schema 1 缓存与拆进程结果均不是权威证据，schema 2 会明确拒绝它们。
- 2026-07-26 已删除工作区旧缓存并使用当前 cv2pm snapshot
  `6d4d9d20e37336c8e4a38177ca672acf3fa3db300015e03649bbf7725c1c61db` 重建全部
  schema 2 缓存：3520 条记录、70400 个逻辑 seed，其中 3448 条产生 PlanMemory
  oracle、60 条超时、12 条确定性 pre-PlanMemory 失败；实际保存 68960 个 plan seed
  结果。全部记录均属于同一二进制哈希，没有混合 snapshot。
- 同一快照上的 160 × 22 × 20 只读模型验证最终为 `matched=68972`、
  `failures=0`，包含 12 条确定性失败对齐和 18 条精确 variant match。60 条 timeout
  没有 oracle，单列且不计为通过。
- 初次 primary 比较的 18 个差异只出现在两个 fused-attention 输入的 9 个
  auto-multi-buffer 场景，都是 seed-sensitive `plan/lifetime/inplace` 置换，peak/required
  不变。根因是生产 `currentlyLiveValuesOrdered()` 仍遍历 `LivenessBlockInfo::in()` 的
  `SmallPtrSet<Value, 16>`，跨进程 `Value*` 地址哈希会产生两种真实 cv2pm 顺序。18 个
  第二合同已精确观测并持久化到同一快照缓存；不修改 BiSheng 核心遍历，也不给
  模型增加 kernel/seed 特例。
- 完整 `bash ub_overflow_model_cpp/tests/run_tests.sh` 通过。
- 2026-07-27 模型新增 `enableHIVMCrossCoreGSS`、
  `enableHIVMInjectBlockAllSync`、`disableAutoInjectBlockSync` 三个有效参数，
  options version 升为 4；模型按生产 `HIVMPipelines.cpp` 的原条件选择
  CrossCoreGSS/InjectBlockSync，且 prediction pass 直接从真实 pipeline options 传值。
  默认回归全通过；fused attention 的 disabled、block-all、普通 InjectBlockSync 三条
  非默认路径各 20 seeds，以及另外 5 个 MIX kernel 的普通 InjectBlockSync seed 0，均与 cv2pm 的
  peak/required/offset/lifetime/multi/inplace 全字段一致。该定向证据不能替代后续新增场景的
  20-seed 矩阵缓存认证。
- 2026-07-27 将同步分支 5 个场景合入统一的 27 场景矩阵：普通 InjectBlockSync、
  block-all、disable-auto，以及普通 InjectBlockSync 与 preload、auto multi-buffer 的两个
  交互。原 22 场景显式补入三个新参数的生产默认值；命令语义等价的旧 schema-2 记录
  通过审计式身份迁移复用，不重新运行 oracle。
- 2026-07-27 已审计 Triton-Ascend A2/A3 的真实调用合同并完善进程内集成：BiSheng 在
  CVPipelining 前以独立 `UBOverflowPrediction` module pass 调用模型，prediction 与真实
  CVPipelining 共享同一个 resolved `CVPipeliningOptions`，其余 UB 字段直接来自本轮
  `HIVMPipelineOptions`。stderr 合同版本为 1，返回 status/precision/overflow、peak、
  required、capacity、selected seed、耗时、digest 和稳定诊断类别。精确 overflow 同时保留
  `predicted_ub_overflow` 类别与真实文本 `ub overflow, requires ...`，从而复用 BiSheng
  先关 code motion、再关 auto multi-buffer 的 fallback，以及 Triton UBTuner 的识别逻辑。
  本轮所有修改只落在当前 AscendNPU-IR；同级 `triton-ascend` 只读审计，之后整体同步。
- 2026-07-27 生产默认已收敛：A2/A3 Triton membase 的 prediction 与 exact-overflow prune
  默认开启，overflow attempt 不进入真实 CVPipelining；普通运行不打印模型 result、trace、
  validation 或 dump。机器记录通过 `BISHENGIR_UB_MODEL_EMIT_RESULT=1` 显式开启，同进程
  validation 自动开启；手工阶段/参数输出通过 `BISHENGIR_UB_FLOW_TRACE=1` 显式开启。
  overflow 最终错误仍保留 `ub overflow`，供 BiSheng fallback 和 autotune 识别。
- 2026-07-27 统一缓存使用 cv2pm snapshot
  `077458d14e644fd6862495bf09b6eeda0cc20cb69c49709cf8f73eac2cc428c7` 完成：4320 条记录、
  86400 个逻辑 seed，3520 条旧场景记录通过显式默认参数审计迁移，另有 5 条定向记录
  直接迁入；本次实际启动 15786 个 cv2pm 进程。78 个配置/输入为前缀失败，其中 66 个
  timeout 无 oracle、12 个确定性失败可比较；其余保存 84840 个 PlanMemory seed 结果。
  最终 27 场景分片只读验证为 `84852/84852` matched、0 failures：包含 84840 个 exact
  PlanMemory seed、12 个确定性失败对齐和 21 次非 primary 合同命中。缓存共保存 23 条
  非 primary 合同。`ascend_tutorial_06-fused-attention` 的 `inject_block_auto_mb` seed 10
  曾出现一次地址/分配历史相关置换，已由当前 cv2pm 实跑观测并作为第二合同持久化；
  没有修改模型逻辑或增加 kernel/seed 特例。
- 新矩阵首次在 python fused-attention 上跑 5 × 20 时，`inject_block_auto_mb` 曾有
  15 个 primary mismatch（2 个真实 cv2pm 变体可解释，13 个仍不同）。2026-07-27 已按
  原生 `SyncCodegen::CreateSetWaitBlockOpForMultiBuffer` 修复：普通 InjectBlockSync 复用
  完整的 BLOCKSYNC 前向/反向调度，但把 CV-unroll 动态 selector 的
  `constant/addi/index_cast` 链放在同步端点，而非 GSS 的 loop-body start。专用矩阵现为
  `100/100` 全字段通过；原 22 场景 fused-attention 快速回归为 `440/440`，完整单测通过。
- 2026-07-26 的 P0 实现优化后再次只读验证当前 schema 2 缓存：`68972/68972`
  可比较任务匹配，12 条确定性失败对齐，18 条已持久化真实变体命中，60 条 cv2pm
  timeout 仍因没有 oracle 而排除。原先同样会在模型侧超时的这 60 个组合现在均能产生
  `exact success`；这证明模型长尾已解除，但不能把无 cv2pm 结果的 60 项计为 parity 通过。
- 2026-07-26 的 P1 实现优化复用 canonicalization 稠密 topology/use-list、合并 AIC
  Infer/Copy verifier 的 Cube projection，并减少 PlanMemoryInput bridge 的 placement 和 SSA
  重复物化。完整只读缓存验证仍为 `68972/68972` matched、0 failures、12 条确定性失败
  对齐、18 条真实 variant 命中，60 条 cv2pm timeout 排除。160 个 production-default
  retry-only 输入的生产默认墙钟中位数约 1.20 秒，P0 同口径约 1.261 秒。

## 不可破坏的原则

- 修复差异必须定位到 cv2pm 对应的真实 BiSheng/MLIR pass，实现其通用语义；禁止针对
  adapter、kernel、SSA 名、buffer 数量或 seed 加特例。
- 不得修改生产 BiSheng 核心语义来让 cv2pm 验证通过。dump 和边界停止必须显式开启、
  不修改 IR，并保持边界之前的配置和 pass 行为不变。
- `BISHENGIR_STOP_BEFORE_LOCAL_PLAN_MEMORY` 与 `BISHENGIR_STOP_AFTER_LOCAL_PLAN_MEMORY`
  只能改变停止边界；HIVMC version detection 等边界前配置解析必须始终按生产路径执行。
- 性能优化只能去除冗余转换、遍历、索引重建和重复 canonicalization，不得改变 UB
  peak、required、lifetime、offset、inplace、multi-buffer 或 retry 语义。
- 正确性使用 20 个显式 seeds；性能测量使用真实 retry-only，并关闭 dump/快照开销。
- 正确性必须以同一真实 `bishengir-compile` 进程中的 embedded model 与主 pipeline
  PlanMemory 对比为准；不得再把 cv2pm 或其缓存作为当前标准答案。
- 生产 `evaluate()` 默认不采集逐 stage timing，只保留 `totalTimeNs`；详细 stage timing
  必须通过 debug/oracle 控制显式开启。
- 正确性 oracle 的后缀和 PlanMemory 不得拆成两个进程；before-PlanMemory dump 只可用于
  诊断，不能作为另一个 PlanMemory 进程的权威输入。
- 保留 exact 覆盖率和 mismatch 数两个独立指标，禁止用 `exact -> blocker` 制造表面
  改善。

## 继续工作前的阅读顺序

1. [code_map.md](code_map.md)：当前源码、数据和可执行命令。
2. [validation.md](validation.md)：两层验证、比较合同、缓存和最新证据。
3. [workflow.md](workflow.md)：修复纪律、临时产物与检查点策略。
