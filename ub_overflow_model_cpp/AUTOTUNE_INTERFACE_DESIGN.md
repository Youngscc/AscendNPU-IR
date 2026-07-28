# UB Overflow Model Autotune 进程内接口规范

状态：进程内模型 API 已实现；BiShengIR prediction pass 接入待完成

版本：In-Process API v1；UBRelevantCompileOptions v4

更新时间：2026-07-24

当前实现位置：

```text
include/ub_overflow_model/api.hpp        稳定公共 DTO 和 evaluate() 声明
src/api.cpp                              内存 IR、参数适配、诊断、摘要和计时实现
output/lib/libub_overflow_model.a        build.sh 生成的可链接静态库
tests/test_in_process_api.cpp             接口契约回归
```

当前结论：现有 API 的调用形态可以保留，但 `EffectiveCompileOptions v1` 尚不足以对所有
真实 pipeline 分支声明峰值完全一致。在本文要求的 options v4、prediction pass 和
differential Exact 认证完成前，只允许 shadow/fail-open，不允许据此 prune candidate。

## 1. 目标与边界

Triton-Ascend 为每个 autotune candidate 生成 kernel 输入和一组编译选项，然后调用
`bishengir-compile`。本方案把 UB Overflow Model 链接进 `bishengir-compile`，在真实
`CVPipelining` 之前调用模型：

```text
candidate 的 constexpr/shape 参数
        + Ascend autotune 编译参数
        + 固定 backend/target/环境配置
        + 输入 TTAdapter/MLIR 文件
                      |
                      v
               bishengir-compile
                      |
          编译到 pre-CVPipelining ModuleOp
                      |
          UB Overflow Model（进程内）
             |                     |
      exact overflow         其他结果
             |                     |
      终止该 candidate       继续真实编译
             |                     |
             +--------> autotune <+
```

这里存在两层接口：

1. **外层编译接口**：输入文件和完整 BiShengIR 编译参数进入 `bishengir-compile`；
2. **内层模型接口**：pre-CVPipelining IR 和归一化后的 UB 相关有效参数进入模型。

模型不能直接读取 autotune 的原始 `triton.Config`。它必须读取与真实 pipeline 共用的
`HIVMPipelineOptions` 或等价的归一化配置，避免默认值、别名和优先级发生漂移。

## 2. 判定策略

```text
exact + overflow      -> 终止当前 candidate，向 autotune 报告 predicted UB overflow
exact + non-overflow  -> 在未修改的真实 ModuleOp 上继续编译
incomplete/blocker    -> fail-open，继续真实编译
模型内部错误           -> fail-open，继续真实编译并记录诊断
```

模型执行在独立的模型 IR 上，不能修改真实编译的 `ModuleOp`。v1 可以把 `ModuleOp` 打印
为内存字符串后交给模型现有 Generic IR parser；不得写临时文件。以后可把 parser 改为
直接读取 MLIR operation，以进一步减少序列化和解析开销。

## 3. 参数来源和归一化

一个 candidate 的最终编译参数来自四层：

```text
BiShengIR/后端默认值
  <- target 和环境变量
  <- 用户固定的 NPUOptions / launch meta-parameters
  <- 当前 autotune candidate
```

同名字段以后者覆盖前者。调用模型前必须完成以下归一化：

```text
multibuffer=false 或 num_stages=1 -> enable_auto_multi_buffer=false
否则，只要二者显式出现             -> enable_auto_multi_buffer=true
None/未传入                         -> 展开为 BiShengIR 的实际默认值
disable_*                           -> 转换为正向的最终 pass 开关
target                              -> 同时确定 target 能力和 UB capacity
```

模型请求中不允许保留 `None` 表示“让模型猜默认值”。响应或 debug trace 应记录
`effective_options_digest`，用于确认模型和真实编译使用的是同一组参数。

### 3.1 峰值完全一致的必要条件

本规范中的“完全一致”不是只指 `overflow` 布尔值一致，而是对同一个 compiler attempt
至少满足：

```text
每个 AIV function 的 ub_peak_bits 一致
每个 AIV function 的 required_bits 一致
selected_seed 一致
multi-buffer 数量一致
最终顶层 overflow 一致
```

只有同时满足以下条件，模型才允许返回 `Precision::Exact`：

1. 输入确实来自指定 BiShengIR build 的 `createCVPipeliningPass` 之前一刻；
2. 输入使用 Generic MLIR 形式打印，且保留所有 attribute、property、region 和 symbol；
3. 模型收到从当前 `HIVMPipelineOptions` 读取的有效值，而不是重新解析 CLI 或使用 DTO
   默认值；
4. 从该边界到本地 `PlanMemory` 的所有条件分支都已模拟，或已被证明对 UB
   `PlanMemory` 不变；
5. 使用与真实 PlanMemory 相同的 seed retry、alignment、inplace、capacity 和
   multi-buffer 规则；
6. 当前 compiler build、pipeline fingerprint、target 和选项组合属于 differential
   oracle 已认证范围。

任一条件无法确认时必须返回 `Incomplete + Blocker`，不得返回 non-overflow。API 的默认值
只服务独立 CLI 和单元测试；prediction pass 必须逐字段覆盖生产请求。

## 4. Ascend Autotune 可控参数

当前 Ascend autotune 专用编译参数共有 **10 个参数名、9 个独立维度**。
`multibuffer` 是 `num_stages` 的布尔别名。不同 kernel 类型只搜索其中的子集。

| autotune 参数 | BiShengIR 有效参数 | MIX | 是否需要进入 UB 模型 | 原因 |
|---|---|:---:|:---:|---|
| `num_stages` | `enable_auto_multi_buffer` | 是 | 是 | `1` 关闭，否则开启 auto multi-buffer |
| `multibuffer` | `enable_auto_multi_buffer` | 别名 | 是 | 与 `num_stages` 归一为一个字段 |
| `unit_flag` | `enable_hivm_unit_flag_sync` | 是 | 否 | 正常同步注入位于本地 PlanMemory 之后 |
| `limit_auto_multi_buffer_only_for_local_buffer` | 同名 | 是 | 输入已编码 | 第一次 MarkMultiBuffer 决定是否给 workspace 加标记；CVPipelining 会读取该标记并可能改变展开深度。模型不必再收一个重复字段，但输入必须按同一参数生成 |
| `limit_auto_multi_buffer_of_local_buffer` | 同名 | 是 | 是 | 决定 UB/L1/L0C 等 local buffer 的 multi-buffer 标记 |
| `set_workspace_multibuffer` | 同名 | 是 | 输入已编码 | 虽然标记对象是 global workspace，CVPipelining 会读取其 multi-buffer 数并据此展开局部值，因此可能间接改变 UB；该值必须与生成 before-CVPipelining IR 时一致 |
| `enable_hivm_auto_cv_balance` | 同名 | 是 | 当前否 | 当前 checkout 只转发选项，未接入任何 HIVM pass |
| `tile_mix_vector_loop` | 同名 | 是 | 是 | CVPipelining 后改变 Vector loop 和 buffer lifetime |
| `tile_mix_cube_loop` | 同名 | 是 | 是 | CVPipelining 后改变 Cube loop 和 buffer lifetime |
| `enable_ubuf_saving` | 同名 | 是 | 是 | sink/clone pass 会改变 UB lifetime 和分配 |

