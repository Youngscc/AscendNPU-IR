# `attn_fwd.ttadapter` local PlanMemory 代码阅读报告

## 范围和文件

本报告以 `cvpipeline_ub_model_cpp/data/adapter/attn_fwd.ttadapter` 为例，解释第二次 PlanMemory，也就是
`hivmPostBufferizationOptimizationPipeline` 中的 **local PlanMemory**。

先看这几个文件：

```text
before_local_plan_memory.mlir
after_local_plan_memory__attn_fwd_mix_aic.mlir
after_local_plan_memory__attn_fwd_mix_aiv.mlir
memory_info_aic.json
memory_info_aiv.json
compile.stderr.log
```

`before_local_plan_memory.mlir` 是 local PlanMemory 前的整模块 IR。这里的
`memref.alloc` 还只是“我要一块 UB/CBUF/CC 本地内存”的抽象申请，PlanMemory 还没有给它们安排
scope 内 byte offset。

`after_local_plan_memory__attn_fwd_mix_aic.mlir` 是 local PlanMemory 处理完
`@_attn_fwd_mix_aic` 后的 IR。AIC 侧规划成功，所以能看到 CBUF/CC 的 `memref.alloc`
被改成 `hivm.hir.pointer_cast(offset)`。

`after_local_plan_memory__attn_fwd_mix_aiv.mlir` 是 local PlanMemory 继续处理到
`@_attn_fwd_mix_aiv` 后的 IR。AIV 侧 UB overflow，所以 AIV 里的 UB `memref.alloc`
仍然保留。

顶层 `after_local_plan_memory.mlir` 现在会选最后一个 entry 的 local PlanMemory dump，可用于快速查看；
但为了讲清楚 AIC/AIV 各自发生了什么，本报告优先使用上面两个按 entry 拆开的 after 文件。

本 case 的最终结果是：

- `@_attn_fwd_mix_aic` 成功：CBUF/CC 的 local alloc 被替换成 `pointer_cast(offset)`。
- `@_attn_fwd_mix_aiv` 失败：UB overflow，`requires 1716224 bits while 1572864 bits available`。
- AIV 失败后不会执行最终 rewrite，所以 after-failed IR 里仍有 UB `memref.alloc`。

## IR 语法速读

可以先把 MLIR 当成一种“带类型的数据流文本”：

```mlir
%结果名 = dialect.op_name(参数) {属性} : 参数类型 -> 结果类型
```

很多 HIVM op 用 `ins(...) outs(...)` 表示输入和输出 buffer：

```mlir
hivm.hir.vbrc
  ins(%cst : f16)
  outs(%collapse_shape : memref<16384xf16, #hivm.address_space<cbuf>>)
```

这表示把输入值 broadcast/fill 到 `outs` 指定的 buffer 中。PlanMemory 关心的是：

- `memref.alloc`：真正申请 local storage。
- `outs(...)`：说明某个 op 写哪个 buffer，会影响 liveness。
- `temp_buffer(...)`：说明 op 需要额外 scratch buffer。
- `scf.for iter_args(...)`：说明 buffer 跨循环迭代活着，生命周期会变长。

常见地址空间：

| address space | 含义 |
| - | - |
| `gm` | global memory/workspace，不是 local PlanMemory 主要对象 |
| `cbuf` | CBUF/L1，AIC 用 |
| `cc` | CC/L0C，AIC 用 |
| `ub` | UB，AIV 用 |

常见 op：

| op | 怎么读 |
| - | - |
| `memref.alloc()` | 声明一块 local buffer |
| `hivm.hir.pointer_cast(%c32768_i64)` | PlanMemory 后，从 scope 内 byte offset 32768 构造 typed buffer view |
| `memref.cast` | 改类型描述，不新分配 storage |
| `memref.subview` | 从已有 buffer 切片，不新分配 storage |
| `memref.collapse_shape` / `expand_shape` | reshape view，不新分配 storage |
| `memref.reinterpret_cast` / `memref.view` | 从 GM/workspace 或 byte buffer 构造 view |
| `memref_ext.alloc_workspace` | 从 GM workspace 参数中按 offset 取一块 |
| `hivm.hir.nd2nz` | DMA copy，并做 ND 到 NZ layout 转换 |
| `hivm.hir.mmadL1` | CUBE 矩阵乘加，输入在 CBUF，输出在 CC |
| `hivm.hir.fixpipe` | 把 CC 结果搬回 GM/workspace |
| `hivm.hir.vbrc/vmul/vexp/vreduce/...` | VECTOR 侧计算，通常读写 UB |

## Before IR 逐段导读

这一节先不讲 PlanMemory 的 C++ 代码，只讲 `before_local_plan_memory.mlir` 每一部分是什么意思。

### 1. `#map` 和 module attributes，行 1-24

行 1-8 是 affine map，也就是后面 `affine.apply/min/max` 会复用的小公式。

- `#map` 做 `base + block * stride`，常用于算 GM tensor 的线性 offset。
- `#map4` 做 `(remain + 15) floordiv 16`，常用于把实际剩余长度换成 16 对齐 tile 数。
- `#map7` 做 8 对齐，给后面临时 view 的大小计算用。

行 9-24 是 module 级硬件和编译属性：

| 属性 | 含义 |
| - | - |
| `UB_SIZE = 1572864` | UB 容量，单位 bit，即 192 KiB |
| `L1_SIZE = 4194304` | L1/CBUF 容量，单位 bit，即 512 KiB |
| `L0C_SIZE = 1048576` | L0C/CC 容量，单位 bit，即 128 KiB |
| `UB_ALIGN_SIZE = 256` | UB 对齐，单位 bit，即 32 B |
| `L1_ALIGN_SIZE = 256` | CBUF 对齐，单位 bit，即 32 B |
| `L0C_ALIGN_SIZE = 4096` | CC 对齐，单位 bit，即 512 B |
| `hivm.module_core_type = MIX` | 这个 module 同时包含 AIC 和 AIV 代码 |

PlanMemory 后面判断是否 overflow，就是拿这些容量和对齐约束去放置 local `memref.alloc`。

### 2. 两个 host helper，行 25-36

```text
@_attn_fwd_infer_workspace_shape_function
@_attn_fwd_infer_task_type_function
```

这两个函数带 `hacc.function_kind = HOST`，不是 device kernel 主体。local PlanMemory 的
`runOnOperation()` 会跳过 host function。

- `infer_workspace_shape_function` 返回 workspace 大小 `98336`。
- `infer_task_type_function` 返回 task type 常量 `32`。

读 before IR 时可以先略过这两个函数。

### 3. AIC 函数头，行 37-73

`@_attn_fwd_mix_aic` 是 CUBE/AIC 侧入口：

```text
hacc.entry
hacc.function_kind = DEVICE
hivm.func_core_type = AIC
hivm.part_of_mix
```

参数大致分成：

| 参数 | 含义 |
| - | - |
| `%arg0` | FFTS base address |
| `%arg1` | sync block lock GM buffer |
| `%arg2` | workspace GM buffer |
| `%arg3/%arg4/%arg5` | Q/K/V 等输入 GM tensor |
| `%arg7/%arg8` | 输出或中间输出 GM tensor |
| `%arg9` 之后的 `i32` | stride、shape、block/tile 参数 |

这一段只是定义接口和 core 类型，还没有 local alloc。

### 4. AIC 初始化和 block 索引，行 74-125

行 74-84 定义常量，例如 `0`、`32`、`128`、workspace offset `8224/24608/32800`。

行 85-86：

```text
hivm.hir.set_ffts_base_addr %arg0
hivm.hir.set_mask_norm
```

这是 AIC 运行时环境设置，不是内存分配。

行 87-124 根据 `block_idx` 计算当前 core 负责哪块 tile：

- `hivm.hir.get_block_idx` 取当前 block id。
- `divsi/remsi/muli/addi` 把一维 block id 拆成多维 tile 坐标。
- `affine.apply #map` 算 GM tensor 上的 offset。

行 125：

```text
%reinterpret_cast = memref.reinterpret_cast %arg3 ...
```

它把 GM 输入 `%arg3` 按当前 offset 看成 `128x128xf16` view，不新分配内存。

### 5. AIC 第一块 CBUF：加载 Q tile，行 126-144

行 126 第一次出现 PlanMemory 真正关心的 local alloc：

```text
%alloc = memref.alloc() : memref<8x8x16x16xf16, cbuf>
```

它表示在 CBUF/L1 里申请一块 `8*8*16*16*f16 = 32768 B` 的 buffer。

后面几行的含义：

- `memref.cast %alloc`：把静态 shape 转成动态 shape，方便 DMA op 接受。
- `memref.subview %reinterpret_cast`：从 GM 的 Q tensor view 里切出本 tile。
- `memref.subview %alloc`：从 CBUF buffer 里切出写入区域。
- `scf.if %32 { vbrc 0 }`：边界 tile 不满时，先把 CBUF buffer 填 0。
- `hivm.hir.nd2nz`：把 GM 里的 Q tile 搬到 CBUF，同时做 ND 到 NZ layout 转换。

直觉上就是：

```text
从 GM 读取当前 Q tile -> 放进 CBUF -> 给后续 cube matmul 使用
```

