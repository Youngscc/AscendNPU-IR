#ifndef CVPIPELINE_UB_MODEL_CPP_TILE_AND_BIND_SUB_BLOCK_HPP
#define CVPIPELINE_UB_MODEL_CPP_TILE_AND_BIND_SUB_BLOCK_HPP

#include "../analysis/hivm_dimension_analyzer.hpp"
#include "../ir/operation_folder.hpp"
#include "canonicalization_hivm_pipeline.hpp"
#include "split_mix_kernel.hpp"


namespace cvub {

// CanonicalizeAllocToTensor from TileUtils.cpp. This rewrite runs before
// collecting MIX functions and before every TileAndBindSubBlock early exit.
inline GenericModule RunTileAndBindSubBlockEarlyPatterns(
    GenericModule module) {
  std::map<int, std::vector<int>> users;
  for (const GenericOperation &operation : module.operations)
    for (int operand : operation.operands)
      users[operand].push_back(operation.id);

  GenericRewriter rewriter(module);
  for (const GenericOperation &allocationSnapshot : module.operations) {
    if (allocationSnapshot.name != "memref.alloc" ||
        allocationSnapshot.results.size() != 1 ||
        allocationSnapshot.blockId < 0)
      continue;
    const int allocationValue = allocationSnapshot.results.front();
    auto uses = users.find(allocationValue);
    if (uses == users.end() || uses->second.size() != 1)
      continue;
    GenericOperation &toTensor =
        module.operations.at(static_cast<size_t>(uses->second.front()));
    if (toTensor.name != "bufferization.to_tensor" ||
        toTensor.operands.size() != 1 || toTensor.results.size() != 1)
      continue;

    toTensor.name = "tensor.empty";
    toTensor.operands.clear();
    toTensor.operandTypes.clear();
    toTensor.properties.clear();
    toTensor.attributes.clear();
    toTensor.effects.clear();
    toTensor.dpsInputs.clear();
    toTensor.dpsInits.clear();
    rewriter.removeFromBlock(allocationSnapshot.blockId,
                             allocationSnapshot.id);
  }
  ApplyOperationSemanticsToAll(module.operations);
  return CompactGenericModule(std::move(module));
}

inline bool IsMixAIVFunction(const GenericOperation &operation) {
  return operation.name == "func.func" &&
         SplitMixEnumValue(FindDictionaryValue(
             operation.attributes, "hivm.func_core_type")) == "AIV" &&
         HasSplitMixDictionaryEntry(operation.attributes, "hivm.part_of_mix");
}

inline bool IsMixAICFunction(const GenericOperation &operation) {
  return operation.name == "func.func" &&
         SplitMixEnumValue(FindDictionaryValue(
             operation.attributes, "hivm.func_core_type")) == "AIC" &&
         HasSplitMixDictionaryEntry(operation.attributes, "hivm.part_of_mix");
}

inline bool IsCopyToL1ForTileAndBind(const GenericOperation &operation) {
  if (operation.name != "hivm.hir.copy" || operation.dpsInits.empty())
    return false;
  for (size_t index = 0; index < operation.operands.size(); ++index) {
    if (operation.operands[index] != operation.dpsInits.front() ||
        index >= operation.operandTypes.size())
      continue;
    const std::optional<MemRefTypeModel> type =
        ParseMemRefType(operation.operandTypes[index]);
    return type && type->addressSpace == AddressSpace::L1;
  }
  return false;
}

inline bool ShouldLimitUniqueSubBlock(const GenericOperation &operation) {
  // LimitUniqueSubBlockIdToStoreCopy deliberately leaves operations that were
  // successfully sliced by TileAndBindSubBlock alone: every sub-block owns a
  // different tile for those operations.  Only the remaining, unsliced
  // stores/copies must execute on sub-block 0.
  return !HasSplitMixDictionaryEntry(operation.attributes, "tiled_op") &&
         (operation.name == "hivm.hir.store" ||
          operation.name == "hivm.hir.indirect_store" ||
          IsCopyToL1ForTileAndBind(operation));
}

inline bool IsAlreadyLimitedToUniqueSubBlock(
    const GenericModule &module, const GenericOperation &operation) {
  if (operation.parentId < 0)
    return false;
  const GenericOperation &parent =
      module.operations.at(static_cast<size_t>(operation.parentId));
  return parent.name == "scf.if" &&
         HasSplitMixDictionaryEntry(parent.attributes,
                                    "limit_sub_block_id0");
}

inline std::vector<int> GetTileAndBindDescendants(
    const GenericModule &module, const GenericOperation &function) {
  std::vector<int> result;
  std::function<void(int)> collect = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          result.push_back(child);
          collect(child);
        }
  };
  collect(function.id);
  return result;
}

inline std::vector<int> GetTileAndBindGreedyRewriteOrder(
    const GenericModule &module, const GenericOperation &function) {
  std::vector<int> worklist;
  std::function<void(int)> collectPostOrder = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          collectPostOrder(child);
    worklist.push_back(operationId);
  };
  for (int regionId : function.regions)
    for (int blockId :
         module.regions.at(static_cast<size_t>(regionId)).blocks)
      for (int operationId :
           module.blocks.at(static_cast<size_t>(blockId)).operations)
        collectPostOrder(operationId);
  std::reverse(worklist.begin(), worklist.end());
  return worklist;
}

inline bool HasBatchMatmulLoopInAICFunctions(const GenericModule &module) {
  for (const GenericOperation &function : module.operations) {
    if (!IsMixAICFunction(function))
      continue;
    for (int operationId : GetTileAndBindDescendants(module, function)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name == "hivm.hir.mmadL1" &&
          HasSplitMixDictionaryEntry(operation.attributes, "batch_matmul"))
        return true;
    }
  }
  return false;
}

inline bool HasImplicitTransposeWithLastAxisInAIVFunctions(
    const GenericModule &module) {
  for (const GenericOperation &function : module.operations) {
    if (!IsMixAIVFunction(function))
      continue;
    for (int operationId : GetTileAndBindDescendants(module, function)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name == "annotation.mark" &&
          HasSplitMixDictionaryEntry(
              operation.attributes,
              "MayImplicitTransposeWithLastAxis"))
        return true;
    }
  }
  return false;
}

inline const GenericOperation *TraceTileAndBindReinterpretCast(
    int value,
    const std::map<int, const GenericOperation *> &definitions) {
  std::set<int> visited;
  while (visited.insert(value).second) {
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return nullptr;
    const GenericOperation *operation = definition->second;
    if (operation->name == "memref.reinterpret_cast")
      return operation;
    if ((operation->name != "memref.subview" &&
         operation->name != "memref.memory_space_cast" &&
         operation->name != "memref.cast") ||
        operation->operands.empty())
      return nullptr;
    value = operation->operands.front();
  }
  return nullptr;
}

inline bool AreLoadAndStoreSameAddress(const GenericModule &module) {
  const auto definitions = DefiningOperations(module);
  for (const GenericOperation &function : module.operations) {
    if (!IsMixAIVFunction(function) || function.regions.size() != 1)
      continue;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(function.regions.front()));
    if (region.blocks.empty())
      continue;
    const GenericBlock &entry =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    const std::set<int> arguments(entry.arguments.begin(), entry.arguments.end());
    std::set<int> loadedArguments;
    for (int operationId : GetTileAndBindDescendants(module, function)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "hivm.hir.load" || operation.operands.empty())
        continue;
      const GenericOperation *reinterpret = TraceTileAndBindReinterpretCast(
          operation.operands.front(), definitions);
      if (reinterpret && !reinterpret->operands.empty() &&
          arguments.count(reinterpret->operands.front()) != 0)
        loadedArguments.insert(reinterpret->operands.front());
    }
    for (int operationId : GetTileAndBindDescendants(module, function)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "hivm.hir.store" ||
          operation.operands.size() < 2)
        continue;
      const GenericOperation *reinterpret = TraceTileAndBindReinterpretCast(
          operation.operands[1], definitions);
      if (reinterpret && !reinterpret->operands.empty() &&
          loadedArguments.count(reinterpret->operands.front()) != 0)
        return true;
    }
  }
  return false;
}

inline bool VerifyTileAndBindReshapeTypes(const GenericModule &module) {
  const std::map<int, std::string> valueTypes = ValueTypes(module);
  for (const GenericOperation &operation : module.operations) {
    if ((operation.name != "tensor.collapse_shape" &&
         operation.name != "memref.collapse_shape" &&
         operation.name != "tensor.expand_shape" &&
         operation.name != "memref.expand_shape") ||
        operation.operands.empty() || operation.resultTypes.empty())
      continue;
    auto inputType = valueTypes.find(operation.operands.front());
    if (inputType == valueTypes.end())
      continue;
    const std::optional<ShapedTypeModel> input =
        ParseShapedTypeForDimensionAnalysis(inputType->second);
    const std::optional<ShapedTypeModel> output =
        ParseShapedTypeForDimensionAnalysis(operation.resultTypes.front());
    const std::vector<std::vector<int64_t>> reassociation =
        ParseReassociationIndices(operation.properties);
    if (!input || !output || reassociation.empty())
      continue;

    const bool collapse = operation.name.find("collapse_shape") !=
                          std::string::npos;
    const auto &expandedShape = collapse ? input->shape : output->shape;
    const auto &collapsedShape = collapse ? output->shape : input->shape;
    if (reassociation.size() != collapsedShape.size())
      return false;
    for (size_t collapsedAxis = 0; collapsedAxis < reassociation.size();
         ++collapsedAxis) {
      if (!collapsedShape[collapsedAxis])
        continue;
      int64_t product = 1;
      bool isStatic = true;
      for (int64_t expandedAxis : reassociation[collapsedAxis]) {
        if (expandedAxis < 0 ||
            static_cast<size_t>(expandedAxis) >= expandedShape.size())
          return false;
        const std::optional<int64_t> extent =
            expandedShape[static_cast<size_t>(expandedAxis)];
        if (!extent) {
          isStatic = false;
          break;
        }
        product *= *extent;
      }
      if (isStatic && product != *collapsedShape[collapsedAxis])
        return false;
    }
  }
  return true;
}

inline std::string ReplaceTileAndBindShapeDimension(
    const std::string &type, size_t axis,
    const std::optional<int64_t> &extent) {
  const size_t prefix = startsWith(type, "tensor<") ? 7 :
                        startsWith(type, "memref<") ? 7 : std::string::npos;
  if (prefix == std::string::npos)
    return type;
  const size_t close = findBalancedClose(type, prefix - 1);
  if (close == std::string::npos)
    throw std::runtime_error("TileAndBindSubBlock: malformed shaped type");
  const std::string body = type.substr(prefix, close - prefix);
  std::vector<std::string> fields = splitTopLevel(body);
  if (fields.empty())
    return type;
  std::vector<std::string> shapeAndElement =
      detail::SplitTypeTextAtTopLevel(fields.front(), 'x');
  if (shapeAndElement.size() < 2 || axis + 1 >= shapeAndElement.size())
    return type;
  shapeAndElement[axis] = extent ? std::to_string(*extent) : "?";
  std::string rebuilt;
  for (size_t index = 0; index < shapeAndElement.size(); ++index) {
    if (index != 0)
      rebuilt += "x";
    rebuilt += shapeAndElement[index];
  }
  fields.front() = rebuilt;
  std::string result = type.substr(0, prefix);
  for (size_t index = 0; index < fields.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += fields[index];
  }
  return result + type.substr(close);
}

inline std::string ReplaceTileAndBindShapeDimension(
    const std::string &type, size_t axis, int64_t extent) {
  return ReplaceTileAndBindShapeDimension(
      type, axis, std::optional<int64_t>(extent));
}

inline std::string AddTileAndBindUnitAttribute(
    const std::string &dictionary, const std::string &name) {
  if (HasSplitMixDictionaryEntry(dictionary, name))
    return dictionary;
  std::string result = trim(dictionary);
  if (result.empty() || result == "{}")
    return "{" + name + "}";
  const size_t close = result.rfind('}');
  if (close == std::string::npos)
    throw std::runtime_error("TileAndBindSubBlock: malformed attributes");
  result.insert(close, ", " + name);
  return result;
}

inline std::string SetTileAndBindDictionaryValue(
    const std::string &dictionary, const std::string &name,
    const std::string &value) {
  std::vector<std::string> entries;
  bool replaced = false;
  std::string body = dictionary;
  const bool properties = startsWith(body, "<{") && endsWith(body, "}>");
  if (properties)
    body = body.substr(1, body.size() - 2);
  if (body.size() >= 2 && body.front() == '{' && body.back() == '}') {
    entries = splitTopLevel(body.substr(1, body.size() - 2));
    for (std::string &entry : entries) {
      const size_t equal = entry.find('=');
      if (equal != std::string::npos && trim(entry.substr(0, equal)) == name) {
        entry = name + " = " + value;
        replaced = true;
      }
    }
  }
  if (!replaced)
    entries.push_back(name + " = " + value);
  std::string result = properties ? "<{" : "{";
  for (size_t index = 0; index < entries.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += trim(entries[index]);
  }
  return result + (properties ? "}>" : "}");
}

inline void SetTileAndBindProperty(GenericOperation &operation,
                                   const std::string &name,
                                   const std::string &value) {
  operation.properties =
      SetTileAndBindDictionaryValue(operation.properties, name, value);
  // The parser exposes inherent properties in the merged attribute view too.
  // A real PatternRewriter updates one Operation state, so keep both views in
  // sync whenever a TileAndBind rewrite changes an inherent property.
  operation.attributes =
      SetTileAndBindDictionaryValue(operation.attributes, name, value);
}

inline std::string TileAndBindStaticShape(
    const std::vector<std::optional<int64_t>> &shape) {
  std::string result = "array<i64:";
  for (size_t axis = 0; axis < shape.size(); ++axis) {
    if (axis != 0)
      result += ",";
    result += shape[axis] ? " " + std::to_string(*shape[axis]) :
                           " -9223372036854775808";
  }
  return result + ">";
}

inline std::string TileAndBindSliceProperties(
    const std::vector<std::optional<int64_t>> &shape, size_t axis) {
  std::string offsets = "array<i64:";
  std::string strides = "array<i64:";
  for (size_t index = 0; index < shape.size(); ++index) {
    if (index != 0) {
      offsets += ",";
      strides += ",";
    }
    offsets += index == axis ? " -9223372036854775808" : " 0";
    strides += " 1";
  }
  offsets += ">";
  strides += ">";
  return "{operandSegmentSizes = array<i32: 1, 1, 0, 0>, "
         "static_offsets = " + offsets + ", static_sizes = " +
         TileAndBindStaticShape(shape) + ", static_strides = " + strides +
         "}";
}

inline void ReplaceTileAndBindValueExcept(GenericModule &module, int from,
                                          int to, int exceptOperation) {
  for (GenericOperation &operation : module.operations) {
    if (operation.id == exceptOperation)
      continue;
    for (int &operand : operation.operands)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInputs)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInits)
      if (operand == from)
        operand = to;
  }
}

inline std::optional<int> FindTileAndBindIndexConstant(
    const GenericModule &module, int block, int64_t value) {
  return FindArithConstantValue(module, block, "index", std::to_string(value));
}

