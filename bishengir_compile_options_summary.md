# bishengir-compile 编译选项总览

本文总结当前仓库中 `bishengir-compile` 注册的编译选项、取值范围、默认值和主要作用。

主要依据：

- `bishengir/include/bishengir/Tools/bishengir-compile/Options.td`
- `bishengir/lib/Tools/bishengir-compile/BiShengIRCompileConfig.cpp`
- `bishengir/lib/Tools/bishengir-compile/PassPipeline.cpp`
- `bishengir/lib/Tools/bishengir-compile/BiShengIRCompileMain.cpp`
- `bishengir/lib/Dialect/HFusion/Pipelines/HFusionPipelines.cpp`
- `bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`

## 1. 基本用法

```bash
build/bin/bishengir-compile input.mlir \
  -o output.o \
  --target=Ascend910_9382 \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true
```

Triton / `.ttadapter` 路径常见用法：

```bash
build/bin/bishengir-compile data/attn_fwd.ttadapter \
  -o output.o \
  --target=Ascend910_9382 \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true
```

PlanMemory 调试常见用法：

```bash
BISHENGIR_DUMP_BEFORE_PLAN_MEMORY=before_plan_memory.mlir \
build/bin/bishengir-compile data/attn_fwd.ttadapter \
  -o output.o \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  --enable-memory-display=true \
  --mlir-disable-threading \
  --mlir-print-ir-after-all \
  --mlir-print-ir-module-scope \
  --mlir-print-ir-tree-dir=ir_tree
```

## 2. 取值规则

### 2.1 布尔选项

类型为 `bool` 的选项取值为：

```text
true / false
```

一般可以写成：

```bash
--enable-hfusion-compile=true
--enable-hfusion-compile=false
```

LLVM 命令行通常也支持把布尔 flag 单独写出表示 `true`：

```bash
--enable-hfusion-compile
```

为了脚本可读性和避免歧义，建议统一写 `=true` 或 `=false`。

### 2.2 整数选项

| 类型 | 取值范围 | 说明 |
|---|---|---|
| `unsigned` | 非负整数 | 语义上多数选项需要正整数；例如 `block-dim` 通常应大于 0。 |
| `int32_t` | 32 位有符号整数 | 个别选项用 `-1` 表示不限。 |
| `int64_t` | 64 位有符号整数 | 多用于 tuning 参数。 |

### 2.3 列表选项

`ListOption` 使用 LLVM `cl::list`，并隐式开启 `CommaSeparated`，所以可以逗号分隔：

```bash
--cube-tiling-tuning=32,64,128
--link-aicore-bitcode=/abs/a.bc,/abs/b.bc
```

列表选项也可以重复出现，最终进入一个 vector。`--hivmc-args` 在收集时还会按空格拆开，适合追加多个下游 `hivmc` 参数。

### 2.4 枚举选项

`MultiBufferStrategy` 的命令行取值如下：

```text
no-limit
no-l0c
only-cube
only-vector
```

不同选项允许的枚举子集不同，见下文表格。

## 3. 输入输出选项

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `<input file>` | string | `-` | 文件路径或 `-` | 输入 MLIR / `.ttadapter`。 |
| `-o <file>` | string | `-` | 文件路径或 `-` | 输出目标文件名。作为 pass option 时也注册为 `o`。 |

## 4. Feature Control Options

