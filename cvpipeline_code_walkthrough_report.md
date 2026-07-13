# HIVM CVPipeline 代码阅读报告

## 范围和文件

本报告解释 HIVM 中的 **CVPipelining** pass，也就是命令行里的：

```text
bishengir-opt -cv-pipelining
```

它的核心目标是：在 MIX kernel 中，把同一个 `scf.for` 里的 Cube 计算和 Vector 计算切成多个
Work Item，再通过 multi-buffer 让不同 Work Item 在不同 stage 上流水并行。

本文主要看这些文件：

```text
bishengir/lib/Dialect/HIVM/Transforms/CVPipelining.cpp
bishengir/include/bishengir/Dialect/HIVM/Transforms/Passes.td
bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp
bishengir/lib/Dialect/HIVM/Transforms/MarkMultiBuffer.cpp
bishengir/lib/Dialect/HIVM/Transforms/CreatePreload.cpp
bishengir/lib/Dialect/HIVM/Transforms/GraphSyncSolver/SyncSolverIRTranslator.cpp
bishengir/test/Dialect/HIVM/cv-pipelining.mlir
bishengir/test/Dialect/HIVM/cv-pipelining-preload.mlir
bishengir/test/Dialect/HIVM/cv-pipelining-no-multibuffer-attr.mlir
docs/source/zh_cn/developer_guide/features/CVPipeline/CVPipelining.md
```

为了避免混淆，本文里：

- **CVPipeline / CVPipelining** 指 `CVPipelining.cpp` 里的 pass。
- **普通模式 / unroll mode** 指默认 `-cv-pipelining`。
- **preload 模式 / skew mode** 指 `-cv-pipelining="enable-skew-mode"`，在完整 compile
  pipeline 中由 `--enable-preload` 触发。
- **Work Item** 指 pass 内部抽象出来的一段同 core 类型的工作，后面会变成一个小 loop 或一个 scope。

这个 pass 和 PlanMemory 的关系很密切，但它不是 PlanMemory：

- CVPipeline 在 tensor/pre-bufferization 阶段运行，主要改控制流和中间 tensor/workspace 形状。
- global workspace PlanMemory 在它后面，对 `memref_ext.alloc_workspace` 做 offset 规划。
- local PlanMemory 在 bufferization 之后，对 UB/CBUF/CC 的 `memref.alloc` 做 local offset 规划。

## 一句话结论

CVPipeline 做的事情可以概括成：

```text
原来：
  for i:
    Cube(i)
    Vector(i)
    Cube(i)
    Vector(i)

变成：
  for outer i step S * multibuffer:
    for stage in 0..multibuffer:
      Cube(outer + stage * S)
    for stage in 0..multibuffer:
      Vector(outer + stage * S)
    for stage in 0..multibuffer:
      Cube(outer + stage * S)
    for stage in 0..multibuffer:
      Vector(outer + stage * S)
```

如果开启 preload/skew，则不是生成一组内部 `scf.for`，而是在原 loop 内生成一串
`scope.scope`，并打上 `hivm.preload_num` / `hivm.max_preload_num`，后面由
`create-preload` 继续展开。

## IR 语法速读

可以把 MLIR 先看成一种带 SSA 名字和类型的数据流文本：

```mlir
%result = dialect.op_name operands {attrs} : operand_types -> result_types
```

HIVM 结构化 op 常见形式是：

```mlir
%out = hivm.hir.vadd
  ins(%a, %b : tensor<16x16xf16>, tensor<16x16xf16>)
  outs(%dest : tensor<16x16xf16>)
  -> tensor<16x16xf16>
```

CVPipeline 重点关心这些东西：

| IR 元素 | 在 CVPipeline 中的意义 |
| - | - |
| `scf.for` | 可能被 pipelining 的目标 loop |
| `hivm.hir.mmadL1` | 典型 Cube op |
| `hivm.hir.vadd/vexp/vreduce/vcast/...` | 典型 Vector op |
| `hivm.hir.fixpipe` | 常见 Cube 到 workspace/GM 的边界，也常作为 separator |
| `hivm.hir.store` | 常见 Vector 到 workspace/GM 的边界，也常作为 separator |
| `hivm.hir.load` | 从 workspace/GM/中间 tensor 重新加载，常被拉入消费者 Work Item |
| `memref_ext.alloc_workspace` | CV 中间结果在 GM workspace 上的承载对象 |
| `annotation.mark {hivm.multi_buffer = N}` | 告诉 CVPipeline 需要 N 槽 multi-buffer |
| `bufferization.to_tensor` | 把 workspace/local memref 包成 tensor，供 pre-bufferization HIVM op 使用 |
| `tensor.extract_slice` / `tensor.insert_slice` | multi-buffer 后按 stage 切片和写回 |
| `memref.subview` / `memref.collapse_shape` | multi-buffer workspace 后按 stage 切片和降 rank |

常见 core 类型：

| 名称 | 含义 |
| - | - |
| `TCoreType::CUBE` | Cube 核心执行 |
| `TCoreType::VECTOR` | Vector 核心执行 |
| `TCoreType::CUBE_OR_VECTOR` | 暂时还无法区分，需要推断或放弃 |

## pass 注册和选项

Pass 在 `Passes.td` 中注册：

```tablegen
def CVPipelining : Pass<"cv-pipelining", "func::FuncOp"> {
  let summary = "Cube and vector core pipelining for multi-buffer'ed mix-cv ops";
  let constructor = "mlir::hivm::createCVPipeliningPass()";
  let options = [
      Option<"enableSkewMode", "enable-skew-mode", "bool",
            /*default=*/"false",
            "Enable skew-mode cv-pipelining instead of the default unroll-mode.">,
      Option<"setDepthInUnrollMode", "set-depth-in-unroll-mode", "int",
            /*default=*/"-1",
            "Specify pipeline depth for cv-pipelining.">,
      Option<"enableLazyLoading", "enable-lazy-loading", "bool",
            /*default=*/"false",
            "Allow load ops to be cloned into multiple work items ...">];
}
```

实际构造函数在 `CVPipelining.cpp` 末尾：

```cpp
std::unique_ptr<Pass>
hivm::createCVPipeliningPass(const CVPipeliningOptions &options) {
  return std::make_unique<CVPipeliningPass>(options);
}
```

几个选项含义：

| 选项 | 默认 | 含义 |
| - | - | - |
| `enable-skew-mode` | false | 走 preload/skew 路径，生成 `scope.scope` 和 preload 属性 |
| `set-depth-in-unroll-mode` | -1 | 强制指定普通模式的 pipeline depth；`0/1` 时直接不做 |
| `enable-lazy-loading` | false | 允许某些 `hivm.hir.load` 被 clone 到多个 Work Item，减少中间 tensor 扩展 |

注意：`docs/source/en/developer_guide/passes/HIVMPasses.md` 里的 `-cv-pipelining`
选项说明可能落后于源码，源码当前以上面的 `Passes.td` 为准。

## 在完整 pipeline 里的位置

CVPipeline 在 `hivmPreBufferizationOptimizationPipeline` 中运行。核心片段在
`bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp`：

```cpp
if (!hivmPipelineOptions.disableAutoCVWorkSpaceManage &&
    !hivmPipelineOptions.disableAutoCVMarkMultiBuffer) {
  MarkMultiBufferOptions multiBufferOptions;
  multiBufferOptions.workspaceMultiBufferNum =
      hivmPipelineOptions.setWorkspaceMultibuffer;
  pm.addNestedPass<func::FuncOp>(
      createMarkMultiBufferPass(multiBufferOptions));
}

pm.addPass(bishengir::createExtendedCanonicalizerPass());
pm.nest<func::FuncOp>().addPass(createInlineOTFBroadcastPass());

if (!hivmPipelineOptions.disableAutoCVWorkSpaceManage &&
    !hivmPipelineOptions.disableCVPipelining) {
  CVPipeliningOptions pipelineOptions;
  pipelineOptions.enableSkewMode = hivmPipelineOptions.enablePreload;
  pm.nest<func::FuncOp>().addPass(createCVPipeliningPass(pipelineOptions));
}
```

