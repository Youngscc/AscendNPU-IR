# 当前验证与性能基线

最后核实：2026-07-30，产品 commit `7afe56bab`。

before-AutoBlockify 产品边界及其全量正确性、同边界 full-plan 性能均已完成验证。2026-07-29 的
before-CVPipelining 数据只保留为历史基线，不能与新边界倍率混用。

## 1. 正确性 oracle

唯一主 oracle 是同一真实 `bishengir-compile` attempt 中的原生 local PlanMemory：

1. 从 adapter 运行真实 BiSheng prefix；
2. before-AutoBlockify 运行 embedded lightweight model，使用本轮真实 resolved options；
3. validation 模式固定一个 seed，模型即使提前证明 non-overflow 也继续完整 PlanMemory；
4. 主 pipeline 继续到原生 local PlanMemory，并在该边界后停止；
5. 比较 status、overflow、required、peak、buffer extent/offset、lifetime、multi-buffer 和
   applied inplace；
6. 完整结论覆盖独立 attempts 的 seeds 0～19。

入口：

```bash
.venv/bin/python3 ub_overflow_model_cpp/scripts/run_bisheng_embedded_matrix.py \
  --seeds 0-19 --jobs 12
```

每个 seed 必须独立启动真实 compiler。不得用缓存替代本次原生 PlanMemory，也不得把同进程
连续运行 20 seeds 当作独立生产 attempt。

## 2. before-AutoBlockify 扩展的逐 pass oracle

新增前缀不能只验证最终 peak。每个 pass 必须从同一个真实 before-AutoBlockify `ModuleOp`
分叉，分别执行原生 pass 和当前累计轻量前缀，并比较稳定结构 checkpoint。

checkpoint 顺序：

```text
after AutoBlockify
after pre-CV MarkMultiBuffer
after outer ExtendedCanonicalizer
after ArithToAffine
after CanonicalizeIterArg
after ExtendedCanonicalizer(module)
after SCFForLoopCanonicalization
after CSE
after ExtendedCanonicalizer(func)
after HIVMOptSinglePoint
after ExtendedCanonicalizer(func)
after MemrefDeadStoreElimination
after InlineOTFBroadcast == before CVPipelining
```

每个 checkpoint 至少比较：

- operation/block/region 顺序和层级；
- result、operand、iter_arg、yield 的对应关系；
- type、shape、layout、address space 和 effects；
- workspace、multi-buffer、preload、broadcast、core-type、AutoBlockify 属性；
- create/erase/replace 后的 use-list 和 value identity。

如果序列化中的 SSA 编号不同，应先建立结构映射再比较；不能直接忽略 operation/value identity
差异。原生无序容器导致的合法置换只能沿用已经有现场证据的分类，不能自动扩展到新增 pass。

每个 pass 的验证闸门：pattern fixture → 单 pass checkpoint → 从 AutoBlockify 开始的累计
checkpoint → 代表输入最终 PlanMemory seeds `{0,13}`。未通过不得开始下一个 pass。

阶段 0 原生 checkpoint 已于 2026-07-30 建立。常用命令：

```bash
python3 ub_overflow_model_cpp/scripts/dump_ub_prefix_checkpoints.py \
  ub_overflow_model_cpp/data/adapter/ascend_tutorial_01-vector-add.ttadapter \
  --compiler build/bin/bishengir-compile \
  --output-dir /tmp/cvub-prefix-checkpoints
```

脚本默认在 `13_after_inline_otf_broadcast` 后停止；不会为取得 checkpoint 继续运行 CVPipelining
或 PlanMemory。若要覆盖非默认参数，在命令最后加 `-- <bishengir-compile arguments...>`。

阶段 1 AutoBlockify 使用同一次 attempt 的前后检查点全量验证：

```bash
python3 ub_overflow_model_cpp/scripts/verify_auto_blockify_pipeline.py \
  --jobs 8 \
  --json-report ub_overflow_model_cpp/output/auto_blockify_pipeline_verification/full.json
```

2026-07-30 结果为 `160/160 PASS`；报告是可再生成本地产物，不提交。禁用 gate 另做原生
before/after 字节一致和模型结构一致检查。