### 6. AIC 主循环：K/V tile、MMAD、写回 workspace，行 145-223

行 145-154 先处理 workspace 和同步：

- `memref_ext.alloc_workspace offset = 0`：从 GM workspace 取一小块 scalar 区域。
- `sync_block_wait/set`：协调 CUBE pipeline，不是 local alloc。

行 155 开始 `scf.for`，步长是 `32`，对应 attention 的 K/V 分块循环。

循环里有两组 cube 计算。

第一组，行 166-184：

| 行 | 含义 |
| - | - |
| 166 | `%alloc_3`：申请 CBUF，放 K/B tile |
| 168 | `nd2nz`：把 `128x32xf16` 搬到 CBUF |
| 170 | `%alloc_5`：申请 CC/L0C，放 matmul 结果 |
| 172-176 | `mmadL1`：用 CBUF 中的输入做矩阵乘加，输出到 CC |
| 180-183 | `fixpipe`：把 CC 结果搬回 GM workspace |

第二组，行 186-216：

| 行 | 含义 |
| - | - |
| 186 | `%alloc_9`：申请 CBUF，放另一侧输入 tile |
| 197 | `nd2nz`：把 GM tile 搬到 CBUF |
| 200 | `%alloc_14`：申请 CBUF，放另一个 `128x32xf16` tile |
| 204 | `%alloc_16`：申请 CC/L0C，放第二次 matmul 结果 |
| 206-211 | `mmadL1`：第二次矩阵乘加 |
| 212-214 | `fixpipe`：把结果写到 workspace |

从 PlanMemory 视角，这一整个 AIC 函数最重要的是：

```text
cbuf: %alloc, %alloc_3, %alloc_9, %alloc_14
cc  : %alloc_5, %alloc_16
```

这些就是后面 AIC 成功 rewrite 成 `hivm.hir.pointer_cast(offset)` 的对象。

### 7. AIV 函数头，行 225-261

`@_attn_fwd_mix_aiv` 是 VECTOR/AIV 侧入口：

```text
hacc.entry
hacc.function_kind = DEVICE
hivm.func_core_type = AIV
hivm.part_of_mix
```

它的参数结构和 AIC 类似，但执行的是向量侧 softmax、归一化、输出写回等逻辑。AIV 的 local memory
scope 主要是 UB，所以这里的 `memref.alloc` 都要由 local PlanMemory 在 UB 里安排 offset。

### 8. AIV 初始化和长生命周期 UB state，行 262-312

行 262-276 是常量：

- `1.44269502` 约等于 `log2(e)`。
- `0xFF800000` 是 `-inf`，用于 softmax 初始 max。
- `0.693147182` 是 `ln(2)`。
- `2.0` 用于后面 log/归一化。

行 277-294 和 AIC 类似，是运行时设置和 block id 拆解。

行 295-312 一开始就申请几块长生命周期 UB buffer：

| buffer | 类型 | 直觉用途 |
| - | - | - |
| `%alloc` | `128x128xf32, ub` | attention 累计矩阵/中间大状态 |
| `%alloc_6` | `128x32xf32, ub` | 每个 K/V block 的 score 或临时矩阵 |
| `%alloc_8` | `128xf32, ub` | 行级向量状态，初值为 `-inf` |
| `%alloc_9` | `128xf32, ub` | 行级归一化分母/累计量，初值为 `1` |

这些 buffer 生命周期很长，是 AIV UB overflow 的核心压力来源。

### 9. AIV 输出 view、workspace scalar 和主循环入口，行 313-366

行 313-335 继续根据 tile 坐标计算 GM 输出 `%arg8` 的 view：

```text
%reinterpret_cast = memref.reinterpret_cast %arg8 ...
```

这只是 GM view，不是 local alloc。

行 340-356 申请 `1xf32` UB buffer `%alloc_10`，把 scale 写到 workspace：

- `%alloc_10` 是 UB 小 scalar buffer。
- `memref_ext.alloc_workspace offset = 0` 取 GM workspace scalar 区域。
- `scf.if %36` 表示只有 sub-block 0 负责 store。
- `annotation.mark %33 {hivm.tcore_type = VECTOR}` 给 workspace value 标注 VECTOR 侧使用。

行 363 开始 AIV 主 `scf.for` 循环。它有 4 个 iter_args：

```text
%arg30 = %alloc_9   // 行级分母/累积量
%arg31 = %alloc     // 128x128 大累计矩阵
%arg32 = %alloc_8   // 行级 max/状态
%arg33 = %38        // GM offset
```

这说明 `%alloc/%alloc_8/%alloc_9` 跨循环活着，所以 lifetime 很长。

### 10. AIV 主循环前半：load K tile 和已有 AIC 结果，行 367-430

行 367-387：

- 从 GM `%arg4` 取当前 `128x32xf16` tile。
- `%alloc_26` 在 UB 中接收这个 tile。
- 边界不足时用 `vbrc 0` padding。
- `hivm.hir.load` 把 GM tile 搬到 UB。

行 388-414：

- 把 `%alloc_26` store 到 workspace。
- 从 workspace offset `8224` 取 AIC 侧写回的中间结果。
- `%alloc_30` 是 `19968xi8` 的 UB scratch，再通过 `memref.view` 看成 `128x?x1xf32`。
- `copy/load` 把已有 score/中间结果搬到 UB view。

行 415-430：

- `%alloc_33` 和 `%alloc_36` 都是 `128x32xf32` UB 临时矩阵。
- `vmul` 乘 scale。
- `vreduce <max>` 对每行做 max，得到 `%alloc_38 : 128x1xf32`。

这段是在为 softmax 做每行最大值准备。

### 11. AIV softmax 稳定性、mask 和 exp，行 431-503

行 431-447：

- `collapse_shape` 把 `128x1xf32` 看成 `128xf32`。
- `%alloc_40/%alloc_42` 是 `128xi1` 的比较结果。
- `vnot` 生成反向 mask。

行 448-463：

- `vmax` 计算新旧 max 的最大值。
- `vsel` 根据 mask 选择正确 max。
- `%alloc_46/%alloc_48` 是 `vsel` 的 temp buffer；后面会被 MemoryDisplay 标成 `is_tmpbuf`。

行 464-503：

- `expand_shape` 把行级 max 变成 `128x1`，方便 broadcast 到 `128x32`。
- `vsub/vmul/vexp` 计算 `exp(score - row_max)`。
- `vreduce <sum>` 得到每行 sum。
- `vsub/vmul/vexp/vmul/vadd` 更新跨 block 的归一化分母。

这段会创建很多 UB 临时 buffer，是 AIV local memory 压力最密集的区域之一。

### 12. AIV 更新累计输出和循环状态，行 504-545

行 504-509：

- 把行级缩放向量 expand 成 `128x1`。
- `%alloc_65 : 128x128xf32` 是新的大 UB buffer。
- `vmul ... broadcast = [1]` 用行级缩放去更新 `128x128` 累计矩阵。
- `%alloc_66` 是 `vmul` 的 temp buffer。

行 510-524：

- 把 softmax 权重从 f32 cast 到 f16。
- 写到 workspace offset `24608`。

行 525-539：

- 从 workspace offset `32800` load 一个 `128x128xf32` 到 `%alloc_69`。
- `%alloc_72` 把它和 `%alloc_65` 相加，得到新的累计矩阵。

行 540-544：

```text
scf.yield %alloc_63, %alloc_72, %alloc_47, %56
```

下一轮循环继续携带：

- 更新后的分母 `%alloc_63`
- 更新后的大矩阵 `%alloc_72`
- 更新后的行级 max `%alloc_47`
- 更新后的 GM offset `%56`

这些值跨循环活跃，会显著增加 UB lifetime。

### 13. AIV 循环后处理和最终写回，行 546-599

行 546-567：

- 等待前面 load/store pipeline。
- `vln/vdiv/vadd` 对分母和 max 做最终归一化。
- `%alloc_11` 到 `%alloc_15` 都是 `128xf32` UB 向量。

行 568-573：

- `%alloc_16 : 128x128xf32` 是最终归一化后的大矩阵。
- `%alloc_17 : 1024xf32` 是 `vdiv` 的 temp buffer。

行 574-585：

- 计算 logsumexp 或相关标量输出的 GM offset。
- 只有 sub-block 0 把 `%alloc_15` 的有效前 `%31` 个元素 store 到 GM `%arg7`。

行 586-598：

- `%alloc_20 : 128x128xf16` 是最终输出的 f16 UB buffer。
- `vcast` 把 `%alloc_16` 从 f32 转成 f16。
- `subview` 切出有效行数。
- 只有 sub-block 0 把结果 store 到 GM `%arg8`。

行 599 `return`，AIV 函数结束。

### 14. 从 PlanMemory 视角看 before IR

如果只从 local PlanMemory 的角度看，before IR 中最重要的信息是：

| 区域 | PlanMemory 关心什么 |
| - | - |
| AIC | `cbuf` 和 `cc` 的 `memref.alloc`，数量少，生命周期相对短 |
| AIV | 大量 `ub` 的 `memref.alloc`，多个 `128x128xf32` 和跨循环 iter_args 生命周期很长 |
| `memref.cast/subview/collapse_shape/view` | 大多只是 view，不新分配 storage |
| `hivm.hir.* outs(...)` | 说明哪个 buffer 被写，影响 liveness/gen/kill |
| `temp_buffer(...)` | 表示 op 需要额外 scratch buffer，会被 MemoryDisplay 标记 |
| `scf.for iter_args` | 表示某些 buffer 跨循环活跃，显著拉长 lifetime |

