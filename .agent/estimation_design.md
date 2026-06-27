# Memory Estimation Design

## 设计目标

构建一个精确、轻量的前置内存估算能力：

- 输入当前 IR 和当前编译配置。
- 在真实 PlanMemory overflow 发生之前计算 UB/L1/L0C 预计占用。
- 不触发已有 overflow diagnostic pattern。
- 不触发 `RetriablePassManager` 的 fallback/rollback。
- 输出结构化结果，方便上层策略决定是否调 tile size 或改编译选项。

## 推荐位置

估算器应该放在 local PlanMemory 之前，也就是：

```text
createMarkMultiBufferPass
<run estimator here>
createPlanMemoryPass
```

原因：

- `AllocExtraBuffer` 已经创建临时 buffer。
- `InferHIVMMemScope` 已经知道 UB/L1/L0C。
- canonicalization 和 inline load copy 已经影响了 IR 形态。
- `MarkMultiBuffer` 已经决定哪些 buffer 会被多缓冲扩大。

如果估算更早做，会漏掉后面 pass 新增的 buffer 或 multi-buffer 扩张。

## 不建议纯外部脚本

纯脚本解析 MLIR 文本可以做粗估，但很难精确。

PlanMemory 的精确结果依赖：

- operation 顺序，
- nested region，
- liveness，
- alias，
- gen/kill，
- inplace reuse，
- multi-buffer 扩张，
- extra buffer，
- deterministic retry 顺序。

所以精确方案应该复用 C++ 内部数据结构。

## 推荐实现方案

新增一个 dry-run estimator pass，或者给 PlanMemory 增加 estimate-only 模式。

更推荐新增 pass，例如：

```text
hivm-estimate-memory
```

它内部复用：

```text
MemLivenessAnalysis
MemPlan
BufferInfo
StorageEntry
buffer2Life
buffer2MultiNum
genKillMap
inplacePairList
```

但是不要做最终 IR rewrite，不要把 `memref.alloc` 改成 `hivm.hir.pointer_cast`。

## 输出格式

建议 JSON：

```json
{
  "function": "kernel_name",
  "mode": "local-mem-plan",
  "scopes": [
    {
      "scope": "UB",
      "capacity_bits": 1572864,
      "required_bits": 1835008,
      "predicted_over_capacity": true,
      "headroom_bits": -262144
    }
  ]
}
```

不要在预测输出里使用：

```text
ub overflow
cbuf overflow
cc overflow
```

这些短语可能被现有 overflow retry policy 匹配。建议用：

```text
predicted_over_capacity
```

## 输出字段建议

每个 function:

```text
function_name
func_core_type
plan_mode
compile_config_snapshot
scope_estimates
```

每个 scope:

```text
scope
align_bits
capacity_bits
required_bits
required_bytes
predicted_over_capacity
headroom_bits
buffers
```

每个 buffer:

```text
value
source_location
defining_op_name
memref_type
address_space
raw_bits
aligned_bits
multi_buffer_num
expanded_bits
alloc_time
free_time
offset
is_tmpbuf
```

## 轻量模式和精确模式

可以保留两个层级：

1. 粗略 fast precheck:
   - 只看 `memref.alloc`、shape、element type、address space。
   - 适合快速筛掉明显安全或明显危险的 case。

2. 精确 dry-run:
   - 复用 PlanMemory 内部 liveness 和 planning。
   - 适合最终判断。

用户当前任务要求“精确”，所以最终必须落到第二种。

## 和现有 MemoryDisplay 的关系

MemoryDisplay 已经能导出：

- buffer
- offset
- extent
- alloc time
- lifetime
- source location
- tmpbuf 标记

可以复用或参考它的 schema。

但 MemoryDisplay 目前是在 PlanMemory 过程中产生调试信息，不是一个独立前置预警 pass。
实现估算器时，可以把 MemoryDisplay 的 JSON builder 改造成更通用的输出工具。