前后关系可以写成：

```text
NormalizeMatmul
InlineFixpipe
InsertLoadStoreForMixCV
InsertWorkSpaceForMixCV
BindWorkSpaceArg
InferFuncCoreType
MarkMultiBuffer
InlineOTFBroadcast
CVPipelining
TileCubeVectorLoop
GLOBAL_WORKSPACE_PLAN PlanMemory
Cross-core sync
SplitMixKernel
Bufferization
...
LOCAL_MEM_PLAN PlanMemory
HIVMLowerToLoops
CreatePreload, if enablePreload
Normal sync
EnableMultiBuffer
```

从这个位置能看出几个关键事实：

- CVPipeline 依赖前面的 `InsertWorkSpaceForMixCV`，否则很多 C/V 之间的边界没有 workspace
  承载。
- CVPipeline 依赖前面的 `MarkMultiBuffer`，否则 `hivm.multi_buffer` 不一定存在，loop 可能不满足
  pipeline depth 要求。
- global workspace PlanMemory 在 CVPipeline 后面，因此 CVPipeline 扩出来的
  `memref_ext.alloc_workspace<2x...>` 会继续参与 workspace offset 规划。
- local PlanMemory 在更后面，因此 CVPipeline 会间接改变后面 UB/CBUF/CC allocation 结构和生命周期。

## 官方文档里的算法模型

`docs/source/zh_cn/developer_guide/features/CVPipeline/CVPipelining.md` 给了一个最简模型：

Before:

```mlir
scf.for 0 to N step S {
    %c = Cube() : tensor<16x16xf32>
    %v = Vector(%c) : tensor<16x16xf32>
    %c1 = Cube(%v) : tensor<16x16xf32>
    %v1 = Vector(%c1) : tensor<16x16xf32>
}
```

After:

```mlir
scf.for 0 to N step 3*S {
    %c = scf.for 0 to 3 -> tensor<3x16x16xf32> {
        Cube()
        tensor.insert_slice
    } {cube_loop}

    %v = scf.for 0 to 3 -> tensor<3x16x16xf32> {
        %c_slice = extract_slice %c
        Vector(%c_slice)
        tensor.insert_slice
    } {vector_loop}

    %c1 = scf.for 0 to 3 -> tensor<3x16x16xf32> {
        %v_slice = extract_slice %v
        Cube(%v_slice)
        tensor.insert_slice
    } {cube_loop}

    %v1 = scf.for 0 to 3 -> tensor<16x16xf32> {
        %c_slice = extract_slice %c1
        Vector(%c_slice)
    } {vector_loop}
}
```

源码里的普通模式基本就是这个模型，只是实际 depth 来自 `hivm.multi_buffer` 或
`set-depth-in-unroll-mode`，并且会处理 workspace、iter_args、atomic、region op 等很多边界。

## 关键数据结构

### `WorkItem`

`CVPipelining.cpp` 里的 `WorkItem` 是这个 pass 的核心抽象：

```cpp
struct WorkItem {
  SmallVector<std::pair<Value, Value>> localOutputs;
  DenseSet<Operation *> ops;
  SmallVector<std::pair<Value, unsigned>> yieldedOutputs;
  TCoreType core;
  scf::ForOp forOp;
  Value reconstructedIV;
  scope::ScopeOp scopeOp;
  unsigned multibuffer;
  SmallVector<Operation *> workspaceOutputs;
  IRMapping irMap;
};
```

逐项解释：

| 字段 | 含义 |
| - | - |
| `ops` | 这个 WorkItem 要 clone 的原 loop 内 op |
| `core` | 只能是 CUBE 或 VECTOR |
| `localOutputs` | 被后续 WorkItem 使用的 tensor，需要扩成 `N x original_shape` |
| `yieldedOutputs` | 原 loop 的 yield 结果，需要映射成新 loop 的 iter/result |
| `workspaceOutputs` | `fixpipe/store` 写入 workspace 的输出，需要改写成写 multi-buffer slot |
| `forOp` | 普通模式下，这个 WorkItem 对应的新内部 `scf.for` |
| `scopeOp` | preload/skew 模式下，这个 WorkItem 对应的新 `scope.scope` |
| `reconstructedIV` | 内部 stage loop 中重建出来的原始 `%i` |
| `irMap` | clone op 时的 SSA value 映射 |

可以把 WorkItem 想成“一个可单独搬走的 CUBE 段或 VECTOR 段”。

### `CVPipelineImpl`

`CVPipelineImpl` 持有整个 loop pipelining 的状态：

```cpp
struct CVPipelineImpl {
  scf::ForOp pipelineLoop;
  scf::ForOp newLoop;
  int numMultibuffer;
  bool enableSkewMode;
  bool enableLazyLoading;

  DenseSet<Operation *> toBePipelined;
  SmallVector<Operation *> separators;
  DenseMap<Operation *, DenseSet<Operation *>> dependenceMap;
  DenseMap<Operation *, DenseSet<Operation *>> loopCarriedDependenceMap;
  SmallVector<std::shared_ptr<WorkItem>> worklist;
  DenseMap<Operation *, SmallVector<WorkItem *>> opToWorkItemMap;
  DenseMap<Value, Value> expandedTensorMap;
  DenseMap<AllocWorkspaceOp, WorkspaceAllocParams> workspaceAllocs_;
  DenseMap<Value, Value> expandedWorkspaceMap_;
};
```

几个重要成员：

| 成员 | 含义 |
| - | - |
| `pipelineLoop` | 当前准备改写的原始 loop |
| `newLoop` | 普通模式下新建的外层 loop |
| `numMultibuffer` | pipeline depth，也是 multi-buffer 槽数 |
| `toBePipelined` | 原 loop 中所有 core op，成功后必须全部被 WorkItem 吃掉 |
| `separators` | `fixpipe/store/cross-core copy`，用来建立依赖边界 |
| `dependenceMap` | `op -> 它依赖哪些 separator` |
| `loopCarriedDependenceMap` | 跨迭代依赖，避免非法重排 |
| `worklist` | 切好的 WorkItem 列表 |
| `opToWorkItemMap` | 某个 op 属于哪些 WorkItem |
| `workspaceAllocs_` | 需要扩展的 workspace alloc 及其 mark/to_tensor |

## 顶层执行流程

`CVPipelineImpl::run()` 是主线：

```cpp
LogicalResult CVPipelineImpl::run() {
  collectAtomicEffects();
  if (createWorkItems().failed()) {
    revert();
    return failure();
  }
  if (failed(absorbMergerOpsIntoWorkItems())) {
    revert();
    return failure();
  }
  markOutputs();
  expandWorkspace(builder);

  if (enableSkewMode) {
    return markScopesForPreload();
  }

  if (failed(createNewLoops())) {
    revert();
    return failure();
  }

  if (failed(migrateOps())) {
    revert();
    return failure();
  }

  pipelineLoop.replaceAllUsesWith(newLoop.getResults());
  if (failed(newLoop.verify())) {
    revert();
    return failure();
  }
  ...
  pipelineLoop->erase();
  return success();
}
```

按阶段拆开：

1. `collectAtomicEffects()`：记录 atomic 状态，clone store/fixpipe 时补 set/unset。
2. `createWorkItems()`：从原 loop 中识别 CUBE/VECTOR 段，建立依赖图，切 WorkItem。
3. `absorbMergerOpsIntoWorkItems()`：把 yield 前的非 core 合并 op 吸收到唯一 WorkItem。
4. `markOutputs()`：找出需要跨 WorkItem 或跨 loop 传递的结果。
5. `expandWorkspace()`：把 workspace 加一个 multi-buffer 维度。
6. 普通模式：`createNewLoops()` 造外层 loop 和每个 WorkItem 的内部 loop，`migrateOps()` clone op。
7. preload 模式：`markScopesForPreload()` 造 scope 并打 preload 属性。
8. 成功后删旧 loop；失败则尽量 `revert()`。

## `runOnOperation()`：遍历哪些 loop

pass 是 `func::FuncOp` 级别：