inline bool IsTileAndBindStoreCopyStartPoint(
    const GenericModule &module, const GenericOperation &operation,
    const DimensionAnalyzer &analyzer,
    const std::map<int, std::string> &valueTypes) {
  if (operation.name != "hivm.hir.store" &&
      operation.name != "hivm.hir.copy" &&
      operation.name != "hivm.hir.indirect_store")
    return false;
  if (operation.operands.empty() ||
      analyzer.getTilingDim(operation.operands.front()) < 0)
    return false;

  if (operation.name != "hivm.hir.indirect_store" &&
      !operation.results.empty()) {
    bool hasMarkUser = false;
    for (const GenericOperation &user : module.operations)
      if (user.name == "annotation.mark" &&
          std::any_of(operation.results.begin(), operation.results.end(),
                      [&](int result) {
                        return std::find(user.operands.begin(),
                                         user.operands.end(), result) !=
                               user.operands.end();
                      })) {
        hasMarkUser = true;
        break;
      }
    if (!hasMarkUser)
      return false;
  }

  auto sourceType = valueTypes.find(operation.operands.front());
  const std::optional<ShapedTypeModel> source =
      sourceType == valueTypes.end()
          ? std::nullopt
          : ParseShapedTypeForDimensionAnalysis(sourceType->second);
  if (!source)
    return false;
  if (operation.name == "hivm.hir.copy" && !source->tensor)
    return false;
  const size_t axis =
      static_cast<size_t>(analyzer.getTilingDim(operation.operands.front()));
  return axis < source->shape.size() &&
         (!source->shape[axis] || *source->shape[axis] >= 2);
}

// tileAndSliceOp creates marked slices at selected store/copy sources and at
// unused results of supported control-flow leaf operations. BubbleUpPattern
// can then reach tensor operands of those sources transitively.
inline std::map<int, size_t> CollectTileAndBindBubbleDims(
    const GenericModule &module, const GenericOperation &function,
    const DimensionAnalyzer &analyzer) {
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  const std::map<int, std::string> valueTypes = ValueTypes(module);
  std::map<int, size_t> dimensions;
  std::function<void(int, size_t)> collect = [&](int value, size_t axis) {
    auto inserted = dimensions.emplace(value, axis);
    if (!inserted.second)
      return;
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return;
    const GenericOperation &operation = *definition->second;

    // LoopBubbleUpStrategy rewrites one scf.for result at a time. The marked
    // slice is moved to the corresponding yielded value and init, while the
    // matching region iter_arg is narrowed to the sliced type. Preserve that
    // result-index correspondence here; walking every tensor init would tile
    // unrelated loop-carried values.
    if (operation.name == "scf.for") {
      // LoopBubbleUpStrategy rejects loops produced by ExtractLoadStore.
      // The marked slice remains immediately after that loop instead of
      // changing its result, init, iter_arg, or yielded value types.
      if (HasSplitMixDictionaryEntry(operation.attributes,
                                     "ExtractedLoadOrStore"))
        return;
      auto result =
          std::find(operation.results.begin(), operation.results.end(), value);
      if (result == operation.results.end() || operation.regions.empty())
        return;
      const size_t resultIndex = static_cast<size_t>(
          std::distance(operation.results.begin(), result));
      const size_t initBegin =
          operation.operands.size() >= operation.results.size()
              ? operation.operands.size() - operation.results.size()
              : operation.operands.size();
      auto collectLoopValue = [&](int loopValue) {
        int64_t loopAxis = analyzer.getTilingDim(loopValue);
        collect(loopValue,
                loopAxis >= 0 ? static_cast<size_t>(loopAxis) : axis);
      };
      if (initBegin + resultIndex < operation.operands.size())
        collectLoopValue(operation.operands[initBegin + resultIndex]);

      const GenericRegion &region = module.regions.at(
          static_cast<size_t>(operation.regions.front()));
      if (!region.blocks.empty()) {
        const GenericBlock &block =
            module.blocks.at(static_cast<size_t>(region.blocks.front()));
        if (resultIndex + 1 < block.arguments.size())
          collectLoopValue(block.arguments[resultIndex + 1]);
        if (!block.operations.empty()) {
          const GenericOperation &terminator = module.operations.at(
              static_cast<size_t>(block.operations.back()));
          if (terminator.name == "scf.yield" &&
              resultIndex < terminator.operands.size())
            collectLoopValue(terminator.operands[resultIndex]);
        }
      }
      return;
    }

    const std::vector<int64_t> permutation =
        operation.name == "hivm.hir.vtranspose"
            ? ParseDimensionI64Array(operation.properties, "permutation")
            : std::vector<int64_t>{};
    for (size_t index = 0; index < operation.operands.size(); ++index) {
      const int operand = operation.operands[index];
      auto type = valueTypes.find(operand);
      const std::optional<ShapedTypeModel> shaped =
          type == valueTypes.end()
              ? std::nullopt
              : ParseShapedTypeForDimensionAnalysis(type->second);
      if (!shaped || !shaped->tensor)
        continue;
      int64_t operandAxis = analyzer.getTilingDim(operand);
      // VTransposeBubbleUpStrategy maps dim(dst, i) to dim(src, perm[i]).
      // The destination/init operand remains in destination coordinates.
      if (index == 0 && !permutation.empty() && axis < permutation.size())
        operandAxis = permutation[axis];
      if (operandAxis >= 0 &&
          static_cast<size_t>(operandAxis) < shaped->shape.size())
        collect(operand, static_cast<size_t>(operandAxis));
    }
  };

  for (int operationId : GetTileAndBindDescendants(module, function)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (IsTileAndBindStoreCopyStartPoint(module, operation, analyzer,
                                         valueTypes)) {
      const int64_t axis =
          analyzer.getTilingDim(operation.operands.front());
      collect(operation.operands.front(), static_cast<size_t>(axis));
    }
  }

  // Mirrors TileAndSliceLeaf<scf::ForOp>. A result is a leaf only when it has
  // no users before the temporary extract_slice/annotation pair is inserted.
  for (int operationId : GetTileAndBindDescendants(module, function)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "scf.for")
      continue;
    for (size_t resultIndex = 0;
         resultIndex < operation.results.size() &&
         resultIndex < operation.resultTypes.size();
         ++resultIndex) {
      const int result = operation.results[resultIndex];
      bool hasUser = false;
      for (const GenericOperation &user : module.operations)
        if (std::find(user.operands.begin(), user.operands.end(), result) !=
            user.operands.end()) {
          hasUser = true;
          break;
        }
      if (hasUser)
        continue;
      const auto type = ParseShapedTypeForDimensionAnalysis(
          operation.resultTypes[resultIndex]);
      const int64_t axis = analyzer.getTilingDim(result);
      if (!type || !type->tensor || axis < 0 ||
          static_cast<size_t>(axis) >= type->shape.size() ||
          !type->shape[static_cast<size_t>(axis)] ||
          *type->shape[static_cast<size_t>(axis)] < 2)
        continue;
      collect(result, static_cast<size_t>(axis));
    }
  }
  return dimensions;
}

inline std::vector<int> TileAndBindValueUsers(const GenericModule &module,
                                              int value) {
  std::vector<int> users;
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.operands.begin(), operation.operands.end(), value) !=
        operation.operands.end())
      users.push_back(operation.id);
  return users;
}

// ReplicateEmptyOutPattern from ReplicateOutEmptyTensor.cpp. Only multiple
// destination-style init uses are cloned; non-init uses do not participate in
// the rewrite's threshold or replacement set.
inline void RunReplicateOutEmptyTensor(GenericModule &module,
                                       int functionId) {
  ApplyOperationSemanticsToAll(module.operations);
  const GenericOperation function =
      module.operations.at(static_cast<size_t>(functionId));
  const std::vector<int> descendants =
      GetTileAndBindDescendants(module, function);
  GenericRewriter rewriter(module);
  for (int operationId : descendants) {
    const GenericOperation empty =
        module.operations.at(static_cast<size_t>(operationId));
    if (empty.name != "tensor.empty" || empty.results.size() != 1 ||
        empty.blockId < 0)
      continue;
    const int value = empty.results.front();
    size_t totalUses = 0;
    std::vector<std::pair<int, size_t>> initUses;
    for (const GenericOperation &user : module.operations) {
      const std::vector<size_t> initIndices = DpsInitOperandIndices(
          user.name, user.operands.size(), user.properties);
      const std::set<size_t> initSet(initIndices.begin(), initIndices.end());
      for (size_t operandIndex = 0; operandIndex < user.operands.size();
           ++operandIndex) {
        if (user.operands[operandIndex] != value)
          continue;
        ++totalUses;
        if (initSet.count(operandIndex) != 0)
          initUses.emplace_back(user.id, operandIndex);
      }
    }
    if (totalUses <= 1 || initUses.size() <= 1)
      continue;

    size_t insertion = static_cast<size_t>(empty.ordinal);
    for (const auto &[userId, operandIndex] : initUses) {
      const int clone = rewriter.cloneOperation(
          empty.id, empty.parentId, empty.regionId, empty.blockId, {});
      rewriter.insertToBlock(empty.blockId, insertion++, clone);
      const int clonedValue =
          module.operations.at(static_cast<size_t>(clone)).results.front();
      module.operations.at(static_cast<size_t>(userId))
          .operands[operandIndex] = clonedValue;
    }
  }
  ApplyOperationSemanticsToAll(module.operations);
}

inline void MoveTiledEmptyToDpsInitUser(GenericModule &module,
                                        int emptyOperation) {
  GenericOperation &empty =
      module.operations.at(static_cast<size_t>(emptyOperation));
  if (empty.name != "tensor.empty" || empty.results.size() != 1 ||
      empty.blockId < 0)
    return;
  const int value = empty.results.front();
  std::vector<int> initUsers;
  for (const GenericOperation &user : module.operations) {
    const std::vector<size_t> initIndices = DpsInitOperandIndices(
        user.name, user.operands.size(), user.properties);
    if (std::any_of(initIndices.begin(), initIndices.end(), [&](size_t index) {
          return index < user.operands.size() && user.operands[index] == value;
        }))
      initUsers.push_back(user.id);
  }
  if (initUsers.size() != 1)
    return;
  GenericOperation &user =
      module.operations.at(static_cast<size_t>(initUsers.front()));
  if (user.blockId < 0 || user.blockId != empty.blockId)
    return;
  GenericRewriter rewriter(module);
  rewriter.removeFromBlock(empty.blockId, emptyOperation);
  rewriter.insertToBlock(user.blockId, static_cast<size_t>(user.ordinal),
                         emptyOperation);
}

inline bool HasTileAndBindUnitStride(const GenericOperation &operation) {
  const std::vector<int64_t> strides =
      ParseDimensionI64Array(operation.properties, "static_strides");
  return !strides.empty() &&
         std::all_of(strides.begin(), strides.end(),
                     [](int64_t stride) { return stride == 1; });
}

struct TileAndBindFoldResult {
  std::optional<int64_t> constant;
  int value = -1;
};

struct TileAndBindMixedSlice {
  std::vector<int> prefixOperands;
  std::vector<std::string> prefixOperandTypes;
  std::vector<size_t> prefixSegments;
  std::vector<TileAndBindFoldResult> offsets;
  std::vector<TileAndBindFoldResult> sizes;
  std::vector<TileAndBindFoldResult> strides;
};

inline std::optional<TileAndBindMixedSlice>
ParseTileAndBindMixedSlice(const GenericOperation &operation) {
  const std::vector<size_t> segments =
      OperandSegmentSizes(operation.properties);
  const std::vector<int64_t> staticOffsets =
      ParseDimensionI64Array(operation.properties, "static_offsets");
  const std::vector<int64_t> staticSizes =
      ParseDimensionI64Array(operation.properties, "static_sizes");
  const std::vector<int64_t> staticStrides =
      ParseDimensionI64Array(operation.properties, "static_strides");
  if ((segments.size() != 4 && segments.size() != 5) ||
      staticOffsets.size() != staticSizes.size() ||
      staticSizes.size() != staticStrides.size() || segments.size() < 4 ||
      operation.operands.size() !=
          std::accumulate(segments.begin(), segments.end(), size_t{0}))
    return std::nullopt;

  TileAndBindMixedSlice slice;
  const size_t prefixSegmentCount = segments.size() - 3;
  slice.prefixSegments.assign(segments.begin(),
                              segments.begin() +
                                  static_cast<std::ptrdiff_t>(prefixSegmentCount));
  const size_t prefixOperandCount = std::accumulate(
      slice.prefixSegments.begin(), slice.prefixSegments.end(), size_t{0});
  if (prefixOperandCount == 0)
    return std::nullopt;
  slice.prefixOperands.assign(operation.operands.begin(),
                              operation.operands.begin() +
                                  static_cast<std::ptrdiff_t>(prefixOperandCount));
  slice.prefixOperandTypes.assign(operation.operandTypes.begin(),
                                  operation.operandTypes.begin() +
                                      static_cast<std::ptrdiff_t>(std::min(
                                          prefixOperandCount,
                                          operation.operandTypes.size())));
  size_t dynamicOperand = prefixOperandCount;
  auto parse = [&](const std::vector<int64_t> &values,
                   std::vector<TileAndBindFoldResult> &results) {
    for (int64_t value : values) {
      if (value != kCanonicalizationDynamicIndex) {
        results.push_back({value, -1});
        continue;
      }
      if (dynamicOperand >= operation.operands.size())
        return false;
      results.push_back({std::nullopt,
                         operation.operands[dynamicOperand++]});
    }
    return true;
  };
  if (!parse(staticOffsets, slice.offsets) ||
      !parse(staticSizes, slice.sizes) ||
      !parse(staticStrides, slice.strides) ||
      dynamicOperand != operation.operands.size())
    return std::nullopt;
  return slice;
}

inline bool IsRankReducedTileAndBindSliceType(
    const std::vector<TileAndBindFoldResult> &sizes,
    const std::vector<std::optional<int64_t>> &shape) {
  std::vector<std::vector<int8_t>> memo(
      sizes.size() + 1, std::vector<int8_t>(shape.size() + 1, -1));
  std::function<bool(size_t, size_t)> matches =
      [&](size_t sizeIndex, size_t shapeIndex) {
        int8_t &cached = memo[sizeIndex][shapeIndex];
        if (cached >= 0)
          return cached != 0;
        if (sizeIndex == sizes.size())
          return (cached = shapeIndex == shape.size()) != 0;

        bool result = false;
        if (shapeIndex < shape.size() &&
            (!sizes[sizeIndex].constant || !shape[shapeIndex] ||
             *sizes[sizeIndex].constant == *shape[shapeIndex]))
          result = matches(sizeIndex + 1, shapeIndex + 1);
        if (!result && sizes[sizeIndex].constant &&
            *sizes[sizeIndex].constant == 1)
          result = matches(sizeIndex + 1, shapeIndex);
        cached = result ? 1 : 0;
        return result;
      };
  return matches(0, 0);
}

// Projection of the OffsetSizeAndStrideOpInterface verifiers reached by
// newFunc.verify() in TileAndBindSubBlockPass::attemptBindSubBlock.
inline bool VerifyTileAndBindSliceTypes(const GenericModule &module) {
  const std::map<int, std::string> valueTypes = ValueTypes(module);
  for (const GenericOperation &operation : module.operations) {
    const bool isInsert = operation.name == "tensor.insert_slice";
    const bool isExtract = operation.name == "tensor.extract_slice" ||
                           operation.name == "memref.subview";
    if (!isInsert && !isExtract)
      continue;
    const std::optional<TileAndBindMixedSlice> slice =
        ParseTileAndBindMixedSlice(operation);
    if (!slice || slice->prefixOperands.empty() ||
        (isExtract && operation.results.empty()))
      return false;

    const int shapedValue = isInsert ? slice->prefixOperands.front()
                                     : operation.results.front();
    auto type = valueTypes.find(shapedValue);
    if (type == valueTypes.end())
      return false;
    const std::optional<ShapedTypeModel> shaped =
        ParseShapedTypeForDimensionAnalysis(type->second);
    if (!shaped || !IsRankReducedTileAndBindSliceType(slice->sizes,
                                                       shaped->shape))
      return false;
  }
  return true;
}