所以 before IR 本质上已经暴露了“有哪些本地 buffer、它们在哪个 scope、谁读写它们、生命周期可能有多长”。
local PlanMemory 后面要做的，就是把这些 local `memref.alloc` 具体排布到 UB/CBUF/CC 的 byte offset 上；
排得下就 rewrite 成 `pointer_cast`，排不下就报告 overflow。

## PlanMemory 在这个 before IR 上做什么

### 1. 相关 config 和 pipeline 位置

local PlanMemory 的 pipeline 位置在
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp:407-419`：

```text
createAllocExtraBufferPass
createInferHIVMMemScopePass
canonicalizationHIVMPipeline
createInlineLoadCopyPass
createMarkMultiBufferPass
createDumpIRBeforePlanMemoryPass
createPlanMemoryPass(planMemoryOption)
```

`before_local_plan_memory.mlir` 就是 `createDumpIRBeforePlanMemoryPass` 打出来的文件。
此时 `memref.alloc`、scope、temp buffer、multi-buffer 信息已经基本准备好。

这里分析的是第二次 PlanMemory，也就是 local PlanMemory。前面 pipeline 里还有一次
`GLOBAL_WORKSPACE_PLAN`，主要处理 `memref_ext.alloc_workspace` 这类 GM workspace；这里的
`LOCAL_MEM_PLAN` 处理的是 UB/CBUF/CC 里的 `memref.alloc`。

`Passes.td` 里 `hivm-plan-memory` 的相关 option：

| option | 本 case local PlanMemory 的值 | 作用 |
| - | - | - |
| `mem-plan-mode` | 默认 `local-mem-plan` | 规划 `memref.alloc` |
| `enable-memory-display` | dump 脚本开启 | 输出 `memory_info_aic.json` / `memory_info_aiv.json` |
| `enable-global-workspace-reuse` | local 这里不用 | 只影响 global workspace 规划 |
| `restrict-inplace-as-isa` | 默认 `false` | inplace 复用是否进一步受 ISA 约束 |

容量和对齐来自 `PlanMemory.cpp` 常量：

| scope | capacity | align |
| - | - | - |
| UB | 192 KiB | 32 B |
| CBUF/L1 | 512 KiB | 32 B |
| CC/L0C | 128 KiB | 512 B |

`memory_info*.json` 里 `extent` 是 bit，`offset` 是 byte。

### 2. `PlanMemoryPass::runOnOperation()` 的输入和输出

代码入口：

```cpp
void PlanMemoryPass::runOnOperation()
```

| 项 | 内容 |
| - | - |
| 输入 | 当前正在处理的 `func.func`，通过 `getOperation()` 得到 |
| 返回 | C++ 返回 `void` |
| 成功输出 | 修改当前函数 IR，把 local `memref.alloc` 改成 `hivm.hir.pointer_cast(offset)` |
| 失败输出 | `signalPassFailure()`，保留未 rewrite 的 IR，并在开启 memory display 时写 `memory_info*.json` |
| 主要中间结果 | `plannedBuffer2Offsets`，格式近似 `Value -> [byteOffset0, byteOffset1, ...]` |

对每个 `func.func` 的顺序是：

1. host helper 直接跳过。
2. 对 local mode 规范化 loop iter args。
3. 如果开启 `enable-memory-display`，先给 temp buffer 标记 `is_tmpbuf`。
4. 最多尝试 20 次：每次构建 `MemLivenessAnalysis`，收集 buffer size、scope、gen/kill、lifetime、inplace、multi-buffer。
5. 每次把 `build()` 的结果交给新的 `MemPlan`，调用 `MemPlan::plan()` 为每个 scope 规划 offset。
6. 某次成功时，保存 `plannedBuffer2Offsets`，跳出 retry loop。
7. 20 次都失败时，最后一次输出 `memory_info*.json` 和 overflow 诊断，不做最终 rewrite。
8. 如果成功，才把 `memref.alloc` rewrite 成 `hivm.hir.pointer_cast(offset)`。

这里要特别注意：`runOnOperation()` 对每个 device function 单独运行。所以 AIC 成功时，AIC 函数里
的 CBUF/CC alloc 会被 rewrite；AIV 失败时，AIV 函数里的 UB alloc 仍然保留。

### 3. `MemLivenessAnalysis::build()` 的输入、返回和实际输出

代码入口：

```cpp
void MemLivenessAnalysis::build()
```

它内部第一行是：

```cpp
Region &funcRegion = func_.getBody();
Liveness live(func_);
```

这里：

| 名字 | 格式 | 意思 |
| - | - | - |
| `func_` | `func::FuncOp` | 当前正在处理的函数，例如 `@_attn_fwd_mix_aiv` |
| `funcRegion` | `Region &` | 这个函数 `{ ... }` 里面的整块 IR |
| `live` | `Liveness` | MLIR 自带的活跃性分析对象，用来问“某个 value 在某个 op 后面还用不用” |

`build()` 自己返回 `void`。它真正的输出不是 return value，而是填充 `MemLivenessAnalysis` 对象内部的表：

| 输出表 | 格式可以怎么想 | 用途 |
| - | - | - |
| `linearOperation` | `[OpInfo(op, seqIndex), ...]` | 给 op 排时间顺序 |
| `bufferInfos` | `Value -> BufferInfo` | 每个 buffer 的 scope、大小、类型、定义 op |
| `genKillMap` | `OpInfo -> {gen: [Value], kill: [Value]}` | 哪个 op 让 buffer 开始/结束活跃 |
| `buffer2Life` | `Value -> BufferLife(allocTime, freeTime)` | 每个 buffer 的生命周期数字区间 |
| `buffer2AliasVec` | `Value -> [(aliasValue, hasCond), ...]` | 哪些 value 指向同一块或相关内存 |
| `inplacePairList` | `[(Value A, Value B), ...]` | 哪些 alloc 可以合并成同一组 storage |
| `buffer2MultiNum` | `Value -> N` | multi-buffer 需要展开几份 |

`build()` 的四步是：

```text
RecursionIR(&funcRegion, live)
UpdatePreloadBuffersGenKillMap()
GenerateBufferLife()
InitializeInplacePairList()
```

其中最核心的是 `RecursionIR()`，它负责读 IR 并填 `linearOperation/bufferInfos/genKillMap/buffer2AliasVec`。

### 4. `RecursionIR()` 的输入、返回和输出格式

代码入口：

```cpp
void MemLivenessAnalysis::RecursionIR(Region *region, Liveness live)
```

| 项 | 内容 |
| - | - |
| 输入 `region` | 要扫描的一块 IR，例如函数体、`scf.for` 的 loop body、`scf.if` 的 then/else body |
| 输入 `live` | MLIR 活跃性分析对象，用来判断某个 value 后面还会不会被使用 |
| C++ 返回 | `void` |
| 正常输出 | 通过副作用更新 `linearOperation/bufferInfos/genKillMap/buffer2AliasVec/buffer2MultiNum/preloadBuffers` |
| 异常输出 | 如果发现未知 op 读写 local buffer，会 `interrupt`，最后报 `PlanMemory Traverse IR Failed!` |

`region->walk<WalkOrder::PreOrder>` 的 callback 内部会返回三种控制信息：

| callback 返回 | 意思 |
| - | - |
| `WalkResult::advance()` | 当前 op 处理完，继续自动往下走 |
| `WalkResult::skip()` | 当前 op 的子 region 已经手动递归处理过了，默认 walk 不要重复进入 |
| `WalkResult::interrupt()` | 出错，中断扫描 |

#### 4.1 先处理“里面还有 IR 的 op”

这些 op 内部还有一块或多块代码区域，所以 `RecursionIR()` 会调用专门函数处理，然后 `skip`。

| 分支 | 调用函数 | 输入格式 | 返回/输出格式 | 能识别出什么 |
| - | - | - | - | - |
| `scf.if` | `RecursiveIfOp(scf::IfOp ifOp, Liveness live)` | 一个 if op 和 liveness | 返回 `void`；写 alias、kill 信息 | if 结果可能来自 then buffer，也可能来自 else buffer |
| `scf.for` | `RecursiveForOp(scf::ForOp forOp, Liveness live)` | 一个 for op 和 liveness | 返回 `void`；写 loop begin/end 两个时间点、alias、gen/kill | loop-carried buffer 跨循环活着 |
| `scf.while` | `RecursiveWhileOp(scf::WhileOp whileOp, Liveness live)` | 一个 while op 和 liveness | 返回 `void`；写 before/after region 的 alias、gen/kill | while 的 condition/yield 如何传 buffer |
| `scope::ScopeOp` | `RecursiveScopeOp(scope::ScopeOp scopeOp, Liveness live)` | 一个 scope op 和 liveness | 返回 `void`；写 scope result 和 return operand 的 alias | scope 内部值如何流到外面 |

`scf.if` 例子：

```mlir
%r = scf.if %cond -> memref<128xf32, ub> {
  scf.yield %a : memref<128xf32, ub>
} else {
  scf.yield %b : memref<128xf32, ub>
}
```

识别出的信息格式可以理解为：

```text
buffer2AliasVec[%r] += (%a, hasCond = true)
buffer2AliasVec[%r] += (%b, hasCond = true)
```

`hasCond = true` 表示“这是条件分支关系”。运行时只走一边，所以后面不会把 `%a/%b/%r` 当成普通
inplace 合并对。

`scf.for` 例子来自 AIV：

```mlir
%8:4 = scf.for ... iter_args(%arg30 = %alloc_9,
                             %arg31 = %alloc,
                             %arg32 = %alloc_8,
                             %arg33 = %38) -> ...