```cpp
void CVPipeliningPass::runOnOperation() {
  func::FuncOp func = getOperation();
  DenseSet<scf::ForOp> pipelinedLoops;

  if (this->setDepthInUnrollMode == 1 || this->setDepthInUnrollMode == 0)
    return;

  func->walk<WalkOrder::PreOrder>([&pipelinedLoops, this](scf::ForOp loop) {
    auto parentLoop = loop->getParentOfType<scf::ForOp>();

    while (parentLoop) {
      if (pipelinedLoops.contains(parentLoop))
        return;
      parentLoop = parentLoop->getParentOfType<scf::ForOp>();
    }

    CVPipelineImpl impl(loop, this->setDepthInUnrollMode,
                        this->enableSkewMode,
                        this->enableLazyLoading);
    if (impl.run().succeeded())
      pipelinedLoops.insert(loop);
  });

  removeWorkspaceMultiBufferMarks(func);
}
```

这里有三个要点：

- 如果用户指定 depth 为 `0` 或 `1`，直接不做。因为 pipeline depth 小于 2 没有意义。
- 遍历是 pre-order，先看外层 loop。
- 如果一个外层 loop 已经成功 pipelined，就不会再 pipeline 它里面的子 loop，避免重复改写。

最后会调用 `removeWorkspaceMultiBufferMarks(func)`。这是因为 workspace 的 `hivm.multi_buffer`
已经被 CVPipeline 消费掉了，后续不应该再把同一个 workspace mark 当成普通 multi-buffer 标记处理。

## Step 1：识别 core op 和 region op

源码里 `isCoreOp()` 认为这些 op 是 core op：

```cpp
static bool isCoreOp(Operation *op) {
  return op->hasAttr(CubeOnlyAttrName) || op->hasAttr(VecOnlyAttrName) ||
         (isa_and_nonnull<HIVMDialect>(op->getDialect()) &&
          isa<DestinationStyleOpInterface>(op));
}
```

也就是：

- 已经被标成 `pipeline.cubeonly` 的 op。
- 已经被标成 `pipeline.veconly` 的 op。
- HIVM dialect 下的 DPS op。

对于 `scf.if`、`scf.for` 这种带 region 的 op，pass 会看 region 里到底是 Cube 还是 Vector：

```cpp
static bool illegalRegionedOp(Operation *op) {
  if (op->getRegions().empty())
    return false;
  bool hasCube = false;
  bool hasVector = false;
  WalkResult result = op->walk([&hasCube, &hasVector](Operation *curOp) {
    ...
    if (core == TCoreType::VECTOR) {
      if (hasCube)
        return WalkResult::interrupt();
      hasVector = true;
    } else if (core == TCoreType::CUBE) {
      if (hasVector)
        return WalkResult::interrupt();
      hasCube = true;
    }
    return WalkResult::advance();
  });
  if (result.wasInterrupted()) {
    op->emitWarning("[cv-pipelining] unsupported regioned op");
    return true;
  }

  if (hasCube)
    op->setAttr(CubeOnlyAttrName, unit);
  else if (hasVector)
    op->setAttr(VecOnlyAttrName, unit);
  return false;
}
```

例子：全 Vector 的 `scf.if` 可以被 pipeline。

```mlir
scf.if %cond {
  %cast = hivm.hir.vcast ...
  %store = hivm.hir.store ...
}
```

输出里会带：

```mlir
scf.if %cond {
  ...
} {pipeline.veconly}
```

反例：同一个 `scf.if` 里既有 `mmadL1` 又有 `vadd`，pass 不能把这个 region
归到一个 WorkItem，就会 warning 并放弃这个 loop。

## Step 2：识别 separator

`createWorkItems()` 会把这些 op 放进 `separators`：

```cpp
if (isa<FixpipeOp, StoreOp>(&op) || isCrossCoreCopy(&op))
  separators.push_back(&op);
```

separator 可以理解成“跨 C/V 段的数据边界”。常见模式：

```text
Cube -> fixpipe -> workspace -> load -> Vector
Vector -> store   -> workspace -> load -> Cube
Cube -> fixpipe -> workspace -> load -> Cube
Vector -> store   -> workspace -> load -> Vector
```

这些 separator 后面用于建立 `dependenceMap`。

例子：

```mlir
%dot = hivm.hir.mmadL1 ...
%ws = memref_ext.alloc_workspace() ...
%wst = bufferization.to_tensor %ws
%gmdot = hivm.hir.fixpipe ins(%dot) outs(%wst)

%loaded = hivm.hir.load ins(%gmdot) outs(%vdest)
%exp = hivm.hir.vexp ins(%loaded) outs(%vdest1)
```

这里 `fixpipe` 是 separator。Vector 段依赖它，因为 `%loaded/%exp` 最终读的是
`fixpipe` 写出的中间结果。

## Step 3：读取 multi-buffer depth

`createWorkItems()` 初始化：

```cpp
int multibuffer = numMultibuffer > 1 ? numMultibuffer : 2;
```

如果用户没有通过 `set-depth-in-unroll-mode` 强制指定，pass 会在 loop body 中找：

```mlir
annotation.mark %ws {hivm.multi_buffer = 2 : i32}
```

逻辑是：

```cpp
int markMultibuffer = getMultibufferCount(mark);
if (markMultibuffer == -1)
  continue;
if (multibuffer <= 2)
  multibuffer = markMultibuffer;
else if (multibuffer != markMultibuffer)
  multibuffer = std::min(multibuffer, markMultibuffer);
```

也就是说：

- 没有显式 depth 时，默认先按 2。
- 看到 mark 后，用 mark 的值。
- 如果多个 mark 不一致，取较小值，比较保守。
- 最终 `multibuffer < 2` 时放弃。

`cv-pipelining-no-multibuffer-attr.mlir` 专门测了一个边界：如果某个
`annotation.mark` 没有 `hivm.multi_buffer`，pass 不应该 crash。

## Step 4：建立普通依赖 `dependenceMap`

核心函数是：

```cpp
LogicalResult CVPipelineImpl::populateDependencies(Operation *separator)
```

它从每个 separator 开始 DFS：

```cpp
SmallVector<Operation *> dfsStack = {separator};
while (!dfsStack.empty()) {
  Operation *op = dfsStack.pop_back_val();
  ...
  for (Value result : op->getResults()) {
    if (!isa<ShapedType>(result.getType()))
      continue;

    for (OpOperand &use : result.getUses()) {
      Operation *usr = use.getOwner();
      Operation *scopedUsr = getContainedParent(pipelineLoop, usr);
      if (isa<scf::YieldOp, scf::ConditionOp>(scopedUsr) ||
          scopedUsr == separator)
        continue;
      dfsStack.push_back(scopedUsr);
      dependenceMap[scopedUsr].insert(separator);
    }
  }
}
```

这张表的语义是：

```text
dependenceMap[某个 op] = 这个 op 必须等哪些 separator 对应的 WorkItem 完成
```

例子：

```mlir
%fix = hivm.hir.fixpipe %dot -> workspace
%load = hivm.hir.load %fix
%v = hivm.hir.vexp %load
```

会得到类似：

```text
dependenceMap[load] contains fix
dependenceMap[vexp] contains fix
```

当 `fixpipe` 所在的 Cube WorkItem 被抽出来并处理完后，后续会从其他 op 的依赖集合里删掉这个
separator。依赖清空的 Vector op 才能成为下一个 WorkItem。

## Step 5：建立 loop-carried 依赖

`populateLoopCarriedDependencies()` 处理 `scf.for iter_args` 和 `scf.yield`。

简化模型：

```mlir
%res = scf.for %i = %lb to %ub step %s
  iter_args(%state = %init) -> tensor<...> {
    %x = Vector(%state)
    %y = Cube(%x)
    scf.yield %y
}
```

这里下一轮的 `%state` 来自上一轮的 `%y`。如果 CVPipeline 随便把 `%x` 和 `%y`
拆开/重排，就可能改变跨迭代语义。

源码只关注 tensor yield：

