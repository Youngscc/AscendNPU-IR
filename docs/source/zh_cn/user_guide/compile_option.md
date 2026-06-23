# 编译选项

## bishengir-compile 编译选项

### BiShengIR 功能控制选项

| 选项名 | 描述 | 类型 | 默认值 |
|--------|------|------|--------|
| --enable-triton-kernel-compile | 启用 Triton 内核编译功能 | bool | false |
| --enable-torch-compile | 启用 Torch-MLIR 编译功能 | bool | false（仅在定义 `BISHENGIR_ENABLE_TORCH_CONVERSIONS` 编译宏时提供该选项） |
| --enable-hivm-compile | 启用 BiShengHIR HIVM 编译功能 | bool | true |
| --enable-hfusion-compile | 启用 BiShengHIR HFusion 编译功能 | bool | false |
| --enable-symbol-analysis | 启用符号分析功能 | bool | false |
| --enable-multi-kernel | 关闭时计算图必须融合为单个内核；开启时支持外提生成多个内核 | bool | false |
| --enable-manage-host-resources | 启用主机函数的资源管理功能 | bool | false |
| --ensure-no-implicit-broadcast | 是否确保不存在隐式广播语义。若存在动态维度到动态维度的广播，将触发运行时错误 | bool | false（仅在定义 `BISHENGIR_ENABLE_TORCH_CONVERSIONS` 编译宏时提供该选项） |
| --disable-auto-inject-block-sync | 禁用 `injectBlockSync` Pass 自动生成块同步 `wait/set` 指令 | bool | false |
| --enable-hivm-graph-sync-solver | 使用 HIVM 计算图同步求解器替代同步注入机制 | bool | false |
| --disable-auto-cv-work-space-manage | 需与 `disableAutoInjectBlockSync` 选项搭配使用 | bool | false |
| --disable-hivm-auto-inject-sync | 禁用核内自动注入同步操作 | bool | false |
| --disable-hivm-tensor-compile | 禁用 BiShengHIR HIVM 张量编译功能 | bool | false |

### BiShengIR 通用优化选项

| 选项名 | 描述 | 类型 | 默认值 |
|--------|------|------|--------|
| --enable-auto-multi-buffer | 启用自动多缓冲优化 | bool | false |
| --limit-auto-multi-buffer-only-for-local-buffer | 开启自动多缓冲后，限定该优化仅对本地缓冲区生效 | bool | true |
| --enable-tuning-mode | 启用调优模式，内存规划失败时不会重试多次编译 | bool | false |
| --block-dim=\<uint> | 指定使用的块数量 | unsigned | 1 |

### BiShengIR HFusion 优化选项

| 选项名 | 描述 | 类型 | 默认值 |
|--------|------|------|--------|
| --enable-deterministic-computing | 开启时计算结果具备确定性；关闭时将启用额外优化以提升性能（例如将规约操作绑定到多个核心执行），但计算结果将不具备确定性 | bool | true |
| --enable-ops-reorder | 在优化流水线中启用算子重排优化 | bool | true |
| --hfusion-max-horizontal-fusion-size=\<int> | 水平融合的最大尝试次数（默认无限制） | int32_t | -1 |
| --hfusion-max-buffer-count-tuning=\<long> | HFusion 自动调度中的最大缓冲区数量调优阈值 | int64_t | 0 |
| --cube-tiling-tuning=\<long> | HFusion 自动调度中的 Cube 分块尺寸调优参数 | list int64_t | "" |
| --enable-hfusion-count-buffer-dma-opt | 开启后，DMA 操作使用的缓冲区不会被向量运算复用 | bool | false |

### BiShengIR 目标平台选项

编译选项格式为 `--target=Ascend<Name>`，用于指定 MLIR 编译的目标硬件平台。其中 `<Name>` 为占位符，需根据实际 AI 处理器型号，通过对应的查询命令获取具体取值。

AI 处理器型号及对应查询方式如下：

**方式一：通过 `npu-smi info` 命令查询**

**适用产品**：

- Atlas A2 训练系列产品 / Atlas A2 推理系列产品
- Atlas 200I/500 A2 推理产品
- Atlas 推理系列产品
- Atlas 训练系列产品

在安装 AI 处理器的服务器上执行该命令，查询得到 `<Name>` 的对应取值，完整配置值为 `Ascend<Name>`。示例：若 `<Name>` 取值为 `xxx`，则配置值为 `Ascendxxx`。

**方式二：通过 `npu-smi info -t board -i id -c chip_id` 命令查询**

**适用产品**：

- Atlas 350 加速卡
- Atlas A3 训练系列产品 / Atlas A3 推理系列产品

在安装 AI 处理器的服务器上执行该命令，获取 **Chip Name** 和 **NPU Name** 信息，实际配置值为 `<Chip Name>_<NPU Name>`。示例：若 Chip Name 为 `Ascendxxx`、NPU Name 为 `yyy`，则配置值为 `Ascendxxx_yyy`。

命令参数说明：

- `id`：设备 ID，通过 `npu-smi info -l` 命令查询得到的 NPU ID 即为设备 ID。
- `chip_id`：芯片 ID，通过 `npu-smi info -m` 命令查询得到的 Chip ID 即为芯片 ID。