```

识别出的信息可以理解为：

```text
%arg30 alias %alloc_9
%arg31 alias %alloc
%arg32 alias %alloc_8
yield 的第 0/1/2 个 memref alias 对应 region iter arg
for result %8#0/%8#1/%8#2 alias 最后一轮 yield 出来的值
```

所以 PlanMemory 后面知道：`%alloc_9/%alloc/%alloc_8` 不是短临时 buffer，而是循环状态，会跨整个
主循环活着。

#### 4.1.1 `RecursiveIfOp()`：把 if 的 then/else 分支摊平成可规划的时间轴

代码入口：

```cpp
void MemLivenessAnalysis::RecursiveIfOp(scf::IfOp ifOp, Liveness live)
```

| 项 | 内容 |
| - | - |
| 输入 `ifOp` | 一个 `scf.if` op，里面可能有 then region 和 else region |
| 输入 `live` | 同一个函数级 `Liveness` 对象 |
| C++ 返回 | `void` |
| 主要输出 | 更新 `linearOperation`、分支内部 buffer 的 `gen/kill`、if result 和 yield operand 的 alias |

函数主体可以按这个顺序读：

```cpp
(void)UpdateLinearOperation(ifOp.getOperation());
RecursionIR(&ifOp.getThenRegion(), live);
auto curIfElse = UpdateLinearOperation(ifOp.getOperation());
UpdateIfOpBufferAlias(ifOp, ifOp.thenYield());

auto curIfEnd = curIfElse;
if (ifOp.elseBlock()) {
  RecursionIR(&ifOp.getElseRegion(), live);
  curIfEnd = UpdateLinearOperation(ifOp.getOperation());
  UpdateIfOpBufferAlias(ifOp, ifOp.elseYield());
}
OpKillHandle(curIfEnd, live, ifOp->getBlock());
```

第一句 `UpdateLinearOperation(ifOp)` 是记录一个“进入 if 的时间点”。注意这里不是把 if 当成普通
算子算一次，而是给后面的生命周期表一个边界：从这里开始，控制流进入分支。

然后它先递归扫描 then region：

```cpp
RecursionIR(&ifOp.getThenRegion(), live);
```

这会把 then 里面的 `alloc`、`copy`、`load/store`、`yield` 等 op 按普通规则收集起来。比如：

```mlir
%r = scf.if %cond -> memref<128xf32, ub> {
  %a = memref.alloc() : memref<128xf32, ub>
  hivm.hir.vadd ... outs(%a)
  scf.yield %a : memref<128xf32, ub>
} else {
  %b = memref.alloc() : memref<128xf32, ub>
  hivm.hir.vmul ... outs(%b)
  scf.yield %b : memref<128xf32, ub>
}
```

扫描 then 之后，`%a` 会被记进：

```text
bufferInfos[%a] = BufferInfo(...)
genKillMap[then_vadd].gen += %a
```

接着：

```cpp
auto curIfElse = UpdateLinearOperation(ifOp.getOperation());
UpdateIfOpBufferAlias(ifOp, ifOp.thenYield());
```

这里又把同一个 `ifOp` 放进 `linearOperation` 一次。它表示“then 分支结束，准备进入 else 分支或
离开 if 的时间点”。同一个 IR op 指针可以出现多次，因为 PlanMemory 不是只保存 IR 树结构，它还要构造
一条线性的时间轴。

`UpdateIfOpBufferAlias(ifOp, ifOp.thenYield())` 的输入/输出是：

| 函数 | 输入 | 输出 |
| - | - | - |
| `UpdateIfOpBufferAlias` | `ifOp` 和某个分支的 `scf.yield` | 把 `ifOp` 的 result 和该分支 yield 出来的 operand 记成 alias |

它内部逻辑是：

```cpp
for each yield operand i:
  UpdateBufferAlias(ifOp->getResult(i), yieldOp->getOperand(i),
                    hasCond = true)
```

以上面的例子为例，then 分支之后会得到：

```text
buffer2AliasVec[%r] += (%a, hasCond = true)
```

如果有 else 分支，它再递归扫描 else region：

```cpp
RecursionIR(&ifOp.getElseRegion(), live);
curIfEnd = UpdateLinearOperation(ifOp.getOperation());
UpdateIfOpBufferAlias(ifOp, ifOp.elseYield());
```

于是 else 分支会补上：

```text
bufferInfos[%b] = BufferInfo(...)
genKillMap[else_vmul].gen += %b
buffer2AliasVec[%r] += (%b, hasCond = true)
```

这里的 `hasCond = true` 很重要。它的意思不是“`%r` 同时等于 `%a` 和 `%b`”，而是“运行时根据条件，
`%r` 可能来自 `%a`，也可能来自 `%b`”。PlanMemory 后续看到带条件的 alias 时，会更保守，不把它当成
普通的一定同源关系去随便 inplace。

最后：

```cpp
OpKillHandle(curIfEnd, live, ifOp->getBlock());
```

这一句是在 if 整体结束的位置检查：if 的结果、if 外层 block 里的 buffer，哪些在 if 之后已经没有用了。
如果 `%r` 后面还会被用，它不能 kill；如果 then/else 内部某个临时 buffer 没有流出分支，后续就可以结束
生命周期。

所以 `RecursiveIfOp()` 的整体效果可以概括成：

```text
if begin marker
  扫 then 内部 op
if then-end marker
  记录 if result 可能来自 then yield
  扫 else 内部 op
if else-end marker
  记录 if result 可能来自 else yield
  在 if 整体结束处做 kill 判断
```

#### 4.1.2 `RecursiveForOp()`：处理 loop-carried buffer，让循环入口和出口都有生命周期边界

代码入口：

```cpp
void MemLivenessAnalysis::RecursiveForOp(scf::ForOp forOp, Liveness live)
```

| 项 | 内容 |
| - | - |
| 输入 `forOp` | 一个 `scf.for`，可能带 `iter_args` 和 result |
| 输入 `live` | 函数级 `Liveness` |
| C++ 返回 | `void` |
| 主要输出 | loop begin/end 两个时间点、循环外传入 buffer 的 gen、iter arg/yield/result alias、循环结束 kill |

函数主体是：

```cpp
auto forBeginSeq = UpdateLinearOperation(forOp.getOperation());
UpdateOpGenInfo(forBeginSeq, GetLiveBuffersInLoop(forOp, live));
UpdateForOpInitArgsAlias(forOp);
RecursionIR(&forOp.getRegion(), live);
UpdateForOpBufferAlias(forOp);
auto forEndSeq = UpdateLinearOperation(forOp.getOperation());
OpKillHandle(forEndSeq, live, forOp->getBlock());
```

第一步：

```cpp
auto forBeginSeq = UpdateLinearOperation(forOp.getOperation());
```

记录“进入 for 的时间点”。后面 `forEndSeq` 还会再记录同一个 `forOp`，表示“离开 for 的时间点”。
这两个时间点把整个循环包起来。

第二步：

```cpp
UpdateOpGenInfo(forBeginSeq, GetLiveBuffersInLoop(forOp, live));
```

这里有一个关键辅助函数：

```cpp
SmallVector<Value>
GetLiveBuffersInLoop(LoopLikeOpInterface loopOp, Liveness live)
```

它的输入/输出可以这样理解：

| 项 | 内容 |
| - | - |
| 输入 `loopOp` | 当前 for 或 while |
| 输入 `live` | 活跃性分析 |
| 返回 | `SmallVector<Value>`，也就是“循环开始前已经存在，并且在循环附近仍然活着的 buffer” |
| 副作用 | 无，只返回列表 |

它内部做的是：

```cpp
live.getLiveness(loopOp->getBlock())
currentlyLiveValuesOrdered(liveBlockInfo, loopOp.getOperation())
for each live value:
  aliasBuffers = GetAliasBuffers(value) + value itself
  if alias buffer exists in buffer2status:
    push into allocBeforeLoopBuffers
```

翻成白话就是：在循环开始这个位置，看看有哪些 value 已经是活的；再把它们的 alias 找出来；如果某个
alias 是 PlanMemory 已经认识的 buffer，就认为它需要被 loop begin 覆盖住。

例子：

```mlir
%buf = memref.alloc() : memref<128xf32, ub>
hivm.hir.copy ... outs(%buf)

