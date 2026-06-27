# Task Context

## 用户目标

用户当前任务的中心是：

在 overflow 真实发生之前，提前建立模型，精确计算未来编译过程中预计的内存占用。
这样可以在真实编译环境中避免 PlanMemory overflow，避免编译器触发 fallback 或 rollback。

这个任务不是简单判断某个测试能不能过，而是要根据当前编译选项、pipeline 状态和 IR
内容，预测后续 PlanMemory 会看到的内存压力。

## 当前确认结论

- 用户关心的 overflow 主要发生在 BiShengIR 自己的 `hivm-plan-memory` pass。
- 该阶段是静态 IR 分析和内存规划阶段，不需要 CANN，不需要 `hivmc`，也不需要真实昇腾卡。
- macOS 上只要仓库已经构建出 `build/bin/bishengir-opt`，就可以复现 PlanMemory overflow。
- 如果输入已经到了 HIVM/memref 层级，PlanMemory 可以直接分析。
- 如果输入还停留在纯 tensor/TTIR 层级，就太早，必须先经过前面的 lowering/bufferization/mem scope 推断。

## 任务边界

当前优先关注 local memory:

- UB
- L1/CBUF
- L0C/CC

global workspace 也和 PlanMemory 相关，但用户目前最关心的是 tile size 调大导致的 local memory overflow。

## 不要混淆的问题

这些不是当前任务的核心：

- `hivmc` 找不到。
- CANN 没有安装。
- 昇腾卡环境不可用。
- macOS 上 full compile 或 full test 不通过。
- Sphinx docs warning。

这些问题都可能影响完整编译或测试，但不影响当前做 PlanMemory 前置内存估算的核心任务。

## 用户样例文件的关系

用户提到过一个文件夹里有：

- `attention.py`
- `_attn_fwd.ttir`
- `_att_fwd.ttadapter`
- `debug.log`

当前理解：

- `attention.py` 是原始 Python/Triton/模型侧代码。
- `.ttir` 是较早的 tensor IR。
- `.ttadapter` 是继续 lower 后的 adapter IR，用户观察到其中已经有一部分 tensor、一部分 memref。
- 如果 `.ttadapter` 已经包含 `memref<... #hivm.address_space<ub/cbuf/cc>>`，它可能已经接近 PlanMemory 可分析的层级。

不要在记忆文件中记录这些私有文件的具体内容，除非用户明确提供并允许记录。
