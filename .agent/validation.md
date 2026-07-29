# 当前验证与性能基线

最后核实：2026-07-29，commit `1dfddd59f3f9835e4f51fd50f79aa75454b02e27`。

2026-07-30 起的当前开发任务是把输入边界前移到 before-AutoBlockify。下述 2026-07-29 数据
仍是旧 before-CVPipelining 产品基线；在新入口完成前不得把它描述为新边界结果。

## 1. 正确性 oracle

唯一主 oracle 是同一真实 `bishengir-compile` attempt 中的原生 local PlanMemory：

1. 从 adapter 运行真实 BiSheng prefix；
2. before-CVPipelining 运行 embedded lightweight model，使用本轮真实 resolved options；
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

## 3. 正确性分类

- `matched`：完整合同一致；
- `strict different`：字段存在差异；已知等尺寸 buffer identity 置换必须单独标注；
- `unavailable`：一侧没有稳定可比较观测，例如产品 pass 不插入或原生 SIGABRT；
- `timeout`：达到当前明确的超时门槛；
- `known-timeout skipped`：由已审核清单排除，不进入报告分母，必须单独给数量。

任何 unavailable、timeout 或 skipped 都不能写成 matched，也不能笼统写成“全通过”。

## 4. 2026-07-29 全量正确性基线

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
- 相同 160 inputs × 27 read-only configs；
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

## 6. 2026-07-29 完整路径性能基线

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

## 8. 新边界的性能口径

新模型增加 AutoBlockify→before-CV 的真实工作，因此旧 `CVPipelining→PlanMemory` 的 `3.95x`
不是新边界的加速比。完成组合前缀后必须重新测量：

```text
new lightweight before-AutoBlockify->PlanMemory internal time
native BiSheng AutoBlockify->local PlanMemory boundary wall delta
AutoBlockify->before-CV prefix stage time
existing CV->PlanMemory stage time
BiSheng/model ratio at the same new boundary
```

主结构性能仍关闭提前 non-overflow 返回和证明计算，全部执行到完整 PlanMemory。入口切换前后
工作量不同，旧/新模型总时间只能分解说明，不能包装成严格同工作量 A/B。

## 9. 每个优化批次的验证门槛

1. 相关单测；
2. `bash ub_overflow_model_cpp/tests/run_tests.sh`；
3. simple AIV、MIX、attention overflow、late-seed success、auto-MB、UB-saving、
   InjectBlockSync 的 seeds 0～19 embedded 对比；
4. ordering、identity、PlanMemory 或输入合同变化时，执行当前有意义配置的 160 × 27 × 20
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