```cpp
for (OpOperand &yieldOperand : *maybeYield) {
  Value yieldVal = yieldOperand.get();
  if (!isa<TensorType>(yieldVal.getType()))
    continue;
  Operation *defining = yieldVal.getDefiningOp();
  ...
  BlockArgument iterArg =
      pipelineLoop.getRegionIterArgs()[yieldOperand.getOperandNumber()];
  SmallVector<Operation *> dfsStack(iterArg.getUsers());
  ...
  if (isa<DestinationStyleOpInterface>(op)) {
    loopCarriedDependenceMap[op].insert(defining);
    continue;
  }
}
```

文档里给过一个典型不能 pipeline 的例子：

```mlir
scf.for iter_args(%arg0 = %init) {
    %v0 = Vector(%arg0)
    %c = Cube(%v0)
    %v1 = Vector(%c)
    yield %v1
}
```

如果 `%v0` 和 `%v1` 不能被放到同一个 Vector WorkItem，而 `%v0` 又依赖上一轮 `%v1`，
那么这个循环的跨迭代依赖就不能安全拆开，pass 会保守放弃。

另一个可行变体：

```mlir
scf.for iter_args(%arg0 = %init) {
    %v0 = Vector(%arg0)
    %c = Cube()
    %v1 = Vector(%c, %v0)
    yield %v1
}
```

如果 `%c` 不依赖 `%v0`，那么 `%v0` 有机会被下沉到 `%v1` 所在的 Vector WorkItem，
整个 loop 仍可能 pipelining 成功。

## Step 6：抽取 WorkItem

抽取逻辑由两个函数合作：

```cpp
extractAvailableOps(independentOps, core)
populateWorkItem(independentOps, core)
```

`extractAvailableOps()` 会扫描 loop body，找当前依赖已经清空的 core op：

```cpp
if (!dependenceMap.contains(&op) || dependenceMap[&op].empty())
  potentiallyAvailable.insert(&op);
```

它还会强制同一轮只收集同一种 core：

```cpp
if (((maybeCore == TCoreType::VECTOR || isCrossCoreCopy(&op)) &&
     core == TCoreType::CUBE) ||
    ((maybeCore == TCoreType::CUBE && core == TCoreType::VECTOR)))
  continue;
core = maybeCore;
```

抽出一个 WorkItem 后，会从其他 op 的依赖集合里删除已经处理过的 op：

```cpp
for (auto &[op, dependant] : dependenceMap) {
  for (Operation *processed : independentOps)
    dependant.erase(processed);
}
```

然后 core 类型交替：

```cpp
if (core == TCoreType::VECTOR)
  core = TCoreType::CUBE;
else if (core == TCoreType::CUBE)
  core = TCoreType::VECTOR;
```

这个交替是 CVPipeline 的本质：尽量得到 CUBE/VECTOR/CUBE/VECTOR 的 WorkItem 序列。

## Step 7：把依赖 op 拉进 WorkItem

`populateWorkItem()` 先把 `availableOps` 放进新 WorkItem：

```cpp
auto item = std::make_shared<WorkItem>();
item->core = core;
for (Operation *op : availableOps) {
  mapOpToItem(op, item.get());
}
```

然后调用：

```cpp
traceDependentOps(item.get())
```

这个函数会把 core op 依赖的非 core op、alloc、cast、to_tensor、debug、mark 等也拉进来。

几个重要规则：

1. 如果遇到另一个已经属于别的 WorkItem 的 core op，默认不拉进来。
2. 如果遇到未被依赖满足的非 load core op，说明拆分不安全，返回 failure。
3. `hivm.hir.load` 可以被消费者拉进 WorkItem。
4. `enableLazyLoading` 时，某些 load/to_tensor 可以被多个 WorkItem clone。
5. workspace 相关 `to_tensor` 在一些路径下会跳过，因为 workspace 会被单独扩维处理。

例子：

```mlir
%alloc = memref.alloc() : memref<16x16xf16>
hivm.hir.load ins(%gm) outs(%alloc)
%t = bufferization.to_tensor %alloc
%dot = hivm.hir.mmadL1 ins(%a, %t) outs(%empty)
```

如果 `%dot` 是当前 Cube WorkItem 的核心 op，`traceDependentOps()` 会把：

```text
memref.alloc
hivm.hir.load
bufferization.to_tensor
tensor.empty
mmadL1
```

一起放进这个 Cube WorkItem，否则 clone 后 `%dot` 的 operand 就找不到定义。

## Step 8：memref subnet 追踪

`traceMemrefSubnet()` 专门处理这种链：

```text
alloc -> cast/view/subview/... -> to_tensor -> dps writer
```

它要保证一个临时 memref 只有一个 writer 和一个 `to_tensor`：

```cpp
if (auto dps = dyn_cast<DestinationStyleOpInterface>(usr)) {
  if (writer)
    return usr->emitWarning("[cv-pipelining] expecting only one op "
                            "writing to a defined memref");
  writer = dps;
  continue;
}
if (auto tt = dyn_cast<bufferization::ToTensorOp>(usr)) {
  if (toTensor)
    return usr->emitWarning(
        "[cv-pipelining] expecting only one toTensor for a defined memref");
  toTensor = tt;
  workingStack.push_back(usr);
  continue;
}
```

为什么要这么保守？

因为 CVPipeline 要把原来的 tensor/memref 结果扩成 multi-buffer slot。如果一个 memref 有多个 writer，
或者有多个 `to_tensor` view，pass 很难证明 clone 和 slice 后仍然等价，所以直接放弃。

## Step 9：吸收 merger op

`absorbMergerOpsIntoWorkItems()` 处理一种细节：yield 前可能有非 core op。

例子：

```mlir
%v = hivm.hir.vadd ...
%cmp = arith.cmpi ...
%sel = arith.select %cmp, %v, %old
scf.yield %sel
```

`arith.cmpi` / `arith.select` 不是 HIVM core op。如果不把它们 clone 到某个 WorkItem，
新 loop 里的 `scf.yield` 可能引用旧 loop 内即将被删掉的 op。

源码做法：

- 从每个 yield operand 往回走。
- 收集未被 WorkItem 占有的非 core chain。
- 找这个 chain 最终依赖的 WorkItem producer。
- 如果 producer 恰好只有一个，就把整个 chain 吸收到这个 WorkItem。
- 如果 producer 是 0 个或多个，说明无法安全归属，放弃这个 loop。

这一步是很多 SSA 重写 pass 里容易漏的地方。这里源码专门加了注释说明：

```text
Without this, those ops are never cloned into any work-item forOp nor into
newLoop, so the trailing terminator clone in migrateOps copies the operand
reference verbatim and ends up pointing at an op inside the soon-to-be-erased
pipelineLoop.
```

## Step 10：标记输出

`markOutputs()` 判断哪些结果需要特殊处理。

它会产生三类输出：

### 1. workspace outputs

如果一个 `StoreOp` 或 `FixpipeOp` 写的是 workspace：

```cpp
if (isa<StoreOp, FixpipeOp>(op) &&
    getAllocWorkspace(cast<DestinationStyleOpInterface>(op)
                          .getDpsInitOperand(0)->get())) {
  item->workspaceOutputs.push_back(op);
  continue;
}
```

那么后面不会用普通 tensor `insert_slice` 处理，而是把 workspace 扩成
`memref<multibuffer x ...>`，再让这个 store/fixpipe 写对应 stage 的 subview。

### 2. yielded outputs

如果某个 result 正好是原 loop yield 的值：

```cpp
if (yieldedVals.contains(result)) {
  item->yieldedOutputs.push_back({result, opNumber});
  continue;
}
```

那么新 loop 也要保持这个 result 的 iter_arg/result 语义。

### 3. local outputs

如果某个 tensor result 被其他 WorkItem 使用：

```cpp
for (Operation *usr : result.getUsers()) {
  if (opToWorkItemMap.contains(usr) && !item->ops.contains(usr)) {
    item->localOutputs.push_back({result, nullptr});
    break;
  }
}
```

那么它需要扩成 multi-buffer tensor：

```text
tensor<16x16xf16> -> tensor<Nx16x16xf16>
```

## Step 11：扩展 workspace