inline std::string TileAndBindFoldResultArray(
    const std::vector<TileAndBindFoldResult> &values) {
  std::vector<std::optional<int64_t>> staticValues;
  staticValues.reserve(values.size());
  for (const TileAndBindFoldResult &value : values)
    staticValues.push_back(value.constant);
  return TileAndBindStaticShape(staticValues);
}

inline void SetTileAndBindMixedSlice(GenericOperation &operation,
                                     const TileAndBindMixedSlice &slice) {
  operation.operands = slice.prefixOperands;
  operation.operandTypes = slice.prefixOperandTypes;
  std::vector<size_t> segments = slice.prefixSegments;
  segments.resize(segments.size() + 3, 0);
  const size_t offsetSegment = segments.size() - 3;
  const size_t sizeSegment = segments.size() - 2;
  const size_t strideSegment = segments.size() - 1;
  auto append = [&](const std::vector<TileAndBindFoldResult> &values,
                    size_t segment) {
    for (const TileAndBindFoldResult &value : values) {
      if (value.constant)
        continue;
      operation.operands.push_back(value.value);
      operation.operandTypes.push_back("index");
      ++segments[segment];
    }
  };
  append(slice.offsets, offsetSegment);
  append(slice.sizes, sizeSegment);
  append(slice.strides, strideSegment);
  std::string segmentText = "array<i32:";
  for (size_t segment : segments)
    segmentText += " " + std::to_string(segment) + ",";
  segmentText.back() = '>';
  SetTileAndBindProperty(operation, "operandSegmentSizes", segmentText);
  SetTileAndBindProperty(operation, "static_offsets",
                         TileAndBindFoldResultArray(slice.offsets));
  SetTileAndBindProperty(operation, "static_sizes",
                         TileAndBindFoldResultArray(slice.sizes));
  SetTileAndBindProperty(operation, "static_strides",
                         TileAndBindFoldResultArray(slice.strides));
}

inline std::set<size_t> GetTileAndBindExtractOrInsertDims(
    const GenericModule &module, const GenericOperation &operation) {
  const std::optional<TileAndBindMixedSlice> slice =
      ParseTileAndBindMixedSlice(operation);
  if (!slice || slice->prefixOperands.empty())
    return {};
  const std::map<int, std::string> valueTypes = ValueTypes(module);
  auto sourceType = valueTypes.find(slice->prefixOperands.front());
  const std::optional<ShapedTypeModel> source =
      sourceType == valueTypes.end()
          ? std::nullopt
          : ParseShapedTypeForDimensionAnalysis(sourceType->second);
  if (!source || source->shape.size() != slice->sizes.size())
    return {};
  std::set<size_t> dimensions;
  for (size_t dimension = 0; dimension < slice->sizes.size(); ++dimension) {
    const TileAndBindFoldResult &size = slice->sizes[dimension];
    if (!size.constant || !source->shape[dimension] ||
        *size.constant != *source->shape[dimension])
      dimensions.insert(dimension);
  }
  return dimensions;
}

// Projection of TileAndBindSubBlock/Helper.cpp::createdByTiling for the
// normalized 1:2 loop used by TileAndBindSubBlock.
inline bool CreatedByTileAndBindTiling(const GenericModule &module,
                                       const GenericOperation &sliceOperation) {
  const std::optional<TileAndBindMixedSlice> slice =
      ParseTileAndBindMixedSlice(sliceOperation);
  const std::set<size_t> dimensions =
      GetTileAndBindExtractOrInsertDims(module, sliceOperation);
  if (!slice || slice->prefixOperands.empty() || dimensions.size() != 1)
    return false;
  const size_t tilingDimension = *dimensions.begin();
  if (tilingDimension >= slice->offsets.size() ||
      tilingDimension >= slice->sizes.size() ||
      std::any_of(slice->strides.begin(), slice->strides.end(),
                  [](const TileAndBindFoldResult &stride) {
                    return !stride.constant || *stride.constant != 1;
                  }))
    return false;

  const std::map<int, std::string> valueTypes = ValueTypes(module);
  auto sourceType = valueTypes.find(slice->prefixOperands.front());
  const std::optional<ShapedTypeModel> source =
      sourceType == valueTypes.end()
          ? std::nullopt
          : ParseShapedTypeForDimensionAnalysis(sourceType->second);
  if (!source || tilingDimension >= source->shape.size() ||
      !source->shape[tilingDimension])
    return false;
  for (size_t dimension = 0; dimension < source->shape.size(); ++dimension) {
    if (!source->shape[dimension] || dimension >= slice->sizes.size() ||
        (dimension != tilingDimension &&
         (!slice->sizes[dimension].constant ||
          *slice->sizes[dimension].constant != *source->shape[dimension])))
      return false;
  }

  const GenericOperation *tilingLoop = nullptr;
  int parent = sliceOperation.parentId;
  while (parent >= 0) {
    const GenericOperation &ancestor =
        module.operations.at(static_cast<size_t>(parent));
    if (ancestor.name == "scf.for") {
      tilingLoop = &ancestor;
      break;
    }
    parent = ancestor.parentId;
  }
  if (!tilingLoop || tilingLoop->operands.size() < 3 ||
      tilingLoop->regions.empty())
    return false;

  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  const auto constant = [&](int value) -> std::optional<int64_t> {
    auto definition = definitions.find(value);
    if (definition == definitions.end() ||
        definition->second->name != "arith.constant")
      return std::nullopt;
    std::string text =
        FindDictionaryValue(definition->second->properties, "value");
    if (text.empty())
      text = FindDictionaryValue(definition->second->attributes, "value");
    const size_t suffix = text.find(" : ");
    if (suffix != std::string::npos)
      text = trim(text.substr(0, suffix));
    try {
      size_t consumed = 0;
      const int64_t result = std::stoll(text, &consumed, 0);
      return consumed == text.size() ? std::optional<int64_t>(result)
                                     : std::nullopt;
    } catch (const std::exception &) {
      return std::nullopt;
    }
  };
  const std::optional<int64_t> lowerBound = constant(tilingLoop->operands[0]);
  const std::optional<int64_t> upperBound = constant(tilingLoop->operands[1]);
  const std::optional<int64_t> step = constant(tilingLoop->operands[2]);
  if (!lowerBound || !upperBound || !step || *lowerBound != 0 || *step != 1 ||
      *upperBound <= 0 ||
      *source->shape[tilingDimension] % *upperBound != 0)
    return false;
  const int64_t tileSize =
      *source->shape[tilingDimension] / *upperBound;
  if (!slice->sizes[tilingDimension].constant ||
      *slice->sizes[tilingDimension].constant != tileSize)
    return false;
  for (size_t dimension = 0; dimension < slice->offsets.size(); ++dimension) {
    if (dimension == tilingDimension)
      continue;
    if (!slice->offsets[dimension].constant ||
        *slice->offsets[dimension].constant != 0)
      return false;
  }
  if (slice->offsets[tilingDimension].constant)
    return false;
  auto offsetDefinition =
      definitions.find(slice->offsets[tilingDimension].value);
  if (offsetDefinition == definitions.end() ||
      offsetDefinition->second->name != "affine.apply" ||
      offsetDefinition->second->operands.size() != 1)
    return false;

  const GenericRegion &loopRegion = module.regions.at(
      static_cast<size_t>(tilingLoop->regions.front()));
  if (loopRegion.blocks.empty())
    return false;
  const GenericBlock &loopBlock =
      module.blocks.at(static_cast<size_t>(loopRegion.blocks.front()));
  if (loopBlock.arguments.empty() ||
      offsetDefinition->second->operands.front() != loopBlock.arguments.front())
    return false;
  const std::optional<std::string> expression =
      ExistingAffineApplyExpression(*offsetDefinition->second);
  if (!expression)
    return false;
  const std::string induction =
      AffineValueExpression(loopBlock.arguments.front());
  const std::string size = "c(" + std::to_string(tileSize) + ")";
  return *expression == MakeAffineBinaryExpression("mul", induction, size) ||
         *expression == MakeAffineBinaryExpression("mul", size, induction);
}

// Projection of TileAndBindSubBlock/Helper.cpp::handleExtractOfExtract.
// Keep the arith chain intact here; the real bubble-up pipeline canonicalizes
// it only after both intersecting slices have been materialized.
inline bool HandleTileAndBindExtractOfExtract(
    GenericModule &module, GenericRewriter &rewriter, int sliceId,
    size_t tilingDim, int tiledOffset, int64_t tiledSize) {
  const GenericOperation snapshot =
      module.operations.at(static_cast<size_t>(sliceId));
  std::optional<TileAndBindMixedSlice> parsed =
      ParseTileAndBindMixedSlice(snapshot);
  if (!parsed || tilingDim >= parsed->offsets.size())
    return false;
  TileAndBindMixedSlice slice = std::move(*parsed);
  size_t insertionPosition = static_cast<size_t>(snapshot.ordinal);

  auto materialize = [&](const TileAndBindFoldResult &foldResult) {
    if (!foldResult.constant)
      return foldResult.value;
    if (std::optional<int> existing = FindTileAndBindIndexConstant(
            module, snapshot.blockId, *foldResult.constant))
      return *existing;
    const int constant = rewriter.createOperation(
        snapshot.parentId, snapshot.regionId, snapshot.blockId,
        "arith.constant", {"index"}, {}, {},
        "{value = " + std::to_string(*foldResult.constant) + " : index}");
    rewriter.insertToBlock(snapshot.blockId, insertionPosition++, constant);
    return module.operations.at(static_cast<size_t>(constant)).results.front();
  };
  auto createBinary = [&](const std::string &name, int lhs, int rhs) {
    const std::string properties =
        name == "arith.addi" || name == "arith.subi"
            ? "{overflowFlags = #arith.overflow<none>}"
            : "";
    const int operation = rewriter.createOperation(
        snapshot.parentId, snapshot.regionId, snapshot.blockId, name,
        {"index"}, {lhs, rhs}, {"index", "index"}, properties);
    rewriter.insertToBlock(snapshot.blockId, insertionPosition++, operation);
    return module.operations.at(static_cast<size_t>(operation)).results.front();
  };

  int lowerBound = tiledOffset;
  const int upperBound =
      materialize(TileAndBindFoldResult{tiledSize, -1});
  int currentLowerBound = materialize(slice.offsets[tilingDim]);
  int currentUpperBound = materialize(slice.sizes[tilingDim]);
  if (slice.offsets[tilingDim].constant &&
      *slice.offsets[tilingDim].constant == 0) {
    lowerBound = createBinary("arith.minsi", lowerBound,
                              currentUpperBound);
    currentUpperBound = createBinary("arith.subi", currentUpperBound,
                                     lowerBound);
    currentUpperBound = createBinary("arith.minsi", currentUpperBound,
                                     upperBound);
    slice.sizes[tilingDim] = {std::nullopt, currentUpperBound};
    SetTileAndBindMixedSlice(
        module.operations.at(static_cast<size_t>(sliceId)), slice);
    return true;
  }

  int tiledUpperBound =
      createBinary("arith.addi", lowerBound, upperBound);
  currentUpperBound = createBinary("arith.addi", currentLowerBound,
                                   currentUpperBound);
  currentLowerBound = createBinary("arith.maxsi", currentLowerBound,
                                   lowerBound);
  currentUpperBound = createBinary("arith.minsi", currentUpperBound,
                                   tiledUpperBound);
  currentUpperBound = createBinary("arith.maxsi", currentLowerBound,
                                   currentUpperBound);
  currentUpperBound = createBinary("arith.subi", currentUpperBound,
                                   currentLowerBound);
  currentLowerBound = createBinary("arith.subi", currentLowerBound,
                                   lowerBound);
  slice.offsets[tilingDim] = {std::nullopt, currentLowerBound};
  slice.sizes[tilingDim] = {std::nullopt, currentUpperBound};
  SetTileAndBindMixedSlice(
      module.operations.at(static_cast<size_t>(sliceId)), slice);
  return true;
}

// UB-visible projection of
// InsertSliceBubbleUpStrategy::handleExtractInsertExtractCase. The real
// rewrite creates tiled source/destination slices, then rebuilds the original
// extract/insert pair with a min(sub(size, offset), tileSize) extent. The
// surrounding lightweight bubble-up already represents those tiled values;
// this function preserves the exact mixed-size SSA and shaped-type semantics
// consumed by OneShotBufferize.
inline bool HandleTileAndBindExtractInsertExtractCase(
    GenericModule &module, GenericRewriter &rewriter, int insertSliceId,
    int sourceExtractId, size_t tilingDim, int tiledOffset, int64_t tiledSize,
    std::map<int, std::string> &valueTypes) {
  const GenericOperation sourceExtractSnapshot =
      module.operations.at(static_cast<size_t>(sourceExtractId));
  const GenericOperation insertSliceSnapshot =
      module.operations.at(static_cast<size_t>(insertSliceId));
  if (sourceExtractSnapshot.name != "tensor.extract_slice" ||
      insertSliceSnapshot.name != "tensor.insert_slice" ||
      sourceExtractSnapshot.results.size() != 1 ||
      insertSliceSnapshot.results.size() != 1)
    return false;

  if (!HandleTileAndBindExtractOfExtract(module, rewriter, sourceExtractId,
                                         tilingDim, tiledOffset, tiledSize))
    return false;
  GenericOperation &sourceExtract =
      module.operations.at(static_cast<size_t>(sourceExtractId));
  GenericOperation &insertSlice =
      module.operations.at(static_cast<size_t>(insertSliceId));
  std::optional<TileAndBindMixedSlice> sourceSlice =
      ParseTileAndBindMixedSlice(sourceExtract);
  std::optional<TileAndBindMixedSlice> destinationSlice =
      ParseTileAndBindMixedSlice(insertSlice);
  if (!sourceSlice || !destinationSlice ||
      tilingDim >= sourceSlice->sizes.size() ||
      tilingDim >= destinationSlice->sizes.size())
    return false;

  destinationSlice->sizes[tilingDim] = sourceSlice->sizes[tilingDim];
  SetTileAndBindMixedSlice(insertSlice, *destinationSlice);
  sourceExtract.resultTypes.front() = ReplaceTileAndBindShapeDimension(
      sourceExtract.resultTypes.front(), tilingDim, std::nullopt);
  valueTypes[sourceExtract.results.front()] =
      sourceExtract.resultTypes.front();
  return true;
}

