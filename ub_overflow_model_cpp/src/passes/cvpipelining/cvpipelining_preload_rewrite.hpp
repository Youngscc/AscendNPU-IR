#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_PRELOAD_REWRITE_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_PRELOAD_REWRITE_HPP

#include "../../ir/post_pipeline_ir_utils.hpp"
#include "cvpipelining_rewrite.hpp"

namespace cvub {

class CVPipelinePreloadRewriter {
public:
  CVPipelinePreloadRewriter(GenericModule &inputModule,
                            CVPipelineAnalysisResult analysisResult)
      : module(inputModule), analysis(std::move(analysisResult)),
        rewriter(inputModule) {
    index();
  }

  bool rewrite() {
    if (!analysis.success || analysis.worklist.empty())
      return false;
    const GenericOperation &loop =
        module.operations.at(static_cast<size_t>(analysis.loop));
    if (loop.name != "scf.for" || loop.regions.size() != 1)
      return false;
    const int body = loopBody();
    if (body < 0)
      return false;
    expandWorkspace(body);
    int preloadNum = static_cast<int>(analysis.worklist.size()) - 1;
    for (size_t itemIndex = 0; itemIndex < analysis.worklist.size();
         ++itemIndex, --preloadNum) {
      if (!createScopeForWorkItem(itemIndex, preloadNum, body))
        return false;
    }
    migrateWorkspaceOutputs();
    // Workspace migration rewrites Load operands after cloning.  Refresh the
    // cached destination-style projections before dependency-driven erasure;
    // MLIR has a single operand list, while GenericOperation keeps these
    // projections explicitly for later passes.
    rewriter.applyDirtyOperationSemantics();
    if (!removeOriginalOps(body)) {
      // CVPipeliningPass intentionally ignores a failed loop rewrite.  The
      // production implementation may already have erased use-free members
      // of toErase at that point, so preserve that valid partial state for
      // the following passes instead of manufacturing dangling SSA values.
      module = CompactGenericModule(std::move(module));
      return false;
    }
    module = CompactGenericModule(std::move(module));
    return true;
  }

private:
  void index() {
    for (const GenericBlock &block : module.blocks)
      for (size_t i = 0; i < block.arguments.size(); ++i)
        valueTypes[block.arguments[i]] = block.argumentTypes[i];
    for (const GenericOperation &operation : module.operations) {
      for (size_t i = 0; i < operation.results.size(); ++i) {
        definitions[operation.results[i]] = operation.id;
        valueTypes[operation.results[i]] = operation.resultTypes[i];
      }
      for (int operand : operation.operands)
        users[operand].push_back(operation.id);
    }
  }

  int loopBody() const {
    const GenericOperation &loop =
        module.operations.at(static_cast<size_t>(analysis.loop));
    if (loop.regions.empty())
      return -1;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(loop.regions.front()));
    if (region.blocks.empty())
      return -1;
    return region.blocks.front();
  }

  int result(int operation, size_t index = 0) const {
    return module.operations.at(static_cast<size_t>(operation)).results.at(index);
  }

  const std::string &valueType(int value, const char *context) const {
    auto found = valueTypes.find(value);
    if (found == valueTypes.end())
      throw std::runtime_error(std::string("CVPipelining preload: missing ") +
                               context + " type for value " +
                               std::to_string(value));
    return found->second;
  }

  int createInBlock(int parent, int region, int block, const std::string &name,
                    const std::vector<std::string> &resultTypes,
                    const std::vector<int> &operands,
                    const std::vector<std::string> &operandTypes,
                    const std::string &properties = "",
                    const std::string &attributes = "{}") {
    const int operation = rewriter.createOperation(
        parent, region, block, name, resultTypes, operands, operandTypes,
        properties, attributes);
    rewriter.appendToBlock(block, operation);
    for (size_t i = 0; i < resultTypes.size(); ++i)
      valueTypes[result(operation, i)] = resultTypes[i];
    return operation;
  }