因此，模型 v1 从 autotune candidate 必须消费的是 **6 个参数名、5 个独立维度**：

```text
num_stages / multibuffer
limit_auto_multi_buffer_of_local_buffer
tile_mix_vector_loop
tile_mix_cube_loop
enable_ubuf_saving
```

其余 autotune 参数仍由真实 `bishengir-compile` 接收；模型明确将它们标记为
`UB-invariant` 或 `not-wired`，不能因为没有放入模型 options 就从真实编译命令删除。

## 5. 重新规划后的最小 UB 参数集

参数是否进入 API 的判断标准不是“autotune 能否修改”，而是：在已经固定的
pre-CVPipelining 输入边界之后，它的取值是否可能改变本地 UB PlanMemory 看到的 buffer、
extent、multi-buffer 数量、lifetime、alignment 或 inplace 关系。

### 5.1 每个请求必须传入的 16 个有效值

| 调用方有效值 | API 字段 | 影响阶段 | 为什么必须传 |
|---|---|---|---|
| CV pipeline depth | `cvPipelineDepth` | CVPipelining | 改变展开深度、复制和 lifetime |
| 禁用自动 CV/workspace 管理 | `disableAutoCVWorkSpaceManage` | CVPipelining、workspace plan | 选择是否执行 CVPipelining 及全局 workspace 管理路径 |
| CV lazy loading | `enableCVLazyLoading` | CVPipelining | 改变 load 克隆和中间 buffer 扩张 |
| `enable_preload` | `enablePreload` | CVPipelining | 选择 skew/preload 模式 |
| `code_motion` | `enableCodeMotion` | CV 后、bufferization 前 | 改变 op 位置和 lifetime |
| `enable_auto_bind_sub_block` | `enableAutoBindSubBlock` | SplitMix 后 | 改变 tile、shape 和 buffer extent |
| `enable_ubuf_saving` | `enableUbufSaving` | CV 后及 bufferization 前 | sink/clone 改变 lifetime |
| `multibuffer/num_stages` 归一值 | `enableAutoMultiBuffer` | 最终 MarkMultiBuffer | 改变 buffer 副本数 |
| `storage_align` | `enableHIVMAutoStorageAlign` | bufferization 后 | 控制 stride-alignment 标记 |
| `tile_mix_vector_loop` | `tileMixVectorLoop` | CV 后 | 改变 Vector loop 和 lifetime |
| `tile_mix_cube_loop` | `tileMixCubeLoop` | CV 后 | 改变 Cube loop 和 lifetime |
| local multi-buffer 策略 | `localMultiBufferStrategy` | 最终 MarkMultiBuffer | 决定 local address space 是否扩张 |
| MIX multi-buffer 策略 | `mixMultiBufferStrategy` | 最终 MarkMultiBuffer | 决定 Cube/Vector 哪类 buffer 扩张 |
| cross-core GSS 选择 | `enableHIVMCrossCoreGSS` | MarkRealCoreType 后 | 决定使用 CrossCoreGSS 还是原生 InjectBlockSync 分支 |
| block-all sync | `enableHIVMInjectBlockAllSync` | MarkRealCoreType 后 | 强制 InjectBlockSync 为目标 op 前后插入 block-all 序列 |
| 禁用自动 block sync | `disableAutoInjectBlockSync` | MarkRealCoreType 后 | 进入 InjectBlockSync 但仅保留 SetFFTSBaseAddr |

调用方还必须传 `target`，但 `capacityBits` 不再作为可独立修改的生产参数：模型根据
`target + compilerProfile` 查询 UB capacity。调用方可以附带 expected capacity 做一致性
断言，但不能覆盖模型能力表。

其中当前真实 pipeline 的 `cvPipelineDepth=-1`、`enableCVLazyLoading=false`，仍然要求
prediction pass 显式传入。这样未来真实 pipeline 接线后不会因为模型继续使用旧默认值而
静默产生错误 Exact 结果。

当前 checkout 中，生产 `HIVMPipelines.cpp` 只把 `enablePreload` 接到
`CVPipeliningOptions.enableSkewMode`；命令行虽然声明了 `enable_lazy_loading`，但尚未写入
生产 CVPipelining options，显式 depth 也只属于实验 suffix。因此二者是必须记录的固定
profile 事实，不是当前场景矩阵的 sweep 维度。

### 5.2 不逐请求传递、但模型必须知道的固定编译事实

以下事实会影响 UB，但在本实验的 Triton membase profile 中不是 candidate 参数。它们由
`CompilerProfile + compilerPipelineFingerprint` 绑定，不作为每个请求的自由字段：

```text
CVPipelining 必定执行
enableTritonKernelCompile = true
AlignAllocSize 必定执行
EnableStrideAlign 必定执行
InferHIVMDataLayout 必定执行
最终 MarkMultiBuffer 的 only-for-local = true
PlanMemory 使用真实默认 seed 0..19 retry
PlanMemory 使用生产 inplace 规则（restrictInplaceAsISA = false）
```

profile 中任一事实与真实 pipeline 不符时，调用方不得调用 prune API，模型也不得返回
Exact。独立 CLI 的 disable-pass、固定 seed、capacity override 等实验开关放入单独的
`DebugModelControls`，不能出现在 autotune 生产 Request 中。

profile 还必须声明不会传入模型的旁路分支允许值，例如 global-workspace reuse。
cross-core sync 的三个实际分支选择值已经逐请求传入，prediction pass 不得再用固定默认值
代替真实编译参数。

### 5.3 不需要传入 API 的参数

