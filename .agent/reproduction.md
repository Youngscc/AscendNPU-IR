# Reproduction Notes

## macOS 能做什么

macOS 上可以复现和调试 PlanMemory overflow，只要仓库已经构建出：

```text
build/bin/bishengir-opt
```

不需要：

- CANN
- `hivmc`
- 昇腾卡

## 仓库自带 overflow 复现

从仓库根目录执行：

```bash
build/bin/bishengir-opt \
  bishengir/test/Dialect/HIVM/plan-memory.mlir \
  --split-input-file \
  --hivm-plan-memory='mem-plan-mode=local-mem-plan'
```

预期会看到类似：

```text
ub overflow, requires 2560000 bits while 1572864 bits available
```

这是预期失败，因为测试文件里包含故意构造的 overflow case。

## 对用户自己的 ttadapter 运行

如果用户的 `.ttadapter` 已经是 HIVM/memref 层级，可以尝试：

```bash
build/bin/bishengir-opt input.ttadapter \
  --hivm-plan-memory='mem-plan-mode=local-mem-plan enable-memory-display=true' \
  --mlir-print-ir-after-failure
```

如果失败原因是 parser 或 dialect 相关，说明 `.ttadapter` 还需要经过前面的 pipeline 或需要加对应 dialect/allow-unregistered 参数。

## Pass pipeline 写法

需要显式嵌套 func pass 时可以写：

```bash
build/bin/bishengir-opt input.mlir \
  --pass-pipeline='builtin.module(func.func(hivm-plan-memory{mem-plan-mode=local-mem-plan enable-memory-display=true}))'
```

## Memory display JSON

加：

```text
enable-memory-display=true
```

可能生成：

```text
memory_info.json
memory_info_aic.json
memory_info_aiv.json
```

注意：

- 文件生成在当前工作目录。
- 多次运行可能覆盖。
- 这个 JSON 是调试信息来源，可用于验证估算器结果。

## 复现时避免混入后端环境问题

如果目标只是 PlanMemory，不要优先使用完整 `bishengir-compile` 跑到 `hivmc`。

优先使用：

```text
bishengir-opt + hivm-plan-memory
```

这样可以绕开 CANN、hivmc、真实卡环境，把问题限制在本项目自己的静态分析阶段。