`expandWorkspace()` 会把 workspace 前面加一个 multi-buffer 维度。

源码注释直接给了例子：

```text
enable cvpipelining:
  if multibuffer is 2,
  original workspace is <16x16xf16>,
  expanded workspace is <2x16x16xf16>

enable preload:
  if work item num is 4,
  original workspace is <16x16xf16>,
  expanded workspace is <4x16x16xf16>
```

核心代码：

```cpp
ArrayRef<int64_t> origShape = origType.getShape();
int64_t expandSize = info.multibuffer;
SmallVector<int64_t> newShape = {expandSize};
newShape.append(origShape.begin(), origShape.end());
auto newType = MemRefType::get(newShape, origType.getElementType());
auto newAlloc = builder.create<AllocWorkspaceOp>(
    loc, newType, alloc.getWorkspaceArg(), alloc.getDynamicSize(),
    alloc.getOffset());

expandedWorkspaceMap_[alloc] = newAlloc;
info.marker.getSrcMutable().set(newAlloc);
info.marker->removeAttr(MultiBufferAttr::name);
toErase.insert(alloc);
toErase.insert(info.toTensor);
```

例子：

Before:

```mlir
%ws = memref_ext.alloc_workspace() from %arg0
  : from memref<?xi8> to memref<16x16xf16>
annotation.mark %ws {hivm.multi_buffer = 2 : i32}
%wst = bufferization.to_tensor %ws
%fix = hivm.hir.fixpipe ins(%dot) outs(%wst)
```

After:

```mlir
%ws2 = memref_ext.alloc_workspace() from %arg0
  : from memref<?xi8> to memref<2x16x16xf16>

%subview = memref.subview %ws2[%stage, 0, 0] [1, 16, 16] [1, 1, 1]
%slot = memref.collapse_shape %subview [[0, 1], [2]]
  : memref<1x16x16xf16> into memref<16x16xf16>
hivm.hir.fixpipe ins(%dot) outs(%slot)
```

这个变化会直接影响后面的 global workspace PlanMemory：原来申请一份 workspace，现在变成申请
`N` 份容量。

## 普通模式：`createNewLoops()`

普通模式会新建一个外层 loop 和多个内部 WorkItem loop。

原 loop：

```mlir
scf.for %i = %lb to %ub step %S iter_args(...) {
  ...
}
```

新外层 loop 的 step 是：

```cpp
Value unrollVal = constant(numMultibuffer);
Value newStep = arith.muli originStep, unrollVal;
newLoop = builder.create<scf::ForOp>(
    loc, lb, ub, newStep, pipelineLoop.getInits());
```

也就是：

```mlir
scf.for %outer = %lb to %ub step (%S * N) iter_args(...) {
  ...
}
```

内部每个 WorkItem loop 的上界是：

```cpp
cappedUB = ceildiv(ub - outer, originalStep)
actualUB = min(cappedUB, numMultibuffer)
```

这样尾块不会越界。比如 `N = 2`，原 loop 还有 1 次迭代时，内部 loop 只跑 1 次。

每个 WorkItem loop 都有两个重要属性：

```cpp
item->forOp->setAttrs({
  NamedAttribute("hivm.loop_core_type", TCoreTypeAttr::get(ctx, item->core)),
  NamedAttribute("multibuffer_unroll_factor", i32(numMultibuffer))
});
```

输出类似：

```mlir
scf.for %stage = %c0 to %actualUB step %c1 {
  ...
} {
  hivm.loop_core_type = #hivm.tcore_type<CUBE>,
  multibuffer_unroll_factor = 2 : i32
}
```

内部 loop 里需要重建原始 induction variable：

```cpp
// original i = innerIV * originalStep + outerIV
item->reconstructedIV = affine.apply #map(workItemIV)[originStep, outerIV]
```

例子：

```text
原来 i = 0, 2, 4, 6, 8 ...
multibuffer = 2

外层 outer = 0, 4, 8 ...
内部 stage = 0, 1

重建 i:
  outer=0, stage=0 -> i=0
  outer=0, stage=1 -> i=2
  outer=4, stage=0 -> i=4
  outer=4, stage=1 -> i=6
```

## 普通模式：`migrateOps()`

`migrateOps()` 做真正的 clone：

```cpp
for (Operation &op : pipelineLoop.getBody()->getOperations()) {
  auto it = opToWorkItemMap.find(&op);
  if (it == opToWorkItemMap.end())
    continue;
  for (WorkItem *target : it->getSecond()) {
    builder.setInsertionPointToEnd(target->forOp.getBody());
    builder.clone(op, target->irMap);
  }
}
```

clone 之后还有几类修补。

### 修补 workspace output

如果 WorkItem 里有写 workspace 的 `fixpipe/store`，调用：

```cpp
processWorkspaceOutputs(builder, item.get(), expandedWorkspaceMap_, item->irMap);
```

它会把原来的写：

```mlir
hivm.hir.fixpipe ins(%dot) outs(%old_workspace_tensor)
```

改成写：

```mlir
%slot = createWorkspaceSubview(%expanded_workspace, %stage)
hivm.hir.fixpipe ins(%dot) outs(%slot)
```

然后在 WorkItem loop 后重新建：

```mlir
%expanded_tensor = bufferization.to_tensor %expanded_workspace restrict
```

后续消费者用 `tensor.extract_slice` 从里面取当前 stage。

### 修补 tensor local output

如果 WorkItem 产生 `%exp : tensor<16x16xf16>`，后续 WorkItem 也要用它：

```mlir
%expanded = tensor.empty() : tensor<2x16x16xf16>
```

在 WorkItem loop 内：

```mlir
%slice = tensor.extract_slice %expanded[%stage, 0, 0]
%exp = hivm.hir.vexp ... outs(%slice)
%expanded2 = tensor.insert_slice %exp into %expanded[%stage, 0, 0]
scf.yield %expanded2
```

后续 WorkItem：

```mlir
%exp_i = tensor.extract_slice %expanded2[%stage, 0, 0]
```

### 修补 loop yield

如果原 loop 有：

```mlir
scf.yield %next, %newinc, %dot1
```

新外层 loop 最终也要 yield 对应的新值。每个 WorkItem 的 yieldedOutputs 会决定：

- 哪些值成为 WorkItem loop 的 iter_args。
- 哪些 WorkItem result 映射回原 loop result。
- 最后 `builder.clone(*pipelineLoop.getBody()->getTerminator(), globalIRMap)` 克隆 terminator。

## 普通模式完整例子：`cv-pipelining.mlir`

测试输入在：

```text
bishengir/test/Dialect/HIVM/cv-pipelining.mlir
```

原 loop 的结构可以简化成：

```mlir
scf.for %i = %c0 to %bound step %step
  iter_args(%sliding_input, %inc, %itercube) {

  // WorkItem 0: Cube
  load sliding_input
  mmadL1
  fixpipe -> workspace0
  %next = reinterpret_cast input2[newinc]

  // WorkItem 1: Vector
  load workspace0
  vexp
  scf.if all vector
  store -> workspace1

  // WorkItem 2: Cube
  load workspace1
  mmadL1 using %itercube
  fixpipe -> workspace2

  // WorkItem 3: Vector
  load workspace2
  vadd with %exp
  store final

  scf.yield %next, %newinc, %dot1
}
```

运行：

```bash
build/bin/bishengir-opt \
  -cv-pipelining \
  -allow-unregistered-dialect \
  bishengir/test/Dialect/HIVM/cv-pipelining.mlir
```

输出骨架是：