// Mirrors BufferizationBubbleUpStrategy pattern 2: an extract_slice of a
// to_tensor backed by alloc/subview/load is pushed through the load, and the
// local allocation itself becomes the per-sub-block allocation.
inline bool RunBufferizationBubbleUpStrategy(
    GenericModule &module, GenericRewriter &rewriter, int toTensorId,
    int sliceId, size_t tilingDim, int64_t tileSize, int offsetValue,
    std::map<int, std::string> &valueTypes) {
  const GenericOperation toTensorSnapshot =
      module.operations.at(static_cast<size_t>(toTensorId));
  const GenericOperation sliceSnapshot =
      module.operations.at(static_cast<size_t>(sliceId));
  if (toTensorSnapshot.name != "bufferization.to_tensor" ||
      toTensorSnapshot.operands.size() != 1 ||
      toTensorSnapshot.results.size() != 1 ||
      sliceSnapshot.name != "tensor.extract_slice" ||
      sliceSnapshot.results.size() != 1)
    return false;

  const auto definitions = DefiningOperations(module);
  auto allocDefinition = definitions.find(toTensorSnapshot.operands.front());
  if (allocDefinition == definitions.end() ||
      allocDefinition->second->name != "memref.alloc" ||
      allocDefinition->second->results.size() != 1 ||
      allocDefinition->second->resultTypes.size() != 1)
    return false;

  const int allocId = allocDefinition->second->id;
  const int allocValue = allocDefinition->second->results.front();
  const std::vector<int> allocUsers = TileAndBindValueUsers(module, allocValue);
  for (int loadId : allocUsers) {
    const GenericOperation loadSnapshot =
        module.operations.at(static_cast<size_t>(loadId));
    if (loadSnapshot.name != "hivm.hir.load" ||
        loadSnapshot.operands.size() < 2 ||
        loadSnapshot.operands[1] != allocValue)
      continue;
    auto sourceDefinition = definitions.find(loadSnapshot.operands.front());
    if (sourceDefinition == definitions.end() ||
        sourceDefinition->second->name != "memref.reinterpret_cast" ||
        sourceDefinition->second->resultTypes.empty())
      continue;
    const std::optional<ShapedTypeModel> sourceType =
        ParseShapedTypeForDimensionAnalysis(
            sourceDefinition->second->resultTypes.front());
    if (!sourceType || tilingDim >= sourceType->shape.size() ||
        !sourceType->shape[tilingDim])
      continue;

    std::vector<std::optional<int64_t>> tileShape = sourceType->shape;
    tileShape[tilingDim] = tileSize;
    const std::string tiledSourceType = ReplaceTileAndBindShapeDimension(
        sourceDefinition->second->resultTypes.front(), tilingDim, tileSize);
    const int sourceSubview = rewriter.createOperation(
        loadSnapshot.parentId, loadSnapshot.regionId, loadSnapshot.blockId,
        "memref.subview", {tiledSourceType},
        {loadSnapshot.operands.front(), offsetValue},
        {sourceDefinition->second->resultTypes.front(), "index"},
        TileAndBindSliceProperties(tileShape, tilingDim));
    rewriter.insertToBlock(loadSnapshot.blockId,
                           static_cast<size_t>(loadSnapshot.ordinal),
                           sourceSubview);
    const int sourceSubviewValue =
        module.operations.at(static_cast<size_t>(sourceSubview)).results.front();
    module.operations.at(static_cast<size_t>(loadId)).operands[0] =
        sourceSubviewValue;

    GenericOperation &alloc =
        module.operations.at(static_cast<size_t>(allocId));
    const std::string tiledAllocType = ReplaceTileAndBindShapeDimension(
        alloc.resultTypes.front(), tilingDim, tileSize);
    alloc.resultTypes.front() = tiledAllocType;
    valueTypes[allocValue] = tiledAllocType;

    GenericOperation &toTensor =
        module.operations.at(static_cast<size_t>(toTensorId));
    const std::string tiledTensorType = ReplaceTileAndBindShapeDimension(
        toTensor.resultTypes.front(), tilingDim, tileSize);
    toTensor.resultTypes.front() = tiledTensorType;
    valueTypes[toTensor.results.front()] = tiledTensorType;
    valueTypes[sourceSubviewValue] = tiledSourceType;

    ReplaceTileAndBindValueExcept(module, sliceSnapshot.results.front(),
                                  toTensor.results.front(), sliceId);
    rewriter.removeFromBlock(sliceSnapshot.blockId, sliceId);
    return true;
  }

  for (int subviewId : allocUsers) {
    const GenericOperation destinationSubview =
        module.operations.at(static_cast<size_t>(subviewId));
    if (destinationSubview.name != "memref.subview" ||
        destinationSubview.results.size() != 1 ||
        destinationSubview.operands.empty())
      continue;

    const std::vector<int> subviewUsers =
        TileAndBindValueUsers(module, destinationSubview.results.front());
    if (subviewUsers.size() != 1)
      continue;
    const GenericOperation load =
        module.operations.at(static_cast<size_t>(subviewUsers.front()));
    if (load.name != "hivm.hir.load" || load.operands.size() < 2 ||
        load.operands[1] != destinationSubview.results.front())
      continue;

    auto sourceSubviewDefinition = definitions.find(load.operands.front());
    if (sourceSubviewDefinition == definitions.end() ||
        sourceSubviewDefinition->second->name != "memref.subview" ||
        sourceSubviewDefinition->second->operands.empty() ||
        sourceSubviewDefinition->second->results.size() != 1 ||
        sourceSubviewDefinition->second->resultTypes.size() != 1)
      continue;
    const GenericOperation sourceSubview = *sourceSubviewDefinition->second;
    auto reinterpretDefinition =
        definitions.find(sourceSubview.operands.front());
    if (reinterpretDefinition == definitions.end() ||
        reinterpretDefinition->second->name != "memref.reinterpret_cast" ||
        reinterpretDefinition->second->resultTypes.empty())
      continue;

    const std::optional<ShapedTypeModel> sourceParentType =
        ParseShapedTypeForDimensionAnalysis(
            reinterpretDefinition->second->resultTypes.front());
    if (!sourceParentType || tilingDim >= sourceParentType->shape.size() ||
        !sourceParentType->shape[tilingDim])
      continue;

    if (!HandleTileAndBindExtractOfExtract(
            module, rewriter, destinationSubview.id, tilingDim, offsetValue,
            tileSize) ||
        !HandleTileAndBindExtractOfExtract(
            module, rewriter, sourceSubview.id, tilingDim, offsetValue,
            tileSize))
      continue;

    std::vector<std::optional<int64_t>> tileShape = sourceParentType->shape;
    tileShape[tilingDim] = tileSize;
    const std::string tiledSourceParentType =
        ReplaceTileAndBindShapeDimension(
            reinterpretDefinition->second->resultTypes.front(), tilingDim,
            tileSize);
    const int parentSubview = rewriter.createOperation(
        sourceSubview.parentId, sourceSubview.regionId, sourceSubview.blockId,
        "memref.subview", {tiledSourceParentType},
        {sourceSubview.operands.front(), offsetValue},
        {reinterpretDefinition->second->resultTypes.front(), "index"},
        TileAndBindSliceProperties(tileShape, tilingDim));
    const GenericOperation &currentSourceSubview =
        module.operations.at(static_cast<size_t>(sourceSubview.id));
    rewriter.insertToBlock(sourceSubview.blockId,
                           static_cast<size_t>(currentSourceSubview.ordinal),
                           parentSubview);
    const int parentSubviewValue =
        module.operations.at(static_cast<size_t>(parentSubview))
            .results.front();
    module.operations.at(static_cast<size_t>(sourceSubview.id)).operands[0] =
        parentSubviewValue;

    GenericOperation &alloc =
        module.operations.at(static_cast<size_t>(allocId));
    const std::string tiledAllocType = ReplaceTileAndBindShapeDimension(
        alloc.resultTypes.front(), tilingDim, tileSize);
    alloc.resultTypes.front() = tiledAllocType;
    valueTypes[allocValue] = tiledAllocType;

    GenericOperation &toTensor =
        module.operations.at(static_cast<size_t>(toTensorId));
    const std::string tiledTensorType = ReplaceTileAndBindShapeDimension(
        toTensor.resultTypes.front(), tilingDim, tileSize);
    toTensor.resultTypes.front() = tiledTensorType;
    valueTypes[toTensor.results.front()] = tiledTensorType;
    valueTypes[parentSubviewValue] = tiledSourceParentType;

    ReplaceTileAndBindValueExcept(module, sliceSnapshot.results.front(),
                                  toTensor.results.front(), sliceId);
    rewriter.removeFromBlock(sliceSnapshot.blockId, sliceId);
    return true;
  }
  return false;
}

// HoistAffinePattern from BubbleUpExtractSlice/HoistAffine.cpp, applied to a
// single operation. The combined greedy driver calls this immediately for
// newly inserted affine operations because its worklist is LIFO.
inline bool ApplyHoistAffinePattern(GenericModule &module, int operationId) {
  const GenericOperation snapshot =
      module.operations.at(static_cast<size_t>(operationId));
  if ((snapshot.name != "affine.apply" && snapshot.name != "affine.min" &&
       snapshot.name != "affine.max") ||
      snapshot.blockId < 0)
    return false;

  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  std::map<int, int> blockArguments;
  for (const GenericBlock &block : module.blocks)
    for (int argument : block.arguments)
      blockArguments[argument] = block.id;

  auto blockDominates = [&](int candidateBlock, int operationBlock) {
    if (candidateBlock == operationBlock)
      return true;
    int cursorBlock = operationBlock;
    while (cursorBlock >= 0) {
      const int regionId =
          module.blocks.at(static_cast<size_t>(cursorBlock)).regionId;
      if (regionId < 0)
        break;
      const int parent = module.regions.at(static_cast<size_t>(regionId))
                             .parentOperation;
      if (parent < 0)
        break;
      const GenericOperation &parentOperation =
          module.operations.at(static_cast<size_t>(parent));
      if (parentOperation.blockId == candidateBlock)
        return true;
      cursorBlock = parentOperation.blockId;
    }
    return false;
  };

  const GenericOperation *lastDefiningOperation = nullptr;
  int lastDefiningValue = -1;
  int lastBlockArgument = -1;
  for (int operand : snapshot.operands) {
    auto blockArgument = blockArguments.find(operand);
    if (blockArgument != blockArguments.end()) {
      if (lastBlockArgument < 0 ||
          blockDominates(blockArguments.at(lastBlockArgument),
                         blockArgument->second))
        lastBlockArgument = operand;
      continue;
    }
    auto definition = definitions.find(operand);
    if (definition == definitions.end())
      continue;
    if (!lastDefiningOperation ||
        GenericOperationDominates(module, *lastDefiningOperation,
                                  *definition->second)) {
      lastDefiningOperation = definition->second;
      lastDefiningValue = operand;
    }
  }

  int insertionBlock = snapshot.blockId;
  size_t insertionPosition = 0;
  if (lastDefiningOperation && lastBlockArgument < 0) {
    insertionBlock = lastDefiningOperation->blockId;
    insertionPosition =
        static_cast<size_t>(lastDefiningOperation->ordinal + 1);
  } else if (!lastDefiningOperation && lastBlockArgument >= 0) {
    insertionBlock = blockArguments.at(lastBlockArgument);
    lastDefiningValue = lastBlockArgument;
  } else if (lastDefiningOperation && lastBlockArgument >= 0) {
    const int argumentBlock = blockArguments.at(lastBlockArgument);
    if (blockDominates(argumentBlock, lastDefiningOperation->blockId)) {
      insertionBlock = lastDefiningOperation->blockId;
      insertionPosition =
          static_cast<size_t>(lastDefiningOperation->ordinal + 1);
    } else {
      insertionBlock = argumentBlock;
      lastDefiningValue = lastBlockArgument;
    }
  } else {
    return false;
  }

  GenericRewriter rewriter(module);
  if (insertionBlock != snapshot.blockId) {
    rewriter.removeFromBlock(snapshot.blockId, operationId);
    rewriter.insertToBlock(insertionBlock, insertionPosition, operationId);
    return true;
  }

  const std::vector<int> operations =
      module.blocks.at(static_cast<size_t>(snapshot.blockId)).operations;
  const auto current =
      std::find(operations.begin(), operations.end(), operationId);
  if (current == operations.end())
    return false;
  const size_t currentPosition =
      static_cast<size_t>(std::distance(operations.begin(), current));
  while (insertionPosition < operations.size()) {
    const GenericOperation &candidate = module.operations.at(
        static_cast<size_t>(operations[insertionPosition]));
    if (std::find(candidate.operands.begin(), candidate.operands.end(),
                  lastDefiningValue) == candidate.operands.end())
      break;
    ++insertionPosition;
  }
  if (insertionPosition >= currentPosition)
    return false;

  const bool allAffineBetween = std::all_of(
      operations.begin() + static_cast<std::ptrdiff_t>(insertionPosition),
      current, [&](int betweenId) {
        const std::string &name =
            module.operations.at(static_cast<size_t>(betweenId)).name;
        return name == "affine.apply" || name == "affine.min" ||
               name == "affine.max";
      });
  if (allAffineBetween)
    return false;

  rewriter.removeFromBlock(snapshot.blockId, operationId);
  rewriter.insertToBlock(snapshot.blockId, insertionPosition, operationId);
  return true;
}

inline bool ApplyCSEAffineApplyPattern(GenericModule &module,
                                       int operationId);

// Mirrors TileAndBindSubBlock.cpp::tileAndSliceOp's dynamic-shape store
// precondition. The real pass rejects the cloned 1:2 candidate before adding
// any temporary slices when a store cannot be traced back to equal, static
// parent shapes.
inline bool TileAndSliceOpHasUnsupportedStore(
    const GenericModule &module, const GenericOperation &function) {
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  const std::map<int, std::string> valueTypes = ValueTypes(module);
  const auto shapedType = [&](int value) {
    auto type = valueTypes.find(value);
    return type == valueTypes.end()
               ? std::optional<ShapedTypeModel>{}
               : ParseShapedTypeForDimensionAnalysis(type->second);
  };
  const auto hasDynamicShape = [](const ShapedTypeModel &type) {
    return std::any_of(type.shape.begin(), type.shape.end(),
                       [](const std::optional<int64_t> &extent) {
                         return !extent.has_value();
                       });
  };
  const auto sourceOfView = [&](int value) {
    auto definition = definitions.find(value);
    if (definition == definitions.end() ||
        (definition->second->name != "tensor.extract_slice" &&
         definition->second->name != "memref.subview") ||
        definition->second->operands.empty())
      return value;
    return definition->second->operands.front();
  };
  const auto isInsideExtractedLoadOrStoreLoop =
      [&](const GenericOperation &operation) {
        int parent = operation.parentId;
        while (parent >= 0) {
          const GenericOperation &ancestor =
              module.operations.at(static_cast<size_t>(parent));
          if (ancestor.name == "scf.for")
            return HasSplitMixDictionaryEntry(ancestor.attributes,
                                               "ExtractedLoadOrStore");
          parent = ancestor.parentId;
        }
        return false;
      };

  for (int operationId : GetTileAndBindDescendants(module, function)) {
    const GenericOperation &store =
        module.operations.at(static_cast<size_t>(operationId));
    if (store.name != "hivm.hir.store" || store.operands.size() < 2)
      continue;
    const std::optional<ShapedTypeModel> source =
        shapedType(store.operands[0]);
    const std::optional<ShapedTypeModel> destination =
        shapedType(store.operands[1]);
    if (!source || !destination ||
        isInsideExtractedLoadOrStoreLoop(store)) {
      return true;
    }
    if (!hasDynamicShape(*source) && !hasDynamicShape(*destination))
      continue;

    const std::optional<ShapedTypeModel> sourceParent =
        shapedType(sourceOfView(store.operands[0]));
    const std::optional<ShapedTypeModel> destinationParent =
        shapedType(sourceOfView(store.operands[1]));
    if (!sourceParent || !destinationParent ||
        hasDynamicShape(*sourceParent) ||
        hasDynamicShape(*destinationParent) ||
        sourceParent->shape != destinationParent->shape) {
      return true;
    }
  }
  return false;
}