scf.for %i = %c0 to %c10 step %c1 {
  hivm.hir.vadd ins(%buf) outs(...)
}
```

`%buf` 在循环前已经分配并写好了，循环体里还会读它。为了避免 PlanMemory 只把 `%buf` 的生命算到
循环前，`GetLiveBuffersInLoop()` 会把 `%buf` 找出来，然后：

```text
genKillMap[forBeginSeq].gen += %buf
```

这表示 `%buf` 的有效生命至少覆盖整个 loop begin 到 loop end。

第三步：

```cpp
UpdateForOpInitArgsAlias(forOp);
```

它处理的是 `iter_args` 的入口关系。例子：

```mlir
%out = scf.for %i = %c0 to %c10 step %c1
       iter_args(%arg = %init) -> memref<128xf32, ub> {
  ...
  scf.yield %next : memref<128xf32, ub>
}
```

在循环体里面，`%arg` 是 block argument；在第一轮开始时，它来自外面的 `%init`。所以这个函数记录：

```text
buffer2AliasVec[%arg] += (%init, hasCond = false)
```

它不是说文本上 `%arg` 和 `%init` 是同一个名字，而是告诉 PlanMemory：循环体里看到 `%arg`，要知道它
代表的是外面传进来的那条内存状态。

第四步：

```cpp
RecursionIR(&forOp.getRegion(), live);
```

递归扫描循环体内部。循环体里如果有：

```mlir
%tmp = memref.alloc() : memref<128xf32, ub>
hivm.hir.vadd ins(%arg) outs(%tmp)
scf.yield %tmp : memref<128xf32, ub>
```

那么递归扫描时会记录：

```text
bufferInfos[%tmp] = BufferInfo(...)
genKillMap[vadd].gen += %tmp
```

第五步：

```cpp
UpdateForOpBufferAlias(forOp);
```

它处理的是循环出口关系，分两层：

```cpp
for each region iter arg i:
  UpdateBufferAlias(forOp.getYieldedValues()[i], regionIterArgs[i])

for each for result i:
  UpdateBufferAlias(forOp->getResult(i), forOp.getYieldedValues()[i])
```

还是用上面的例子：

```mlir
%out = scf.for ... iter_args(%arg = %init) -> memref<128xf32, ub> {
  %tmp = memref.alloc() : memref<128xf32, ub>
  ...
  scf.yield %tmp : memref<128xf32, ub>
}
```

会得到：

```text
buffer2AliasVec[%tmp] += (%arg, hasCond = false)
buffer2AliasVec[%out] += (%tmp, hasCond = false)
```

这就是 loop-carried buffer 的完整链：

```text
%init -> %arg -> %tmp -> %out
```

这里容易误解：`yield %tmp` 不只是“返回 `%tmp`”。对 `scf.for iter_args` 来说，它表示“这一轮结束后，
下一轮的 `%arg` 要拿 `%tmp` 继续跑”。所以 PlanMemory 要把 `%tmp` 和 `%arg` 的关系记下来，否则会把
循环内新 alloc 的 `%tmp` 当成普通短临时 buffer。

最后：

```cpp
auto forEndSeq = UpdateLinearOperation(forOp.getOperation());
OpKillHandle(forEndSeq, live, forOp->getBlock());
```

这记录“离开 for 的时间点”，然后判断循环整体结束之后，哪些 buffer 后面不再用了。比如：

```mlir
%out = scf.for ... { ... }
hivm.hir.store %out, %gm
```

如果 `%out` 后面还有 `store`，for end 不能 kill `%out`。如果某个外部 buffer 只在循环体里最后一次
被读，for end 就可能成为它的 kill 点。

所以 `RecursiveForOp()` 的流程是：

```text
for begin marker
  把循环开始前已活跃、循环相关的 buffer 标成 gen
  建立 init arg -> region iter arg 的 alias
  扫循环体内部
  建立 yield value -> region iter arg 的 alias
  建立 for result -> yield value 的 alias
for end marker
  在循环整体结束处做 kill 判断
```

#### 4.1.3 `RecursiveWhileOp()`：同时处理 before region、condition 传参、after region 和下一轮 yield

代码入口：

```cpp
void MemLivenessAnalysis::RecursiveWhileOp(scf::WhileOp whileOp,
                                           Liveness live)
```

| 项 | 内容 |
| - | - |
| 输入 `whileOp` | 一个 `scf.while`，包含 before region 和 after region |
| 输入 `live` | 函数级 `Liveness` |
| C++ 返回 | `void` |
| 主要输出 | while begin/end 时间点、inits/condition/yield/result 之间的 alias、循环结束 kill |

`scf.while` 比 `scf.for` 绕一点，因为它有两块 region：

```mlir
%res = scf.while (%arg0 = %init) : (memref<128xf32, ub>) -> memref<128xf32, ub> {
  %cond = ...
  scf.condition(%cond) %arg0 : memref<128xf32, ub>
} do {
^bb0(%afterArg: memref<128xf32, ub>):
  %next = memref.alloc() : memref<128xf32, ub>
  hivm.hir.vadd ins(%afterArg) outs(%next)
  scf.yield %next : memref<128xf32, ub>
}
```

可以先按运行含义理解：

```text
%init 传给 before region 的 %arg0
before region 计算条件
scf.condition(%cond) %arg0 把 %arg0 传给 after region 的 %afterArg
after region 算出 %next
scf.yield %next 把 %next 传回下一轮 before region 的 %arg0
while 结束后产生 %res
```

函数主体是：

```cpp
auto whileBeginSeq = UpdateLinearOperation(whileOp.getOperation());
UpdateOpGenInfo(whileBeginSeq, GetLiveBuffersInLoop(whileOp, live));
UpdateWhileOpInitArgsAlias(whileOp);
RecursionIR(&whileOp.getBefore(), live);
RecursionIR(&whileOp.getAfter(), live);
UpdateWhileOpBufferAlias(whileOp);
auto whileEndSeq = UpdateLinearOperation(whileOp.getOperation());
OpKillHandle(whileEndSeq, live, whileOp->getBlock());
```

前两步和 for 类似：

```cpp
auto whileBeginSeq = UpdateLinearOperation(whileOp.getOperation());
UpdateOpGenInfo(whileBeginSeq, GetLiveBuffersInLoop(whileOp, live));
```

它们记录 while begin，并把“while 开始前已经活着、且和循环有关的 buffer”标成 gen，保证这些 buffer
的生命周期覆盖整个 while。

然后：

```cpp
UpdateWhileOpInitArgsAlias(whileOp);
```

它处理入口：

```cpp
for each (iterArg, init):
  UpdateBufferAlias(iterArg, init)
```

对应例子就是：

```text
buffer2AliasVec[%arg0] += (%init, hasCond = false)
```

接下来先扫 before region：

```cpp
RecursionIR(&whileOp.getBefore(), live);
```

before region 里最特殊的是：

```mlir
scf.condition(%cond) %arg0 : memref<128xf32, ub>
```

普通 `RecursionIR()` 遇到 `scf.condition` 时会调用：

```cpp
UpdateConditionOpBufferAlias(conditionOp)
```

这个函数的输入/输出是：

| 函数 | 输入 | 输出 |
| - | - | - |
| `UpdateConditionOpBufferAlias` | 一个 `scf.condition` | 把 while after region 的 block argument 和 condition operand 记成 alias |

也就是：

```text
buffer2AliasVec[%afterArg] += (%arg0, hasCond = false)
```

为什么要这样？因为 after region 里的 `%afterArg` 不是凭空来的，它就是 `scf.condition` 后面跟着传进去的
那个 `%arg0`。如果不记录这条关系，after region 里读 `%afterArg` 时，PlanMemory 就不知道它和外层传入的
`%init/%arg0` 是同一条循环状态链。

然后扫 after region：

```cpp
RecursionIR(&whileOp.getAfter(), live);
```

after region 里如果有：

```mlir
%next = memref.alloc() : memref<128xf32, ub>
hivm.hir.vadd ins(%afterArg) outs(%next)
scf.yield %next : memref<128xf32, ub>
```

会记录：

```text
bufferInfos[%next] = BufferInfo(...)
genKillMap[vadd].gen += %next
```

after region 扫完后：

```cpp
UpdateWhileOpBufferAlias(whileOp);
```

它处理两类出口关系：

```cpp
for each (yieldVal, iterArg):
  UpdateBufferAlias(yieldVal, iterArg)

for each while result i:
  UpdateBufferAlias(whileOp->getResult(i), whileOp.getAfterArguments()[i])
```

对应例子就是：

```text
buffer2AliasVec[%next] += (%arg0, hasCond = false)
buffer2AliasVec[%res]  += (%afterArg, hasCond = false)
```

再加上前面 condition 建出的：

```text
%afterArg alias %arg0
```

整个 while 的 buffer 传递链就变成：

```text
%init -> %arg0 -> %afterArg -> %next -> 下一轮 %arg0
                    |
                    +-> %res