  void expandWorkspace(int body) {
    (void)body;
    const GenericOperation loop =
        module.operations.at(static_cast<size_t>(analysis.loop));
    const int parentOperation = loop.parentId;
    const int parentRegion = loop.regionId;
    const int parentBlock = loop.blockId;
    GenericBlock &parent =
        module.blocks.at(static_cast<size_t>(parentBlock));
    auto loopPosition = std::find(parent.operations.begin(),
                                  parent.operations.end(), analysis.loop);
    size_t insertion = loopPosition == parent.operations.end()
                           ? parent.operations.size()
                           : static_cast<size_t>(loopPosition -
                                                 parent.operations.begin());
    const unsigned expandSize = static_cast<unsigned>(analysis.worklist.size());
    for (const auto &[allocation, info] : analysis.workspaceAllocs) {
      const GenericOperation source =
          module.operations.at(static_cast<size_t>(allocation));
      if (source.resultTypes.empty())
        continue;
      const std::string expandedType =
          CVPipelineExpandShapedType(source.resultTypes.front(), expandSize);
      const int created = rewriter.createOperation(
          parentOperation, parentRegion, parentBlock, source.name,
          {expandedType}, source.operands, source.operandTypes,
          source.properties, source.attributes);
      rewriter.insertToBlock(parentBlock, insertion++, created);
      expandedWorkspaceValue[allocation] = result(created);
      valueTypes[result(created)] = expandedType;
      if (info.marker >= 0) {
        GenericOperation &marker =
            module.operations.at(static_cast<size_t>(info.marker));
        if (!marker.operands.empty())
          marker.operands.front() = result(created);
        marker.operandTypes = {expandedType};
        marker.attributes =
            removeDictionaryEntry(marker.attributes, "hivm.multi_buffer");
      }
    }
  }

  std::string removeDictionaryEntry(const std::string &dictionary,
                                    const std::string &key) const {
    if (dictionary.size() < 2 || dictionary.front() != '{')
      return dictionary;
    std::vector<std::string> kept;
    for (const std::string &entry :
         splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
      const size_t equal = entry.find('=');
      if (trim(entry.substr(0, equal)) != key)
        kept.push_back(entry);
    }
    std::string result = "{";
    for (size_t i = 0; i < kept.size(); ++i) {
      if (i)
        result += ", ";
      result += kept[i];
    }
    result += "}";
    return result;
  }

  std::vector<std::pair<int, size_t>>
  returnValuesForItem(const CVPipelineWorkItem &item) const {
    std::vector<std::pair<int, size_t>> returns;
    size_t ordinal = 0;
    for (const auto &[value, unused] : item.localOutputs) {
      (void)unused;
      returns.push_back({value, ordinal++});
    }
    for (const auto &[value, unused] : item.yieldedOutputs) {
      (void)unused;
      returns.push_back({value, ordinal++});
    }
    return returns;
  }

  bool createScopeForWorkItem(size_t itemIndex, int preloadNum, int body) {
    const CVPipelineWorkItem &item = analysis.worklist[itemIndex];
    const std::vector<std::pair<int, size_t>> returns = returnValuesForItem(item);
    if (returns.empty() && item.workspaceOutputs.empty())
      return true;

    std::vector<std::string> resultTypes;
    for (const auto &[value, ordinal] : returns) {
      (void)ordinal;
      resultTypes.push_back(valueType(value, "scope result"));
    }
    const std::string attrs =
        "{hivm.loop_core_type = #hivm.tcore_type<" +
        std::string(item.core == CVPipelineCoreType::Cube ? "CUBE" : "VECTOR") +
        ">, hivm.max_preload_num = " +
        std::to_string(analysis.worklist.size()) +
        " : i32, hivm.preload_num = " + std::to_string(preloadNum) +
        " : i32}";
    const int parent = analysis.loop;
    const int region = module.blocks.at(static_cast<size_t>(body)).regionId;
    const std::vector<int> sourceOperations =
        module.blocks.at(static_cast<size_t>(body)).operations;
    const int scope =
        rewriter.createOperation(parent, region, body, "scope.scope",
                                 resultTypes, {}, {}, "{no_inline}", attrs);
    for (size_t i = 0; i < resultTypes.size(); ++i)
      valueTypes[result(scope, i)] = resultTypes[i];
    const size_t terminator = sourceOperations.empty()
                                  ? 0
                                  : sourceOperations.size() - 1;
    rewriter.insertToBlock(body, terminator, scope);
    const int scopeRegion = rewriter.createRegion(scope);
    const int scopeBlock = rewriter.createBlock(scopeRegion, {});

    std::map<int, int> scopeMap = globalValues;
    std::set<int> workspaceSet(item.workspaceOutputs.begin(),
                               item.workspaceOutputs.end());
    WorkspaceMigration migration;
    migration.scope = scope;
    for (int operationId : sourceOperations) {
      if (operationId == scope || item.ops.count(operationId) == 0)
        continue;
      materializeWorkspaceTensorSlices(operationId, scope, scopeRegion,
                                       scopeBlock, scopeMap);
      const int cloned =
          rewriter.cloneOperationTree(operationId, scope, scopeRegion,
                                      scopeBlock, scopeMap);
      rewriter.appendToBlock(scopeBlock, cloned);
      indexOperationTree(cloned);
      if (workspaceSet.count(operationId) != 0)
        migration.outputs.push_back({operationId, cloned});
      toErase.insert(operationId);
    }

    // Production assigns globalIRMap = scopeMap after cloning every work-item
    // scope.  Keep all mappings here as well; later work items may consume a
    // value cloned by an earlier item even when that value is not returned by
    // the scope.
    globalValues = scopeMap;

    std::vector<int> returned;
    std::vector<std::string> returnTypes;
    for (const auto &[value, ordinal] : returns) {
      (void)ordinal;
      returned.push_back(scopeMap.count(value) ? scopeMap.at(value) : value);
      returnTypes.push_back(valueType(value, "scope return"));
    }
    createInBlock(scope, scopeRegion, scopeBlock, "scope.return", {}, returned,
                  returnTypes);

    const GenericOperation &scopeOp =
        module.operations.at(static_cast<size_t>(scope));
    for (const auto &[value, ordinal] : returns) {
      if (ordinal < scopeOp.results.size())
        globalValues[value] = scopeOp.results[ordinal];
    }
    updateLoopYieldOperands(returns, scopeOp);
    workspaceMigrations.push_back(std::move(migration));
    return true;
  }