```mlir
%ws0 = memref_ext.alloc_workspace() ... to memref<2x16x16xf16>
%ws1 = memref_ext.alloc_workspace() ... to memref<2x16x16xf16>
%ws2 = memref_ext.alloc_workspace() ... to memref<2x16x16xf16>

%new:3 = scf.for %outer = %c0_i32 to %bound step %step_times_2
  iter_args(%sliding_input, %inc, %itercube) {

  %cube0:2 = scf.for %stage = %c0 to %actualUB step %c1
    iter_args(%inc_arg, %sliding_arg) {
      ...
      hivm.hir.mmadL1 ...
      %slot = subview %ws2[%stage, ...]
      hivm.hir.fixpipe ... outs(%slot)
      scf.yield %newinc, %next
  } {hivm.loop_core_type = CUBE, multibuffer_unroll_factor = 2}

  %vec0 = scf.for %stage = %c0 to %actualUB step %c1
    iter_args(%expanded_exp_init) {
      %fix_slice = tensor.extract_slice %ws2_tensor[%stage, ...]
      hivm.hir.load ...
      hivm.hir.vexp ...
      scf.if %cond { ... } {pipeline.veconly}
      %slot = subview %ws1[%stage, ...]
      hivm.hir.store ... outs(%slot)
      scf.yield %expanded_exp
  } {hivm.loop_core_type = VECTOR, multibuffer_unroll_factor = 2}

  %cube1 = scf.for ...
  } {hivm.loop_core_type = CUBE, multibuffer_unroll_factor = 2}

  scf.for ...
  } {hivm.loop_core_type = VECTOR, multibuffer_unroll_factor = 2}

  scf.yield ...
}
```

这个例子展示了几个典型行为：

- workspace 从 `memref<16x16xf16>` 变成 `memref<2x16x16xf16>`。
- Cube/Vector 被拆成 4 个内部 loop。
- 每个内部 loop 都有 `hivm.loop_core_type`。
- 中间 tensor `%exp` 因为后续 Vector 还要用，被扩成 `tensor<2x16x16xf16>`。
- `scf.if` 因为内部全是 Vector，被标成 `pipeline.veconly` 并放进 Vector WorkItem。

## preload/skew 模式

当 `enableSkewMode = true` 时，`run()` 不走 `createNewLoops()` / `migrateOps()`：

```cpp
if (enableSkewMode) {
  return markScopesForPreload();
}
```

这条路径主要做两件事：

1. `createNewLoopsForPreloadWithScopes()`：为每个 WorkItem 创建一个 `scope.scope`。
2. `migrateOpsForPreload()`：把 workspace output 改成 preload workspace 的 slice。

创建 scope 的关键代码：

```cpp
auto newScopeOp =
    builder.create<scope::ScopeOp>(loc, TypeRange(returnTensors));
newScopeOp.setNoInline(true);
newScopeOp->setAttr(kPipelinedLoopCoreTypeAttrName,
                    TCoreTypeAttr::get(ctx, item->core));
newScopeOp->setAttr(hivm::PreloadNumAttr::name, i32(preloadNum));
newScopeOp->setAttr(hivm::MaxPreloadNumAttr::name, i32(worklist.size()));
```

`preloadNum` 从 `worklist.size() - 1` 递减到 0：

```text
WorkItem 0 -> preload_num = 3
WorkItem 1 -> preload_num = 2
WorkItem 2 -> preload_num = 1
WorkItem 3 -> preload_num = 0
```

测试输入：

```text
bishengir/test/Dialect/HIVM/cv-pipelining-preload.mlir
```

运行：

```bash
build/bin/bishengir-opt \
  -cv-pipelining="enable-skew-mode" \
  -allow-unregistered-dialect \
  bishengir/test/Dialect/HIVM/cv-pipelining-preload.mlir
```

输出骨架：

```mlir
scf.for ... {
  %scope0 = scope.scope : () -> i32 {
    hivm.hir.mmadL1 ...
    hivm.hir.fixpipe ...
    scope.return ...
  } {
    hivm.loop_core_type = #hivm.tcore_type<CUBE>,
    hivm.max_preload_num = 4 : i32,
    hivm.preload_num = 3 : i32,
    no_inline
  }

  %scope1:3 = scope.scope : () -> (...) {
    hivm.hir.vreduce <max>
    hivm.hir.vreduce <sum>
    scope.return ...
  } {
    hivm.loop_core_type = #hivm.tcore_type<VECTOR>,
    hivm.max_preload_num = 4 : i32,
    hivm.preload_num = 2 : i32,
    no_inline
  }

  %scope2 = scope.scope : () -> i32 {
    hivm.hir.mmadL1 ...
    hivm.hir.fixpipe ...
    scope.return ...
  } {
    hivm.loop_core_type = #hivm.tcore_type<CUBE>,
    hivm.preload_num = 1 : i32,
    ...
  }

  %scope3 = scope.scope : () -> tensor<128x128xf32> {
    hivm.hir.vmul
    hivm.hir.vadd
    scope.return ...
  } {
    hivm.loop_core_type = #hivm.tcore_type<VECTOR>,
    hivm.preload_num = 0 : i32,
    ...
  }
}
```

后面的 `CreatePreloadPass` 会收集这些 scope：

```cpp
moduleOp->walk([&](scope::ScopeOp scopeOp) {
  if (auto maxPreloadNumAttr = scopeOp->getAttrOfType<IntegerAttr>(
          hivm::MaxPreloadNumAttr::name)) {
    auto parentForOp = cast<scf::ForOp>(scopeOp->getParentOp());
    preload[parentForOp].resize(maxPreloadNumAttr.getInt(), nullptr);
  }
  if (auto preloadNumAttr =
          scopeOp->getAttrOfType<IntegerAttr>(hivm::PreloadNumAttr::name)) {
    ...
    preloadVec[preloadNum] = scopeOp;
  }
});
```

所以 preload 模式下，CVPipeline 不是最终完成 preload 的 pass，它更像是“给后续 preload
生成结构化 scope 和属性”。

## 例子：C -> V 的最小流水

Before:

```mlir
scf.for %i = %lb to %ub step %s {
  %dot = Cube(%a, %b)
  %ws = memref_ext.alloc_workspace() : memref<16x16xf16>
  annotation.mark %ws {hivm.multi_buffer = 2 : i32}
  %wst = bufferization.to_tensor %ws
  %fix = hivm.hir.fixpipe ins(%dot) outs(%wst)

  %loaded = hivm.hir.load ins(%fix) outs(%empty)
  %v = Vector(%loaded)
}
```

After 普通模式：

```mlir
%ws2 = memref_ext.alloc_workspace() : memref<2x16x16xf16>

scf.for %outer = %lb to %ub step %s2 {
  scf.for %stage = 0 to %actualUB step 1 {
    %i = %outer + %stage * %s
    %dot = Cube(%a, %b)
    %slot = subview %ws2[%stage, 0, 0]
    hivm.hir.fixpipe ins(%dot) outs(%slot)
  } {hivm.loop_core_type = CUBE}

  %ws2_tensor = bufferization.to_tensor %ws2
  scf.for %stage = 0 to %actualUB step 1 {
    %fix_i = tensor.extract_slice %ws2_tensor[%stage, 0, 0]
    %loaded = hivm.hir.load ins(%fix_i) outs(%empty)
    %v = Vector(%loaded)
  } {hivm.loop_core_type = VECTOR}
}
```

语义上相当于：

```text
先批量做 N 个 Cube stage，把结果分别写到 workspace slot 0..N-1；
再批量做 N 个 Vector stage，从对应 slot 读回来。
```

## 例子：C -> V -> C -> V

Before:

```mlir
scf.for %i = %lb to %ub step %s {
  %c0 = Cube()
  %w0 = fixpipe_to_workspace(%c0)

  %v0_in = load(%w0)
  %v0 = Vector(%v0_in)
  %w1 = store_to_workspace(%v0)

  %c1_in = load(%w1)
  %c1 = Cube(%c1_in)
  %w2 = fixpipe_to_workspace(%c1)

  %v1_in = load(%w2)
  %v1 = Vector(%v1_in)
}
```

After：

```text
outer loop:
  WorkItem 0, CUBE:
    c0 + fixpipe_to_workspace slot[stage]

  WorkItem 1, VECTOR:
    load slot[stage] + v0 + store_to_workspace slot[stage]

  WorkItem 2, CUBE:
    load slot[stage] + c1 + fixpipe_to_workspace slot[stage]

  WorkItem 3, VECTOR:
    load slot[stage] + v1
```