```

这里的 `%res alias %afterArg` 看起来有点奇怪，因为 `%res` 是 while 结束后的结果，而 `%afterArg` 是 after
region 的参数。代码是通过 `condition operand -> after argument -> while result` 这条链，把“condition
传出来的值最终也可能成为 while 结果”表达出来。

最后：

```cpp
auto whileEndSeq = UpdateLinearOperation(whileOp.getOperation());
OpKillHandle(whileEndSeq, live, whileOp->getBlock());
```

这和 for 一样，记录 while end，并在 while 整体结束处判断外层还有没有继续使用这些 buffer。

所以 `RecursiveWhileOp()` 的流程是：

```text
while begin marker
  把 while 开始前已活跃、while 相关的 buffer 标成 gen
  建立 init -> before iter arg 的 alias
  扫 before region
    在 scf.condition 处建立 condition operand -> after arg 的 alias
  扫 after region
  建立 yield value -> before iter arg 的 alias
  建立 while result -> after arg 的 alias
while end marker
  在 while 整体结束处做 kill 判断
```

#### 4.2 普通 op：先入时间轴，再按类型更新表

如果不是 `if/for/while/scope`，先执行：

```cpp
auto curOpInfo = UpdateLinearOperation(op);
```

这个函数的输入/输出是：

| 函数 | 输入 | 返回 | 副作用输出 |
| - | - | - | - |
| `UpdateLinearOperation(Operation *op)` | 一个 op 指针 | `OpInfo *`，指向刚创建的 op 记录 | `linearOperation.push_back(OpInfo(op, seqIndex++))` |

`OpInfo` 可以理解成：

```text
OpInfo {
  operation = 当前 op
  seqIndex  = PlanMemory 给它排的序号
}
```

然后进入普通 op 分支：

| 普通 op 分支 | 调用函数 | 输入格式 | 返回/输出格式 | 能识别出什么 |
| - | - | - | - | - |
| alias op，例如 `cast/subview/collapse_shape/view` | `getOperationAliasInfo(op)` + `UpdateBufferAlias(a, b)` | 当前 op | `getOperationAliasInfo` 返回 alias 对；`UpdateBufferAlias` 写 `buffer2AliasVec` | 新 value 只是旧 buffer 的另一种看法 |
| local `memref.alloc` | `CheckLocalBufferAllocOp(op)` + `UpdateOpBufferInfo(op, op->getResults())` | alloc op 的 results | 检查返回 `LogicalResult`；成功后写 `bufferInfos` 和 `buffer2status` | 发现一块 UB/CBUF/CC local buffer |
| global `memref_ext.alloc_workspace` | `UpdateOpBufferInfo(op, op->getResults())` | workspace alloc result | 写 `bufferInfos` | 发现 GM workspace buffer |
| `memref.load` | `OpKillHandle(curOpInfo, live, op->getBlock())` | 当前 opInfo、liveness、所在 block | 返回 `void`；可能写 `genKillMap[opInfo].kill` | load 后某些 buffer 可能最后一次使用完 |
| `memref.store` | `UpdateStoreOpInfo(curOpInfo, storeOp.getMemRef(), live)` | 当前 opInfo、被 store 的 memref、liveness | 返回 `void`；可能写 gen 和 kill | store 目标可能从这里开始承载有效数据 |
| `DestinationStyleOpInterface` | `UpdateOpGenInfo()`、`UpdateOpTempGenInfo()`、`OpKillHandle()` | 有 `outs(...)` / `temp_buffer(...)` 的 op | 返回 `void`；写 gen、temp gen、kill | 识别主要输出 buffer 和临时工作 buffer |
| `arith.select` | `UpdateBufferAlias(result, trueVal/falseVal, hasCond=true, isIgnoreInplace=true)` | select result 和两边输入 | 返回 `void`；写带条件 alias | select 结果来自二选一，不能普通 inplace |
| `annotation.mark` | `ProcessMarkOp(markOp, curOpInfo, live)` | mark op | 返回 `void`；写 multi-buffer/preload 信息，可能写 kill | 读取 `MultiBuffer`、`PreloadLocalBuffer` 这类标记 |
| `scf.condition` | `UpdateConditionOpBufferAlias(conditionOp)` | while condition op | 返回 `void`；写 alias | while before region 传到 after region 的值 |
| `cf.cond_branch` / `cf.branch` | `UpdateBranchOpAlias(destBlock, destOperands)` | 目标 block 和传入参数 | 返回 `void`；写 alias | branch operand 和目标 block argument 的关系 |
| `hivm::DebugOp` | `OpKillHandle()` | 当前 debug op | 返回 `void`；可能写 kill | debug op 可能是某些 value 的最后使用点 |
| 未识别 op | `CheckIfUnknownOpTouchBuffer(op)` | 当前 op | 返回 `LogicalResult`；失败则中断 | 如果未知 op 读写 local buffer，直接报错防止漏算 |

几个具体例子：

```mlir
%alloc = memref.alloc() : memref<8x8x16x16xf16, #hivm.address_space<cbuf>>
```

这会进入 local `memref.alloc` 分支。输出信息可以理解为：

```text
bufferInfos[%alloc] = {
  operation   = 这条 memref.alloc
  bufferScope = cbuf
  bufferType  = f16
  constBits   = 8*8*16*16*16 = 262144
}
buffer2status[%alloc] = DEFINED
```

注意：`DEFINED` 只是“看见这块 buffer 了”，还不是 lifetime 的开始。

```mlir
%subview_0 = memref.subview %alloc[0, 0, 0, 0] ...
```

这会进入 alias 分支，输出类似：

```text
buffer2AliasVec[%subview_0] += (%alloc, hasCond = false)
buffer2AliasVec[%alloc]     += (%subview_0, hasCond = false)
```

意思是 `%subview_0` 不新分配内存，它只是 `%alloc` 的切片。

```mlir
hivm.hir.nd2nz
  ins(%reinterpret_cast : memref<...gm>)
  outs(%subview_0 : memref<...cbuf>)
```

这会进入 `DestinationStyleOpInterface` 分支。`outs(%subview_0)` 会作为输出 buffer 传给
`UpdateOpGenInfo()`。因为 `%subview_0 alias %alloc`，最后效果可以理解为：

```text
genKillMap[curOpInfo].gen += %alloc
buffer2status[%alloc] = GENED
```

这就是 `%alloc` lifetime 的开始位置。

再看 AIV 临时 buffer：

```mlir
%alloc_48 = memref.alloc() : memref<8xf32, #hivm.address_space<ub>>
hivm.hir.vsel ... outs(%alloc_47) temp_buffer(%alloc_48)
```

`vsel` 的输出格式可以理解为：

```text
gen += %alloc_47       // 主要输出
gen += %alloc_48       // 临时工作 buffer
bufferInfos[%alloc_48].ignoreInplace = true
```

`ignoreInplace = true` 的意思是：这个临时工作 buffer 不参与普通 inplace 合并，避免把算子内部临时空间
和长期状态 buffer 随便合在一起。

`OpKillHandle()` 的输出格式：

```text
如果某个 buffer 以及它的所有 alias 在当前 op 后都不再使用：
  genKillMap[curOpInfo].kill += buffer
  buffer2status[buffer] = KILLED
```

它必须把 alias 一起看。比如 `%alloc/%cast/%subview_0` 都指向同一块 storage，只有这些名字后面都不用了，
`%alloc` 才能 kill。

### 5. `UpdatePreloadBuffersGenKillMap()` 的输入和输出

代码入口：

```cpp
void MemLivenessAnalysis::UpdatePreloadBuffersGenKillMap()
```

| 项 | 内容 |
| - | - |
| 输入 | 无显式参数；读取前面 `RecursionIR()` 收集到的 `linearOperation` 和 `preloadBuffers` |
| 返回 | `void` |
| 输出 | 修改 `genKillMap`，把 preload buffer 的 gen/kill 调整到父 loop 的开始/结束 |

它处理的是带特殊标记的 preload local buffer。可以理解成：有些 buffer 虽然定义在内部 scope 里，
但实际使用范围应该提升到外层 loop 来看。本例主 overflow 不是靠这一步触发，但它说明 `build()` 会根据
标记修正 lifetime，不只是机械看 `alloc` 所在行。

### 6. `GenerateBufferLife()` 的输入和输出

代码入口：

```cpp
void MemLivenessAnalysis::GenerateBufferLife()
```

| 项 | 内容 |
| - | - |
| 输入 | 无显式参数；读取 `linearOperation` 和 `genKillMap` |
| 返回 | `void` |
| 输出 | 填 `buffer2Life`，格式是 `Value -> BufferLife{allocTime, freeTime}` |

算法很直接：按 `linearOperation` 顺序从前到后数 `scopeTime`。

```text
遇到 gen(buffer)  -> buffer.allocTime = 当前 scopeTime
遇到 kill(buffer) -> buffer.freeTime  = 当前 scopeTime
每处理一个 op    -> scopeTime + 1
```

因此 `memory_info_aiv.json` 里：

```text
%alloc : life_time_in_ir [29, 217]
%alloc_65 : life_time_in_ir [173, 196]
```

可以读成：`%alloc` 从第 29 个时间点活到第 217 个时间点；`%alloc_65` 只在第 173 到 196
这个更短的区间活着。这里的数字不是 MLIR 文件行号。

### 7. `InitializeInplacePairList()` 的输入和输出

代码入口：

```cpp
void MemLivenessAnalysis::InitializeInplacePairList()
```

| 项 | 内容 |
| - | - |
| 输入 | 无显式参数；读取 `bufferInfos` 和 `buffer2AliasVec` |
| 返回 | `void` |
| 输出 | 填 `inplacePairList`，格式是 `[(bufferA, bufferB), ...]` |

它会遍历 alias 关系。如果两个 alias value 都能追到实际 alloc，而且这个 alias 不是条件分支造成的
不确定关系，就把它们加入 `inplacePairList`。

直觉上：

```text
如果 A 和 B 是同一个迭代状态的前后两代，
并且不是 if/select 这种二选一关系，
PlanMemory 后面可以考虑让它们共用一块 storage。
```

后面的 `MemPlan::MergeInplaceSE()` 会读取 `inplacePairList`，把可复用的 StorageEntry 合并。

`memory_info*.json` 里 `extent` 是 bit，`offset` 是 byte。

### 8. `build()` 之后：20 次 `MemPlan` 重新规划

`MemLivenessAnalysis::build()` 结束后，PlanMemory 还没有真的给 buffer 安排地址。`build()` 只是准备了：

```text
linearOperation
bufferInfos
buffer2Life
genKillMap
buffer2MultiNum
inplacePairList
```

真正安排 offset 的代码在 `PlanMemoryPass::runOnOperation()` 后半段：

```cpp
constexpr int kPlanRetryCount = 20;
DenseMap<Value, SmallVector<uint64_t>> plannedBuffer2Offsets;