inline bool AttemptBindSubBlock(GenericModule &module, int functionId) {
  RunReplicateOutEmptyTensor(module, functionId);
  DimensionAnalyzer tiledAnalyzer(module);
  if (!tiledAnalyzer.initialize())
    return false;
  tiledAnalyzer.computeTilingDim(
      module.operations.at(static_cast<size_t>(functionId)));
  const GenericOperation function =
      module.operations.at(static_cast<size_t>(functionId));
  if (TileAndSliceOpHasUnsupportedStore(module, function))
    return false;
  if (function.regions.size() != 1)
    return false;
  const GenericRegion &functionRegion =
      module.regions.at(static_cast<size_t>(function.regions.front()));
  if (functionRegion.blocks.size() != 1)
    return false;
  const int entryBlock = functionRegion.blocks.front();
  if (module.blocks.at(static_cast<size_t>(entryBlock)).operations.empty())
    return false;

  const std::vector<int> originalOperations =
      module.blocks.at(static_cast<size_t>(entryBlock)).operations;
  int returnOperation = -1;
  for (int operationId : originalOperations)
    if (module.operations.at(static_cast<size_t>(operationId)).name ==
        "func.return")
      returnOperation = operationId;
  if (returnOperation < 0)
    return false;

  GenericRewriter rewriter(module);
  std::optional<int> zeroValue =
      FindTileAndBindIndexConstant(module, entryBlock, 0);
  int zeroOperation = -1;
  if (zeroValue) {
    const auto definitions = DefiningOperations(module);
    zeroOperation = definitions.at(*zeroValue)->id;
    rewriter.removeFromBlock(entryBlock, zeroOperation);
  } else {
    zeroOperation = rewriter.createOperation(
        functionId, function.regions.front(), entryBlock, "arith.constant",
        {"index"}, {}, {}, "{value = 0 : index}");
    zeroValue = module.operations.at(static_cast<size_t>(zeroOperation))
                    .results.front();
  }
  const int oneOperation = rewriter.createOperation(
      functionId, function.regions.front(), entryBlock, "arith.constant",
      {"index"}, {}, {}, "{value = 1 : index}");
  const int twoOperation = rewriter.createOperation(
      functionId, function.regions.front(), entryBlock, "arith.constant",
      {"index"}, {}, {}, "{value = 2 : index}");
  const int oneValue =
      module.operations.at(static_cast<size_t>(oneOperation)).results.front();
  const int twoValue =
      module.operations.at(static_cast<size_t>(twoOperation)).results.front();
  const int subBlockLoop = rewriter.createOperation(
      functionId, function.regions.front(), entryBlock, "scf.for", {},
      {*zeroValue, twoValue, oneValue}, {"index", "index", "index"}, "",
      "{map_for_to_forall, mapping = [#hivm.sub_block<x>]}");
  const int loopRegion = rewriter.createRegion(subBlockLoop);
  const int loopBlock = rewriter.createBlock(loopRegion, {"index"});
  const int inductionVariable =
      module.blocks.at(static_cast<size_t>(loopBlock)).arguments.front();

  module.blocks.at(static_cast<size_t>(entryBlock)).operations.clear();
  rewriter.appendToBlock(entryBlock, zeroOperation);
  rewriter.appendToBlock(entryBlock, oneOperation);
  rewriter.appendToBlock(entryBlock, twoOperation);
  rewriter.appendToBlock(entryBlock, subBlockLoop);
  rewriter.appendToBlock(entryBlock, returnOperation);
  for (int operationId : originalOperations) {
    if (operationId == zeroOperation || operationId == returnOperation)
      continue;
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    operation.parentId = subBlockLoop;
    operation.regionId = loopRegion;
    operation.blockId = loopBlock;
    rewriter.appendToBlock(loopBlock, operationId);
  }
  const int loopYield = rewriter.createOperation(
      subBlockLoop, loopRegion, loopBlock, "scf.yield", {});
  rewriter.appendToBlock(loopBlock, loopYield);

  const std::map<int, size_t> bubbleDims = CollectTileAndBindBubbleDims(
      module, function, tiledAnalyzer);
  std::map<int, std::string> valueTypes = ValueTypes(module);
  struct TileAndSliceOffsetRequest {
    int operationId = -1;
    int value = -1;
    int64_t tileSize = 0;
    bool storeCopy = false;
  };
  std::vector<TileAndSliceOffsetRequest> offsetRequests;
  std::vector<std::pair<int, int64_t>> storeCopyTiles;
  std::map<int, int> storeCopyStartValues;
  const std::map<int, const GenericOperation *> tileAndSliceDefinitions =
      DefiningOperations(module);
  auto valueHasUser = [&](int value) {
    return std::any_of(module.operations.begin(), module.operations.end(),
                       [&](const GenericOperation &user) {
                         return std::find(user.operands.begin(),
                                          user.operands.end(), value) !=
                                user.operands.end();
                       });
  };
  for (int operationId :
       GetTileAndBindGreedyRewriteOrder(module, function)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (IsTileAndBindStoreCopyStartPoint(module, operation, tiledAnalyzer,
                                         valueTypes)) {
      const int64_t axis =
          tiledAnalyzer.getTilingDim(operation.operands.front());
      int valueToSlice = operation.operands.front();
      auto sourceType = valueTypes.find(valueToSlice);
      std::optional<ShapedTypeModel> source =
          sourceType == valueTypes.end()
              ? std::nullopt
              : ParseShapedTypeForDimensionAnalysis(sourceType->second);
      if (source && axis >= 0 &&
          static_cast<size_t>(axis) < source->shape.size() &&
          !source->shape[static_cast<size_t>(axis)] &&
          operation.operands.size() >= 2) {
        const auto sourceView = tileAndSliceDefinitions.find(valueToSlice);
        const auto destinationView =
            tileAndSliceDefinitions.find(operation.operands[1]);
        if (sourceView != tileAndSliceDefinitions.end() &&
            destinationView != tileAndSliceDefinitions.end() &&
            (sourceView->second->name == "tensor.extract_slice" ||
             sourceView->second->name == "memref.subview") &&
            (destinationView->second->name == "tensor.extract_slice" ||
             destinationView->second->name == "memref.subview") &&
            !sourceView->second->operands.empty() &&
            !destinationView->second->operands.empty() &&
            HasTileAndBindUnitStride(*sourceView->second) &&
            HasTileAndBindUnitStride(*destinationView->second)) {
          const int sourceParent = sourceView->second->operands.front();
          const int destinationParent =
              destinationView->second->operands.front();
          auto sourceParentType = valueTypes.find(sourceParent);
          auto destinationParentType = valueTypes.find(destinationParent);
          const std::optional<ShapedTypeModel> sourceParentShape =
              sourceParentType == valueTypes.end()
                  ? std::nullopt
                  : ParseShapedTypeForDimensionAnalysis(
                        sourceParentType->second);
          const std::optional<ShapedTypeModel> destinationParentShape =
              destinationParentType == valueTypes.end()
                  ? std::nullopt
                  : ParseShapedTypeForDimensionAnalysis(
                        destinationParentType->second);
          if (sourceParentShape && destinationParentShape &&
              sourceParentShape->shape == destinationParentShape->shape &&
              static_cast<size_t>(axis) < sourceParentShape->shape.size() &&
              sourceParentShape->shape[static_cast<size_t>(axis)] &&
              *sourceParentShape->shape[static_cast<size_t>(axis)] >= 2) {
            valueToSlice = sourceParent;
            source = sourceParentShape;
          }
        }
      }
      if (source && axis >= 0 &&
          static_cast<size_t>(axis) < source->shape.size() &&
          source->shape[static_cast<size_t>(axis)]) {
        const int64_t tileSize =
            (*source->shape[static_cast<size_t>(axis)] + 1) / 2;
        storeCopyTiles.emplace_back(operationId, tileSize);
        storeCopyStartValues[operationId] = valueToSlice;
        offsetRequests.push_back(
            {operationId, valueToSlice, tileSize, true});
      }
    }

    if (operation.name != "scf.for" && operation.name != "scf.while" &&
        operation.name != "scf.if")
      continue;
    for (size_t resultIndex = 0;
         resultIndex < operation.results.size() &&
         resultIndex < operation.resultTypes.size();
         ++resultIndex) {
      const int value = operation.results[resultIndex];
      const int64_t axis = tiledAnalyzer.getTilingDim(value);
      const auto type = ParseShapedTypeForDimensionAnalysis(
          operation.resultTypes[resultIndex]);
      if (axis < 0 || valueHasUser(value) || !type || !type->tensor ||
          static_cast<size_t>(axis) >= type->shape.size() ||
          !type->shape[static_cast<size_t>(axis)] ||
          *type->shape[static_cast<size_t>(axis)] < 2)
        continue;
      offsetRequests.push_back(
          {operationId, value,
           (*type->shape[static_cast<size_t>(axis)] + 1) / 2, false});
    }
  }

  // TileAndSliceStoreCopyOp::modifyStoreCopyOp creates one independent
  // calculateOffsetAtTilingDim result for every successful pattern rewrite.
  // Do not merge these values here: HIVMBubbleUpExtractSlice owns the later
  // HoistAffine/CSE decision and therefore also owns the representative SSA
  // value and operation order.
  auto createOffsetAtTilingDim = [&](int64_t tileSize, int insertionBlock,
                                     size_t insertionPosition) {
    const int apply = rewriter.createOperation(
        subBlockLoop, loopRegion, insertionBlock, "affine.apply", {"index"},
        {inductionVariable}, {"index"},
        "{map = affine.apply(mul(v(" +
            std::to_string(inductionVariable) + "),c(" +
            std::to_string(tileSize) + ")))}");
    rewriter.insertToBlock(insertionBlock, insertionPosition, apply);
    return module.operations.at(static_cast<size_t>(apply)).results.front();
  };

  std::map<int, int> storeCopyOffsets;
  std::map<int, int> leafOffsets;
  for (const TileAndSliceOffsetRequest &request : offsetRequests) {
    const int offset =
        createOffsetAtTilingDim(request.tileSize, loopBlock, 0);
    if (request.storeCopy)
      storeCopyOffsets[request.operationId] = offset;
    else
      leafOffsets[request.value] = offset;
  }

  // Track the marked extract_slice request backwards through the same value
  // edges used by BubbleUpPattern. This is only the affine offset carried by
  // that request; tensor rewrites below still follow the corresponding source
  // strategy.
  std::map<int, int> bubbleOffsets;
  const std::map<int, const GenericOperation *> offsetDefinitions =
      DefiningOperations(module);
  std::function<void(int, int)> propagateBubbleOffset =
      [&](int value, int offset) {
        if (bubbleDims.count(value) == 0 ||
            !bubbleOffsets.emplace(value, offset).second)
          return;
        auto definition = offsetDefinitions.find(value);
        if (definition == offsetDefinitions.end())
          return;
        const GenericOperation &operation = *definition->second;
        if (operation.name == "scf.for") {
          auto result = std::find(operation.results.begin(),
                                  operation.results.end(), value);
          if (result == operation.results.end() || operation.regions.empty())
            return;
          const size_t resultIndex = static_cast<size_t>(
              std::distance(operation.results.begin(), result));
          const size_t initBegin =
              operation.operands.size() >= operation.results.size()
                  ? operation.operands.size() - operation.results.size()
                  : operation.operands.size();
          if (initBegin + resultIndex < operation.operands.size())
            propagateBubbleOffset(operation.operands[initBegin + resultIndex],
                                  offset);
          const GenericRegion &region = module.regions.at(
              static_cast<size_t>(operation.regions.front()));
          if (region.blocks.empty())
            return;
          const GenericBlock &block =
              module.blocks.at(static_cast<size_t>(region.blocks.front()));
          if (resultIndex + 1 < block.arguments.size())
            propagateBubbleOffset(block.arguments[resultIndex + 1], offset);
          if (!block.operations.empty()) {
            const GenericOperation &terminator = module.operations.at(
                static_cast<size_t>(block.operations.back()));
            if (terminator.name == "scf.yield" &&
                resultIndex < terminator.operands.size())
              propagateBubbleOffset(terminator.operands[resultIndex], offset);
          }
          return;
        }
        for (int operand : operation.operands)
          propagateBubbleOffset(operand, offset);
      };
  for (const auto &[operationId, tileSize] : storeCopyTiles) {
    (void)tileSize;
    auto startValue = storeCopyStartValues.find(operationId);
    if (startValue != storeCopyStartValues.end())
      propagateBubbleOffset(startValue->second,
                            storeCopyOffsets.at(operationId));
  }
  for (const auto &[value, offset] : leafOffsets)
    propagateBubbleOffset(value, offset);

  struct TiledValue {
    size_t axis = 0;
    int64_t tileSize = 0;
    int offset = -1;
  };
  std::map<int, TiledValue> tiledValues;
  for (GenericBlock &block : module.blocks) {
    const GenericOperation &blockParent = module.operations.at(
        static_cast<size_t>(module.regions.at(
            static_cast<size_t>(block.regionId)).parentOperation));
    if (blockParent.name == "scf.for" &&
        HasSplitMixDictionaryEntry(blockParent.attributes,
                                   "ExtractedLoadOrStore"))
      continue;
    for (size_t index = 0; index < block.arguments.size(); ++index) {
      const int value = block.arguments[index];
      auto bubbleDim = bubbleDims.find(value);
      if (bubbleDim == bubbleDims.end())
        continue;
      const int64_t axis = static_cast<int64_t>(bubbleDim->second);
      auto shaped = ParseShapedTypeForDimensionAnalysis(block.argumentTypes[index]);
      if (axis < 0 || !shaped || !shaped->tensor ||
          static_cast<size_t>(axis) >= shaped->shape.size() ||
          !shaped->shape[static_cast<size_t>(axis)] ||
          *shaped->shape[static_cast<size_t>(axis)] < 2)
        continue;
      const int64_t tileSize =
          (*shaped->shape[static_cast<size_t>(axis)] + 1) / 2;
      block.argumentTypes[index] = ReplaceTileAndBindShapeDimension(
          block.argumentTypes[index], static_cast<size_t>(axis), tileSize);
      valueTypes[value] = block.argumentTypes[index];
      auto offset = bubbleOffsets.find(value);
      tiledValues[value] = {static_cast<size_t>(axis), tileSize,
                            offset == bubbleOffsets.end() ? -1
                                                          : offset->second};
    }
  }

  std::vector<int> boundaryToTensor;
  std::vector<std::pair<int, int>> varangeOffsets;
  const std::vector<int> bubbleRewriteOrder =
      GetTileAndBindGreedyRewriteOrder(module, function);

  struct BubbleUpRequest {
    int value = -1;
    int offset = -1;
    size_t initialOrder = 0;
  };
  const auto initialDefinitions = DefiningOperations(module);
  std::map<int, size_t> greedyOrder;
  for (size_t index = 0; index < bubbleRewriteOrder.size(); ++index)
    greedyOrder[bubbleRewriteOrder[index]] = index;
  std::vector<BubbleUpRequest> initialRequests;
  initialRequests.reserve(offsetRequests.size());
  for (size_t index = 0; index < offsetRequests.size(); ++index) {
    const TileAndSliceOffsetRequest &request = offsetRequests[index];
    const int offset = request.storeCopy
                           ? storeCopyOffsets.at(request.operationId)
                           : leafOffsets.at(request.value);
    size_t order = bubbleRewriteOrder.size();
    auto definition = initialDefinitions.find(request.value);
    if (definition != initialDefinitions.end()) {
      auto position = greedyOrder.find(definition->second->id);
      if (position != greedyOrder.end())
        order = position->second;
    }
    // Preserve TileAndSlice pattern creation order for slices inserted after
    // the same defining value. Repeated setInsertionPointAfter inserts newer
    // slices before older ones, and the reverse-postorder worklist reverses
    // that relation once more.
    initialRequests.push_back(
        {request.value, offset, order * (offsetRequests.size() + 1) + index});
  }
  std::stable_sort(initialRequests.begin(), initialRequests.end(),
                   [](const BubbleUpRequest &lhs,
                      const BubbleUpRequest &rhs) {
                     return lhs.initialOrder < rhs.initialOrder;
                   });

  bool bubbleUpFailed = false;
  std::function<void(int, int, std::set<int>)> runBubbleUpRequest =
      [&](int value, int offset, std::set<int> path) {
        if (bubbleUpFailed)
          return;
        if (!path.insert(value).second || bubbleDims.count(value) == 0)
          return;
        const auto definitions = DefiningOperations(module);
        auto definition = definitions.find(value);
        if (definition == definitions.end())
          return;
        const GenericOperation operationSnapshot = *definition->second;
        if (operationSnapshot.name == "tensor.empty" ||
            operationSnapshot.name == "bufferization.to_tensor")
          return;

        if (operationSnapshot.name == "tensor.extract_slice") {
          const std::optional<TileAndBindMixedSlice> parentSlice =
              ParseTileAndBindMixedSlice(operationSnapshot);
          const bool parentIsStatic =
              parentSlice &&
              std::all_of(parentSlice->sizes.begin(),
                          parentSlice->sizes.end(),
                          [](const TileAndBindFoldResult &size) {
                            return size.constant.has_value();
                          });
          const std::set<size_t> parentDimensions =
              GetTileAndBindExtractOrInsertDims(module, operationSnapshot);
          const size_t childDimension = bubbleDims.at(value);
          if (parentIsStatic &&
              parentDimensions.count(childDimension) != 0 &&
              !CreatedByTileAndBindTiling(module, operationSnapshot)) {
            bubbleUpFailed = true;
            return;
          }
        }

        if (operationSnapshot.name == "scf.for") {
          if (HasSplitMixDictionaryEntry(operationSnapshot.attributes,
                                         "ExtractedLoadOrStore"))
            return;
          auto result = std::find(operationSnapshot.results.begin(),
                                  operationSnapshot.results.end(), value);
          if (result == operationSnapshot.results.end() ||
              operationSnapshot.regions.empty())
            return;
          const size_t resultIndex = static_cast<size_t>(
              std::distance(operationSnapshot.results.begin(), result));
          std::vector<int> newSlices;
          const size_t initBegin =
              operationSnapshot.operands.size() >=
                      operationSnapshot.results.size()
                  ? operationSnapshot.operands.size() -
                        operationSnapshot.results.size()
                  : operationSnapshot.operands.size();
          if (initBegin + resultIndex < operationSnapshot.operands.size())
            newSlices.push_back(
                operationSnapshot.operands[initBegin + resultIndex]);
          const GenericRegion &region = module.regions.at(
              static_cast<size_t>(operationSnapshot.regions.front()));
          if (!region.blocks.empty()) {
            const GenericBlock &block =
                module.blocks.at(static_cast<size_t>(region.blocks.front()));
            if (!block.operations.empty()) {
              const GenericOperation &terminator = module.operations.at(
                  static_cast<size_t>(block.operations.back()));
              if (terminator.name == "scf.yield" &&
                  resultIndex < terminator.operands.size())
                newSlices.push_back(terminator.operands[resultIndex]);
            }
          }
          for (auto operand = newSlices.rbegin(); operand != newSlices.rend();
               ++operand)
            runBubbleUpRequest(*operand, offset, path);
          return;
        }

        const bool elementwise =
            IsElementwiseNaryOp(operationSnapshot.name) ||
            operationSnapshot.name == "hivm.hir.load" ||
            operationSnapshot.name == "hivm.hir.store" ||
            operationSnapshot.name == "hivm.hir.copy";
        std::vector<std::pair<int, int>> insertedSlices;
        const auto offsetDefinition = definitions.find(offset);
        const std::optional<GenericOperation> sourceApply =
            offsetDefinition != definitions.end() &&
                    offsetDefinition->second->name == "affine.apply"
                ? std::optional<GenericOperation>(*offsetDefinition->second)
                : std::nullopt;
        for (int operand : operationSnapshot.operands) {
          auto operandDim = bubbleDims.find(operand);
          auto operandType = valueTypes.find(operand);
          if (operandDim == bubbleDims.end() ||
              operandType == valueTypes.end())
            continue;
          const auto shaped =
              ParseShapedTypeForDimensionAnalysis(operandType->second);
          if (!shaped || !shaped->tensor)
            continue;
          int composedOffset = offset;
          int composedApply = -1;
          if (elementwise && sourceApply && operationSnapshot.blockId >= 0) {
            const GenericOperation currentOperation =
                module.operations.at(static_cast<size_t>(operationSnapshot.id));
            composedApply = rewriter.createOperation(
                currentOperation.parentId, currentOperation.regionId,
                currentOperation.blockId, sourceApply->name,
                sourceApply->resultTypes, sourceApply->operands,
                sourceApply->operandTypes, sourceApply->properties,
                sourceApply->attributes);
            rewriter.insertToBlock(
                currentOperation.blockId,
                static_cast<size_t>(currentOperation.ordinal), composedApply);
            composedOffset =
                module.operations.at(static_cast<size_t>(composedApply))
                    .results.front();
          }
          bubbleOffsets.emplace(operand, composedOffset);
          insertedSlices.emplace_back(operand, composedApply);
        }

        // computeAllSliceParameters creates every affine.apply in operand
        // order before makeTiledShapes materializes any extract_slice. The
        // structured op clone is inserted last. Consequently the LIFO greedy
        // worklist drains all slices in reverse operand order before it can
        // reach any of this level's affine.apply operations.
        for (auto inserted = insertedSlices.rbegin();
             inserted != insertedSlices.rend(); ++inserted) {
          const int operand = inserted->first;
          const int composedApply = inserted->second;
          const int composedOffset =
              composedApply < 0
                  ? offset
                  : module.operations.at(static_cast<size_t>(composedApply))
                        .results.front();
          runBubbleUpRequest(operand, composedOffset, path);
        }
        for (auto inserted = insertedSlices.rbegin();
             inserted != insertedSlices.rend(); ++inserted)
          if (inserted->second >= 0) {
            ApplyHoistAffinePattern(module, inserted->second);
            ApplyCSEAffineApplyPattern(module, inserted->second);
          }
      };
  for (const BubbleUpRequest &request : initialRequests)
    runBubbleUpRequest(request.value, request.offset, {});
  if (bubbleUpFailed)
    return false;

  struct ExtractInsertExtractRewrite {
    int sourceExtract = -1;
    size_t tilingDim = 0;
    int64_t tileSize = 0;
    int tileOffset = -1;
  };
  std::map<int, ExtractInsertExtractRewrite>
      extractInsertExtractRewrites;
  std::set<int> extractInsertExtractSources;
  const std::map<int, const GenericOperation *> bubbleDefinitions =
      DefiningOperations(module);
  for (int operationId : bubbleRewriteOrder) {
    const GenericOperation &insertSlice =
        module.operations.at(static_cast<size_t>(operationId));
    if (insertSlice.name != "tensor.insert_slice" ||
        insertSlice.operands.size() < 2 || insertSlice.results.size() != 1 ||
        insertSlice.resultTypes.size() != 1)
      continue;
    const int result = insertSlice.results.front();
    auto bubbleDim = bubbleDims.find(result);
    auto bubbleOffset = bubbleOffsets.find(result);
    if (bubbleDim == bubbleDims.end() || bubbleOffset == bubbleOffsets.end())
      continue;
    auto sourceDefinition = bubbleDefinitions.find(insertSlice.operands.front());
    if (sourceDefinition == bubbleDefinitions.end() ||
        sourceDefinition->second->name != "tensor.extract_slice" ||
        sourceDefinition->second->operands.empty() ||
        sourceDefinition->second->results.size() != 1 ||
        TileAndBindValueUsers(module,
                              sourceDefinition->second->results.front())
                .size() != 1)
      continue;

    const GenericOperation &sourceExtract = *sourceDefinition->second;
    auto sourceType = valueTypes.find(sourceExtract.operands.front());
    if (sourceType == valueTypes.end() ||
        sourceType->second != insertSlice.resultTypes.front())
      continue;
    const std::optional<TileAndBindMixedSlice> sourceSlice =
        ParseTileAndBindMixedSlice(sourceExtract);
    if (!sourceSlice || bubbleDim->second >= sourceSlice->sizes.size())
      continue;

    auto offsetDefinitionIt = bubbleDefinitions.find(bubbleOffset->second);
    const GenericOperation *offsetDefinition =
        offsetDefinitionIt == bubbleDefinitions.end()
            ? nullptr
            : offsetDefinitionIt->second;
    const bool childHasZeroOffset =
        offsetDefinition && offsetDefinition->name == "arith.constant" &&
        FindDictionaryValue(offsetDefinition->properties, "value") ==
            "0 : index";
    const bool sourceHasZeroOffset = std::all_of(
        sourceSlice->offsets.begin(), sourceSlice->offsets.end(),
        [](const TileAndBindFoldResult &offset) {
          return offset.constant && *offset.constant == 0;
        });
    const bool sourceHasUnitStride = std::all_of(
        sourceSlice->strides.begin(), sourceSlice->strides.end(),
        [](const TileAndBindFoldResult &stride) {
          return stride.constant && *stride.constant == 1;
        });
    if (childHasZeroOffset && sourceHasZeroOffset && sourceHasUnitStride)
      continue;

    const std::optional<ShapedTypeModel> resultType =
        ParseShapedTypeForDimensionAnalysis(insertSlice.resultTypes.front());
    if (!resultType || bubbleDim->second >= resultType->shape.size() ||
        !resultType->shape[bubbleDim->second] ||
        *resultType->shape[bubbleDim->second] < 2)
      continue;
    const int64_t tileSize =
        (*resultType->shape[bubbleDim->second] + 1) / 2;
    extractInsertExtractRewrites.emplace(
        operationId,
        ExtractInsertExtractRewrite{sourceExtract.id, bubbleDim->second,
                                    tileSize, bubbleOffset->second});
    extractInsertExtractSources.insert(sourceExtract.id);
  }

  auto isBubbleTerminal = [&](int value) {
    const auto definitions = DefiningOperations(module);
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return false;
    const GenericOperation &operation = *definition->second;
    return operation.name == "tensor.empty" ||
           (operation.name == "scf.for" &&
            HasSplitMixDictionaryEntry(operation.attributes,
                                       "ExtractedLoadOrStore"));
  };
  auto materializeTerminalSlice = [&](int consumerId, size_t operandIndex) {
    const GenericOperation consumerSnapshot =
        module.operations.at(static_cast<size_t>(consumerId));
    if (consumerSnapshot.blockId < 0 ||
        operandIndex >= consumerSnapshot.operands.size())
      return false;
    const int source = consumerSnapshot.operands[operandIndex];
    auto dimension = bubbleDims.find(source);
    auto offset = bubbleOffsets.find(source);
    auto sourceType = valueTypes.find(source);
    if (dimension == bubbleDims.end() || offset == bubbleOffsets.end() ||
        sourceType == valueTypes.end() || !isBubbleTerminal(source))
      return false;
    const std::optional<ShapedTypeModel> shaped =
        ParseShapedTypeForDimensionAnalysis(sourceType->second);
    const size_t axis = dimension->second;
    if (!shaped || !shaped->tensor || axis >= shaped->shape.size() ||
        !shaped->shape[axis] || *shaped->shape[axis] < 2)
      return false;
    const int64_t tileSize = (*shaped->shape[axis] + 1) / 2;
    std::vector<std::optional<int64_t>> tileShape = shaped->shape;
    tileShape[axis] = tileSize;
    const std::string tileType = ReplaceTileAndBindShapeDimension(
        sourceType->second, axis, tileSize);
    int insertionParent = consumerSnapshot.parentId;
    int insertionRegion = consumerSnapshot.regionId;
    int insertionBlock = consumerSnapshot.blockId;
    size_t insertionPosition =
        static_cast<size_t>(consumerSnapshot.ordinal);
    const auto definitions = DefiningOperations(module);
    auto sourceDefinition = definitions.find(source);
    if (sourceDefinition != definitions.end() &&
        sourceDefinition->second->name == "scf.for" &&
        HasSplitMixDictionaryEntry(sourceDefinition->second->attributes,
                                   "ExtractedLoadOrStore")) {
      insertionParent = sourceDefinition->second->parentId;
      insertionRegion = sourceDefinition->second->regionId;
      insertionBlock = sourceDefinition->second->blockId;
      insertionPosition =
          static_cast<size_t>(sourceDefinition->second->ordinal + 1);
    }
    const int slice = rewriter.createOperation(
        insertionParent, insertionRegion, insertionBlock,
        "tensor.extract_slice", {tileType},
        {source, offset->second}, {sourceType->second, "index"},
        TileAndBindSliceProperties(tileShape, axis),
        "{to_be_bubbled_slice}");
    rewriter.insertToBlock(insertionBlock, insertionPosition, slice);
    const int slicedValue =
        module.operations.at(static_cast<size_t>(slice)).results.front();
    GenericOperation &consumer =
        module.operations.at(static_cast<size_t>(consumerId));
    consumer.operands[operandIndex] = slicedValue;
    if (operandIndex < consumer.operandTypes.size())
      consumer.operandTypes[operandIndex] = tileType;
    valueTypes[slicedValue] = tileType;
    tiledValues[slicedValue] = {axis, tileSize, offset->second};
    return true;
  };

  for (int operationId : bubbleRewriteOrder) {
    auto extractInsertExtractRewrite =
        extractInsertExtractRewrites.find(operationId);
    if (extractInsertExtractRewrite != extractInsertExtractRewrites.end() &&
        !HandleTileAndBindExtractInsertExtractCase(
            module, rewriter, operationId,
            extractInsertExtractRewrite->second.sourceExtract,
            extractInsertExtractRewrite->second.tilingDim,
            extractInsertExtractRewrite->second.tileOffset,
            extractInsertExtractRewrite->second.tileSize, valueTypes))
      return false;
    const GenericOperation operationBeforeSlices =
        module.operations.at(static_cast<size_t>(operationId));
    const bool resultIsBubbled = std::any_of(
        operationBeforeSlices.results.begin(),
        operationBeforeSlices.results.end(),
        [&](int result) { return bubbleDims.count(result) != 0; });
    if (resultIsBubbled &&
        (IsElementwiseNaryOp(operationBeforeSlices.name) ||
         operationBeforeSlices.name == "hivm.hir.vbrc" ||
         operationBeforeSlices.name == "hivm.hir.vreduce" ||
         operationBeforeSlices.name == "hivm.hir.varange" ||
         operationBeforeSlices.name == "hivm.hir.load" ||
         operationBeforeSlices.name == "hivm.hir.store" ||
         operationBeforeSlices.name == "hivm.hir.copy"))
      for (size_t operandIndex = 0;
           operandIndex < operationBeforeSlices.operands.size();
           ++operandIndex)
        materializeTerminalSlice(operationId, operandIndex);
    if (operationBeforeSlices.name == "tensor.extract_slice" &&
        !operationBeforeSlices.operands.empty())
      materializeTerminalSlice(operationId, 0);

    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (size_t index = 0;
         index < operation.results.size() && index < operation.resultTypes.size();
         ++index) {
      const int value = operation.results[index];
      auto bubbleDim = bubbleDims.find(value);
      if (bubbleDim == bubbleDims.end())
        continue;
      const int64_t axis = static_cast<int64_t>(bubbleDim->second);
      auto shaped = ParseShapedTypeForDimensionAnalysis(operation.resultTypes[index]);
      if (axis < 0 || !shaped || !shaped->tensor ||
          static_cast<size_t>(axis) >= shaped->shape.size() ||
          !shaped->shape[static_cast<size_t>(axis)] ||
          *shaped->shape[static_cast<size_t>(axis)] < 2)
        continue;
      const int64_t tileSize =
          (*shaped->shape[static_cast<size_t>(axis)] + 1) / 2;
      if (operation.name == "bufferization.to_tensor") {
        boundaryToTensor.push_back(operation.id);
        continue;
      }
      if (operation.name == "tensor.empty" ||
          (operation.name == "scf.for" &&
           HasSplitMixDictionaryEntry(operation.attributes,
                                      "ExtractedLoadOrStore")))
        continue;
      if (extractInsertExtractSources.count(operation.id) != 0)
        continue;

      operation.resultTypes[index] = ReplaceTileAndBindShapeDimension(
          operation.resultTypes[index], static_cast<size_t>(axis), tileSize);
      if (operation.name == "tensor.extract_slice") {
        std::vector<std::optional<int64_t>> shape = shaped->shape;
        shape[static_cast<size_t>(axis)] = tileSize;
        SetTileAndBindProperty(operation, "static_sizes",
                               TileAndBindStaticShape(shape));
      } else if (operation.name == "tensor.expand_shape") {
        const auto resultShape = ParseShapedTypeForDimensionAnalysis(
            operation.resultTypes[index]);
        if (resultShape)
          SetTileAndBindProperty(operation, "static_output_shape",
                                 TileAndBindStaticShape(resultShape->shape));
      } else if (operation.name == "hivm.hir.varange" &&
                 shaped->shape.size() == 1 &&
                 operation.operands.size() >= 2) {
        // VarangeBubbleUpStrategy creates a new varange whose offset is the
        // original offset plus the extract_slice offset. Without this, two
        // varanges from adjacent tiles can become identical and be CSE'd.
        auto offset = bubbleOffsets.find(value);
        if (offset != bubbleOffsets.end())
          varangeOffsets.emplace_back(operation.id, offset->second);
      }
      valueTypes[value] = operation.resultTypes[index];
      auto offset = bubbleOffsets.find(value);
      tiledValues[value] = {static_cast<size_t>(axis), tileSize,
                            offset == bubbleOffsets.end() ? -1
                                                          : offset->second};
      if (operation.name == "tensor.empty")
        MoveTiledEmptyToDpsInitUser(module, operation.id);
    }
  }

  for (const auto &[operationId, tileOffset] : varangeOffsets) {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    if (snapshot.blockId < 0 || snapshot.operands.size() < 2)
      continue;
    const int originalOffset = snapshot.operands[1];
    const std::string offsetType =
        snapshot.operandTypes.size() > 1 ? snapshot.operandTypes[1] : "index";
    const int add = rewriter.createOperation(
        snapshot.parentId, snapshot.regionId, snapshot.blockId, "arith.addi",
        {offsetType}, {originalOffset, tileOffset},
        {offsetType, offsetType});
    rewriter.insertToBlock(snapshot.blockId,
                           static_cast<size_t>(snapshot.ordinal), add);
    GenericOperation &varange =
        module.operations.at(static_cast<size_t>(operationId));
    varange.operands[1] =
        module.operations.at(static_cast<size_t>(add)).results.front();
    if (varange.operandTypes.size() > 1)
      varange.operandTypes[1] = offsetType;
  }

  for (int operationId : boundaryToTensor) {
    const GenericOperation toTensor =
        module.operations.at(static_cast<size_t>(operationId));
    const int sourceValue = toTensor.results.front();
    const size_t axis = bubbleDims.at(sourceValue);
    const auto full = ParseShapedTypeForDimensionAnalysis(
        toTensor.resultTypes.front());
    if (!full || axis >= full->shape.size() || !full->shape[axis])
      continue;
    const int64_t tileSize = (*full->shape[axis] + 1) / 2;
    auto tileOffset = bubbleOffsets.find(sourceValue);
    if (tileOffset == bubbleOffsets.end())
      throw std::runtime_error(
          "TileAndBindSubBlock: BubbleUpPattern lost the offset for a "
          "bufferization.to_tensor result");
    std::vector<std::optional<int64_t>> tileShape = full->shape;
    tileShape[axis] = tileSize;
    const std::string tileType = ReplaceTileAndBindShapeDimension(
        toTensor.resultTypes.front(), axis, tileSize);
    int insertionBlock = toTensor.blockId;
    size_t insertionPosition = static_cast<size_t>(toTensor.ordinal + 1);
    int insertionParent = toTensor.parentId;
    int insertionRegion = toTensor.regionId;
    const auto definitions = DefiningOperations(module);
    for (const GenericOperation &user : module.operations) {
      if (user.operands.empty() || user.operands.front() != sourceValue ||
          user.name != "hivm.hir.load")
        continue;
      insertionBlock = user.blockId;
      insertionPosition = static_cast<size_t>(user.ordinal);
      insertionParent = user.parentId;
      insertionRegion = user.regionId;
      if (user.operands.size() > 1) {
        const auto destination = definitions.find(user.operands[1]);
        if (destination != definitions.end() &&
            (destination->second->name == "tensor.empty" ||
             destination->second->name == "tensor.extract_slice") &&
            destination->second->blockId == user.blockId)
          insertionPosition =
              static_cast<size_t>(destination->second->ordinal);
      }
      break;
    }
    const int slice = rewriter.createOperation(
        insertionParent, insertionRegion, insertionBlock,
        "tensor.extract_slice", {tileType},
        {sourceValue, tileOffset->second},
        {toTensor.resultTypes.front(), "index"},
        TileAndBindSliceProperties(tileShape, axis),
        "{to_be_bubbled_slice}");
    const int sliceValue =
        module.operations.at(static_cast<size_t>(slice)).results.front();
    rewriter.insertToBlock(insertionBlock, insertionPosition, slice);
    ReplaceTileAndBindValueExcept(module, sourceValue, sliceValue, slice);
    valueTypes[sliceValue] = tileType;
    tiledValues[sliceValue] = {axis, tileSize};
    if (RunBufferizationBubbleUpStrategy(
            module, rewriter, operationId, slice, axis, tileSize,
            tileOffset->second, valueTypes))
      tiledValues[sourceValue] = {axis, tileSize, tileOffset->second};
  }

  std::vector<int> stores;
  for (const auto &[operationId, tileSize] : storeCopyTiles) {
    (void)tileSize;
    if (module.operations.at(static_cast<size_t>(operationId)).operands.size() >=
        2)
      stores.push_back(operationId);
  }
  for (int operationId : stores) {
    const GenericOperation storeSnapshot =
        module.operations.at(static_cast<size_t>(operationId));
    GenericOperation &store =
        module.operations.at(static_cast<size_t>(operationId));
    const size_t axis =
        static_cast<size_t>(
            tiledAnalyzer.getTilingDim(store.operands.front()));
    const std::string destinationType = valueTypes.at(store.operands[1]);
    const auto destination =
        ParseShapedTypeForDimensionAnalysis(destinationType);
    if (!destination || axis >= destination->shape.size())
      continue;
    const bool dynamicDestination = std::any_of(
        destination->shape.begin(), destination->shape.end(),
        [](const std::optional<int64_t> &extent) { return !extent; });
    if (dynamicDestination) {
      const auto definitions = DefiningOperations(module);
      auto sourceSliceDefinition = definitions.find(store.operands.front());
      auto destinationSubviewDefinition = definitions.find(store.operands[1]);
      if (sourceSliceDefinition == definitions.end() ||
          destinationSubviewDefinition == definitions.end() ||
          (sourceSliceDefinition->second->name != "tensor.extract_slice" &&
           sourceSliceDefinition->second->name != "memref.subview") ||
          destinationSubviewDefinition->second->name != "memref.subview" ||
          sourceSliceDefinition->second->operands.empty() ||
          destinationSubviewDefinition->second->operands.empty() ||
          !HasTileAndBindUnitStride(*sourceSliceDefinition->second) ||
          !HasTileAndBindUnitStride(*destinationSubviewDefinition->second))
        continue;

      auto sourceParentType =
          valueTypes.find(sourceSliceDefinition->second->operands.front());
      auto destinationParentType =
          valueTypes.find(destinationSubviewDefinition->second->operands.front());
      const std::optional<ShapedTypeModel> sourceParent =
          sourceParentType == valueTypes.end()
              ? std::nullopt
              : ParseShapedTypeForDimensionAnalysis(sourceParentType->second);
      const std::optional<ShapedTypeModel> destinationParent =
          destinationParentType == valueTypes.end()
              ? std::nullopt
              : ParseShapedTypeForDimensionAnalysis(
                    destinationParentType->second);
      if (!sourceParent || !destinationParent ||
          sourceParent->shape.size() != destinationParent->shape.size() ||
          axis >= sourceParent->shape.size() || !sourceParent->shape[axis] ||
          !destinationParent->shape[axis] ||
          *destinationParent->shape[axis] < 2)
        continue;
      bool parentShapesMatch = true;
      for (size_t dimension = 0; dimension < sourceParent->shape.size();
           ++dimension) {
        if (dimension == axis)
          continue;
        parentShapesMatch &=
            sourceParent->shape[dimension] == destinationParent->shape[dimension];
      }
      const int64_t tileSize = (*destinationParent->shape[axis] + 1) / 2;
      parentShapesMatch &= *sourceParent->shape[axis] ==
                           *destinationParent->shape[axis] ||
                           *sourceParent->shape[axis] == tileSize;
      if (!parentShapesMatch)
        continue;
      // handleMaskedStore calls modifyStoreCopyOp first and then computes a
      // second offset for the extract-of-extract rewrite.
      const int tileOffset =
          createOffsetAtTilingDim(tileSize, loopBlock, 0);
      const int sourceSliceId = sourceSliceDefinition->second->id;
      const int destinationSubviewId =
          destinationSubviewDefinition->second->id;
      const GenericOperation destinationSubview =
          *destinationSubviewDefinition->second;
      auto destinationParentDefinition = definitions.find(
          destinationSubview.operands.front());
      if (destinationParentDefinition == definitions.end() ||
          destinationParentDefinition->second->blockId < 0)
        continue;
      const GenericOperation destinationParentOperation =
          *destinationParentDefinition->second;
      if (!HandleTileAndBindExtractOfExtract(
              module, rewriter, sourceSliceId, axis, tileOffset, tileSize) ||
          !HandleTileAndBindExtractOfExtract(
              module, rewriter, destinationSubviewId, axis, tileOffset,
              tileSize))
        continue;
      std::vector<std::optional<int64_t>> tileShape =
          destinationParent->shape;
      tileShape[axis] = tileSize;
      const std::string tiledParentType = ReplaceTileAndBindShapeDimension(
          destinationParentType->second, axis, tileSize);
      const int parentSubview = rewriter.createOperation(
          destinationParentOperation.parentId,
          destinationParentOperation.regionId,
          destinationParentOperation.blockId, "memref.subview",
          {tiledParentType},
          {destinationSubview.operands.front(), tileOffset},
          {destinationParentType->second, "index"},
          TileAndBindSliceProperties(tileShape, axis),
          "{to_be_bubbled_slice}");
      rewriter.insertToBlock(
          destinationParentOperation.blockId,
          static_cast<size_t>(destinationParentOperation.ordinal + 1),
          parentSubview);
      const int parentSubviewValue =
          module.operations.at(static_cast<size_t>(parentSubview))
              .results.front();
      module.operations.at(static_cast<size_t>(destinationSubview.id))
          .operands[0] = parentSubviewValue;
      valueTypes[parentSubviewValue] = tiledParentType;
      GenericOperation &updatedStore =
          module.operations.at(static_cast<size_t>(operationId));
      updatedStore.attributes = AddTileAndBindUnitAttribute(
          updatedStore.attributes, "tiled_op");
      continue;
    }
    if (!destination->shape[axis] || *destination->shape[axis] < 2)
      continue;
    int64_t tileSize = (*destination->shape[axis] + 1) / 2;
    auto tiledSource = tiledValues.find(store.operands.front());
    if (tiledSource != tiledValues.end() &&
        tiledSource->second.axis == axis)
      tileSize = tiledSource->second.tileSize;
    auto storeOffset = storeCopyOffsets.find(operationId);
    if (storeOffset == storeCopyOffsets.end())
      throw std::runtime_error(
          "TileAndBindSubBlock: TileAndSliceStoreCopyOp has no offset");
    const std::string tileType = ReplaceTileAndBindShapeDimension(
        destinationType, axis, tileSize);
    const bool destinationNeedsSlice =
        *destination->shape[axis] != tileSize;
    if (destinationNeedsSlice) {
      std::vector<std::optional<int64_t>> tileShape = destination->shape;
      tileShape[axis] = tileSize;
      int insertionBlock = storeSnapshot.blockId;
      size_t insertionPosition = static_cast<size_t>(storeSnapshot.ordinal);
      int insertionParent = storeSnapshot.parentId;
      int insertionRegion = storeSnapshot.regionId;
      const auto definitions = DefiningOperations(module);
      auto destinationDefinition =
          definitions.find(storeSnapshot.operands[1]);
      if (destinationDefinition != definitions.end() &&
          destinationDefinition->second->blockId >= 0) {
        const GenericOperation &anchor = *destinationDefinition->second;
        insertionBlock = anchor.blockId;
        insertionPosition = static_cast<size_t>(anchor.ordinal + 1);
        insertionParent = anchor.parentId;
        insertionRegion = anchor.regionId;
      }
      const std::string sliceName =
          destination->tensor ? "tensor.extract_slice" : "memref.subview";
      const int subview = rewriter.createOperation(
          insertionParent, insertionRegion, insertionBlock,
          sliceName, {tileType},
          {storeSnapshot.operands[1], storeOffset->second},
          {destinationType, "index"},
          TileAndBindSliceProperties(tileShape, axis),
          "{to_be_bubbled_slice}");
      const int subviewValue =
          module.operations.at(static_cast<size_t>(subview)).results.front();
      rewriter.insertToBlock(insertionBlock, insertionPosition, subview);
      module.operations.at(static_cast<size_t>(operationId)).operands[1] =
          subviewValue;
      valueTypes[subviewValue] = tileType;
    }
    GenericOperation &updatedStore =
        module.operations.at(static_cast<size_t>(operationId));
    updatedStore.attributes =
        AddTileAndBindUnitAttribute(updatedStore.attributes, "tiled_op");
    if (!updatedStore.results.empty()) {
      updatedStore.resultTypes.front() = ReplaceTileAndBindShapeDimension(
          updatedStore.resultTypes.front(), axis, tileSize);
      valueTypes[updatedStore.results.front()] =
          updatedStore.resultTypes.front();
      tiledValues[updatedStore.results.front()] = {
          axis, tileSize, storeOffset->second};
    }
  }

  valueTypes = ValueTypes(module);
  for (GenericOperation &operation : module.operations)
    for (size_t index = 0;
         index < operation.operands.size() && index < operation.operandTypes.size();
         ++index) {
      auto type = valueTypes.find(operation.operands[index]);
      if (type != valueTypes.end())
        operation.operandTypes[index] = type->second;
    }
  return true;
}