这些选项决定大方向：走 Torch、HFusion、HIVM、Triton，还是跳过某些自动管理阶段。

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `--enable-triton-kernel-compile` | bool | `false` | `true/false` | 使能 Triton kernel 编译路径。会影响 HFusion preprocess、HFusionToHIVM 的 matmul map mode、Triton 参数转换、retry policy 等。 |
| `--enable-ubuf-saving` | bool | `false` | `true/false` | 开启 UB saving 模式。会影响 ConvertToHIVM 和 HIVM bufferization 前的相关优化。 |
| `--disable-size-align-for-cast` | bool | `false` | `true/false` | 禁用 VCast alloc size alignment，使用新的 s32/s16 到 s8 temp buffer 方案。 |
| `--enable-torch-compile` | bool | `false` | `true/false` | 使能 Torch-MLIR 编译路径。仅在 `BISHENGIR_ENABLE_TORCH_CONVERSIONS` 构建开启时存在。 |
| `--enable-hivm-compile` | bool | `true` | `true/false` | 使能 HIVM 编译 pipeline。关闭后不会执行 ConvertToHIVM / OptimizeHIVM 主流程。 |
| `--enable-hfusion-compile` | bool | `false` | `true/false` | 使能 HFusion 编译 pipeline。高层 Linalg/Tensor/Triton 通常需要打开。 |
| `--enable-symbol-analysis` | bool | `false` | `true/false` | HFusion 中保留并传播 symbol 信息。关闭时会先 erase symbol。 |
| `--enable-multi-kernel` | bool | `false` | `true/false` | 允许 outline 多个 kernel；关闭时图需要尽量融合成单 kernel。 |
| `--enable-manage-host-resources` | bool | `false` | `true/false` | 开启 Host 函数资源管理，主要影响 HFusion auto schedule。 |
| `--enable-static-bare-ptr` | bool | `true` | `true/false` | 为静态 shape kernel 生成 bare pointer 调用约定。该选项会传递给下游工具链。 |
| `--enable-bin-relocation` | bool | `true` | `true/false` | 使能二进制重定位。该选项会传递给下游工具链。 |
| `--ensure-no-implicit-broadcast` | bool | `false` | `true/false` | Torch 路径选项；要求没有隐式 broadcast，若存在动态到动态维 broadcast 可报运行时错误。仅在 Torch conversion 构建开启时存在。 |
| `--enable-lir-compile` | bool | `true` | `true/false` | 使能 BiShengLIR 编译。非 publish 构建存在，并传给下游工具链。 |
| `--disable-auto-inject-block-sync` | bool | `false` | `true/false` | 关闭 block sync wait/set 自动插入。 |
| `--enable-hivm-graph-sync-solver` | bool | `true` | `true/false` | intra-core sync 使用 HIVM graph-sync-solver，而不是普通 inject-sync。 |
| `--disable-auto-cv-work-space-manage` | bool | `false` | `true/false` | 关闭 mix CV workspace 自动管理。打开后会跳过 InsertLoadStoreForMixCV、InsertWorkSpaceForMixCV、pre-bufferization MarkMultiBuffer、CVPipelining、global workspace PlanMemory 等。 |
| `--enable-hivm-cross-core-gss` | bool | `true` | `true/false` | cross-core/block sync 使用 graph-sync-solver，而不是 inject-block-sync。 |
| `--disable-hivm-auto-inject-sync` | bool | `false` | `true/false` | 关闭 intra-core auto inject sync。 |
| `--disable-hivm-tensor-compile` | bool | `false` | `true/false` | 跳过 HIVM tensor compile 阶段，直接进入 post-bufferization optimization。只有输入已经满足后续要求时才适合打开。 |
| `--enable-legacy-insert-load-store-for-mix-cv` | bool | `false` | `true/false` | 使用 legacy 的 InsertLoadStoreForMixCV 路径。 |

## 5. DFX / Debug / 诊断选项

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `--enable-sanitizer` | bool | `false` | `true/false` | 开启 Ascend sanitizer。该选项会传给下游工具链；同时会开启 debug info 打印。 |
| `--enable-memory-display` | bool | `false` | `true/false` | 开启 memory display。PlanMemory 会给 temp buffer 标记并输出 `memory_info*.json`。同时会开启 debug info 打印。 |
| `--enable-debug-info` | bool | `false` | `true/false` | 开启 debug info。会传给下游工具链。 |
| `--enable-cpu-trace-intrinsic` | bool | `false` | `true/false` | 生成 host-accepted IR，消除部分 HIVM special traits。非 publish 构建存在，并传给下游工具链。 |
| `--inject-ir-from-file=<path>` | string | 空 | 文件路径 | 调试用：从文件加载函数并替换当前 module 中的同名函数。会传给下游工具链。 |

## 6. General Optimization Options

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `--enable-auto-multi-buffer` | bool | `false` | `true/false` | 开启自动 multi-buffer。会影响 HFusion auto schedule、HIVM MarkMultiBuffer、PlanMemory 看到的 buffer 数量和生命周期。 |
| `--limit-auto-multi-buffer-only-for-local-buffer` | bool | `true` | `true/false` | 当开启 auto multi-buffer 时，限制其只作用于 local buffer。 |
| `--enable-tuning-mode` | bool | `false` | `true/false` | 开启 tuning mode。源码注释为 PlanMemory 失败时不尝试多次编译；实际 `runBiShengIRPipeline` 中在非 Triton 路径会启用 `TuningRetryPolicy`。 |
| `--block-dim=<n>` | unsigned | `1` | 非负整数；实际建议 `n >= 1` | 指定 block 数。HFusion auto schedule 会读取；kernel 也会携带相关 block dim 属性。 |

