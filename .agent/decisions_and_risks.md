# Decisions And Risks

## 已做判断

- 当前任务不依赖 CANN、`hivmc` 或真实昇腾卡。
- 当前短期主线是 dump 和解释第二次 PlanMemory，即 local
  `PlanMemory(LOCAL_MEM_PLAN)`，不是继续推进逐 pass Python estimator。
- 精确内存规划信息以真实 PlanMemory 和 `memory_info*.json` 为准。
- `tools/dump_planmemory_ir.sh` 是批量观察 `data/` 输入前后 IR 的当前入口。
  默认只输出 before、memory_info 和日志；after dump 依赖 `--dump-full-ir-tree` 或
  `--drop-full-tree`。
- `tools/run_ub_peak_pass_ablation.sh` 是第一阶段 UB peak pass-group 消融入口；它只用现有
  `bishengir-compile` 开关，不改 compiler 源码，默认不创建 full `ir_tree/`。
- `BISHENGIR_DUMP_BEFORE_PLAN_MEMORY` 的 dump 点位于 `MarkMultiBuffer` 和 local
  PlanMemory 之间；现在还需要 `--enable-dump-ir-before-plan-memory` 才会插入该 debug
  pass。
- 新增 compile option 后需要重新 build；旧 `build/bin/bishengir-compile` 可能不认识
  `--enable-dump-ir-before-plan-memory`。
- `memory_info*.json` 中 `offset` 是 byte，`extent` 是 bit；不要把两者混算。
- 对 `data/attn_fwd.ttadapter` 的第一阶段现有开关矩阵，`cv_workspace_off` 是唯一明显降低
  AIV UB peak 的配置；它是 coarse pass-group 结论，不能直接归因到单个 pass。
- 旧的 `memory_modeling/` Python 框架仍在仓库中，但当前已暂停；除非用户明确恢复，
  不再把它作为主要路线。

## 主要风险

### 混淆 before/after dump

同一目录里可能有多个 `*_hivm-plan-memory.mlir`，包括 host helper、global workspace
PlanMemory、AIC/AIV 函数、成功/失败 dump。

缓解：分析 local PlanMemory 时优先使用 `before_local_plan_memory.mlir` 和
`plan_memory_after_dumps/` 下对应 `hacc.entry` 函数的 after dump。不要只看旧脚本生成的
顶层 `after_local_plan_memory.mlir`。

### after dump 不存在

`after_local_plan_memory*.mlir` 不是专门的环境变量 dump，而是从 MLIR full pass
`ir_tree/` 中拷贝出来的。默认不创建 full tree 时，after dump 和
`plan_memory_after_dumps/` 不会生成。

缓解：需要 after dump 时运行 `tools/dump_planmemory_ir.sh --drop-full-tree`；需要逐 pass
IR 全量观察时运行 `--dump-full-ir-tree`。

### AIV 失败 dump 误读

AIV 等 overflow case 里，after-failed dump 可能仍保留 `memref.alloc`。这不是 rewrite
遗漏，而是 PlanMemory 失败后不会执行 `MemrefAllocaOpToPointerCastOpPattern`。

缓解：结合 `memory_info_aiv.json` 和 stderr 的 `requires/capacity` 判断失败原因；
`is_tmpbuf = true` 只是 MemoryDisplay 前置标记，不代表最终 lowering。

### 动态 shape

PlanMemory 精确规划依赖静态 buffer size。动态 shape 需要明确策略：

```text
拒绝 / 上界 / 符号表达 / 从 tiling 或 compile config 取具体值
```

### 和 PlanMemory 漂移

手工解释或外部脚本很容易漏掉 liveness、alias、inplace、multi-buffer、rollback、
spec-level、20 seed retry。

缓解：报告中引用 PlanMemory 代码路径和真实 `memory_info*.json`，不要凭裸 size 求和下结论。

### `requires` 误解

PlanMemory 的 `requires N bits` 不是裸 buffer size，也不是所有 UB buffer 求和；它来自
失败后重新规划失败 scope 得到的峰值地址边界。

缓解：使用 PlanMemory 或后续 dry-run oracle 的结果作为基准。

## 开放问题

- 是否需要为 report 生成自动化 before/after IR diff 摘要。
- 是否需要把 after PlanMemory 也改造成一个专门 debug pass，从而不依赖 full
  `--mlir-print-ir-after-all` tree。
- 如果恢复 estimator 主线，再回到 `pass_memory_modeling_guide.md` 和 `memory_modeling/`。