这个模式是 FlashAttention 类 kernel 里很常见的结构：Cube 产生中间矩阵，Vector 做 softmax
或 mask，再把结果喂给下一段 Cube/Vector。

## 例子：local tensor output 为什么要扩维

假设 WorkItem 1 产生 `%exp`，WorkItem 3 还要用 `%exp`：

```mlir
// WorkItem 1
%exp = hivm.hir.vexp %x -> tensor<16x16xf16>

// WorkItem 3
%add = hivm.hir.vadd %later, %exp -> tensor<16x16xf16>
```

如果不扩维，在普通模式里会有问题：

```text
stage 0 的 %exp 和 stage 1 的 %exp 都叫同一条 SSA 链上的结果；
WorkItem 3 按 stage 读取时无法区分它要的是哪一轮的 %exp。
```

因此 CVPipeline 生成：

```mlir
%exp_buf = tensor.empty() : tensor<2x16x16xf16>

// WorkItem 1 inner loop
%exp_init_i = tensor.extract_slice %exp_buf[%stage, 0, 0]
%exp_i = hivm.hir.vexp %x_i outs(%exp_init_i)
%exp_buf_next = tensor.insert_slice %exp_i into %exp_buf[%stage, 0, 0]

// WorkItem 3 inner loop
%exp_i = tensor.extract_slice %exp_buf_next[%stage, 0, 0]
%add_i = hivm.hir.vadd %later_i, %exp_i
```

这就是 `localOutputs` 的用途。

## 例子：workspace output 为什么不用 tensor.insert_slice

workspace output 和普通 tensor output 不一样。普通 tensor output 可以：

```mlir
tensor.insert_slice %value into %expanded_tensor[%stage, ...]
```

但 workspace 是 `memref_ext.alloc_workspace`，本质是 GM workspace 上的 memref。pass 选择直接改
writer 的 destination：

```mlir
%subview = memref.subview %expanded_workspace[%stage, ...]
%slot = memref.collapse_shape %subview ...
hivm.hir.fixpipe ins(%dot) outs(%slot)
```

这样 `fixpipe/store` 直接写进对应 stage 的 workspace slot。后续如果需要 tensor 形式，再对整个
expanded workspace 做：

```mlir
%t = bufferization.to_tensor %expanded_workspace restrict
%slice = tensor.extract_slice %t[%stage, ...]
```

这个策略避免了 `fixpipe/store -> tensor.insert_slice -> yield` 的复杂中间模式。

## 例子：lazy loading

默认情况下，某个 `bufferization.to_tensor` 如果已经分配给一个 WorkItem，其他 WorkItem 不会再 clone
它，以免重复定义同一个中间值。

`enableLazyLoading` 打开后，有一个例外：

```cpp
if (enableLazyLoading && skipToTensor) {
  auto tt = cast<bufferization::ToTensorOp>(op);
  auto it = outputMemrefMap.find(tt);
  if (it != outputMemrefMap.end() && isa<LoadOp>(it->second.getOperation()))
    skipToTensor = false;
}
```

含义是：如果这个 `to_tensor` 背后的 writer 是 `hivm.hir.load`，可以允许多个 WorkItem 各自 clone
load/to_tensor。这样可能减少一些中间 tensor 扩维，但代价是重复 load。

简化例子：

```mlir
%alloc = memref.alloc()
hivm.hir.load ins(%gm) outs(%alloc)
%t = bufferization.to_tensor %alloc

%c = Cube(%t)
%v = Vector(%t)
```

不开 lazy loading 时，`%t` 很难同时安全地属于两个 WorkItem。开 lazy loading 时，Cube 和 Vector
WorkItem 可以各自 clone 一份 load 链。

## 例子：atomic effect

如果原 loop 中有：

```mlir
hivm.hir.set_atomic <add>
hivm.hir.store ...
hivm.hir.set_atomic <none>
```

clone store 时必须保留 atomic 状态。`collectAtomicEffects()` 扫描原 loop：

```cpp
std::optional<AtomicEffect> current;
for (Operation &op : *pipelineLoop.getBody()) {
  if (auto setAtomic = dyn_cast<SetAtomicOp>(&op)) {
    if (setAtomic.getKind() != AtomicKind::NONE)
      current = AtomicEffect{setAtomic.getKind(), setAtomic.getTypeAttr()};
    else
      current = std::nullopt;
    continue;
  }
  if (current && isa<FixpipeOp, StoreOp>(&op))
    atomicEffectMap[&op] = *current;
}
```

`migrateOps()` clone 对应 store/fixpipe 前后会补：

```cpp
builder.create<SetAtomicOp>(op.getLoc(), effect.kind, effect.type);
builder.clone(op, target->irMap);
builder.create<SetAtomicOp>(op.getLoc(), AtomicKind::NONE, effect.type);
```

这保证被搬到 WorkItem loop 里的 store/fixpipe 仍处于正确 atomic 上下文。

## 失败和 no-op 条件

CVPipeline 是很保守的 pass。常见失败或不改写条件：

| 条件 | 结果 |
| - | - |
| `set-depth-in-unroll-mode` 是 0 或 1 | pass 直接返回 |
| 找不到 `hivm.multi_buffer >= 2`，且没有有效 depth | 当前 loop 不改 |
| WorkItem 少于 2 个 | 没有 C/V 流水价值，不改 |
| region op 内同时含 Cube 和 Vector | warning `unsupported regioned op`，不改 |
| memref 链上出现未知 op | warning `unexpected memref op in chain`，不改 |
| 一个 defined memref 有多个 writer | warning `expecting only one op writing...`，不改 |
| 一个 defined memref 有多个 `to_tensor` | warning `expecting only one toTensor...`，不改 |
| `to_tensor` 没有 DPS writer | warning `expecting toTensor to have dps op...`，不改 |
| local output 的临时 memref 是动态 shape | warning `expected temporary buffer to be static`，不改 |
| loop-carried dependency 无法拆干净 | warning `cannot pipeline loop due to loop carried dependencies`，不改 |
| merger chain 依赖 0 个或多个 WorkItem producer | warning 并放弃 |

大多数失败不是 pass failure，而是对当前 loop no-op。源码中多处调用 `revert()`，尽量撤回已经创建的新
IR。

## 和后续 pass 的关系

### GraphSyncSolver / InjectSync

普通模式生成的内部 loop 有：

```mlir
multibuffer_unroll_factor = N
hivm.loop_core_type = CUBE/VECTOR
```

`GraphSyncSolver` 的 IR translator 会读取：

```cpp
if (auto intAttr =
        forOp->getAttrOfType<IntegerAttr>(kMultibufferUnrollAttrName)) {
  if (!scf::utils::isNormalized(forOp)) {
    forOp->emitOpError("multibuffer-enabled loop expected to be normalized");
    return {};
  }
  return intAttr.getInt();
}
```

所以 CVPipeline 生成的内部 loop 必须是 normalized loop，后续同步求解依赖这个假设。

### SplitMixKernel

CVPipeline 标出的：

```mlir
hivm.loop_core_type = #hivm.tcore_type<CUBE>
hivm.loop_core_type = #hivm.tcore_type<VECTOR>
```

会帮助后续 mix kernel 拆分，把 Cube 侧和 Vector 侧逻辑拆到 AIC/AIV。

### CreatePreload

preload 模式生成：

```mlir
scope.scope { ... } {
  hivm.loop_core_type = ...,
  hivm.preload_num = ...,
  hivm.max_preload_num = ...,
  no_inline
}
```

`CreatePreloadPass` 会按 `preload_num` 把这些 scope 组织成 preload loop，再 canonicalize 和 inline scope。

### PlanMemory

CVPipeline 会改变 PlanMemory 看到的内存形态：

1. workspace 从 `memref<shape>` 变成 `memref<N x shape>`，global workspace 需求可能增加。
2. 普通 tensor local output 可能变成 `tensor<N x shape>`，bufferization 后可能产生更大的 local alloc。
3. WorkItem loop 改变 op 顺序和生命周期，local PlanMemory 看到的 liveness 也会变。
4. `removeWorkspaceMultiBufferMarks()` 会移除已消费的 workspace multi-buffer mark，避免后续重复解释。