| 参数类别 | 示例 | 不传原因 |
|---|---|---|
| 已体现在输入 IR 中 | auto-blockify、HFusion、ops reorder、第一次 MarkMultiBuffer 的 workspace 配置（含 `limit_auto_multi_buffer_only_for_local_buffer`、`set_workspace_multibuffer`） | 模型输入位于这些 pass 之后；workspace multi-buffer 标记仍可被 CVPipelining 读取并间接影响 UB，所以生成输入时必须匹配 |
| global workspace-only | `enable_global_workspace_reuse` | 只改变 global workspace 规划，不进入本地 UB address space |
| 本地 PM 后同步 | `unit_flag`、graph sync、barrier sync | 在本地 PlanMemory 后生效 |
| InjectBlockSync 内部固定事实 | `enable_hivm_assume_alive_loops=false` | 当前 autotune 不传该参数；绑定在 compiler profile 中。若未来成为可控参数，必须新增模型字段和原生循环分支实现 |
| 当前未接线 | `enable_hivm_auto_cv_balance` | 当前 checkout 没有 pass 消费者 |
| DFX/codegen | debug、memory display、sanitizer、bitcode、输出路径 | 不改变本地 UB 规划 |

三个当前可控的 cross-core 分支参数已经进入 API，并由模型复刻原生分支。其余只影响同步
内部、且当前不可控的事实仍由 fingerprint/profile 绑定，未知取值必须 fail-open。

### 5.4 当前 API 必须调整

当前 `EffectiveCompileOptions v1` 把生产编译参数、固定 pipeline 事实和模型调试控制混在
一个结构体中。当前已经升级为 `UBRelevantCompileOptions v4`：

1. 只保留第 5.1 节的 16 个 UB 相关有效值；
2. `target` 移到 Request/CompilerProfile，capacity 由模型推导；
3. 删除生产 Request 中的 `disableCVPipelining`、`enableTritonKernelCompile`、
   `alignAllocSize`、`enableStrideAlign`、`inferHIVMDataLayout`、`planMemorySeed`、
   `restrictInplaceAsISA`；这些改由 profile 或 DebugModelControls 管理；
4. 增加 `inputContractVersion`、`compilerProfile`、`compilerPipelineFingerprint`；
5. prediction pass 必须从实际 `CVPipeliningOptions/HIVMPipelineOptions` 逐字段填满 16 个值，
   不得从 `NPUOptions`、autotune 字典或 DTO 默认值反推；
6. `localMultiBufferStrategy` 只接受真实 membase compiler 的 `no-limit/no-l0c`；
   `mixMultiBufferStrategy` 只接受 `no-limit/only-cube/only-vector`；
7. `disable_multi_buffer_on_ub/l0c/l1` 当前尚未接入真实 MarkMultiBuffer。一旦接线，它们会
   直接改变 UB 副本数，必须新增到 API、bump options version 并重新做 Exact 认证。
8. 模型按照原生条件在 `CrossCoreGSS` 与 `InjectBlockSync` 之间选择；三个分支参数必须
   来自同一份真实 `HIVMPipelineOptions`，不能由模型默认值代替。

### 5.5 有意义的参数场景

不对上述字段做笛卡尔积。仓库中的
`config/ub_relevant_parameter_scenarios.tsv` 使用三条规则构造场景：

1. multi-buffer 关闭时不枚举 local/mix strategy 和 workspace depth，因为这些字段无效；
2. preload、MIX tile 因子只投放到 MIX kernel，避免在非 MIX kernel 上制造重复用例；
3. 会改变第一次 MarkMultiBuffer 或 CV workspace 分支的场景标记为
   `regenerate_before_cv`，必须从 adapter 用同一组选项重新生成输入，不能拿固定的
   before-CVPipelining IR 只修改后缀参数。

当前统一场景集共 27 组：原 22 组之外增加普通 InjectBlockSync、block-all、禁用自动
InjectBlockSync，以及普通 InjectBlockSync 与 preload/auto multi-buffer 的两个交互场景。
新增 5 组不增加 pre-CVPipelining profile；原 22 组中 9 组复用默认输入，另外 13 组映射到
7 套变化 profile，因此仍不需要为每个后缀场景各生成一份输入。
组合只保留单因素机制覆盖和少量明确有耦合的交互：preload × multi-buffer、
ubuf-saving × multi-buffer、MIX tiling × multi-buffer，以及关闭 CV/workspace 管理后仍执行
最终 local MarkMultiBuffer 的分支。

默认 profile 加上上述 7 套变化一共 8 套，参数文件位于 `config/pre_cv_profiles/`。统一生成：

```bash
.venv/bin/python3 \
  ub_overflow_model_cpp/scripts/generate_before_cvpipelining_profiles.py \
  --output-root Output/before_cvpipelining_profiles \
  --profile-jobs 4 --timeout 180
```

输出按 `/<pre_cv_profile>/<adapter>/before_cvpipelining.mlirbc` 组织。这里必须使用 bytecode，
因为文本 MLIR 不能保存 CVPipelining 会观察到的 SSA use-list 顺序。测试 27 组场景时，
先按场景 TSV 的 `pre_cv_profile` 选择输入，再把该行边界之后的有效参数传给原生 BiSheng。

## 6. 完整 BiShengIR 编译输入清单

下面统计的是 Triton-Ascend 当前 `compiler.py` 可能传给 `bishengir-compile` 的参数并集。
不是每个 target 或每次调用都会出现。它们仍属于外层编译接口，但大部分不需要重复进入
模型的内部 options。

### 6.1 输入、target 和输出

| 命令行输入 | 来源 | 模型处理 |
|---|---|---|
| 输入 `.ttadapter.mlir`/`.mlir` 文件 | 当前 candidate 生成的 IR | 编译到 pre-CVPipelining 后作为模型 IR |
| `--target` | target/device | 进入模型 |
| `-o` | 临时输出路径 | 与 UB 判定无关 |

### 6.2 Pipeline 和内存相关参数