阶段 2 pre-CV MarkMultiBuffer 使用 8 个真实 profile 同时检查单 pass 和累计前缀：

```bash
python3 ub_overflow_model_cpp/scripts/verify_pre_cv_mark_multi_buffer_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/pre_cv_mark_mb_full.json
```

2026-07-30 结果为 `8 profiles × 160 inputs = 1280/1280 PASS`。每项同时比较
`01_after_auto_blockify -> MarkMultiBuffer` 与
`00_before_auto_blockify -> AutoBlockify -> MarkMultiBuffer` 到同一原生
`02_after_pre_cv_mark_multi_buffer` checkpoint。fixture 另由真实 `bishengir-opt` 证明 local Load、
Fixpipe 和 scope preload pattern 一致。代表下游 PlanMemory 小回归为 8/8 matched；该小回归证明
当前既有 before-CV→PlanMemory 路径未回退，新增 prefix 的直接证明仍以同-attempt 结构完全一致
为准。

阶段 3 outer module-level ExtendedCanonicalizer 使用同一 8-profile 矩阵：

```bash
python3 ub_overflow_model_cpp/scripts/verify_outer_extended_canonicalizer_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/outer_extended_canonicalizer_phase3.json
```

2026-07-30 结果为 `8 profiles × 160 inputs = 1280/1280 PASS`；每项同时比较单 pass
`02 -> outer canonicalizer` 与累计 `00 -> AutoBlockify -> MarkMultiBuffer -> outer canonicalizer`
到同一原生 `03_after_outer_extended_canonicalizer`。原生 `bishengir-opt --canonicalize-ext`
fixture 结构完全一致；代表下游 PlanMemory 小回归为 `8/8 matched`。

阶段 4.1 ArithToAffine：

```bash
python3 ub_overflow_model_cpp/scripts/verify_arith_to_affine_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/arith_to_affine_phase4_1.json
```

2026-07-30 的单 pass `03 -> 04` 与累计 `00 -> 04` 同-attempt checkpoint 结果均为
`8 profiles × 160 inputs = 1280/1280 PASS`；全运算 fixture 与真实
`bishengir-opt --convert-arith-to-affine` 精确一致；代表下游 PlanMemory 为 `8/8 matched`。

阶段 4.2 CanonicalizeIterArg：

```bash
python3 ub_overflow_model_cpp/scripts/verify_canonicalize_iter_arg_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/canonicalize_iter_arg_phase4_2_final.json
```