## 7. HFusion Optimization Options

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `--enable-deterministic-computing` | bool | `true` | `true/false` | 开启确定性计算。关闭后允许可能带来性能提升但结果非确定的优化，例如 reduce 多核绑定。 |
| `--enable-ops-reorder` | bool | `true` | `true/false` | 开启 HFusion op reorder，用于优化 pipeline。 |
| `--hfusion-max-horizontal-fusion-size=<n>` | int32_t | `-1` | `-1` 或 32 位有符号整数 | 横向 fusion 尝试上限。`-1` 表示不限。 |
| `--hfusion-max-buffer-count-tuning=<n>` | int64_t | `0` | 64 位有符号整数 | HFusion auto schedule 中的 max buffer count tuning 参数。 |
| `--cube-tiling-tuning=<list>` | list<int64_t> | 空 | 逗号分隔的 int64 列表 | HFusion auto schedule 中的 cube block size tuning 列表。 |
| `--enable-hfusion-count-buffer-dma-opt` | bool | `false` | `true/false` | 开启后，DMA 使用的 buffer 不会被 Vector op 复用。 |

## 8. HIVM Optimization Options

这些选项对 PlanMemory、UB overflow、CV pipeline、multi-buffer 最相关。

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `--limit-auto-multi-buffer-of-local-buffer=<mode>` | enum | `no-l0c` | `no-limit` / `no-l0c` | 当开启 auto multi-buffer 时，限制 local buffer 的 multi-buffer 模式。`no-l0c` 表示禁用 L0C multi-buffer。 |
| `--limit-auto-multi-buffer-buffer=<mode>` | enum | `only-cube` | `no-limit` / `only-cube` / `only-vector` | 当开启 auto multi-buffer 时，限制作用于 cube buffer、vector buffer 或不限。 |
| `--enable-auto-bind-sub-block` | bool | `true` | `true/false` | 开启 auto bind sub block。 |
| `--enable-code-motion` | bool | `true` | `true/false` | 开启 code motion / subset hoist。 |
| `--enable-hivm-inject-barrier-all-sync` | bool | `false` | `true/false` | HIVM inject sync 使用 barrier-all 模式。会传给下游工具链。 |
| `--enable-hivm-unit-flag-sync` | bool | `false` | `true/false` | inject sync 使用 unit-flag 模式。 |
| `--enable-hivm-assume-alive-loops` | bool | `false` | `true/false` | 假设所有 `for` / `while` loop 至少执行一次。 |
| `--enable-hivm-inject-block-all-sync` | bool | `false` | `true/false` | HIVM inject block sync 使用 block-all 模式。 |
| `--set-workspace-multibuffer=<n>` | unsigned | `2` | 非负整数；实际建议 `n >= 1`，multi-buffer 场景常用 `n >= 2` | 覆盖 workspace multi-buffer 数。pre-bufferization MarkMultiBuffer 会读取。 |
| `--enable-hivm-global-workspace-reuse` | bool | `false` | `true/false` | 开启 global workspace reuse，影响第一次 `PlanMemory(GLOBAL_WORKSPACE_PLAN)`。 |
| `--enable-hivm-auto-cv-balance` | bool | `false` | `true/false` | 开启 CV pipelining 中的 balancing 相关能力。 |
| `--enable-preload` | bool | `false` | `true/false` | 开启 preload。CVPipelining 会进入 skew/preload 相关模式；local PlanMemory 后还会运行 `CreatePreloadPass`。 |
| `--enable-hivm-auto-storage-align` | bool | `true` | `true/false` | 开启 storage align 标记和生效流程。影响 alloc size / stride align / PlanMemory 前 buffer 形态。 |
| `--enable-hivm-nd2nz-on-vector` | bool | `false` | `true/false` | 在 vector 路径启用 nd2nz。 |
| `--enable-auto-blockify-loop` | bool | `false` | `true/false` | 开启 parallel loop auto blockify。当前 HIVM pipeline 中要求 `enable-triton-kernel-compile=true` 时才会运行对应 pass。 |
| `--tile-mix-vector-loop=<n>` | unsigned | `1` | 非负整数；`1` 表示不额外 tile | mix kernel 中 vector loop 的 tile trip count。 |
| `--tile-mix-cube-loop=<n>` | unsigned | `1` | 非负整数；`1` 表示不额外 tile | mix kernel 中 cube loop 的 tile trip count。 |

## 9. Target Options

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `--target=<device>` | enum | `Ascend910B1` | 见下方列表 | 指定目标芯片。用于追加 device spec，并传给下游工具链。 |

当前源码中注册的 target：