| BiShengIR 参数 | compiler.py 来源 | 模型分类 |
|---|---|---|
| `--enable-auto-multi-buffer` | `multibuffer/num_stages` | 必需 |
| `--enable-ubuf-saving` | `enable_ubuf_saving` | 必需 |
| `--enable-hivm-auto-storage-align` | `storage_align` | 必需 |
| `--enable-code-motion` | `code_motion` | 必需 |
| `--enable-preload` | `enable_preload` | 必需 |
| `--enable-auto-bind-sub-block` | 用户值或 IR metadata | 必需 |
| `--tile-mix-vector-loop` | autotune/NPUOptions | 必需 |
| `--tile-mix-cube-loop` | autotune/NPUOptions | 必需 |
| `--limit-auto-multi-buffer-of-local-buffer` | autotune/NPUOptions | 必需 |
| `--limit-auto-multi-buffer-buffer` | NPUOptions；当前 A2/A3 调用未转发，使用 BiSheng 默认 `only-cube` | 必需；prediction 从真实 resolved option 读取 |
| `--limit-auto-multi-buffer-only-for-local-buffer` | autotune/NPUOptions | 本地 UB 不需要 |
| `--set-workspace-multibuffer` | autotune/NPUOptions | workspace-only |
| `--enable-hivm-auto-cv-balance` | autotune/NPUOptions | 当前未接线 |
| `--enable-hivm-global-workspace-reuse` | HIVM pipeline option | global workspace-only，不传 |
| `--enable-hivm-cross-core-gss` | `sync_solver`/HIVM 默认值 | 必需；选择 GSS/InjectBlockSync |
| `--enable-hivm-inject-block-all-sync` | NPUOptions/HIVM 默认值 | 必需；选择 block-all 分支 |
| `--disable-auto-inject-block-sync` | NPUOptions/HIVM 默认值 | 必需；选择仅设置 FFTS base address 分支 |
| `--enable-hivm-assume-alive-loops` | HIVM pipeline option | 仅同步分支使用，UBInvariant 认证后不传 |
| `--disable-size-align-for-cast` | NPUOptions | 边界前 conversion 已体现在输入 IR |
| `--enable-auto-blockify-loop` | 环境和 IR blacklist | 在边界前生效，结果已体现在 preCV IR，不再作为模型字段 |
| `--disable-tightly-coupled-buffer-reuse` | 910/95 NPUOptions | target-specific，需在支持 910/95 时审计 |

对当前 A2/A3 autotune 路径，命令生成还有四个容易混淆的事实：

1. `NPUOptions.num_stages=2`、`multibuffer=true` 是 Triton 默认值，因此 autotune 默认会显式
   传 `--enable-auto-multi-buffer=true`；这与单独运行 BiSheng 时该选项的默认 `false` 不同。
2. `--enable-auto-bind-sub-block=<effective value>` 每次都会显式传入；其余大部分可调项仅在
   metadata 非 `None` 时出现，省略时由 BiSheng 自己解析默认值。
3. `sync_solver` 同时生成 `--enable-hivm-graph-sync-solver` 和
   `--enable-hivm-cross-core-gss`。前者主要作用于本地 PlanMemory 后，后者选择本地
   PlanMemory 前的 CrossCoreGSS/InjectBlockSync 分支，因此只有后者进入模型 DTO。
4. `NPUOptions.limit_auto_multi_buffer_buffer` 虽然存在，但当前 A2/A3 命令生成函数没有转发
   它；BiSheng 和 prediction 实际都看到默认 `only-cube`。如果要让 autotune 调这个维度，
   需要在 Triton 侧另行补转发；当前仓库的 BiSheng CLI 与轻量模型 API 已能接收它。

### 6.3 主要在 PlanMemory 后生效或已体现在输入 IR 中

| BiShengIR 参数 | compiler.py 来源 | 不进入 v1 模型的原因 |
|---|---|---|
| `--enable-ops-reorder` | `ops_reorder` | 若在边界前生效，结果已体现在 preCV IR |
| `--vf-fusion-mode` | `vf_fusion_mode` | HFusion/边界前配置，结果已体现在输入 IR |
| `--enable-hivm-graph-sync-solver` | `sync_solver` | 正常同步主要在本地 PlanMemory 后 |
| `--enable-hivm-unit-flag-sync` | `unit_flag` | 本地 PlanMemory 后 |
| `--enable-hivm-inject-barrier-all-sync` | `inject_barrier_all` | 同步/codegen，不改变本地 UB 规划 |
| `--enable-drop-unit-dims` | NPUOptions | 需要以 preCV IR 或 differential oracle 证明 UB 等价 |
| `--enable-flatten` | NPUOptions | 需要以 preCV IR 或 differential oracle 证明 UB 等价 |
| `--enable-auto-vectorize-v2` | NPUOptions | HFusion/边界前效果应已体现在输入 IR |
| `--hfusion-max-fused-ops-in-auto-vectorize-v2` | NPUOptions | HFusion/边界前 |
| `--hfusion-max-fused-elementwise-ops` | NPUOptions | HFusion/边界前 |
| `--enable-mixed-cv` | 910/95 NPUOptions | HFusion/路径选择；应体现在 preCV IR |
| `--enable-vf-fusion` | 910/95 NPUOptions | HFusion/边界前 |
| `--enable-vf-merge-level` | NPUOptions | HFusion/边界前 |
| `--hfusion-enable-multiple-consumer-fusion` | NPUOptions | HFusion/边界前 |
| `--hfusion-enable-cross-if-fusion` | NPUOptions | HFusion/边界前 |
| `--disable-hfusion-vectorize` | kernel mix mode | HFusion/边界前 |

标为“已体现在 preCV IR”的参数不能简单永久忽略。若以后把模型输入边界提前，这些参数必须
重新进入模型请求。

### 6.3.1 autotune 的实际返回合同

CVPipelining 到 PlanMemory 的生产 pass 本身没有跨进程 C++ 返回对象：成功时它们原地改写
`ModuleOp`，PlanMemory 最终把每个 local buffer 的 offsets 写回 alloc 相关 IR，随后总编译
继续生成二进制。Triton 最终拿到的是编译产物 bytes 和 callback metadata，而不是一份 UB
plan。`compiler.py` 在诊断开关开启时还会尝试从 stdout 匹配 `UB size = <bits> bits` 写入
`metadata["required_ub_bits"]`，但当前 A2/A3 membase pipeline 中没有找到该文本的生产者，
不能把这条兼容解析当作稳定 UB 接口。

真实本地 PlanMemory 在 20 个 seed 全部失败后发出：

```text
ub overflow, requires <required_bits> bits while <capacity_bits> bits available
```

这不是一个 Python 返回值，而是 MLIR error diagnostic。BiSheng 的
`RetriablePassManager` 先用它触发 `enable-code-motion=false`，再用它触发
`enable-auto-multi-buffer=false`；fallback 全部耗尽后，编译进程以非零状态退出。
`compiler.py` 使用 `subprocess.run(..., check=True)`，因此非零状态先成为
`CalledProcessError`，再由 Triton 编译阶段包装成 `MLIRCompilationError`。UBTuner 同样用
文本 `ub overflow` 判断是否进入后续搜索。

轻量 prediction pass 必须兼容上述诊断语义。普通运行默认不打印中间记录；显式设置
`BISHENGIR_UB_MODEL_EMIT_RESULT=1` 或进入同进程 validation 时，提供更完整的结构化
stderr 记录：

```text
BISHENGIR_UB_MODEL_RESULT contract_version=1 status=<success|overflow|blocker|internal_error> precision=<exact|incomplete> overflow=<true|false|unknown> ub_peak_bits=<bits|unknown> required_bits=<bits|unknown> capacity_bits=<bits> selected_seed=<0..19|unknown> serialize_ns=<ns> model_ns=<ns> input_digest=<hex> options_digest=<hex> diagnostic_category=<none|stable_category>
```