2026-07-30 的单 pass `04 -> 05` 与累计 `00 -> 05` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`；For/While/backward/vector gate fixture 与真实
`bishengir-opt --scf-canonicalize-iter-arg` 精确一致；完整模型测试通过；代表下游 PlanMemory 为
`8/8 matched`。

阶段 4.3 module-level ExtendedCanonicalizer：

```bash
python3 ub_overflow_model_cpp/scripts/verify_module_extended_canonicalizer_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/module_extended_canonicalizer_phase4_3.json
```

2026-07-30 的单 pass `05 -> 06` 与累计 `00 -> 06` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。聚焦 fixture 验证 affine fixed point、fresh constant
物化/CSE/hoist 和 semi-affine local 项顺序；阶段 4.2 的 1280 项回归、完整模型测试和代表下游
PlanMemory `8/8 matched` 通过。

阶段 4.4 SCFForLoopCanonicalization：

```bash
python3 ub_overflow_model_cpp/scripts/verify_scf_for_loop_canonicalization_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/scf_for_loop_phase4_4.json
```

2026-07-30 的单 pass `06 -> 07` 与累计 `00 -> 07` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。fixture 另外与真实
`bishengir-opt --scf-for-loop-canonicalization` 比较，覆盖 affine min/max 的 complete/partial/no-op、
nested/parallel/forall bounds，以及 iter-arg/loop-result shape-preserving dim folding；完整测试通过，
代表下游 PlanMemory 为 `8/8 matched`。

阶段 4.5 CSE：

```bash
python3 ub_overflow_model_cpp/scripts/verify_cse_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/cse_phase4_5.json
```

2026-07-30 的单 pass `07 -> 08` 与累计 `00 -> 08` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。fixture 另外与真实 `bishengir-opt --cse` 比较，覆盖
commutative CSE、nested visibility、sibling scope、read-only/write barrier、pure single-block
region-op equivalence 和 trivially-dead erase；完整模型测试通过，代表下游 PlanMemory 为
`8/8 matched`。

阶段 4.6 第一次 func-scoped ExtendedCanonicalizer：

```bash
python3 ub_overflow_model_cpp/scripts/verify_first_func_extended_canonicalizer_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/first_func_canonicalizer_phase4_6.json
```

2026-07-30 的单 pass `08 -> 09` 与累计 `00 -> 09` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。双函数 fixture 与真实嵌套
`builtin.module(func.func(canonicalize-ext))` 比较，确认 constant folder/worklist 不跨 function；
完整模型测试通过，代表下游 PlanMemory 为 `8/8 matched`。

阶段 4.7 HIVMOptSinglePoint：

```bash
python3 ub_overflow_model_cpp/scripts/verify_hivm_opt_single_point_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/hivm_single_point.json
```

2026-07-30 的单 pass `09 -> 10` 与累计 `00 -> 10` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。定向 fixture 与真实
`builtin.module(func.func(hivm-opt-single-point))` 精确一致，覆盖 f32/i64 elementwise、VBrc、
Copy/Load、host、no-IO-alias、只读 user closure 和不匹配分支；另证实原生
`VMax/VMin<ui64>` 的 verifier 失败并在模型中 fail open。完整测试通过，代表下游 PlanMemory
为 `8/8 matched`。

阶段 4.8 第二次 func-scoped ExtendedCanonicalizer：

```bash
python3 ub_overflow_model_cpp/scripts/verify_second_func_extended_canonicalizer_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/second_func_canonicalizer.json
```

2026-07-30 的单 pass `10 -> 11` 与累计 `00 -> 11` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。定向双函数 fixture 与真实嵌套
`builtin.module(func.func(canonicalize-ext))` 精确一致；完整测试通过，代表下游 PlanMemory 为
`8/8 matched`。

阶段 4.9 MemrefDeadStoreElimination：

```bash
python3 ub_overflow_model_cpp/scripts/verify_memref_dse_pipeline.py \
  --jobs 8 \
  --json-report ub_overflow_model_cpp/output/memref_dse.json
```

2026-07-30 的单 pass `11 -> 12` 与累计 `00 -> 12` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。普通与 Ascend950 reg-based 定向 fixture 分别与
真实 `builtin.module(func.func(memref-dse))` 精确一致，覆盖同层转发、scalar-like memref、
ViewLike alias、write/call/nested-region barrier、死 alloc/subview/store 链和尾部无用 HIVM Load
清理；完整测试通过，代表下游 PlanMemory 为 `8/8 matched`。最小 Ascend950 fixture 证明原生
pass 在 store-to-load forwarding 后会于 reg-based cleanup 中 SIGSEGV；模型将此原生失败显式
fail open，未修改原生逻辑也未把该失败当作 matched。

阶段 5 InlineOTFBroadcast：

```bash
python3 ub_overflow_model_cpp/scripts/verify_inline_otf_broadcast_pipeline.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/inline_otf_broadcast.json
```

2026-07-30 的单 pass `12 -> 13` 与累计 `00 -> 13` 同-attempt checkpoint 均为
`8 profiles × 160 inputs = 1280/1280 PASS`。普通与 Ascend950 定向 fixture 与真实
`builtin.module(func.func(hivm-inline-otf-broadcast))` 的稳定结构 `2/2` 精确一致，覆盖
LAST/non-LAST、类型和 trait 拒绝、多个/部分 user、已有 broadcast axes、重复 DPS use、
buffer semantics、VAbs i16、VIsInf 以及 Ascend950 VShL；完整测试通过，代表下游 PlanMemory
为 `8/8 matched`。

阶段 6 combined before-CV：

```bash
python3 ub_overflow_model_cpp/scripts/verify_combined_pre_cv_prefix.py \
  --jobs 12 \
  --json-report ub_overflow_model_cpp/output/combined_pre_cv_prefix.json