inline void RunTileAndBindOperationFolder(GenericModule &module,
                                          int functionId) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  if (function.regions.size() != 1)
    return;
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(function.regions.front()));
  if (region.blocks.empty())
    return;
  const int entryBlock = region.blocks.front();

  int mappedLoop = -1;
  for (int operationId :
       module.blocks.at(static_cast<size_t>(entryBlock)).operations) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "scf.for" &&
        HasSplitMixDictionaryEntry(operation.attributes,
                                   "map_for_to_forall")) {
      mappedLoop = operationId;
      break;
    }
  }
  if (mappedLoop < 0)
    return;

  std::vector<int> constants;
  std::function<void(int)> collect = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          if (module.operations.at(static_cast<size_t>(child)).name ==
              "arith.constant")
            constants.push_back(child);
          collect(child);
        }
  };
  collect(mappedLoop);

  GenericRewriter rewriter(module);
  // applyPatternsGreedily visits the cloned loop body in reverse order while
  // its operation folder materializes constants in the function entry block.
  // Workspace offsets follow the same ordering as every other constant.
  std::reverse(constants.begin(), constants.end());

  for (int operationId : constants)
    rewriter.removeFromBlock(
        module.operations.at(static_cast<size_t>(operationId)).blockId,
        operationId);

  auto constantKey = [](const GenericOperation &operation) {
    std::string value = FindDictionaryValue(operation.properties, "value");
    if (value.empty())
      value = FindDictionaryValue(operation.attributes, "value");
    return std::make_tuple(operation.name, value,
                           operation.resultTypes.front());
  };
  std::map<std::tuple<std::string, std::string, std::string>, int> unique;
  for (int operationId :
       module.blocks.at(static_cast<size_t>(entryBlock)).operations) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "arith.constant" || operation.results.size() != 1 ||
        operation.resultTypes.size() != 1)
      continue;
    unique[constantKey(operation)] = operationId;
  }
  size_t insertionPosition = 0;
  for (int operationId : constants) {
    const GenericOperation operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.results.size() != 1 || operation.resultTypes.size() != 1)
      continue;
    const auto key = constantKey(operation);
    auto existing = unique.find(key);
    if (existing != unique.end()) {
      ReplaceSplitMixValue(
          module, operation.results.front(),
          module.operations.at(static_cast<size_t>(existing->second))
              .results.front());
      continue;
    }
    unique[key] = operationId;
    rewriter.insertToBlock(entryBlock, insertionPosition++, operationId);
  }
}