```text
Ascend910B1
Ascend910B2
Ascend910B3
Ascend910B4
Ascend910B4-1
Ascend910B2C
Ascend910_9362
Ascend910_9372
Ascend910_9381
Ascend910_9382
Ascend910_9391
Ascend910_9392
Ascend910_950z
Ascend910_9579
Ascend910_957b
Ascend910_957d
Ascend910_9581
Ascend910_9589
Ascend910_958a
Ascend910_958b
Ascend910_9599
Ascend950PR_950z
Ascend950PR_9579
Ascend950PR_957a
Ascend950PR_957b
Ascend950PR_957c
Ascend950PR_957d
Ascend950PR_9589
Ascend950PR_958a
Ascend950PR_958b
Ascend950PR_958c
Ascend950PR_958d
Ascend950PR_9599
Ascend950PR_959a
Ascend950PR_959b
Ascend950DT_950x
Ascend950DT_950y
Ascend950DT_9571
Ascend950DT_9572
Ascend950DT_9573
Ascend950DT_9574
Ascend950DT_9575
Ascend950DT_9576
Ascend950DT_9577
Ascend950DT_9578
Ascend950DT_9581
Ascend950DT_9582
Ascend950DT_9583
Ascend950DT_9584
Ascend950DT_9585
Ascend950DT_9586
Ascend950DT_9587
Ascend950DT_9588
Ascend950DT_9591
Ascend950DT_9592
Ascend950DT_9595
Ascend950DT_9596
Ascend950DT_95A1
Ascend950DT_95A2
Unknown
```

## 10. Other Options

| 选项 | 类型 | 默认值 | 取值范围 | 作用 |
|---|---|---:|---|---|
| `--allow-unregistered-dialects` | bool | `false` | `true/false` | 允许未注册 dialect。仅建议测试时使用。 |
| `--hivmc-version=<str>` | string | 空 | 任意字符串；通常为版本号 | 指定或覆盖 `hivmc` 兼容版本。编译时也会尝试自动探测 `hivmc` 版本。 |
| `--hivmc-args=<args>` | list<string> | 空 | 逗号分隔或重复；收集后还会按空格拆分 | 追加传给下游 `hivmc` 的参数。 |
| `--link-aicore-bitcode=<paths>` | list<string> | 空 | 一个或多个绝对路径，逗号分隔 | 收集 `.bc` 路径并合并成下游 `--link-aicore-bitcode=...`。 |
| `--append-bisheng-options=<str>` | string | 空 | 任意字符串 | 调用 bisheng 时追加选项。会传给下游工具链。 |

## 11. MLIR 通用打印选项

这些不是 `Options.td` 独占注册的普通编译选项，但 `bishengir-compile` 会使用 MLIR pass manager，因此调试时非常常用。源码中还专门捕获 `mlir-print-ir-before-all` / `mlir-print-ir-after-all` 并转发给 `hivmc`。

| 选项 | 类型 | 默认值 | 作用 |
|---|---|---:|---|
| `--mlir-print-ir-before-all` | bool | `false` | 每个 pass 执行前打印 IR。 |
| `--mlir-print-ir-after-all` | bool | `false` | 每个 pass 执行后打印 IR。 |
| `--mlir-print-ir-before=<pass-list>` | string/list | 空 | 指定 pass 前打印 IR。 |
| `--mlir-print-ir-after=<pass-list>` | string/list | 空 | 指定 pass 后打印 IR。 |
| `--mlir-print-ir-after-failure` | bool | `false` | pass 失败后打印 IR。 |
| `--mlir-print-ir-module-scope` | bool | `false` | 以 module scope 打印 IR，便于观察全局上下文。 |
| `--mlir-print-ir-tree-dir=<dir>` | string | 空 | 把 pass IR dump 到目录树。 |
| `--mlir-print-debuginfo` | bool | `false` | 打印 debug location。`enable-sanitizer` / `enable-memory-display` / `enable-debug-info` 会自动打开它。 |
| `--mlir-disable-threading` | bool | `false` | 禁用 MLIR 多线程，便于稳定 dump 顺序。 |

## 12. CPU Runner 选项

这些选项只有在构建时启用了 `MLIR_ENABLE_EXECUTION_ENGINE` 才存在，主要用于把编译停在某个位置并走 CPU runner lowering。