```

2026-07-30 使用唯一 `RunPreCVPrefixPipeline` 从 `00_before_auto_blockify` 一次运行到
`13_after_inline_otf_broadcast`，8 profiles × 160 inputs 为 `1280/1280 PASS`。验证框架为组合
入口的两条执行路径均传递相同 profile 参数；不再把缺少 profile 的二次执行误报为模型差异。
API options v5、before-AutoBlockify contract v2 与新 fingerprint 的构建和完整测试通过；旧
contract v1 仍只执行既有 CV→PlanMemory 路径。

阶段 7 embedded 切换与最终验证：

- 调用点位于 `00_before_auto_blockify` 后、原生 AutoBlockify 前；模型 v2 内部运行前缀，原生
  pipeline 在 validation 中通过显式 `prune=false` 继续到 local PlanMemory。产品源码不再强制
  observe-only，Exact overflow 已恢复剪枝。
- 27 configs × 4 inputs × seeds `{0,13}`：`200 matched / 16 unavailable / 0 different`；16 个
  unavailable 精确对应两个 `disableAutoCVWorkSpaceManage=true` 场景，此时产品按设计不插入
  prediction pass。
- 排除上述两个无模型观测的场景后，25 configs × 4 inputs × seeds `0-19`：1969 strict
  matched，31 个 `python_tutorial_06-fused-attention` 等尺寸 identity permutation，0 个 UB
  决策差异、blocker、unavailable 或 timeout。
- identity permutation 只有在 status/required/peak/multi 已一致，且删除 physical offset 后
  完整 extent multiset、lifetime relation 和 inplace graph 仍一致时才单独分类；不能豁免其他
  plan/lifetime/inplace 差异。

最终全量现场结果：26 个有效配置 × 160 inputs × seeds 0～19，理论 83200；66 个已审核原生
长超时 pair 对应的 1320 个 attempts 按清单排除，实际报告 81880：

```text
matched                                      81789
identity_permutation                            31
different                                        0
unavailable                                     60
timeout                                          0
reported total                               81880
known-timeout skipped                         1320
```

31 项全部来自 `python_tutorial_06-fused-attention` 的严格等尺寸 identity permutation；60 项来自
`preload_auto_mb` 下三个 matmul 输入的原生 SIGABRT（3×20），不是模型差异。新增
`auto_blockify` 配置自身为 3200/3200 strict matched。报告为
`output/validation/before_auto_8ebd4f120_160x26x20.tsv`，本地生成、不提交。

## 3. 正确性分类

- `matched`：完整合同一致；
- `strict different`：字段存在差异；已知等尺寸 buffer identity 置换必须单独标注；
- `unavailable`：一侧没有稳定可比较观测，例如产品 pass 不插入或原生 SIGABRT；
- `timeout`：达到当前明确的超时门槛；
- `known-timeout skipped`：由已审核清单排除，不进入报告分母，必须单独给数量。

任何 unavailable、timeout 或 skipped 都不能写成 matched，也不能笼统写成“全通过”。

## 4. 2026-07-29 旧边界全量正确性基线（历史）

理论规模：`160 × 27 × 20 = 86400`。

已知原生长超时清单包含 66 个 input/config pair，对应 1320 个 seed attempt；按用户既定要求
没有执行。因此现场报告规模为 85080：

```text
matched                                      78583
strict different                                37
unavailable                                   6460
timeout                                          0
reported total                                85080
known-timeout skipped                          1320
```

37 个 strict difference 全部属于已知 fused-attention 等尺寸 buffer identity 置换，只影响
plan/lifetime/inplace；status、overflow、required、peak 和 multi-buffer 一致。

6460 个 unavailable 分布：

- 6400：`cv_workspace_manage_off` 和 `cv_workspace_manage_off_auto_mb` 两个配置下产品 prediction
  pass 不插入；
- 60：三个 matmul 输入在 `preload_auto_mb` 下原生 compiler SIGABRT。

报告：

```text
ub_overflow_model_cpp/output/validation/full_1dfddd59_160x27x20.tsv
```

报告是本地可再生成产物，不提交。

## 5. 性能测量的两种口径

### 5.1 结构优化主口径：完整模型路径

用于判断模型自身基础设施或局部实现是否变快：

- Release/O3；
- 相同 160 inputs × 当前有效 read-only configs；
- 未固定 seed 的真实 retry-only；
- 关闭 validation、dump、stage artifact 和详细 stage timing；
- 关闭提前 non-overflow 返回；
- 同时关闭 conservative non-overflow proof 计算；
- 每个模型 case 必须显示 `decision_path=full_plan`；
- 所有模型 case 完整执行到 PlanMemory。

该口径是后续 P0/P1 优化的主门槛。

### 5.2 产品口径：允许提前 non-overflow

用于观察真实产品默认行为和 fast-path 命中率。它会跳过部分 PlanMemory 工作，因此不能用来
证明基础设施本身变快，也不能与 4.1 混写成同一加速比。

## 6. 2026-07-29 旧边界完整路径性能基线（历史）

输入为 160 × 27 = 4320 个模型 case，提前返回及 proof 均关闭：

```text
model observed                              4320 / 4320
decision_path=full_plan                     4320 / 4320
non_overflow_upper_bound_proven=false       4320 / 4320
status                  4282 success / 26 overflow / 12 blocker
model internal total                         23836.604 ms
per-case median / mean / p95 / max       2.187 / 5.518 / 22.089 / 93.149 ms
model process wall                           111.509 s
model peak RSS                                51.266 MiB
```

原生侧跳过 66 个已知长超时 pair；模型侧仍执行了这 66 项。可配对 4254 项：

```text
paired model internal total                  22621.361 ms
native CVPipelining->local PlanMemory wall   89363.428 ms
BiSheng / model aggregate ratio                  3.9504x
native per-case median / mean / p95 / max
                                      7.478 / 21.007 / 45.232 / 1221.711 ms