默认的 `prune-predicted-ub-overflow=true` 下，只有
`precision=exact && status=overflow` 才让当前
attempt 失败。失败诊断以 `predicted_ub_overflow` 作为机器类别，同时包含字面量
`ub overflow`，从而复用真实 BiSheng fallback 和 Triton UBTuner。一次编译可能产生多条
记录；消费端取最后一条。

### 6.4 编译模式、codegen 和诊断参数

以下参数仍传给真实 `bishengir-compile`，但不进入 UB 模型：

```text
--enable-hfusion-compile=true
--enable-hivm-compile=true 或 --reg-based=true
--enable-triton-kernel-compile=true
--enable-sanitizer=true
--enable-debug-info=true
--enable-print-memory-allocated-size
--enable-memory-display=true
--enable-ms-debug=true
--link-aicore-bitcode=...
--append-bisheng-options=...
--disable-ffts
--mlir-print-ir-after-failure
--mlir-print-stacktrace-on-diagnostic
--bishengir-print-ir-after=...
```

这些字段用于选择总编译路径、链接、设备代码生成或诊断输出。模型只需要由总编译路径
归一化得到的 `enable_triton_kernel_compile=true`，不需要 bitcode 路径、输出路径或
debug print 选项。

## 7. 进程内 C++ 接口

生产请求应包含：**1 份 pre-CVPipelining IR、1 个 target、16 个 UB 相关有效值、
1 个 compiler profile、1 个 input-contract version、1 个 compiler pipeline fingerprint
和 1 个可选 request ID**。

旧 `EffectiveCompileOptions v1` 有 21 个混合字段。当前最小生产 DTO 的 options version
为 `4`。旧调用者只能得到
`unsupported_options_version` blocker，不能按旧默认值继续返回 Exact。

建议公共头文件只暴露稳定 DTO，不暴露 CLI `Options`：

```cpp
namespace cvub {

enum class Precision { Exact, Incomplete };
enum class Status { Success, Overflow, Blocker, InternalError };

enum class CompilerProfile {
  TritonMembaseA2A3,
};

struct UBRelevantCompileOptions {
  int cvPipelineDepth;
  bool enableCVLazyLoading;
  bool enablePreload;
  bool enableCodeMotion;
  bool enableAutoBindSubBlock;
  bool enableUbufSaving;
  bool enableAutoMultiBuffer;
  bool enableHIVMAutoStorageAlign;
  unsigned tileMixVectorLoop;
  unsigned tileMixCubeLoop;
  MultiBufferStrategy localMultiBufferStrategy;
  MultiBufferStrategy mixMultiBufferStrategy;
  bool disableAutoCVWorkSpaceManage;
  bool enableHIVMCrossCoreGSS;
  bool enableHIVMInjectBlockAllSync;
  bool disableAutoInjectBlockSync;
};

struct Request {
  uint32_t apiVersion;
  uint32_t optionsVersion;
  uint32_t inputContractVersion;
  CompilerProfile compilerProfile;
  std::string_view compilerPipelineFingerprint;
  std::string_view target;
  // Generic MLIR 文本，不是文件路径。
  std::string_view beforeCVPipeliningGenericMLIR;
  UBRelevantCompileOptions options;
  std::string_view requestId;
};

struct Result {
  Precision precision;
  Status status;
  std::optional<bool> overflow;
  std::optional<uint64_t> ubPeakBits;
  std::optional<uint64_t> requiredBits;
  uint64_t capacityBits;
  std::optional<uint32_t> selectedSeed;
  uint64_t totalTimeNs;
  // true 表示 evaluate() 通过保守上界证明了 non-overflow，并在
  // PlanMemoryInput/PlanMemory 前返回；此时没有具体内存方案。
  bool decisionOnlyNonOverflow;
  // MarkMultiBuffer 后按独立分配计算的保守 UB 上界。
  std::optional<uint64_t> conservativeUpperBoundBits;
  std::string modelBuildId;
  std::string compilerPipelineFingerprint;
  std::string inputDigest;
  std::string effectiveOptionsDigest;
  std::vector<Diagnostic> diagnostics;
};

Result evaluate(const Request &request) noexcept;

// 仅供 standalone/oracle；prediction pass 禁止调用该入口。
struct DebugModelControls {
  std::optional<uint32_t> fixedPlanMemorySeed;
  std::optional<uint64_t> capacityOverrideBits;
  bool collectStageTimings;
  bool restrictInplaceAsISA;
  bool disableCVPipelining;
  bool disableAlignAllocSize;
  bool disableEnableStrideAlign;
  bool disableInferHIVMDataLayout;
};

Result evaluateForDebug(const Request &request,
                        const DebugModelControls &controls) noexcept;

} // namespace cvub
```

`evaluate()` 不允许异常越过库边界。内部异常转换为 `InternalError`，调用方 fail-open。
`effectiveOptionsDigest` 必须覆盖 target、profile 和 16 个字段，`inputDigest` 必须覆盖实际传入的 Generic
MLIR 文本。`compilerPipelineFingerprint` 至少绑定 BiShengIR commit、目标 pipeline 类型和
从 CVPipelining 到本地 PlanMemory 的 pass manifest；未知 fingerprint 必须 blocker。

`Result` 有两种 exact non-overflow 形态：

- `decisionOnlyNonOverflow=true`：`conservativeUpperBoundBits` 必须存在且不超过
  `capacityBits`；因为没有运行 PlanMemory，`ubPeakBits`、`requiredBits`、`selectedSeed` 和
  具体 function/buffer plan 均不得伪造。
- `decisionOnlyNonOverflow=false`：模型执行了完整 PlanMemory，返回真实 peak、required、seed
  和详细方案。`evaluateForDebug()` 总是使用这条路径；若同时存在
  `conservativeUpperBoundBits`，表示它观察到了提前证明，但为了验证仍继续完成了规划。

这两个字段只描述 non-overflow 结果的来源，不改变 `Exact + Overflow` 才允许剪枝、其他失败
一律 fail-open 的调用合同。

### 7.1 调用方与被调用方责任

调用方（BiShengIR prediction pass）负责：

- 保证 IR 正好来自约定边界；
- 从真实 pipeline option 对象填入 target 和 16 个有效值；
- 校验 compiler profile 的固定事实及 UBInvariant allowlist；
- 提供真实 compiler fingerprint，不提供 capacity/seed 等实验 override；
- 仅对 `Exact + Overflow` 执行 candidate prune。