| 选项 | 格式 | 作用 |
|---|---|---|
| `--enable-cpu-runner=[<options>]` | 可选 options | 在最终输出上启用 CPU runner lowering pipeline。 |
| `--enable-cpu-runner-before=<pass>[,<index>][,<options>]` | pass 名必填 | 在指定 pass 之前启用 CPU runner，并停止后续编译。 |
| `--enable-cpu-runner-after=<pass>[,<index>][,<options>]` | pass 名必填 | 在指定 pass 之后启用 CPU runner，并停止后续编译。 |

`<index>` 是同名 pass 的第几次出现，必须是正整数，默认是 `1`。

## 13. PlanMemory 专用调试环境变量

| 环境变量 | 取值 | 作用 |
|---|---|---|
| `BISHENGIR_DUMP_BEFORE_PLAN_MEMORY` | 输出文件路径 | 在 local `PlanMemory(LOCAL_MEM_PLAN)` 之前 dump 当前 module。dump pass 位于 `MarkMultiBuffer` 和 local PlanMemory 之间。 |

这个不是命令行 option，而是 `HIVMPipelines.cpp` 中 `DumpIRBeforePlanMemoryPass` 读取的环境变量。

## 14. 选项如何实际进入 pipeline

`bishengir-compile` 外层会先构造 `BiShengIRCompileMainConfig`，然后按如下顺序构建主 pipeline：

```text
buildBiShengHIRPipeline
  createCanonicalizeModulePass
  hacc::createAppendDeviceSpecPass(target, hivmcVersion)
  if enableTorchCompile:
    Torch backend -> NamedOp backend pipeline
  if enableHfusionCompile:
    HFusion pipeline
  if enableHIVMCompile:
    ConvertToHIVM pipeline
    OptimizeHIVM pipeline
```

其中：

- `enable-hfusion-compile` 决定是否进入 HFusion pipeline。
- `enable-hivm-compile` 决定是否进入 ConvertToHIVM 和 OptimizeHIVM。
- `enable-triton-kernel-compile` 会影响 HFusion preprocess、HFusionToHIVM map mode、Triton global kernel args 转换、AutoBlockify 条件、Triton 专用 retry policy。
- `disable-hivm-tensor-compile` 会跳过 HIVM pre-bufferization 和 bufferization，直接进入 post-bufferization pipeline。
- `enable-memory-display` 会传入 local PlanMemory，用于输出 memory display 信息。

## 15. 和 UB overflow / PlanMemory 最相关的组合

观察 `data/*.ttadapter` 的 PlanMemory 前后 IR 和 overflow，常用组合：

```bash
BISHENGIR_DUMP_BEFORE_PLAN_MEMORY=before.mlir \
build/bin/bishengir-compile data/attn_fwd.ttadapter \
  -o output.o \
  --target=Ascend910_9382 \
  --enable-hfusion-compile=true \
  --enable-hivm-compile=true \
  --enable-triton-kernel-compile=true \
  --enable-memory-display=true \
  --mlir-disable-threading \
  --mlir-print-ir-after-all \
  --mlir-print-ir-module-scope \
  --mlir-print-ir-tree-dir=ir_tree
```

如果要观察 multi-buffer 对 UB 峰值的影响，可以重点改变：

```bash
--enable-auto-multi-buffer=true
--limit-auto-multi-buffer-of-local-buffer=no-l0c
--limit-auto-multi-buffer-buffer=only-cube
--set-workspace-multibuffer=2
--enable-preload=true
--disable-auto-cv-work-space-manage=true
```

其中：

- `enable-auto-multi-buffer` 会影响 MarkMultiBuffer。
- `limit-auto-multi-buffer-*` 会影响哪些 buffer 被标成 multi-buffer。
- `set-workspace-multibuffer` 会影响 workspace multi-buffer 数。
- `enable-preload` 会影响 CVPipelining 的 skew/preload 路径，并在 local PlanMemory 后运行 preload pass。
- `disable-auto-cv-work-space-manage` 会跳过 CVPipelining 和 global workspace PlanMemory 等关键阶段，适合做对照实验。

## 16. 兼容性注意事项

`runExternalHIVMC` 会把一部分选项继续传给下游 `hivmc`。为了兼容旧版 `hivmc`，源码中有如下处理：

- 如果探测不到 `hivmc` 版本或版本为空，会跳过 debug/print 相关选项。
- legacy `hivmc` 下，如果开启 `enable-triton-kernel-compile`，会额外手动追加 `--enable-triton-kernel-compile=true`。
- 对旧版 `hivmc` 会过滤部分不支持选项，例如 `enable-lir-compile`、`enable-cpu-trace-intrinsic`、`link-aicore-bitcode`。
- `hivmc` 版本为 `0.1.0` 时，会过滤 `link-aicore-bitcode`。

