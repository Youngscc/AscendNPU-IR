# CVPipeline UB Model Memory

只保存当前可执行事实，不记录开发流水账。接手任务先读本文件；定位源码再读
`code_map.md`。不要保存未授权的私有 IR。

## 目标与约束

```text
CVPipeline-before IR + relevant config
  -> lightweight C++ model
  -> exact absolute local UB peak / exact overflow required_bits
```

- Oracle：`build/bin/bishengir-cvpipeline-suffix-compile` 的 minimal suffix。
- 核心实现使用独立 C++；Python 仅用于生成数据和比较。
- 只保留 UB 相关语义，但必须与真实 pass 一致；禁止按 oracle 数值拟合规则。
- 真实 suffix pass 逻辑不可为测试修改；compiler 改动只允许只读 dump。
- 无法证明 exact 时返回 blocker，不能输出估计值冒充 exact。

## 当前状态

- B 完成：PlanMemory-before IR -> lifetime -> plan/offset/peak。8240 tuples 的
  lifetime 与 plan canonical TSV 全部 byte exact；比较器 mutation test 有效。
- C0-C6 完成：171 个可达 adapter、15 configs、2565 tuples、166 unique IR。
- C7 完成：1162 strategy tuples，17836 AIV/UB buffers、23303 accesses byte exact。
- C8 初版完成：结构化 bridge 直接生成 B 的 PlanMemory 输入，不经过临时 MLIR
  文本。166 unique IR 中 165 byte exact；唯一动态 stride-aligned allocation
  在生产入口显式 blocker，因此不会输出错误 peak。
- C9 初版完成：`config/initial_suffix_manifest.tsv` 冻结当前可执行 pass 顺序；
  正式逐 pass 编译器消融证据延后到 exact 版，不阻塞初版交付。
- D 已端到端打通：输入 before-CVPipeline generic IR 与 config，生产入口可直接
  输出接入 C+B 后的 UB plan/peak；D1 复刻 `createCVPipeliningPass` 的 UB
  可见语义，D2 复用当前 C suffix，D3 比较最终 plan/peak。

## C8 已补齐

- tensor/memref bufferization alias、DPS/SCF control、single-point load/store。
- FlattenOps、ReduceRankSubview、LiftLowestStride 的主要 PlanMemory 可见结构。
- VRec/VSub 常量与 fold、same-block pure CSE、loop normalization。
- AtomicXchg、AtomicRMW(and/or/xor)、AtomicCas 的软件锁、临时
  allocation/view、RAUW。
- VCmp/VCast 递归分解的浮点零常量、def-use 和事件顺序。
- 已删除真实 FlattenOps 不存在的“动态 subview 禁止 collapse”模型特判。
- 已补 `collapse(expand(x))`、`collapse(collapse(x))` 组合，以及 insert_slice
  原 rank subview 后再 flatten 的顺序。
- 已补 extended multiply 分解，以及 OneShotBufferize 将 load 目标 RAUW 到
  in-place `tensor.insert_slice` destination subview 的 allocation/order/alias。
- 已按真实 pass 顺序补 vconcat decomposition、静/动态 expand compose、copy 的
  LiftLowestStride；已补 scalar VSub 常量折叠顺序及 scalarize 后 bitcast buffer root。
- 已补 FlattenOps 生成 collapse 的组合追踪，以及仍有用户时不删除旧 collapse 的
  greedy rewrite/DCE 语义；已排除不具备 LiftLowestStride pattern 的 varange。
- 已按 BitcastOpInterface 补齐 bitcast bufferization 保持源 shape/layout/address space、
  仅替换 element type 的规则。

## C 精确版剩余

- 补齐动态 stride-aligned allocation 的最终 rank-reduced subview 物化。
- 对冻结的 suffix 做正式逐 pass 编译器消融并保存证据。

## D 初版划分

- D0：固定 before-CVPipeline 输入/config、after-CVPipeline oracle 和规范化比较。
- D1：完整复刻 `createCVPipeliningPass` 的全部 UB 相关语义；同输入/config 下
  after-CVPipeline 隐藏表示必须信息等价，不能以覆盖率目标删减 D1 逻辑。
- D2：复刻 after-CVPipeline 到 before-OneShotBufferize 之间下游必需且影响 UB
  的真实 pass，输出当前 C 入口。
- D3：连接 D+C+B，比较最终 lifetime、offset、peak/required_bits；不支持的
  整体输入可以 blocker，但不得输出估计值。

当前证据：D0 已有 160 unique before generic objects、171 snapshots × 15 configs
= 2565 after/C1 oracle tuples；provenance 完整。D1 已按
`CVPipelining.cpp` 补齐普通路径和 preload 路径的 workspace expansion、work item
拆分、local tensor/memref output、workspace subview/collapse/to_tensor、preload
scope/extract_slice、DPS init remap、workspace yield 用户和 atomic set/none/trailing
语义；官方 `cv-pipelining.mlir` 与 `cv-pipelining-preload.mlir` 的 UB 关键 op
name/type multiset 与 oracle 一致，adapter corpus 2565/2565 byte exact。
D1 已接入生产 CLI：`--action=plan-before-cvpipeline --before-cvpipeline-ir=<generic.mlir>`。
D3 端到端已通过：生产 `D1+C+B` 与保存的 after-CVPipeline `C+B` oracle 在
2565 tuples（seed=0、restrict=false）上 JSON 摘要和文本 plan buffer 表完全
一致；额外 20 cases × 20 seeds × 2 restrict modes = 800 tuples 的 JSON 摘要
和文本 plan buffer 表也一致。raw SemanticIR diff 仍含 effects/properties 和
SSA 编号顺序差异，验收应使用规范化信息等价比较。

## 验收

- C8 直接比较 model 与真实 PlanMemory-before canonical input，信息完全一致；不含 seed。
- C+B 再比较 lifetime 与 plan canonical TSV；固定 input/config/seed 必须逐字节一致。
- 必须持续通过 C7 1162 tuples 和 C++ unit tests。
- 最终输出含 `ub_peak_bits`、`capacity_bits`、`overflow`、overflow 时的
  `required_bits`；存在 blocker 时 precision 不得为 `exact_plan`。

## 维护

- 本文件只更新当前数字、剩余问题和决策；历史过程不写入。
- `code_map.md` 只放入口与命令。两文件总量尽量低于 150 行。
- 测试默认报告总量；仅失败时分类。