inline bool RunTileAndBindHoistAffineIteration(GenericModule &module,
                                               int functionId) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  const std::vector<int> worklist =
      GetTileAndBindGreedyRewriteOrder(module, function);
  bool changed = false;
  for (int operationId : worklist)
    changed |= ApplyHoistAffinePattern(module, operationId);
  return changed;
}

inline void RunTileAndBindHoistAffine(GenericModule &module,
                                      int functionId) {
  for (size_t iteration = 0; iteration < 50; ++iteration)
    if (!RunTileAndBindHoistAffineIteration(module, functionId))
      return;
  throw std::runtime_error(
      "HIVMBubbleUpExtractSlice: HoistAffine exceeded max iterations");
}

inline bool IsHIVMBubbleUpEquivalentOperation(const GenericOperation &lhs,
                                              const GenericOperation &rhs) {
  return lhs.name == rhs.name && lhs.operands == rhs.operands &&
         lhs.operandTypes == rhs.operandTypes &&
         lhs.resultTypes == rhs.resultTypes &&
         lhs.properties == rhs.properties && lhs.attributes == rhs.attributes;
}

inline std::vector<int> WalkHIVMBubbleUpBlock(const GenericModule &module,
                                             int blockId) {
  std::vector<int> operations;
  std::function<void(int)> walk = [&](int operationId) {
    operations.push_back(operationId);
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int childBlock :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(childBlock)).operations)
          walk(child);
  };
  for (int operationId :
       module.blocks.at(static_cast<size_t>(blockId)).operations)
    walk(operationId);
  return operations;
}