被调用方（UB model library）负责：

- 校验 API/options/input-contract version、profile、fingerprint、target 和枚举取值域；
- 由 target/profile 推导 capacity 和全部固定 pipeline 事实；
- 确保 16 个字段全部进入 digest，并全部映射到对应模型 pass；
- 遇到未知 target、未知 fingerprint、未覆盖 operation 或未认证 profile 时返回 blocker；
- 使用真实默认 seed retry 和 PlanMemory 规则，返回逐函数峰值及顶层峰值；
- 不信任调用方声称的 `Exact`，Exact 资格只能由模型内部 allowlist 决定。

### 7.2 IR 输入形式：编译器对象进，内存文本进核心库

BiShengIR 执行到 CVPipelining 前时，IR 确实是 MLIR C++ 对象，不应在生产路径
上落盘后再读取。但这不等于模型核心公共头文件必须立即暴露
`mlir::ModuleOp`。当前接口采用两层边界：

```text
BiShengIR prediction pass（依赖 MLIR）
    mlir::ModuleOp
        -> Generic MLIR 内存字符串
        -> cvub::Request
UB model core（不依赖 MLIR）
    std::string_view
        -> ParseGenericIRText
        -> 模型内部 GenericModule
        -> cvub::Result
```

生产 `Request` 中只允许 `beforeCVPipeliningGenericMLIR`，禁止增加
`irPath`/`inputFile`。`std::string_view` 只在同步 `evaluate()` 调用期间有效；模型库
不得将其保存到请求结束之后，不得创建临时文件。

第一阶段不把 `ModuleOp` 直接放进模型核心 API，原因是：

- 现有模型使用普通 C++17 独立构建，核心库目前不依赖 MLIR/LLVM；
- 在公共 DTO 中暴露 `ModuleOp` 会引入 MLIR 头文件、链接依赖和 ABI 锁定；
- 核心库仍需被 macOS 独立 CLI、单元测试和 oracle 工具复用；
- 内存打印和解析虽有开销，但无文件 I/O，足以支持第一层收益实验。

### 7.3 后续零序列化优化接口

当第一阶段计时证明 `ModuleOp -> text -> GenericModule` 占比过高时，再在
BiShengIR 适配层实现直接转换：

```cpp
// 位于 BiShengIR adapter，不放入 MLIR-independent 公共头文件。
cvub::GenericModule buildModelIR(mlir::ModuleOp module);

// 模型核心的内部重载。
cvub::Result evaluate(cvub::GenericModule modelIR,
                      const cvub::RequestMetadata &metadata) noexcept;
```

该 adapter 只读遍历 operation/region/block/value/type/attribute，建立模型自己拥有的
`GenericModule`。不允许模型 pass 直接在真实 `ModuleOp` 上运行；如果未来必须
用 MLIR pass 来模拟，应先 `clone()` 并在副本上运行。

### 7.4 Standalone CLI 设计

CLI 是开发、corpus 回归和 differential oracle 入口，不是 autotune 热路径。
保留现有命令形式，并增加标准输入支持：

```text
bishengir-ub-overflow-model \
  --before-cvpipelining-ir=INPUT.mlir \
  --target=Ascend910_9382 \
  --format=json \
  [UBRelevantCompileOptions] \
  [DebugModelControls]

bishengir-ub-overflow-model \
  --before-cvpipelining-ir=- \
  --target=Ascend910_9382 \
  --format=json
```

`--before-cvpipelining-ir=-` 表示从 stdin 读取 Generic MLIR。CLI 将文件或 stdin
一次性读入字符串，然后调用同一个 `cvub::evaluate()`，不得复制一套模型
pipeline。文件路径、`--debug-dir`、固定 seed 和 capacity override 只存在于 CLI
适配层，不进入生产 `Request`。

`bishengir-compile` 内的 prediction pass **禁止** `fork/exec` 该 CLI。否则子进程启动、
文件 I/O 和 JSON 解析会污染本实验要测量的轻量模型时间。

## 8. BiShengIR Pass 接入

应新增 module pass，例如：

```cpp
createUBOverflowPredictionPass(const UBOverflowPredictionOptions &options)
```

并放在真实 `createCVPipeliningPass` 之前。pipeline builder 只负责插入 pass；模型必须在
pass 的 `runOnOperation()` 中调用，因为此时才有正在编译的 `ModuleOp`。

调用步骤：

1. 先校验当前编译满足 `CompilerProfile::TritonMembaseA2A3` 的固定事实和 Exact
   allowlist；
2. 从真实 `CVPipeliningOptions/HIVMPipelineOptions` **逐字段**构造
   `UBRelevantCompileOptions`，不得使用 DTO 默认值；
3. 使用固定 Generic MLIR printing flags 将只读 `ModuleOp` 打印到内存字符串，或转换为
   模型只读 IR；
4. 调用 `cvub::evaluate()`；
5. exact overflow 时发出稳定诊断并 `signalPassFailure()`；
6. 其他结果不修改 module，继续真实 CVPipelining。

调用方应先构造真实 CVPipelining options，再由同一个对象构造模型请求：

```cpp
CVPipeliningOptions cvOptions;
cvOptions.enableSkewMode = hivmOptions.enablePreload;
// 当前真实值仍为 pass default：-1 / false。

UBRelevantCompileOptions ubOptions;
ubOptions.cvPipelineDepth = cvOptions.setDepthInUnrollMode;
ubOptions.enableCVLazyLoading = cvOptions.enableLazyLoading;
ubOptions.enablePreload = cvOptions.enableSkewMode;
ubOptions.enableCodeMotion = hivmOptions.enableCodeMotion;
ubOptions.enableAutoBindSubBlock = hivmOptions.enableAutoBindSubBlock;
ubOptions.enableUbufSaving = hivmOptions.enableUbufSaving;
ubOptions.enableAutoMultiBuffer = hivmOptions.enableAutoMultiBuffer;
ubOptions.enableHIVMAutoStorageAlign =
    hivmOptions.enableHIVMAutoStorageAlign;
ubOptions.tileMixVectorLoop = hivmOptions.tileMixVectorLoop;
ubOptions.tileMixCubeLoop = hivmOptions.tileMixCubeLoop;
ubOptions.localMultiBufferStrategy =
    convert(hivmOptions.limitAutoMultiBufferOfLocalBuffer);
ubOptions.mixMultiBufferStrategy =
    convert(hivmOptions.limitAutoMultiBufferBuffer);
ubOptions.enableHIVMCrossCoreGSS = hivmOptions.enableHIVMCrossCoreGSS;
ubOptions.enableHIVMInjectBlockAllSync =
    hivmOptions.enableHIVMInjectBlockAllSync;
ubOptions.disableAutoInjectBlockSync =
    hivmOptions.disableAutoInjectBlockSync;

pm.addPass(createUBOverflowPredictionPass(profile, target, ubOptions));
pm.nest<func::FuncOp>().addPass(createCVPipeliningPass(cvOptions));
```