  int workspaceAllocationForOutput(int output) const {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(output));
    if (operation.dpsInits.empty())
      return -1;
    return getAllocWorkspace(operation.dpsInits.front());
  }

  int getAllocWorkspace(int value) const {
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return -1;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(definition->second));
    if (operation.name == "memref_ext.alloc_workspace")
      return operation.id;
    if ((operation.name == "bufferization.to_tensor" ||
         (IsDestinationStyleOp(operation.name) && !operation.dpsInits.empty())) &&
        !operation.operands.empty())
      return getAllocWorkspace(operation.name == "bufferization.to_tensor"
                                   ? operation.operands.front()
                                   : operation.dpsInits.front());
    return -1;
  }

  void indexOperationTree(int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (size_t i = 0; i < operation.results.size() &&
                       i < operation.resultTypes.size();
         ++i)
      valueTypes[operation.results[i]] = operation.resultTypes[i];
    for (int regionId : operation.regions) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(regionId));
      for (int blockId : region.blocks) {
        const GenericBlock &block =
            module.blocks.at(static_cast<size_t>(blockId));
        for (size_t i = 0; i < block.arguments.size() &&
                           i < block.argumentTypes.size();
             ++i)
          valueTypes[block.arguments[i]] = block.argumentTypes[i];
        for (int child : block.operations)
          indexOperationTree(child);
      }
    }
  }

  void materializeWorkspaceTensorSlices(int operationId, int scope,
                                        int scopeRegion, int scopeBlock,
                                        std::map<int, int> &scopeMap) {
    const GenericOperation &source =
        module.operations.at(static_cast<size_t>(operationId));
    for (size_t index = 0; index < source.operands.size() &&
                           index < source.operandTypes.size();
         ++index) {
      const int operand = source.operands[index];
      auto mapped = scopeMap.find(operand);
      if (mapped == scopeMap.end())
        continue;
      const std::string mappedType = valueType(mapped->second, "mapped workspace");
      const std::string &targetType = source.operandTypes[index];
      if (!startsWith(mappedType, "tensor<") ||
          !startsWith(targetType, "tensor<") || mappedType == targetType)
        continue;
      const int c0 = createInBlock(scope, scopeRegion, scopeBlock,
                                   "arith.constant", {"index"}, {}, {},
                                   "", "{value = 0 : index}");
      const int slice = createTensorExtract(scope, scopeRegion, scopeBlock,
                                            mapped->second, targetType,
                                            result(c0));
      scopeMap[operand] = result(slice);
    }
  }

  int createTensorExtract(int parent, int region, int block, int expanded,
                          const std::string &resultType, int iv) {
    const std::string expandedType = valueType(expanded, "preload tensor");
    const std::optional<MemRefTypeModel> tensor =
        ParseMemRefType("memref<" + resultType.substr(7));
    const size_t rank = tensor ? tensor->shape.size() : 1;
    std::ostringstream offsets, sizes, strides;
    offsets << "array<i64: -9223372036854775808";
    sizes << "array<i64: 1";
    strides << "array<i64: 1";
    for (size_t axis = 0; axis < rank; ++axis) {
      offsets << ", 0";
      sizes << ", " << (tensor->shape[axis] ? *tensor->shape[axis] : -1);
      strides << ", 1";
    }
    offsets << ">";
    sizes << ">";
    strides << ">";
    return createInBlock(
        parent, region, block, "tensor.extract_slice", {resultType},
        {expanded, iv}, {expandedType, "index"}, "",
        addPreloadWorkspaceAttr(
            "{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = " +
            offsets.str() + ", static_sizes = " + sizes.str() +
            ", static_strides = " + strides.str() + "}"));
  }

  std::string addPreloadWorkspaceAttr(std::string attributes) const {
    if (attributes.size() < 2)
      return "{hivm.preload_workspace}";
    attributes.insert(attributes.size() - 1, ", hivm.preload_workspace");
    return attributes;
  }

  void updateLoopYieldOperands(
      const std::vector<std::pair<int, size_t>> &returns,
      const GenericOperation &scopeOp) {
    const int body = loopBody();
    if (body < 0)
      return;
    GenericBlock &block = module.blocks.at(static_cast<size_t>(body));
    if (block.operations.empty())
      return;
    GenericOperation &yield =
        module.operations.at(static_cast<size_t>(block.operations.back()));
    if (yield.name != "scf.yield")
      return;
    for (const auto &[value, ordinal] : returns) {
      if (ordinal >= scopeOp.results.size())
        continue;
      for (int &operand : yield.operands)
        if (operand == value)
          operand = scopeOp.results[ordinal];
    }
  }

  struct WorkspaceMigration {
    int scope = -1;
    std::vector<std::pair<int, int>> outputs;
  };

  void migrateWorkspaceOutputs() {
    // Match CVPipelineImpl::migrateOpsForPreload: first every scope contains a
    // normal clone of the result-bearing Store/Fixpipe; only afterwards is
    // that clone replaced by a zero-result workspace store and its Load users
    // redirected through a to_tensor/extract_slice pair.
    for (const WorkspaceMigration &migration : workspaceMigrations) {
      if (migration.scope < 0)
        continue;
      const GenericOperation scopeSnapshot =
          module.operations.at(static_cast<size_t>(migration.scope));
      const int body = scopeSnapshot.blockId;
      GenericBlock &block = module.blocks.at(static_cast<size_t>(body));
      auto scopePosition =
          std::find(block.operations.begin(), block.operations.end(),
                    migration.scope);
      size_t insertion =
          scopePosition == block.operations.end()
              ? block.operations.size()
              : static_cast<size_t>(
                    std::distance(block.operations.begin(), scopePosition)) +
                    1;

      for (const auto &[originalOutput, clonedOutput] : migration.outputs) {
        const int allocation = workspaceAllocationForOutput(originalOutput);
        const auto expanded = expandedWorkspaceValue.find(allocation);
        if (expanded == expandedWorkspaceValue.end())
          continue;

        const GenericOperation storeSnapshot =
            module.operations.at(static_cast<size_t>(clonedOutput));
        if (storeSnapshot.operands.empty())
          continue;
        const int scopeBlock = storeSnapshot.blockId;
        const int scopeRegion = storeSnapshot.regionId;
        const int scopeParent = storeSnapshot.parentId;
        const std::string expandedType =
            valueType(expanded->second, "workspace");
        const std::string subviewType = CVPipelineSubviewType(expandedType);
        const std::string collapsedType =
            CVPipelineCollapsedWorkspaceType(expandedType);

        const int c0 = createInBlock(scopeParent, scopeRegion, scopeBlock,
                                     "arith.constant", {"index"}, {}, {}, "",
                                     "{value = 0 : index}");
        MoveOperationBefore(module, c0, clonedOutput);
        const int subview = createInBlock(
            scopeParent, scopeRegion, scopeBlock, "memref.subview",
            {subviewType}, {expanded->second, result(c0)},
            {expandedType, "index"}, "",
            addPreloadWorkspaceAttr(
                CVPipelineWorkspaceSubviewAttributes(expandedType)));
        MoveOperationBefore(module, subview, clonedOutput);
        const int collapsed = createInBlock(
            scopeParent, scopeRegion, scopeBlock, "memref.collapse_shape",
            {collapsedType}, {result(subview)}, {subviewType}, "",
            CVPipelineWorkspaceCollapseReassociation(expandedType));
        MoveOperationBefore(module, collapsed, clonedOutput);

        std::vector<int> storeOperands = storeSnapshot.operands;
        std::vector<std::string> storeOperandTypes = storeSnapshot.operandTypes;
        storeOperands.back() = result(collapsed);
        if (!storeOperandTypes.empty())
          storeOperandTypes.back() = collapsedType;
        const int replacementStore = createInBlock(
            scopeParent, scopeRegion, scopeBlock, storeSnapshot.name, {},
            storeOperands, storeOperandTypes, storeSnapshot.properties,
            storeSnapshot.attributes);
        MoveOperationBefore(module, replacementStore, clonedOutput);

        const std::string tensorType = "tensor<" + expandedType.substr(7);
        const int toTensor = rewriter.createOperation(
            analysis.loop, scopeSnapshot.regionId, body,
            "bufferization.to_tensor", {tensorType}, {expanded->second},
            {expandedType}, "", "{restrict}");
        rewriter.insertToBlock(body, insertion++, toTensor);
        valueTypes[result(toTensor)] = tensorType;

        for (size_t resultIndex = 0;
             resultIndex < storeSnapshot.results.size() &&
             resultIndex < storeSnapshot.resultTypes.size();
             ++resultIndex) {
          const int oldValue = storeSnapshot.results[resultIndex];
          const std::string &oldType = storeSnapshot.resultTypes[resultIndex];
          std::vector<std::pair<int, size_t>> loadUses;
          for (const GenericOperation &candidate : module.operations) {
            if (candidate.name != "hivm.hir.load")
              continue;
            for (size_t operandIndex = 0;
                 operandIndex < candidate.operands.size(); ++operandIndex) {
              if (candidate.operands[operandIndex] != oldValue)
                continue;
              loadUses.push_back({candidate.id, operandIndex});
            }
          }
          for (const auto &[loadId, operandIndex] : loadUses) {
            const GenericOperation loadSnapshot =
                module.operations.at(static_cast<size_t>(loadId));
            const int sliceIndex = createInBlock(
                loadSnapshot.parentId, loadSnapshot.regionId,
                loadSnapshot.blockId, "arith.constant", {"index"}, {}, {},
                "", "{value = 0 : index}");
            MoveOperationBefore(module, sliceIndex, loadId);
            const int slice = createTensorExtract(
                loadSnapshot.parentId, loadSnapshot.regionId,
                loadSnapshot.blockId, result(toTensor), oldType,
                result(sliceIndex));
            MoveOperationBefore(module, slice, loadId);
            rewriter.replaceOperand(loadId, operandIndex, result(slice));
          }
        }
        rewriter.removeFromBlock(scopeBlock, clonedOutput);
      }
    }
  }

  bool isAttached(int operationId) const {
    if (operationId < 0 ||
        static_cast<size_t>(operationId) >= module.operations.size())
      return false;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.blockId < 0 ||
        static_cast<size_t>(operation.blockId) >= module.blocks.size())
      return false;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(operation.blockId));
    return std::find(block.operations.begin(), block.operations.end(),
                     operationId) != block.operations.end();
  }

  int eraseOwnerForUser(int user) const {
    int current = user;
    while (current >= 0 && current != analysis.loop) {
      if (toErase.count(current) != 0)
        return current;
      current = module.operations.at(static_cast<size_t>(current)).parentId;
    }
    return -1;
  }

  int firstAttachedUser(int operationId) const {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int value : operation.results)
      for (const GenericOperation &candidate : module.operations) {
        if (!isAttached(candidate.id))
          continue;
        if (std::find(candidate.operands.begin(), candidate.operands.end(),
                      value) != candidate.operands.end())
          return candidate.id;
      }
    return -1;
  }

  bool removeOriginalOps(int body) {
    (void)body;
    // This is the same dependency-driven erase performed by
    // CVPipelineImpl::markScopesForPreload.  A rewrite that still has a user
    // outside toErase fails without deleting its producer; the pass driver
    // deliberately ignores that per-loop failure and continues.
    while (!toErase.empty()) {
      int eraseOp = *toErase.begin();
      while (true) {
        const int user = firstAttachedUser(eraseOp);
        if (user < 0) {
          const int block =
              module.operations.at(static_cast<size_t>(eraseOp)).blockId;
          rewriter.removeFromBlock(block, eraseOp);
          toErase.erase(eraseOp);
          break;
        }
        const int owner = eraseOwnerForUser(user);
        if (owner < 0)
          return false;
        eraseOp = owner;
      }
    }
    return true;
  }

  GenericModule &module;
  CVPipelineAnalysisResult analysis;
  GenericRewriter rewriter;
  std::map<int, int> definitions;
  std::map<int, std::string> valueTypes;
  std::map<int, std::vector<int>> users;
  std::map<int, int> expandedWorkspaceValue;
  std::map<int, int> globalValues;
  std::vector<WorkspaceMigration> workspaceMigrations;
  std::set<int> toErase;
};

} // namespace cvub

#endif