native peak RSS                               113.563 MiB
```

若仅统计原生单 attempt 的 4219 项：

```text
paired model internal total                  20630.613 ms
native boundary wall delta                   77505.800 ms
BiSheng / model                                   3.7568x
```

正式表述：在当前 160 × 27 read-only、完整 PlanMemory、无提前判定的主口径下，轻量模型约为
原生 BiSheng CVPipelining→local PlanMemory 路径的 `3.95x` 速度。这个比例使用模型内部总时间
对原生边界 wall delta；二者计时来源不同，报告时必须保留该说明。

报告：

```text
ub_overflow_model_cpp/output/performance/1dfddd59_read_only_160x27.tsv
```

## 7. fast-path 参考数据

以下只用于区分昨天/今天看似矛盾的结果：

```text
production-default + fast path                 约 6.81x
production-default + full plan                 约 4.006x
160 × 27 read-only + full plan                 约 3.950x
```

`6.81x` 命中了 147/160 个 exact non-overflow fast path，因此不是完整路径速度。后续结构优化不得
用 fast-path 命中增加代替 core pipeline 加速。

## 8. 2026-07-30 新边界全量性能结果

新模型增加 AutoBlockify→before-CV 的真实工作，因此旧 `CVPipelining→PlanMemory` 的 `3.95x`
不是新边界的加速比。正式测量使用 Release/O3、160 inputs × 26 有效配置 × 3 轮、真实
retry-only、关闭提前 non-overflow 返回及证明计算，所有模型 attempt 都执行到完整 PlanMemory：

```text
model full-plan samples                         12480 / 12480
paired native samples                           12174
known native timeout skipped                      198
native unavailable                                108
model internal total                            273096.841 ms
model median / mean / p95 / max       3.370 / 21.883 / 37.088 / 3056.203 ms
paired model total                              249990.946 ms
native same-boundary total                      249146.574 ms
native median / mean / p95 / max       5.640 / 20.465 / 44.419 / 9669.407 ms
BiSheng / model aggregate ratio                    0.9966x
round ratios                         1.0215x / 0.9787x / 0.9870x
process wall total                                859.586 s
peak RSS                                           111.219 MiB
```

正式结论是模型与原生 BiSheng 在新边界基本持平：`0.9966x` 表示模型约慢 0.34%，而且三轮
波动跨过 1.0，不能宣称稳定加速。模型中位数更快，但 `triton.language.static_range` 的 78 个
配对样本中模型耗时 123.267 s、原生 7.952 s，占配对模型总量约 49.3%；排除它后为 1.9033x，
但该排除倍率只用于定位热点，不能替代正式全量倍率。

一轮 4160 项的附加 stage 诊断（计时探针会产生额外开销，不能替代正式倍率）为：

```text
input bridge                                      0.369 s
pre-CV prefix                                    61.053 s
existing CV->PlanMemory                          25.981 s
four ExtendedCanonicalizer stages                55.181 s
```

四个 canonicalizer 约占该轮模型内部总时间 63%；`static_range` 的 39.487 s 中约 91.8% 同样来自
四个 canonicalizer。正式报告为
`output/performance/before_auto_8ebd4f120_160x26x3.{tsv,json}`，stage 报告为
`output/performance/before_auto_8ebd4f120_stage_160x26.{tsv,json}`，均不提交。

## 9. 每个优化批次的验证门槛

2026-07-30 起，默认参数矩阵新增 4 个非笛卡尔积交互场景：`auto_blockify_preload`、
`auto_blockify_auto_mb`、`auto_blockify_auto_mb_local_only` 和
`auto_blockify_auto_mb_unrestricted`。当前矩阵共 32 组，全部是有效配置；下一次全量正确性
理论规模为 `160×32×20=102400`，三轮 full-plan 性能规模为
`160×32×3=15360`。上文 26 配置结果仍是扩展前已经完成的历史现场基线，不能改写为 32 配置
已经全量通过。

2026-07-30 修正了 prediction 对 workspace manager 开关的错误 gate：
`cv_workspace_manage_off` 与 `cv_workspace_manage_off_auto_mb` 现在仍插入轻量模型，并把
`disable-auto-cv-work-space-manage=true` 同时传给模型和原生流水。原生 greedy canonicalization
会通过 `OperationFolder` 把 loop 内常量提升到 isolated function entry，并合并等价常量；这一
顺序会影响 PlanMemory live-list shuffle 的确定性 RNG 状态，因此轻量模型按原生
`FoldUtils.cpp::insertKnownConstant` 行为补齐，而不是在 PlanMemory 中补偿 RNG。现场验证结果：

```text
configs × inputs × seeds                    2 × 160 × 20 = 6400
strict matched                                             6400
identity permutation / different / unavailable / timeout  0 / 0 / 0 / 0
```

同两组的 Release/O3、production full-plan、160×2×3 性能测量中，960 个模型样本全部
`decision_path=full_plan`，921 个可配对样本的原生/模型 aggregate ratio 为 `0.7301x`，即模型
约为原生耗时的 `1.37x`。这是这两组首次成为有效产品路径后的基线，不宣称为加速；后续性能
优化必须单独处理该回退。完整测试套件同时通过。

1. 相关单测；
2. `bash ub_overflow_model_cpp/tests/run_tests.sh`；
3. simple AIV、MIX、attention overflow、late-seed success、auto-MB、UB-saving、
   InjectBlockSync 的 seeds 0～19 embedded 对比；
4. ordering、identity、PlanMemory 或输入合同变化时，执行当前有效配置的 160 × configs × 20
   现场矩阵；known timeout/unavailable 单列；
5. 完整路径性能使用 baseline/new 同机交错 A/B 至少 3 轮；
6. 报告 internal total、process wall、median、mean、p95、max、RSS、status 分布和
   BiSheng/model ratio；
7. fast path 单独报告命中率、fall-through 和 decision parity。

## 10. 禁止的验证捷径

- 用缓存替代当前 embedded 原生 PlanMemory；
- 减少 seed 或配置来宣布完整正确性；
- 把 mismatch 改成 blocker/incomplete；
- 只比较 peak/overflow，忽略 fixed-seed plan/lifetime/multi/inplace；
- 将 timeout、SIGABRT、pass 未插入或 known-timeout skipped 计为 matched；
- 为通过矩阵修改原生 BiSheng 遍历或增加 kernel/seed/config 特例；
- 开着提前判定，却把结果描述为完整 CVPipelining→PlanMemory 性能。
- 跳过逐 pass checkpoint，只凭最终 PlanMemory 一致宣布新增前缀正确；
- 根据 corpus 差异反推出没有原生源码依据的 rewrite；
- 把当前输入未触发的 canonicalization、DSE 或 single-point pattern 永久写成 no-op。