for (int attempt = 0; attempt < kPlanRetryCount; ++attempt) {
  MemLivenessAnalysis memLiveness(funcOp, this->memMode,
                                  /*randomSeed=*/attempt);
  memLiveness.build();

  MemPlan memPlan(this->memMode, this->enableGlobalReuse,
                  this->enableMemoryDisplay, this->restrictInplaceAsISA);
  memPlan.func_ = funcOp;
  memPlan.SetLinearOperation(memLiveness.linearOperation);
  memPlan.SetBufferInfos(memLiveness.bufferInfos);
  memPlan.SetBuffer2Life(memLiveness.buffer2Life);
  memPlan.SetGenKillMap(memLiveness.genKillMap);
  memPlan.SetBuffer2MultiNum(memLiveness.buffer2MultiNum);
  memPlan.SetInplacePairList(memLiveness.inplacePairList);

  const bool isLastAttempt = attempt == kPlanRetryCount - 1;
  if (succeeded(memPlan.plan(/*emitErrors=*/isLastAttempt))) {
    plannedBuffer2Offsets = memPlan.GetBuffer2Offsets();
    ...
    break;
  }

  if (isLastAttempt) {
    ...
    return signalPassFailure();
  }
}
```

这段的输入/输出格式可以这样看：

| 项 | 内容 |
| - | - |
| 输入 | 当前 `funcOp` 和 pass option：`memMode/enableGlobalReuse/enableMemoryDisplay/restrictInplaceAsISA` |
| 每次 attempt 的输入 | `randomSeed = attempt` |
| 每次 `build()` 输出 | `memLiveness` 内部那几张表 |
| 每次 `MemPlan::plan()` 输出 | 成功时生成 `buffer2Offsets`；失败时记录失败 scope 的申请量 |
| retry loop 成功输出 | `plannedBuffer2Offsets = Value -> [byteOffset0, byteOffset1, ...]` |
| 20 次都失败 | 最后一次 `emitErrors=true`，输出 overflow 诊断并 `signalPassFailure()` |

这里有一个容易忽略的细节：20 次不是直接拿同一份 `build()` 结果反复排地址，而是每次重新创建：

```cpp
MemLivenessAnalysis memLiveness(..., randomSeed = attempt)
```

原因是当前算法对一些候选 buffer 的遍历顺序敏感。`randomSeed` 会进入：

```cpp
std::shuffle(rangeClone.begin(), rangeClone.end(), randomGenerator);
```

它影响两类地方：

```text
GetLiveBuffersInLoop()：for/while 入口处哪些 live buffer 先被处理
OpKillHandle()：当前 op 附近哪些 live value 先尝试 kill
```

所以不同 attempt 可能得到略有差异的 `genKillMap/inplacePairList/storage entry 顺序`，进而让后面的地址规划遇到不同的候选顺序。它不是运行时随机，而是：

```text
attempt 0 用 seed 0
attempt 1 用 seed 1
...
attempt 19 用 seed 19
```

因此编译结果仍然是可复现的。

#### 8.1 每次 `MemPlan` 拿到 `build()` 的哪些结果

`MemPlan` 本身不重新读 IR 来推导生命周期。它直接吃 `MemLivenessAnalysis` 已经收集好的表：

```cpp
memPlan.SetLinearOperation(memLiveness.linearOperation);
memPlan.SetBufferInfos(memLiveness.bufferInfos);
memPlan.SetBuffer2Life(memLiveness.buffer2Life);
memPlan.SetGenKillMap(memLiveness.genKillMap);
memPlan.SetBuffer2MultiNum(memLiveness.buffer2MultiNum);
memPlan.SetInplacePairList(memLiveness.inplacePairList);
```

可以理解为：

| `MemLivenessAnalysis` 表 | `MemPlan` 用它做什么 |
| - | - |
| `linearOperation/genKillMap` | 找哪些 buffer 真正参与规划，也保留 debug 时间轴 |
| `bufferInfos` | 知道每个 buffer 的大小、scope、类型、定义 op |
| `buffer2Life` | 判断两个 buffer 生命周期是否重叠 |
| `buffer2MultiNum` | multi-buffer 时把一份 storage 展开成多份 |
| `inplacePairList` | 把可 inplace 的两个 buffer 合并成同一个 storage entry |

#### 8.2 `MemPlan::plan()` 内部主流程

`MemPlan::plan()` 的入口是：

```cpp
LogicalResult MemPlan::plan(bool emitErrors)
```

主流程很短：

```cpp
GenerateStorageEntry();
PlanStatus as = planMode == LOCAL_MEM_PLAN
                    ? PlanLocalMemAddress()
                    : PlanWorkSpaceMemAddress();
if (as == PLAN_FAILED) {
  if (emitErrors) EmitPlanMemoryFailureInfo();
  if (enableMemoryDisplay) UpdateBuffer2Offsets();
  return failure();
}
UpdateBuffer2Offsets();
return success();
```

分开看：

1. `GenerateStorageEntry()`

   它遍历 `linearOperation` 和 `genKillMap`，只看 `gen` 出来的 buffer。对每个真实 buffer 建一个 `StorageEntry`：

   ```text
   StorageEntry {
     bufInfo        -> bufferInfos 里的大小/scope/type
     bufferLifeVec  -> 这个 buffer 的 [allocTime, freeTime]
     inplaceBuffers -> 当前 entry 包含哪些 Value
     multiBufferNum -> 默认 1；如果 buffer2MultiNum 里有记录，就改成对应份数
   }
   ```

   也就是说，`StorageEntry` 是后面真正排地址的单位。一个 entry 最开始通常对应一个 alloc，后面可能因为 inplace 或 multi-buffer 被合并/复制。

2. `PlanLocalMemAddress()`

   local PlanMemory 走这条路径。它内部顺序是：

   ```cpp
   MergeInplaceSE();
   dmaFirstPipelineOpt.build(func_);
   ExpandMultiBufferStorageEntry();
   MergeSameScopeSE();
   return PlanMemAddressOfWholeLocalBuffer();
   ```

   含义是：

   ```text
   MergeInplaceSE()
     先根据 inplacePairList，把能共用 storage 的 entry 合在一起。

   dmaFirstPipelineOpt.build(func_)
     构建 DMA/流水相关的限制信息，后面避免某些复用导致 pipeline stall。

   ExpandMultiBufferStorageEntry()
     如果某个 entry 有 multiBufferNum = N，就额外复制 N-1 个 entry。
     比如 multi_buffer=4，最终这个 buffer 需要 4 份地址。

   MergeSameScopeSE()
     按 address space 分组，比如 UB 一组、CBUF 一组、CC 一组。
     同一个 scope 共享同一个容量上限。

   PlanMemAddressOfWholeLocalBuffer()
     对每个 scope 真正安排 offset。
   ```

3. `PlanWorkSpaceMemAddress()`

   workspace/global mode 类似，但分组方式不是 UB/CBUF/CC，而是 workspace arg：

   ```text
   MergeInplaceSE()
   ExpandMultiBufferStorageEntry()
   MergeSameWorkSpaceArgSE()
   PlanMemOffsetOfWholeWorkSpace()
   ```

   本报告重点是第二次 local PlanMemory，所以主要看 local 路径。

#### 8.3 local 地址规划怎么成功或失败

`PlanMemAddressOfWholeLocalBuffer()` 会对每个 memory scope 单独规划：

```text
UB    容量 192 KiB，对齐 32 B
CBUF  容量 512 KiB，对齐 32 B
CC    容量 128 KiB，对齐 512 B
```

它对每个 scope 做：

```cpp
auto bufferSpaceInfo = GetBufferSpaceInfo(scope);
align = bufferSpaceInfo.first;
maxBits = bufferSpaceInfo.second;
```

然后先看“不复用也能不能放下”：

```cpp
if (IsEnoughForBuffersNoReuse(rootStorageEntry, maxBits, align)) {
  PlanBuffersWithoutReuse(rootStorageEntry, align);
  continue;
}
```

这就是 AIC 的 CBUF/CC 成功路径很简单的原因：总量小于容量，不需要复杂复用，直接顺序排 offset 就够。

如果不复用放不下，就进入更复杂的复用规划：

```text
GetReorderRootStorageEntry()
MultiSpecPlan()
失败时 ApplyFailStrategy()
```

这里 `MultiSpecPlan()` 会尝试把当前 `StorageEntry` 放进已有 memory outline 的空洞里。它判断能不能放时，会看：

```text
大小是否满足
生命周期是否冲突
spec level 是否允许这种复用
pipeline/ISA 限制是否允许
```

如果当前策略放不下，就进入：

```cpp
ApplyFailStrategy(statusWrapper, maxBits)
```

失败策略大概是：

```text
先 rollback 一些已经安排的 entry
如果 specLevel 还能降，就降一级继续尝试
如果还没启用 splitOutline，就启用后从头重排
如果这些都不行，PLAN_FAILED
```

这里的 `specLevel` 可以粗略理解成“复用保守程度”。源码里定义了 0 到 3：

```text
SPEC_LEVEL_0：生命周期不冲突就可复用
SPEC_LEVEL_1：生命周期错开 1 个时间点也更保守
SPEC_LEVEL_2：考虑同 loop 中 vector/dma pipeline 冲突
SPEC_LEVEL_3：更保守地考虑 pipeline 冲突
```

local path 默认从较保守的高 level 开始：

```cpp
si.specLevel = si.maxLevel; // maxLevel 默认 SPEC_LEVEL_3
```

失败时逐步放宽/回滚。如果最后还是无法在 `maxBits` 容量内放下，就记录：

```text
failApplyBufferInfo[scope] = requiredBits
```

最后一次 attempt 才会调用：

```cpp
EmitPlanMemoryFailureInfo()
```

输出类似：

```text
ub overflow, requires 1716224 bits while 1572864 bits available
```

#### 8.4 成功后如何改 IR

只要某次 attempt 成功：

```cpp
plannedBuffer2Offsets = memPlan.GetBuffer2Offsets();
break;
```

retry loop 结束后，才执行 rewrite：

```cpp
RewritePatternSet patterns(&getContext());
populateBufferAddressToAllocOp(patterns, plannedBuffer2Offsets);
applyPatternsGreedily(funcOp, std::move(patterns));
```

这一步才会把：

```mlir
%buf = memref.alloc() : memref<..., ub>
```

替换成：

```mlir
%buf = hivm.hir.pointer_cast(%offset)
  : memref<..., ub>