禁止 prediction pass 和真实 CVPipelining 分别构造两份带默认值的 options。

第一阶段的 adapter 实现：

```cpp
static cvub::Result evaluateCurrentModule(
    mlir::ModuleOp module, const PredictionConfig &config) {
  std::string ir;
  llvm::raw_string_ostream os(ir);
  mlir::OpPrintingFlags flags;
  flags.printGenericOpForm();
  module.print(os, flags);
  os.flush();

  cvub::Request request;
  request.apiVersion = cvub::kInProcessAPIVersion;
  request.optionsVersion = cvub::kUBRelevantCompileOptionsVersion;
  request.inputContractVersion = config.inputContractVersion;
  request.compilerProfile = config.profile;
  request.compilerPipelineFingerprint = config.pipelineFingerprint;
  request.target = config.target;
  request.beforeCVPipeliningGenericMLIR = ir;
  request.options = config.ubOptions;
  request.requestId = config.requestId;
  return cvub::evaluate(request);
}
```

必须固定与 input contract 绑定的 printing flags。除 Generic form 外，还要保证不启用
`elideLargeElementsAttrs`、debug-info 注入或本地化打印。输入 digest 应对 `ir` 的实际
字节计算。

若只需最小打印示例，核心代码是：

```cpp
std::string ir;
llvm::raw_string_ostream os(ir);
mlir::OpPrintingFlags flags;
flags.printGenericOpForm();
module.print(os, flags);
os.flush();
```

prediction pass 的插入点必须紧邻 `createCVPipeliningPass`，中间不能再有未纳入 input
contract 的 pass。测试必须同时 dump prediction pass 实际传给 API 的文本和真实 compiler
在 `before CVPipelining` 的 snapshot，并校验 digest 相同。

建议 overflow 诊断至少包含：

```text
error category: predicted_ub_overflow
request/candidate id
required_bits
capacity_bits
model build id
effective options digest
```

Triton autotune 应依据稳定错误类别淘汰 candidate，不要依赖完整的人类可读 message。

### 8.1 与 BiShengIR 内部 retry 的关系

当前 `RetriablePassManager` 会把包含文本 `"ub overflow"` 的真实 PlanMemory 失败识别为
可恢复错误，并依次尝试：

```text
enable-code-motion=false
enable-auto-multi-buffer=false
```

prediction pass 的稳定机器类别仍是 `predicted_ub_overflow`，但人类可读诊断必须同时包含
真实合同 `ub overflow, requires ... bits while ... bits available`。这样无需在模型里重新
设计 fallback 状态机：prediction 只判断当前 effective options；现有
`RetriablePassManager` 修改真实 compiler config、克隆原始 module、重建 pipeline，然后在
下一次 attempt 以新的 resolved options 再次调用 prediction。执行顺序与真实 PlanMemory
失败时一致：

1. 当前 options 预测 overflow；
2. `enable-code-motion=false` 后从原始 module 重跑整条 BiSheng pipeline；
3. 仍 overflow 时再置 `enable-auto-multi-buffer=false` 并重跑；
4. 若 prediction 成功则继续真实 CVPipelining/PlanMemory；若仍失败才返回 autotune。

显式开启机器结果输出后，一条编译命令可能输出多条
`BISHENGIR_UB_MODEL_RESULT`，每条描述各自 attempt；最终结果取最后一条。这个设计既保留
稳定机器类别，也不改变 BiSheng 已有恢复语义。

## 9. 计时字段

每个 candidate 至少记录：

```text
prefix_compile_ns       原始输入到 pre-CVPipelining
model_serialize_ns      ModuleOp 到模型输入
model_parse_ns          Generic IR parser
model_cvpipelining_ns
model_post_pipeline_ns
model_plan_memory_ns
model_total_ns
continued_compile_ns    non-overflow/fallback 的真实后续编译
saved_compile_ns        overflow candidate 估计节省的后续时间
candidate_total_ns
```

正式收益使用整个 autotune wall time，同时保留阶段时间解释收益来源。

## 10. 一致性与缓存

缓存键至少包含：

```text
model build id
+ input contract/version
+ pre-CVPipelining IR digest
+ compiler profile/fingerprint
+ target
+ UBRelevantCompileOptions digest
```

Shadow、Prune 和 baseline 必须使用同一 candidate 集合、相同 target、相同有效编译参数和
相同 PlanMemory seed retry 策略。

### 10.1 Exact 认证门槛

不能通过“参数已经传全”直接声明峰值完全一致。每个受支持的 compiler fingerprint 必须
运行 differential oracle，并至少比较：

```text
pre-CVPipelining 输入 digest
最终进入本地 PlanMemory 的 buffer 集合和物理 extent
multi-buffer 数量
每个 seed 的 required_bits（或真实 retry 实际访问的 seed 序列）
selected_seed
ub_peak_bits
required_bits
overflow
```

认证矩阵至少覆盖：

```text
enable_auto_multi_buffer = false/true
local_multi_buffer_strategy = no-limit/no-l0c
tile_mix_vector_loop = 2/4/8
tile_mix_cube_loop = 2/4/8
enable_ubuf_saving = false/true
enable_code_motion = false/true
enable_preload = false/true
enable_auto_bind_sub_block = false/true
enable_hivm_auto_storage_align = false/true
cross-core/global-workspace 选项的 UBInvariant 对照
```

只有 oracle 全字段一致的 target + compiler fingerprint + option profile 才加入 Exact allowlist。
新增/重排从 CVPipelining 到 PlanMemory 的 pass、修改 pass 默认值或修改 PlanMemory 算法时，
必须改变 fingerprint 并重新认证。在未认证 build 上模型只能 shadow/fail-open。

## 11. v1 验收标准

- 模型作为库链接到 `bishengir-compile`，不为每个 candidate 启动子进程；
- prediction pass 位于真实 CVPipelining 前；
- 模型不修改真实 ModuleOp；
- 参数来自真实归一化 pipeline options，不从 autotune 原始字典重新猜测；
- 将混合的 `EffectiveCompileOptions` 重构为只含 16 个字段的
  `UBRelevantCompileOptions v4`；