// populateCSEPattern::CSEAffineApplyPattern. The pattern replaces uses of all
// siblings with siblingGroup[0], but deliberately does not erase any sibling;
// GreedyPatternRewriteDriver owns the later trivially-dead removal.
inline bool ApplyCSEAffineApplyPattern(GenericModule &module,
                                       int operationId) {
  const GenericOperation base =
      module.operations.at(static_cast<size_t>(operationId));
  if (base.name != "affine.apply" || base.blockId < 0)
    return false;
  const std::vector<int> &baseBlock =
      module.blocks.at(static_cast<size_t>(base.blockId)).operations;
  if (std::find(baseBlock.begin(), baseBlock.end(), operationId) ==
      baseBlock.end())
    return false;

  std::vector<int> siblingGroup;
  for (int candidateId : WalkHIVMBubbleUpBlock(module, base.blockId)) {
    const GenericOperation &candidate =
        module.operations.at(static_cast<size_t>(candidateId));
    if (candidate.name == "affine.apply" &&
        IsHIVMBubbleUpEquivalentOperation(candidate, base))
      siblingGroup.push_back(candidateId);
  }
  if (siblingGroup.size() == 1)
    return false;
  const GenericOperation representative =
      module.operations.at(static_cast<size_t>(siblingGroup.front()));
  for (size_t index = 1; index < siblingGroup.size(); ++index) {
    const GenericOperation sibling =
        module.operations.at(static_cast<size_t>(siblingGroup[index]));
    for (size_t result = 0;
         result < sibling.results.size() &&
         result < representative.results.size();
         ++result)
      ReplaceTileAndBindValueExcept(module, sibling.results[result],
                                    representative.results[result], -1);
  }
  return true;
}

inline bool IsTriviallyDeadTileAndBindAffine(const GenericModule &module,
                                             int operationId) {
  const GenericOperation &operation =
      module.operations.at(static_cast<size_t>(operationId));
  if (operation.name != "affine.apply" && operation.name != "affine.min" &&
      operation.name != "affine.max")
    return false;
  for (int result : operation.results)
    for (const GenericOperation &user : module.operations)
      if (std::find(user.operands.begin(), user.operands.end(), result) !=
          user.operands.end())
        return false;
  return true;
}

inline void RunCSEAffineApplyPattern(GenericModule &module, int functionId) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  for (size_t iteration = 0; iteration < 50; ++iteration) {
    bool changed = false;
    GenericRewriter rewriter(module);
    for (int operationId :
         GetTileAndBindGreedyRewriteOrder(module, function)) {
      GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.blockId < 0)
        continue;
      const std::vector<int> &block =
          module.blocks.at(static_cast<size_t>(operation.blockId)).operations;
      if (std::find(block.begin(), block.end(), operationId) == block.end())
        continue;
      if (IsTriviallyDeadTileAndBindAffine(module, operationId)) {
        rewriter.removeFromBlock(operation.blockId, operationId);
        changed = true;
        continue;
      }
      if (ApplyHoistAffinePattern(module, operationId)) {
        changed = true;
        ApplyCSEAffineApplyPattern(module, operationId);
        continue;
      }
      changed |= ApplyCSEAffineApplyPattern(module, operationId);
    }
    if (!changed)
      return;
  }
  throw std::runtime_error(
      "HIVMBubbleUpExtractSlice: affine/CSE greedy rewrite exceeded max "
      "iterations");
}

// populateBindSubBlockBubbleUpPassManager runs CSE after the bubble-up
// canonicalizer. This is intentionally local to the candidate function: the
// outer canonicalizationHIVMPipeline still owns module-wide CSE.
inline void RunTileAndBindBubbleUpCSE(GenericModule &module, int functionId) {
  const std::vector<int> descendants = GetTileAndBindDescendants(
      module, module.operations.at(static_cast<size_t>(functionId)));
  std::map<std::string, std::vector<int>> available;
  GenericRewriter rewriter(module);
  std::map<int, size_t> uses;
  for (const GenericOperation &operation : module.operations)
    for (int operand : operation.operands)
      ++uses[operand];
  for (int operationId : descendants) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "tensor.empty" || operation.blockId < 0 ||
        std::any_of(operation.results.begin(), operation.results.end(),
                    [&](int result) { return uses.count(result) != 0; }))
      continue;
    rewriter.removeFromBlock(operation.blockId, operationId);
  }
  for (int operationId : descendants) {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    if (snapshot.results.empty() || snapshot.blockId < 0 ||
        !snapshot.regions.empty() ||
        (!snapshot.effects.empty() && snapshot.effects != "none" &&
         snapshot.name != "tensor.empty"))
      continue;
    const std::vector<int> &blockOperations =
        module.blocks.at(static_cast<size_t>(snapshot.blockId)).operations;
    if (std::find(blockOperations.begin(), blockOperations.end(),
                  operationId) == blockOperations.end())
      continue;
    std::ostringstream keyStream;
    keyStream << snapshot.name << '\n'
              << JoinDelimited(snapshot.resultTypes, ",") << '\n'
              << JoinDelimited(snapshot.operandTypes, ",") << '\n'
              << joinIds(snapshot.operands) << '\n'
              << snapshot.properties << '\n'
              << snapshot.attributes;
    const std::string key = keyStream.str();
    int dominating = -1;
    for (auto candidate = available[key].rbegin();
         candidate != available[key].rend(); ++candidate) {
      if (GenericOperationDominates(
              module, module.operations.at(static_cast<size_t>(*candidate)),
              snapshot)) {
        dominating = *candidate;
        break;
      }
    }
    if (dominating < 0) {
      available[key].push_back(operationId);
      continue;
    }
    const GenericOperation &candidate =
        module.operations.at(static_cast<size_t>(dominating));
    if (candidate.results.size() != snapshot.results.size())
      continue;
    for (size_t index = 0; index < snapshot.results.size(); ++index)
      ReplaceTileAndBindValueExcept(module, snapshot.results[index],
                                    candidate.results[index], -1);
    rewriter.removeFromBlock(snapshot.blockId, operationId);
  }
}

inline GenericModule LimitUniqueSubBlockToStore(GenericModule module) {
  std::vector<int> candidates;
  for (const GenericOperation &function : module.operations) {
    if (!IsMixAIVFunction(function))
      continue;
    for (int operationId : GetTileAndBindDescendants(module, function)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (ShouldLimitUniqueSubBlock(operation) &&
          !IsAlreadyLimitedToUniqueSubBlock(module, operation))
        candidates.push_back(operationId);
    }
  }
  if (candidates.empty())
    return module;

  GenericRewriter rewriter(module);
  for (int operationId : candidates) {
    const GenericOperation source =
        module.operations.at(static_cast<size_t>(operationId));
    if (source.blockId < 0)
      throw std::runtime_error(
          "TileAndBindSubBlock: store/copy has no parent block");
    if (!source.results.empty() &&
        source.results.size() > source.dpsInits.size())
      throw std::runtime_error(
          "TileAndBindSubBlock: resultful store/copy has no destination");

    const int parent = source.parentId;
    const int region = source.regionId;
    const int block = source.blockId;
    const size_t position = static_cast<size_t>(source.ordinal);

    const int subBlock = rewriter.createOperation(
        parent, region, block, "hivm.hir.get_sub_block_idx", {"i64"});
    const int subBlockValue =
        module.operations.at(static_cast<size_t>(subBlock)).results.front();
    const int indexCast = rewriter.createOperation(
        parent, region, block, "arith.index_cast", {"index"},
        {subBlockValue}, {"i64"});
    const int indexValue =
        module.operations.at(static_cast<size_t>(indexCast)).results.front();
    std::optional<int> zeroValue =
        FindArithConstantValue(module, block, "index", "0");
    std::optional<int> zero;
    if (!zeroValue) {
      zero = rewriter.createOperation(
          parent, region, block, "arith.constant", {"index"}, {}, {},
          "{value = 0 : index}");
      zeroValue = module.operations.at(static_cast<size_t>(*zero))
                      .results.front();
    }
    const int compare = rewriter.createOperation(
        parent, region, block, "arith.cmpi", {"i1"},
        {indexValue, *zeroValue}, {"index", "index"},
        "{predicate = 0 : i64}");
    const int condition =
        module.operations.at(static_cast<size_t>(compare)).results.front();
    const int ifOperation = rewriter.createOperation(
        parent, region, block, "scf.if", source.resultTypes, {condition},
        {"i1"}, "", "{limit_sub_block_id0}");

    const int thenRegion = rewriter.createRegion(ifOperation);
    const int thenBlock = rewriter.createBlock(thenRegion, {});
    std::map<int, int> values;
    const int cloned = rewriter.cloneOperationTree(
        operationId, ifOperation, thenRegion, thenBlock, values);
    rewriter.appendToBlock(thenBlock, cloned);
    const std::vector<int> thenResults =
        module.operations.at(static_cast<size_t>(cloned)).results;
    const std::vector<std::string> yieldTypes = source.resultTypes;
    const int thenYield = rewriter.createOperation(
        ifOperation, thenRegion, thenBlock, "scf.yield", {}, thenResults,
        yieldTypes);
    rewriter.appendToBlock(thenBlock, thenYield);

    if (!source.results.empty()) {
      const int elseRegion = rewriter.createRegion(ifOperation);
      const int elseBlock = rewriter.createBlock(elseRegion, {});
      std::vector<int> elseValues;
      elseValues.reserve(source.results.size());
      for (size_t index = 0; index < source.results.size(); ++index)
        elseValues.push_back(source.dpsInits[index]);
      const int elseYield = rewriter.createOperation(
          ifOperation, elseRegion, elseBlock, "scf.yield", {}, elseValues,
          yieldTypes);
      rewriter.appendToBlock(elseBlock, elseYield);

      const std::vector<int> ifResults =
          module.operations.at(static_cast<size_t>(ifOperation)).results;
      for (size_t index = 0; index < source.results.size(); ++index)
        ReplaceSplitMixValue(module, source.results[index], ifResults[index]);
    }

    rewriter.removeFromBlock(block, operationId);
    rewriter.insertToBlock(block, position, subBlock);
    rewriter.insertToBlock(block, position + 1, indexCast);
    size_t nextPosition = position + 2;
    if (zero)
      rewriter.insertToBlock(block, nextPosition++, *zero);
    rewriter.insertToBlock(block, nextPosition++, compare);
    rewriter.insertToBlock(block, nextPosition, ifOperation);
  }
  ApplyOperationSemanticsToAll(module.operations);
  return CompactGenericModule(std::move(module));
}

inline GenericModule RunTileAndBindSubBlock(GenericModule module) {
  module = RunTileAndBindSubBlockEarlyPatterns(std::move(module));
  std::vector<int> aivFunctions;
  for (const GenericOperation &function : module.operations)
    if (IsMixAIVFunction(function))
      aivFunctions.push_back(function.id);
  if (aivFunctions.empty())
    return module;

  if (HasBatchMatmulLoopInAICFunctions(module) ||
      HasImplicitTransposeWithLastAxisInAIVFunctions(module) ||
      AreLoadAndStoreSameAddress(module))
    return LimitUniqueSubBlockToStore(std::move(module));

  for (int functionId : aivFunctions) {
    GenericModule candidate = module;
    DimensionAnalyzer analyzer(candidate);
    if (!analyzer.initialize())
      return LimitUniqueSubBlockToStore(std::move(module));
    analyzer.computeTilingDim(
        candidate.operations.at(static_cast<size_t>(functionId)));
    if (!analyzer.hasSelectedTilingDim() ||
        !AttemptBindSubBlock(candidate, functionId) ||
        !VerifyTileAndBindReshapeTypes(candidate) ||
        !VerifyTileAndBindSliceTypes(candidate))
      return LimitUniqueSubBlockToStore(std::move(module));
    RunTileAndBindHoistAffine(candidate, functionId);
    RunCSEAffineApplyPattern(candidate, functionId);
    RunTileAndBindOperationFolder(candidate, functionId);
    RunTileAndBindBubbleUpCSE(candidate, functionId);
    RunTileAndBindHoistAffine(candidate, functionId);
    RunCSEAffineApplyPattern(candidate, functionId);
    module = std::move(candidate);
  }
  // The real pass applies limitUniqueSubBlockToStore to the successfully
  // tiled AIV functions as well.  It guards only operations that could not be
  // sliced (for example, a scalar workspace store), while operations marked
  // tiled_op continue to run independently on both sub-blocks.
  module = LimitUniqueSubBlockToStore(std::move(module));
  ApplyOperationSemanticsToAll(module.operations);
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