```

如果某个 buffer 是 multi-buffer，`buffer2Offsets[%buf]` 里会有多个 offset：

```text
%buf -> [offset0, offset1, offset2, offset3]
```

rewrite 后还会调用：

```cpp
fixMultibufferEnabledPointerCastOps(funcOp)
```

它会检查/修正 multi-buffer 场景下的 `pointer_cast` 地址列表，让相同 multi-buffer 关系的 pointer_cast 形状保持一致。

#### 8.5 对 attn_fwd 这个例子的含义

对 AIC 函数，某次 attempt 成功后：

```text
CBUF/CC buffer -> 得到 offset -> memref.alloc 被 pointer_cast 替换
```

所以 after IR 里能看到 CBUF/CC 的 `pointer_cast(%c0_i64)`、`pointer_cast(%c32768_i64)` 等。

对 AIV 函数，20 次都没找到满足 UB 容量的规划：

```text
required = 1716224 bits
available = 1572864 bits
```

所以最后一次 attempt 输出 overflow；因为 `memPlan.plan()` 最终失败，后面的 rewrite 阶段不会执行。于是 AIV after-failed IR 仍然保留 UB `memref.alloc`，只多了一些 debug 标记，例如 `is_tmpbuf = true`。

## AIC：成功路径

AIC 的 local buffers 在 before IR 中有 4 个 `cbuf` alloc 和 2 个 `cc` alloc。

### AIC CBUF

| before alloc | before line | after pointer_cast | offset(B) | extent(B) |
| - | - | - | - | - |
| `%alloc : memref<8x8x16x16xf16, cbuf>` | 126 | `%28 = pointer_cast(%c0_i64)` | 0 | 32768 |
| `%alloc_3 : memref<2x8x16x16xf16, cbuf>` | 166 | `%49 = pointer_cast(%c32768_i64)` | 32768 | 8192 |
| `%alloc_9 : memref<8x2x16x16xf16, cbuf>` | 186 | `%53 = pointer_cast(%c40960_i64)` | 40960 | 8192 |
| `%alloc_14 : memref<2x8x16x16xf16, cbuf>` | 200 | `%55 = pointer_cast(%c49152_i64)` | 49152 | 8192 |

CBUF 总量：

```text
32768 + 8192 + 8192 + 8192 = 57344 B
```

远小于 512 KiB，所以直接顺序分配。

### AIC CC

| before alloc | before line | after pointer_cast | offset(B) | extent(B) |
| - | - | - | - | - |
| `%alloc_5 : memref<2x8x16x16xf32, cc>` | 170 | `%50 = pointer_cast(%c0_i64)` | 0 | 16384 |
| `%alloc_16 : memref<8x8x16x16xf32, cc>` | 204 | `%56 = pointer_cast(%c16384_i64)` | 16384 | 65536 |

CC 总量：

```text
16384 + 65536 = 81920 B
```

小于 128 KiB，所以也顺序分配。

### AIC before/after 怎么对照

before:

```mlir
%alloc = memref.alloc() : memref<8x8x16x16xf16, #hivm.address_space<cbuf>>
%cast = memref.cast %alloc : ...
%subview_0 = memref.subview %alloc[0, 0, 0, 0] ...
```

after:

```mlir
%28 = hivm.hir.pointer_cast(%c0_i64)
  : memref<8x8x16x16xf16, #hivm.address_space<cbuf>>
%cast = memref.cast %28 : ...
%subview_0 = memref.subview %28[0, 0, 0, 0] ...
```

意思是：计算逻辑没变，只是 storage 来源从“抽象 alloc”变成了“CBUF offset 0”。

## AIV：UB overflow 失败路径

AIV 的 local buffers 全在 UB，数量多、生命周期长。`memory_info_aiv.json` 记录：

```text
ub overflow, requires 1716224 bits while 1572864 bits available
```

换成 byte：

```text
required = 214528 B
capacity = 196608 B
delta    = 17920 B
```

几个关键压力来源：

| buffer | shape | life | offset(B) | extent(B) | 说明 |
| - | - | - | - | - | - |
| `%alloc` | `128x128xf32` | 29-217 | 148992 | 65536 | loop 外长生命周期主累计 buffer |
| `%alloc_72` | `128x128xf32` | 29-217 | 148992 | 65536 | 与 `%alloc` 同 offset，来自 inplace/alias 合并 |
| `%alloc_6` | `128x32xf32` | 32-201 | 65536 | 16384 | 长生命周期临时矩阵 |
| `%alloc_8` | `128xf32` | 34-213 | 81920 | 512 | 长生命周期向量 |
| `%alloc_9/%alloc_63` | `128xf32` | 36-217 | 82944 | 512 | 行级累计状态 |
| `%alloc_65` | `128x128xf32` | 173-196 | 83456 | 65536 | loop 内大 buffer |
| `%alloc_20/%alloc_16` | `128x128xf16/f32` | 217-237 | 512 | 65536 each | 尾部输出转换阶段大 buffer |

AIV after-failed IR 仍然保留：

```mlir
%alloc = memref.alloc() {alignment = 64 : i64}
  : memref<128x128xf32, #hivm.address_space<ub>>
```

这是预期行为。因为 `memPlan.plan()` 失败后不会执行 `MemrefAllocaOpToPointerCastOpPattern`。

after-failed IR 中有些 alloc 会多出：

```mlir
{is_tmpbuf = true}
```

这是 `markTempBufForMemoryDisplay()` 在规划前做的 debug 标记，表示这个 alloc 是某个 op 的
`temp_buffer(...)` scratch，不是最终 lowering 语义。

## 从 before 到 after 的整体变化

1. PlanMemory 不改变数学计算，只改变 local buffer 的 storage 来源。
2. AIC 成功：
   - `cbuf/cc` 的 `memref.alloc` 被替换成 `hivm.hir.pointer_cast(byte_offset)`。
   - 后续 `cast/subview/collapse_shape/hivm.hir.*` 只是把源从 alloc 换成 pointer_cast。
3. AIV 失败：
   - UB 最低峰值仍超过容量。
   - rewrite 阶段没有执行。
   - after-failed dump 保留 `memref.alloc`，同时输出 `memory_info_aiv.json` 和 overflow 诊断。
4. GM workspace：
   - `memref_ext.alloc_workspace` 已经带 offset。
   - local PlanMemory 不负责把它 rewrite 成 pointer_cast。

一句话总结：

```text
before IR 说明“有哪些本地 buffer 需要被安排”；
local PlanMemory 计算这些 buffer 在 UB/CBUF/CC 里的 byte offset；
AIC 放得下，所以 alloc -> pointer_cast；
AIV 放不下，所以保留 alloc 并报告 UB overflow。
```