- 请求绑定 input-contract version 和 compiler pipeline fingerprint；
- 覆盖第 4 节列出的 5 个 autotune 独立 UB 维度；
- 覆盖第 5 节全部固定 UB 相关编译上下文；
- 处理 `enable_auto_bind_sub_block` 和 storage-align 语义缺口；
- exact overflow 淘汰 candidate，blocker/error 一律 fail-open；
- prediction overflow 诊断不会触发 BiShengIR 自身的 `"ub overflow"` retry policy；
- overflow 有稳定错误类别和结构化数值；
- non-overflow candidate 继续执行到真实 PlanMemory，并可与模型结果做 differential check；
- Exact allowlist 通过第 10.1 节的逐字段 differential oracle；
- 记录模型内部时间和 candidate/autotune 端到端 wall time；
- 现有独立 CLI 继续作为开发和 oracle 工具，不作为正式 autotune 热路径。

## 12. AscendNPU-IR 可实现性与落地位置

### 12.1 可实现性结论

**第一阶段可以在当前 AscendNPU-IR 中实现，不需要 NPU 真机，也不需要
临时 IR 文件。** 理由是：

- `HIVMPipelines.cpp` 已在 `createCVPipeliningPass()` 前持有完整 pass manager 和
  归一化后的 `HIVMPipelineOptions`；
- prediction module pass 的 `runOnOperation()` 可直接获得当前 `ModuleOp`；
- 当前 `src/api.cpp` 已经通过 `ParseGenericIRText(std::string_view, ...)` 接受内存
  IR，没有把文件 I/O 写死在核心 API 中；
- BiShengIR 自身已链接 MLIR IR/Pass 库，适配层打印 `ModuleOp` 不需要新的
  运行时依赖；
- macOS 实验只需走到模型判定，或对 non-overflow candidate 继续到本地
  PlanMemory 对照，不要求设备 codegen/运行。

`UBRelevantCompileOptions v4`、进程内 prediction pass 和 shadow/prune 开关已经落地。
剩余的正确性前置是按第 10.1 节完成目标 compiler fingerprint 的 oracle 认证；认证前
正式实验应以 shadow 为主，prune 只用于受控对照。

### 12.2 已实现的源码和 CMake 结构

```text
ub_overflow_model_cpp/
  CMakeLists.txt                                  新增模型核心 target
  include/ub_overflow_model/api.hpp               升级为 options v4
  src/api.cpp                                     实现 v4 校验和 evaluate

bishengir/lib/Dialect/HIVM/Pipelines/
  UBOverflowPrediction.h/.cpp                     ModuleOp -> memory -> evaluate
  HIVMPipelines.cpp                               在 CVPipelining 前插入 pass
  CMakeLists.txt                                  编译 pass 并链接模型库

bishengir/include/bishengir/Tools/bishengir-compile/
  Options.td                                      声明 shadow/prune CLI 开关

third_party/ascend/backend/compiler.py            Triton NPUOptions 和命令转发
```

构建从源码生成 CMake target，没有在 BiShengIR CMake 中硬编码链接
`output/lib/libub_overflow_model.a`：

```cmake
# ub_overflow_model_cpp/CMakeLists.txt
set(LLVM_REQUIRES_EH ON)
add_bishengir_library(BiShengIRUBOverflowModel src/api.cpp)
unset(LLVM_REQUIRES_EH)
target_compile_features(BiShengIRUBOverflowModel PUBLIC cxx_std_17)
target_include_directories(BiShengIRUBOverflowModel PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(BiShengIRUBOverflowModel PROPERTIES
  POSITION_INDEPENDENT_CODE ON)

# bishengir/lib/Dialect/HIVM/Pipelines/CMakeLists.txt
# UBOverflowPrediction.cpp 属于 BiShengIRHIVMPipelines，
# BiShengIRUBOverflowModel 属于该 target 的 LINK_LIBS。
```

顶层 CMake 在进入 `bishengir/lib` 前执行 `add_subdirectory(ub_overflow_model_cpp)`，使
pipelines target 可以看到模型 target。模型内部目前使用异常，因此只对模型 target 开启
`LLVM_REQUIRES_EH`；公共 `evaluate()` 边界仍为 `noexcept`。

### 12.3 Triton autotune 调用方式

Triton-Ascend 的 `NPUOptions` 新增：

```text
enable_ub_overflow_prediction = true
prune_predicted_ub_overflow = true
```

它们会进入编译缓存键，并且只在 A2/A3 的 `bishengir-compile` 路径转发。默认配置无需重复
声明；需要逐 candidate 显式记录时可写为：

```python
triton.Config(
    {"enable_ub_overflow_prediction": True,
     "prune_predicted_ub_overflow": True,
     # 其余 candidate 参数
    },
    num_warps=..., num_stages=...,
)
```

`prune_predicted_ub_overflow=True` 但 prediction 未开启会在 option 解析阶段报错。显式设置
`BISHENGIR_UB_MODEL_EMIT_RESULT=1` 时，成功候选的 `BISHENGIR_UB_MODEL_RESULT` 会解析到
编译 metadata 的 `ub_model_result` 字段。BiSheng 内部
fallback 仍无法恢复的 overflow 候选由 `predicted_ub_overflow` 编译诊断进入
`MLIRCompilationError`；普通 autotuner 会淘汰该候选，启用 Ascend UBTuner 时还会因诊断中
保留的 `ub overflow` 进入其既有替代配置搜索。

### 12.4 实施顺序

1. 先完成 `UBRelevantCompileOptions v4` 和 CLI stdin 回归，确认文件、stdin、
   in-process text 三种入口对同一 IR 返回相同 digest/结果；
2. 将模型核心加入 AscendNPU-IR CMake，保持该 target 不依赖 MLIR；
3. 实现 `UBOverflowPrediction` module pass，暂时只记录结果，不中止编译；
4. 在 `HIVMPipelines.cpp` 中紧邻 CVPipelining 前插入，完成 IR digest 和 options
   digest 对齐；
5. 对 non-overflow candidate 继续到真实 PlanMemory，收集 differential oracle；
6. 只对通过 Exact 认证的 profile 开启 prune，其余继续 fail-open。

### 12.5 额外验收项

- prediction pass 路径不出现文件打开、临时目录或子进程调用；
- `std::string_view` 不逃逸同步 `evaluate()` 生命周期；
- 文件 CLI、stdin CLI 和 in-process API 的 IR/options digest 及模型结果一致；
- 同一 pass 内打印的 Generic MLIR 与 `before CVPipelining` snapshot digest 一致；
- macOS 可构建模型库及 prediction pass，并在不使用 NPU 运行时时完成到
  PlanMemory 的对照测试；
- 单独统计 `model_serialize_ns`，为是否实施第 7.3 节直接 adapter 提供依据。
