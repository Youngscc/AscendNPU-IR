# Decisions And Risks

## 已做判断

### 1. 当前任务不依赖 CANN 或昇腾卡

原因：

PlanMemory overflow 是 BiShengIR 内部静态 IR 规划阶段的问题。只要 IR 已经到 HIVM/memref
层级，macOS 上的 `bishengir-opt` 就能复现。

### 2. 估算器应该运行在 PlanMemory 之前

原因：

真实 overflow 一旦由 `hivm-plan-memory` 发出，就可能被 retry/fallback 机制捕获。用户目标是提前预测，避免进入这一流程。

### 3. 精确估算不应完全外置

原因：

PlanMemory 有 liveness、alias、inplace、多缓冲、retry 顺序等内部逻辑。外部脚本很难完全复刻。

### 4. 记忆文件不放入 docs

原因：

这些内容是任务上下文，不是项目用户文档，不应该影响 Sphinx build。

## 主要风险

### 风险 1: 估算位置过早

如果估算在 `AllocExtraBuffer` 或 `MarkMultiBuffer` 之前运行，会漏掉后续新增或扩张的 buffer。

缓解：

把 estimator 放在 local `createPlanMemoryPass` 前一刻。

### 风险 2: 预测输出触发 fallback

现有 retry policy 会查找 overflow diagnostic pattern。

缓解：

预测输出不要使用 `ub overflow`、`cbuf overflow`、`cc overflow` 这类文本。

### 风险 3: 动态 shape

动态 shape 无法简单静态求精确字节数。

缓解方案需要用户确认：

- 拒绝动态 shape；
- 使用上界；
- 符号化表达；
- 从 compile config 或 tiling 参数中取具体值。

### 风险 4: 和真实 PlanMemory 行为漂移

如果估算器复制了一套独立算法，后续 PlanMemory 修改后容易不一致。

缓解：

复用 PlanMemory 内部类和函数，避免双份逻辑。

### 风险 5: PlanMemory 当前有 retry 顺序

`PlanMemoryPass::runOnOperation()` 当前最多尝试 20 次 deterministic shuffle seed。

估算器必须明确：

- 是报告第 1 次尝试的需求；
- 还是模拟全部尝试，报告是否存在可行布局；
- 还是报告 worst case。

用户要“精确避免真实 overflow”，更合理的是模拟真实 PlanMemory 的全部尝试策略。

## 开放问题

- 用户实际失败命令是什么？
- 用户实际打开了哪些 compile options？
- 输入是否总是 `.ttadapter`？
- 是否必须覆盖 workspace，还是只覆盖 local memory？
- 动态 shape 如何处理？
- 估算结果是否只报警，还是自动调小 tile size？
- JSON 输出路径应该如何指定？
