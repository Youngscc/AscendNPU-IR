# 工作与修复纪律

## 先确认方法，再实现

当用户明确说“先不着急修改”、要求分析或评审方案时，只做只读调研：复述目标、指出
验证盲区和风险、给出有依据的推荐；用户拍板后再实现。没有这类限制且任务明确要求
修改时，可以直接推进正常实现和验证。

## 修复 model/cv2pm 差异

1. 用同一 profile input、scenario 和 seed 复现。
2. 确认 cv2pm oracle/cache 是 schema 2 的 `full_cv2pm_per_seed` 单进程结果，身份有效且
   输出稳定；禁止使用拆分后缀和 PlanMemory 进程的旧缓存。
3. 从前向后比较对应 pass 边界，找到第一个语义差异；不要从最终 peak 倒推规则。
4. 阅读 cv2pm 调用的 BiSheng/MLIR pass 源码，实现同一条通用语义。
5. 构建命令必须等到编译进程真实退出，再检查模型二进制时间戳不早于改动源码；不能在
   增量构建仍运行时启动验证，否则会误测旧二进制。
6. 先跑定向样例和相关单测，再扩大到相关场景，最终跑 160×27×20 非超时全量。
7. 同时报告 exact 覆盖、mismatch、确定性失败对齐和 timeout 覆盖盲区。

禁止：

- adapter/kernel/seed/SSA 名/buffer 数量特例；
- 修改生产 pass 遍历顺序或其他 Bisheng 核心语义来消除差异；
- 把 exact mismatch 改成 blocker/incomplete；
- 只比较 peak 而忽略 required、plan、lifetime、multi-buffer 和 inplace；
- 用 suffix 结果代替 cv2pm oracle。

## 性能优化边界

允许优化实现方式和公共基础设施，例如缓存不随 seed 改变的事实、复用索引、合并重复
遍历、减少 IR 转换和避免重复 canonicalization。不得改变 buffer plan、retry、遍历语义
或任何会影响最终 UB 分布的策略。每轮优化后先做快速定向验证，最终用 20 seeds 全量
确认正确性。

## 临时产物

- 每轮诊断使用独立 `mktemp -d`，避免覆盖上一轮产物。
- 全量验证完成前保留本任务生成的 dump；收尾时只清理本任务创建的临时目录。
- `Output/`、`ub_overflow_model_cpp/output/`、`__pycache__/` 等生成物不提交。
- 用户已明确：为了避免睡眠期间审批阻塞，中途可以不清理 tmp；结束前再统一处理。

## 未完成工作与 Git

- 不要求把半成品作为正式产品提交。需要定位回归或保护现场时，在 `codex/` 本地分支
  创建明确标注 `checkpoint(...)` 的本地提交；它不等于完成，也不必 push。
- 完成后按逻辑整理正式提交；不要把缓存、dump、Output 或本地备份混入提交。
- 工作树有用户修改时必须保留，不能使用 destructive reset/checkout 覆盖。

## 维护本目录

- `MEMORY.md` 只放当前架构、当前状态和长期约束；不要复制长篇源码说明。
- `code_map.md` 只放仍存在的路径和可执行命令。
- `validation.md` 中所有数字必须带日期，并区分“当前重新运行”与“历史保留报告”。
- 结论过时后直接更新，不在入口文件堆叠多代状态；需要追溯时使用 Git 历史。
- 新规则优先合并进现有主题文件，避免为一句话再创建一个 Markdown。