所以调 UB/GM overflow 时，CVPipeline 是 PlanMemory 前面非常关键的一步。

## 调试命令

直接跑普通模式：

```bash
build/bin/bishengir-opt \
  -cv-pipelining \
  -allow-unregistered-dialect \
  bishengir/test/Dialect/HIVM/cv-pipelining.mlir
```

跑 preload/skew 模式：

```bash
build/bin/bishengir-opt \
  -cv-pipelining="enable-skew-mode" \
  -allow-unregistered-dialect \
  bishengir/test/Dialect/HIVM/cv-pipelining-preload.mlir
```

测试没有 `hivm.multi_buffer` 的 mark 不 crash：

```bash
build/bin/bishengir-opt \
  -cv-pipelining \
  -allow-unregistered-dialect \
  bishengir/test/Dialect/HIVM/cv-pipelining-no-multibuffer-attr.mlir
```

用 debug 输出看 WorkItem 切分：

```bash
build/bin/bishengir-opt \
  -cv-pipelining \
  -debug-only=cv-pipelining \
  -allow-unregistered-dialect \
  bishengir/test/Dialect/HIVM/cv-pipelining.mlir
```

如果是完整 compile pipeline，可以通过这些选项做开关对照：

```text
--disable-cv-pipelining
--disable-auto-cv-work-space-manage
--disable-auto-cv-mark-multi-buffer
--disable-auto-cv-global-workspace-plan
--set-workspace-multibuffer=<n>
--enable-preload
```

其中：

- `--disable-cv-pipelining` 只关 CVPipeline，保留其他 auto CV workspace manage。
- `--disable-auto-cv-work-space-manage` 会跳过 InsertLoadStore/InsertWorkspace/MarkMultiBuffer/CVPipelining/global workspace PlanMemory 等一整组。
- `--set-workspace-multibuffer=<n>` 会影响前面的 MarkMultiBuffer，进而影响 CVPipeline depth。
- `--enable-preload` 会让 CVPipeline 进入 skew/preload 路径，并在 local PlanMemory 后跑 `CreatePreloadPass`。

## 从源码回看 `cv-pipelining.mlir`

以 `cv-pipelining.mlir` 为例，源码流程可以对应成：

### 1. 进入 loop

`runOnOperation()` 遍历到测试里的主 `scf.for`，创建：

```cpp
CVPipelineImpl impl(loop, -1, false, false);
```

### 2. 收集 multibuffer

loop 里有多个：

```mlir
annotation.mark %ws {hivm.multi_buffer = 2 : i32}
```

所以 `numMultibuffer = 2`。

### 3. 收集 separator

这些 op 成为 separator：

```text
第一个 fixpipe: Cube0 -> workspace0
第一个 store:   Vector0 -> workspace1
第二个 fixpipe: Cube1 -> workspace2
最终 store:     Vector1 -> GM/output
```

### 4. 切 WorkItem

最终得到：

```text
WorkItem 0 CUBE:
  load sliding_input
  mmadL1
  fixpipe workspace0
  arith.addi / reinterpret_cast next

WorkItem 1 VECTOR:
  load workspace0
  vexp
  vector-only scf.if
  store workspace1

WorkItem 2 CUBE:
  load workspace1
  mmadL1
  fixpipe workspace2

WorkItem 3 VECTOR:
  load workspace2
  vadd
  final store
```

### 5. 标记输出

典型输出：

```text
workspaceOutputs:
  fixpipe workspace0
  store workspace1
  fixpipe workspace2

localOutputs:
  %exp，因为最后 vadd 还要用它

yieldedOutputs:
  %next
  %newinc
  %dot1
```

### 6. 扩 workspace

```text
workspace0: memref<16x16xf16> -> memref<2x16x16xf16>
workspace1: memref<16x16xf16> -> memref<2x16x16xf16>
workspace2: memref<16x16xf16> -> memref<2x16x16xf16>
```

### 7. 生成新 loop

```text
outer step = old step * 2
inner actualUB = min(ceil((ub - outer) / old_step), 2)
```

### 8. clone op

每个 WorkItem 内部 clone 自己的 op，并用 `IRMapping` 替换：

```text
原 %i -> outer + stage * old_step
原 iter_arg -> 对应新 loop iter_arg 或 WorkItem iter_arg
原 workspace -> expanded workspace slot
原 local output -> extract/insert slice
```

### 9. 删除旧 loop

`newLoop.verify()` 成功后，过滤不合法 mark，最后 `pipelineLoop->erase()`。

## 和 PlanMemory 报告视角的对应关系

如果用 PlanMemory 报告那种“before 到 after”的视角，可以这样看：

| PlanMemory 视角 | CVPipeline 对应物 |
| - | - |
| before IR 中的 `memref.alloc` | before IR 中一个混合 C/V 的 `scf.for` |
| liveness 分析 | separator/dependence/loop-carried dependency 分析 |
| buffer 是否能复用 | op 是否能归到某个 WorkItem |
| 地址规划成功后 rewrite | WorkItem 切分成功后 clone 到新 loop/scope |
| overflow 时保留 alloc | 依赖/region/memref 不可证明安全时保留原 loop |
| memory_info/json | debug-only 输出和 lit FileCheck |

PlanMemory 的问题是“这些 buffer 能不能放进有限 local memory”。CVPipeline 的问题是“这些 C/V op 能不能按依赖切成可流水执行的阶段”。两者都保守：证明不了就不做。

## 读代码时建议的顺序

如果后面要继续深入，建议按这个顺序读：

1. `Passes.td` 里 `CVPipelining` 的选项和 dependent dialect。
2. `HIVMPipelines.cpp` 中 CVPipeline 前后的 pass。
3. `CVPipelining.cpp` 的 `WorkItem` 和 `CVPipelineImpl` 字段。
4. `CVPipeliningPass::runOnOperation()`。
5. `CVPipelineImpl::run()`。
6. `createWorkItems()`。
7. `populateDependencies()` 和 `populateLoopCarriedDependencies()`。
8. `traceDependentOps()` / `traceMemrefSubnet()`。
9. `markOutputs()` / `expandWorkspace()`。
10. 普通模式：`createNewLoops()` / `migrateOps()`。
11. preload 模式：`createNewLoopsForPreloadWithScopes()` / `migrateOpsForPreload()`。
12. 后续消费者：`CreatePreload.cpp` 和 `GraphSyncSolverIRTranslator.cpp`。

## 总结

CVPipeline 的核心不是简单地把代码移动一下，而是做了一个小型调度和 SSA 重写：

```text
1. 识别 loop 内的 Cube/Vector core op。
2. 用 fixpipe/store/cross-core copy 建立 C/V 边界。
3. 加上 loop-carried dependency，判断能否安全切分。
4. 把 op 分组成 CUBE/VECTOR 交替的 WorkItem。
5. 把跨 WorkItem 的 tensor/workspace 扩成 multi-buffer。
6. 普通模式生成 outer loop + 每个 WorkItem 的 inner loop。
7. preload 模式生成带 preload_num 的 scope。
8. 给后续 sync、split mix、PlanMemory、preload pass 留结构化属性。
```

最常见的成功形态是：

```text
for:
  Cube -> fixpipe -> Vector -> store -> Cube -> fixpipe -> Vector

变成：
outer for step *= N:
  inner CUBE   stage loop
  inner VECTOR stage loop
  inner CUBE   stage loop
  inner VECTOR stage loop
```

最常见的失败原因是：

```text
region 里 C/V 混在一起、
跨迭代依赖无法拆开、
memref/to_tensor/writer 关系不唯一、
临时 buffer 形状不适合扩成 multi-buffer。
```

因此调试 CVPipeline 时，最有效的问题不是“为什么这个 op 没被移动”，而是：

```text
这个 op 属于哪个 core？
它依赖哪个 separator？
它的输入能否被 clone 到同一个 WorkItem？
它的输出是否被其他 WorkItem 使用，需要扩成 multi-buffer？
它所在的 region 或 loop-carried dependency 是否让 pass 无法证明安全？
```

