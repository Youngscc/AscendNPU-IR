#ifndef CVPIPELINE_UB_MODEL_CPP_TILE_AND_BIND_SUB_BLOCK_HPP
#define CVPIPELINE_UB_MODEL_CPP_TILE_AND_BIND_SUB_BLOCK_HPP

#include "../analysis/hivm_dimension_analyzer.hpp"
#include "../analysis/generic_pipeline_context.hpp"
#include "../ir/generic_analysis.hpp"
#include "../ir/operation_folder.hpp"
#include "../support/debug_trace.hpp"
#include "canonicalization_hivm_pipeline.hpp"
#include "fold_tensor_empty.hpp"
#include "split_mix_kernel.hpp"

namespace cvub {

// CanonicalizeAllocToTensor from TileUtils.cpp. This rewrite runs before
// collecting MIX functions and before every TileAndBindSubBlock early exit.
inline GenericModule RunTileAndBindSubBlockEarlyPatterns(
    GenericModule module) {
  struct AllocationUse {
    int operation = -1;
    int user = -1;
    bool multipleUsers = false;
  };
  std::unordered_map<int, AllocationUse> allocationUses;
  std::vector<int> allocationValues;
  allocationUses.reserve(module.operations.size() / 8 + 1);
  for (const GenericOperation &operation : module.operations) {
    if (operation.name == "memref.alloc" && operation.results.size() == 1 &&
        operation.blockId >= 0) {
      allocationUses.emplace(operation.results.front(),
                             AllocationUse{operation.id, -1, false});
      allocationValues.push_back(operation.results.front());
    }
  }
  if (allocationUses.empty())
    return module;

  // CanonicalizeAllocToTensor only queries uses of memref.alloc results.
  // Indexing every operand in the module created a much larger ordered map
  // that was immediately discarded on the common no-match path.
  for (const GenericOperation &operation : module.operations) {
    for (int operand : operation.operands) {
      auto allocation = allocationUses.find(operand);
      if (allocation == allocationUses.end())
        continue;
      AllocationUse &use = allocation->second;
      if (use.user < 0)
        use.user = operation.id;
      else if (use.user != operation.id)
        use.multipleUsers = true;
    }
  }

  GenericRewriter rewriter(module);
  bool changed = false;
  for (int allocationValue : allocationValues) {
    const AllocationUse &use = allocationUses.at(allocationValue);
    if (use.user < 0 || use.multipleUsers)
      continue;
    const GenericOperation &allocationSnapshot =
        module.operations.at(static_cast<size_t>(use.operation));
    const GenericOperation &toTensorCandidate =
        module.operations.at(static_cast<size_t>(use.user));
    if (toTensorCandidate.name != "bufferization.to_tensor" ||
        toTensorCandidate.operands.size() != 1 ||
        toTensorCandidate.results.size() != 1)
      continue;

    GenericOperation &toTensor =
        rewriter.modifyOperation(toTensorCandidate.id);
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
    changed = true;
  }
  if (!changed)
    return module;
  rewriter.applyDirtyOperationSemantics();
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

inline std::vector<int> GetTileAndBindAffineGreedyRewriteOrder(
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
    if (operation.name == "affine.apply" || operation.name == "affine.min" ||
        operation.name == "affine.max")
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

inline bool HasBatchMatmulLoopInAICFunctions(
    const GenericModule &module,
    const GenericModuleAnalysisSnapshot &analysis) {
  for (const GenericOperation &function : module.operations) {
    if (!IsMixAICFunction(function))
      continue;
    for (int operationId : analysis.descendants(function)) {
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
    const GenericModule &module,
    const GenericModuleAnalysisSnapshot &analysis) {
  for (const GenericOperation &function : module.operations) {
    if (!IsMixAIVFunction(function))
      continue;
    for (int operationId : analysis.descendants(function)) {
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
    int value, const GenericModule &module,
    const GenericModuleAnalysisSnapshot &analysis) {
  std::set<int> visited;
  while (visited.insert(value).second) {
    const GenericOperation *operation = analysis.definingOperation(value);
    if (operation) {
      if (operation->name == "memref.reinterpret_cast")
        return operation;

      // Keep this list aligned with the non-materializing view operations
      // crossed by HIVMImpl.h::traceDefOp.  In particular, HIVM load sources
      // normally reach a reinterpret_cast through bufferization.to_tensor.
      static const std::set<std::string> transparentOperations = {
          "tensor.reshape",
          "memref.collapse_shape",
          "tensor.collapse_shape",
          "memref.subview",
          "bufferization.to_memref",
          "bufferization.to_buffer",
          "bufferization.to_tensor",
          "memref.view",
          "memref.reshape",
          "memref.expand_shape",
          "tensor.expand_shape",
          "memref.extract_strided_metadata",
          "memref.cast",
          "memref.memory_space_cast",
          "tensor.extract_slice",
          "tensor.insert_slice"};
      if (transparentOperations.count(operation->name) != 0) {
        if (operation->operands.empty())
          return nullptr;
        value = operation->operands.front();
        continue;
      }

      auto result = std::find(operation->results.begin(),
                              operation->results.end(), value);
      if (result == operation->results.end())
        return nullptr;
      const size_t resultNumber =
          static_cast<size_t>(result - operation->results.begin());
      if (operation->name == "scf.if") {
        if (operation->regions.empty())
          return nullptr;
        const GenericRegion &thenRegion =
            module.regions.at(static_cast<size_t>(operation->regions.front()));
        if (thenRegion.blocks.empty())
          return nullptr;
        const GenericBlock &thenBlock =
            module.blocks.at(static_cast<size_t>(thenRegion.blocks.front()));
        if (thenBlock.operations.empty())
          return nullptr;
        const GenericOperation &yield = module.operations.at(
            static_cast<size_t>(thenBlock.operations.back()));
        if (yield.name != "scf.yield" ||
            resultNumber >= yield.operands.size())
          return nullptr;
        value = yield.operands[resultNumber];
        continue;
      }
      if (operation->name == "scf.for") {
        if (operation->regions.empty())
          return nullptr;
        const GenericRegion &bodyRegion =
            module.regions.at(static_cast<size_t>(operation->regions.front()));
        if (bodyRegion.blocks.empty())
          return nullptr;
        const GenericBlock &body =
            module.blocks.at(static_cast<size_t>(bodyRegion.blocks.front()));
        if (body.operations.empty())
          return nullptr;
        const GenericOperation &yield =
            module.operations.at(static_cast<size_t>(body.operations.back()));
        if (yield.name != "scf.yield" ||
            resultNumber >= yield.operands.size())
          return nullptr;
        value = yield.operands[resultNumber];
        continue;
      }
      return nullptr;
    }

    // traceDefOp also follows a loop block argument to its tied init value.
    const GenericBlock *argumentBlock = nullptr;
    size_t argumentNumber = 0;
    for (const GenericBlock &block : module.blocks) {
      auto argument =
          std::find(block.arguments.begin(), block.arguments.end(), value);
      if (argument == block.arguments.end())
        continue;
      argumentBlock = &block;
      argumentNumber =
          static_cast<size_t>(argument - block.arguments.begin());
      break;
    }
    if (!argumentBlock)
      return nullptr;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(argumentBlock->regionId));
    const GenericOperation &parent = module.operations.at(
        static_cast<size_t>(region.parentOperation));
    if (parent.name != "scf.for" || argumentNumber == 0)
      return nullptr;
    const size_t initNumber = argumentNumber + 2;
    if (initNumber >= parent.operands.size())
      return nullptr;
    value = parent.operands[initNumber];
  }
  return nullptr;
}

inline bool AreLoadAndStoreSameAddress(
    const GenericModule &module,
    const GenericModuleAnalysisSnapshot &analysis) {
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
    const std::vector<int> &descendants = analysis.descendants(function);
    for (int operationId : descendants) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "hivm.hir.load" || operation.operands.empty())
        continue;
      const GenericOperation *reinterpret = TraceTileAndBindReinterpretCast(
          operation.operands.front(), module, analysis);
      if (reinterpret && !reinterpret->operands.empty() &&
          arguments.count(reinterpret->operands.front()) != 0)
        loadedArguments.insert(reinterpret->operands.front());
    }
    for (int operationId : descendants) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "hivm.hir.store" ||
          operation.operands.size() < 2)
        continue;
      const GenericOperation *reinterpret = TraceTileAndBindReinterpretCast(
          operation.operands[1], module, analysis);
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
  const GenericModuleAnalysisSnapshot analysis(
      module, kGenericAnalysisDefinitions | kGenericAnalysisUsers |
                  kGenericAnalysisFunctionDescendants);
  const std::map<int, std::string> valueTypes = ValueTypes(module);
  std::map<int, size_t> dimensions;
  std::function<void(int, size_t)> collect = [&](int value, size_t axis) {
    auto inserted = dimensions.emplace(value, axis);
    if (!inserted.second)
      return;
    const GenericOperation *definition = analysis.definingOperation(value);
    if (!definition)
      return;
    const GenericOperation &operation = *definition;

    // ScopeBubbleUpStrategy follows the region-yield edge from a scope result
    // to the matching scope.return operand. scope.scope has no regular
    // operands, so this edge must be indexed explicitly for the later greedy
    // bubble-up walk.
    if (operation.name == "scope.scope") {
      auto result =
          std::find(operation.results.begin(), operation.results.end(), value);
      if (result == operation.results.end() || operation.regions.size() != 1)
        return;
      const size_t resultIndex = static_cast<size_t>(
          std::distance(operation.results.begin(), result));
      const GenericRegion &region = module.regions.at(
          static_cast<size_t>(operation.regions.front()));
      if (region.blocks.size() != 1)
        return;
      const GenericBlock &block = module.blocks.at(
          static_cast<size_t>(region.blocks.front()));
      if (block.operations.empty())
        return;
      const GenericOperation &terminator = module.operations.at(
          static_cast<size_t>(block.operations.back()));
      if (terminator.name != "scope.return" ||
          resultIndex >= terminator.operands.size())
        return;
      const int returned = terminator.operands[resultIndex];
      const int64_t returnedAxis = analyzer.getTilingDim(returned);
      collect(returned, returnedAxis >= 0
                            ? static_cast<size_t>(returnedAxis)
                            : axis);
      return;
    }

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

  const std::vector<int> &descendants = analysis.descendants(function);
  for (int operationId : descendants) {
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
  for (int operationId : descendants) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "scf.for")
      continue;
    for (size_t resultIndex = 0;
         resultIndex < operation.results.size() &&
         resultIndex < operation.resultTypes.size();
         ++resultIndex) {
      const int result = operation.results[resultIndex];
      if (!analysis.users(result).empty())
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
                                       int functionId,
                                       GenericPipelineContext &context) {
  ApplyOperationSemanticsToAll(module.operations);
  const GenericOperation function =
      module.operations.at(static_cast<size_t>(functionId));
  const std::vector<int> descendants =
      GetTileAndBindDescendants(module, function);
  PipelineAnalysisContext &analysis = context.analysis();
  GenericRewriter rewriter(module, context.listener());
  for (int operationId : descendants) {
    const GenericOperation empty =
        module.operations.at(static_cast<size_t>(operationId));
    if (analysis.operationKind(operationId) !=
            GenericOperationKind::TensorEmpty ||
        empty.results.size() != 1 ||
        empty.blockId < 0)
      continue;
    const int value = empty.results.front();
    const size_t totalUses = analysis.useCount(value);
    std::vector<std::pair<int, size_t>> initUses;
    const std::vector<int> users = analysis.users(value);
    int previousUser = -1;
    for (int userId : users) {
      if (userId == previousUser)
        continue;
      previousUser = userId;
      const GenericOperation &user =
          module.operations.at(static_cast<size_t>(userId));
      const std::vector<size_t> &initIndices =
          context.metadata().dpsInitOperandIndices(
              user.name, user.operands.size(), user.properties);
      const std::set<size_t> initSet(initIndices.begin(), initIndices.end());
      for (size_t operandIndex = 0; operandIndex < user.operands.size();
           ++operandIndex) {
        if (user.operands[operandIndex] != value)
          continue;
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
      rewriter.replaceOperand(userId, operandIndex, clonedValue);
    }
  }
  rewriter.applyDirtyOperationSemantics();
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
  std::vector<int64_t> strides =
      ParseDimensionI64Array(operation.properties, "static_strides");
  if (strides.empty())
    strides =
        ParseDimensionI64Array(operation.attributes, "static_strides");
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
  std::vector<size_t> segments = OperandSegmentSizes(operation.properties);
  if (segments.empty())
    segments = OperandSegmentSizes(operation.attributes);
  const auto arrayValue = [&](const std::string &name) {
    std::vector<int64_t> value =
        ParseDimensionI64Array(operation.properties, name);
    if (value.empty())
      value = ParseDimensionI64Array(operation.attributes, name);
    return value;
  };
  const std::vector<int64_t> staticOffsets = arrayValue("static_offsets");
  const std::vector<int64_t> staticSizes = arrayValue("static_sizes");
  const std::vector<int64_t> staticStrides = arrayValue("static_strides");
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

inline std::optional<size_t> TileAndBindSliceAxisForResultAxis(
    const std::vector<TileAndBindFoldResult> &sizes,
    const std::vector<std::optional<int64_t>> &resultShape,
    size_t requestedResultAxis) {
  size_t resultAxis = 0;
  for (size_t sliceAxis = 0; sliceAxis < sizes.size(); ++sliceAxis) {
    const size_t sliceAxesLeft = sizes.size() - sliceAxis;
    const size_t resultAxesLeft = resultShape.size() - resultAxis;
    const bool mustKeep = sliceAxesLeft == resultAxesLeft;
    const bool compatible =
        resultAxis < resultShape.size() &&
        (!sizes[sliceAxis].constant || !resultShape[resultAxis] ||
         *sizes[sliceAxis].constant == *resultShape[resultAxis]);
    if (mustKeep || compatible) {
      if (resultAxis == requestedResultAxis)
        return sliceAxis;
      ++resultAxis;
      continue;
    }
    if (!sizes[sliceAxis].constant || *sizes[sliceAxis].constant != 1)
      return std::nullopt;
  }
  return std::nullopt;
}

// Projection of the OffsetSizeAndStrideOpInterface verifiers reached by
// newFunc.verify() in TileAndBindSubBlockPass::attemptBindSubBlock.
inline bool VerifyTileAndBindSliceTypes(const GenericModule &module,
                                        int *failedOperation = nullptr) {
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
        (isExtract && operation.results.empty())) {
      if (failedOperation)
        *failedOperation = operation.id;
      return false;
    }

    const int shapedValue = isInsert ? slice->prefixOperands.front()
                                     : operation.results.front();
    auto type = valueTypes.find(shapedValue);
    if (type == valueTypes.end()) {
      if (failedOperation)
        *failedOperation = operation.id;
      return false;
    }
    const std::optional<ShapedTypeModel> shaped =
        ParseShapedTypeForDimensionAnalysis(type->second);
    if (!shaped || !IsRankReducedTileAndBindSliceType(slice->sizes,
                                                       shaped->shape)) {
      if (failedOperation)
        *failedOperation = operation.id;
      return false;
    }
  }
  return true;
}

inline bool IsTileAndBindBlockArgument(const GenericModule &module,
                                       int value) {
  for (const GenericBlock &block : module.blocks)
    if (std::find(block.arguments.begin(), block.arguments.end(), value) !=
        block.arguments.end())
      return true;
  return false;
}

inline bool IsInsideTileAndBindSubBlockLoop(
    const GenericModule &module, const GenericOperation &operation) {
  int parent = operation.parentId;
  size_t remaining = module.operations.size() + 1;
  while (parent >= 0 && remaining-- != 0) {
    const GenericOperation &ancestor =
        module.operations.at(static_cast<size_t>(parent));
    if (ancestor.name == "scf.for" &&
        (HasSplitMixDictionaryEntry(ancestor.attributes,
                                    "map_for_to_forall") ||
         HasSplitMixDictionaryEntry(ancestor.properties,
                                    "map_for_to_forall")) &&
        (ancestor.attributes.find("hivm.sub_block") != std::string::npos ||
         ancestor.properties.find("hivm.sub_block") != std::string::npos))
      return true;
    parent = ancestor.parentId;
  }
  return false;
}

// HIVMImpl.h::traceDefOp<memref::AllocOp> used by
// HIVMBubbleUpExtractSlice::traceAndCheckIsGM.  The verifier only asks whether
// a bufferization.to_tensor source ultimately comes from a local allocation.
inline bool TileAndBindTracesToLocalAllocation(
    const GenericModule &module, int value,
    const GenericModuleAnalysisSnapshot &analysis) {
  static const std::set<std::string> transparentOperations = {
      "tensor.reshape",
      "memref.collapse_shape",
      "tensor.collapse_shape",
      "memref.subview",
      "bufferization.to_memref",
      "bufferization.to_buffer",
      "bufferization.to_tensor",
      "memref.view",
      "memref.reshape",
      "memref.expand_shape",
      "tensor.expand_shape",
      "memref.extract_strided_metadata",
      "memref.cast",
      "memref.memory_space_cast",
      "tensor.extract_slice",
      "tensor.insert_slice"};
  std::set<int> visited;
  while (visited.insert(value).second) {
    const GenericOperation *definition = analysis.definingOperation(value);
    if (definition) {
      if (definition->name == "memref.alloc")
        return true;
      if (transparentOperations.count(definition->name) != 0) {
        if (definition->operands.empty())
          return false;
        value = definition->operands.front();
        continue;
      }
      return false;
    }

    const GenericBlock *argumentBlock = nullptr;
    size_t argumentNumber = 0;
    for (const GenericBlock &block : module.blocks) {
      auto argument =
          std::find(block.arguments.begin(), block.arguments.end(), value);
      if (argument == block.arguments.end())
        continue;
      argumentBlock = &block;
      argumentNumber =
          static_cast<size_t>(argument - block.arguments.begin());
      break;
    }
    if (!argumentBlock)
      return false;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(argumentBlock->regionId));
    const GenericOperation &parent = module.operations.at(
        static_cast<size_t>(region.parentOperation));
    if (parent.name != "scf.for" || argumentNumber == 0)
      return false;
    const size_t initNumber = argumentNumber + 2;
    if (initNumber >= parent.operands.size())
      return false;
    value = parent.operands[initNumber];
  }
  return false;
}

// Exact projection of
// HIVMBubbleUpExtractSlicePass::verifyMarkedExtractSlicesAreBubbledUp for the
// A2/A3 pipeline.  The production pass rejects a candidate even when all
// slice types verify if a strict-mode marker remains inside the sub-block loop.
inline bool VerifyTileAndBindMarkedSlicesAreBubbledUp(
    const GenericModule &module, int functionId, bool strictMode,
    int *failedOperation = nullptr) {
  const GenericModuleAnalysisSnapshot analysis(
      module, kGenericAnalysisDefinitions |
                  kGenericAnalysisUsers |
                  kGenericAnalysisFunctionDescendants);
  // The real pass is nested on func.func and uses funcOp->walk.  GenericModule
  // keeps erased operations as detached tombstones until compaction, so walking
  // the backing operation table would incorrectly verify dead rewrite
  // intermediates that are no longer part of the IR tree.  Walk the live
  // descendants of the same function instead.
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  const std::vector<int> &functionDescendants = analysis.descendants(function);
  const std::set<int> liveDescendants(functionDescendants.begin(),
                                      functionDescendants.end());
  for (int operationId : functionDescendants) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "tensor.insert_slice" &&
        HasSplitMixDictionaryEntry(operation.attributes,
                                   "to_be_canceled_out_insert_slice")) {
      if (failedOperation)
        *failedOperation = operation.id;
      return false;
    }
    if (operation.name != "tensor.extract_slice" ||
        !HasSplitMixDictionaryEntry(operation.attributes,
                                    "to_be_bubbled_slice") ||
        operation.operands.empty())
      continue;

    const int source = operation.operands.front();
    if (IsTileAndBindBlockArgument(module, source))
      continue;
    const GenericOperation *sourceDefinition =
        analysis.definingOperation(source);

    // TileAndBindSubBlock::modifyOpToSliced marks the tensor slice injected
    // for a tiled operand.  On a writable to_tensor destination, when that
    // slice feeds an existing nested slice, the production greedy rewrite can
    // leave the marker on the nested slice after the extract-of-extract
    // transaction.  The compact Generic rewrite updates that child in place
    // and otherwise loses this transient marker, so project the equivalent
    // verifier state here.  Its extract_slice source is unsupported in strict
    // mode and production rolls the complete TileAndBind candidate back.
    if (strictMode && sourceDefinition &&
        sourceDefinition->name == "bufferization.to_tensor" &&
        HasSplitMixDictionaryEntry(sourceDefinition->attributes, "writable") &&
        !operation.results.empty()) {
      for (int userId : analysis.users(operation.results.front())) {
        if (liveDescendants.count(userId) == 0)
          continue;
        const GenericOperation &user =
            module.operations.at(static_cast<size_t>(userId));
        if (user.name != "tensor.extract_slice" || user.operands.empty() ||
            user.operands.front() != operation.results.front() ||
            !IsInsideTileAndBindSubBlockLoop(module, user) ||
            // Preload workspace slices are CVPipelining artifacts.  The
            // production rewrite preserves them instead of taking this
            // writable-destination extract-of-extract marker path.
            HasSplitMixDictionaryEntry(user.attributes,
                                       "hivm.preload_workspace"))
          continue;
        if (failedOperation)
          *failedOperation = user.id;
        return false;
      }
    }
    if (!sourceDefinition ||
        !IsInsideTileAndBindSubBlockLoop(module, *sourceDefinition))
      continue;
    if (sourceDefinition->name == "bufferization.to_tensor") {
      if (strictMode && !sourceDefinition->operands.empty() &&
          TileAndBindTracesToLocalAllocation(
              module, sourceDefinition->operands.front(), analysis)) {
        if (failedOperation)
          *failedOperation = operation.id;
        return false;
      }
      continue;
    }
    if (sourceDefinition->name == "scf.while") {
      if (failedOperation)
        *failedOperation = operation.id;
      return false;
    }
    const bool acceptedSource =
        sourceDefinition->name == "tensor.empty" ||
        (sourceDefinition->name == "scf.for" &&
         HasSplitMixDictionaryEntry(sourceDefinition->attributes,
                                    "ExtractedLoadOrStore")) ||
        HasSplitMixDictionaryEntry(sourceDefinition->attributes, "tiled_op");
    if (!acceptedSource && strictMode) {
      if (failedOperation)
        *failedOperation = operation.id;
      return false;
    }
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
    const GenericModule &module, const GenericOperation &operation,
    const std::map<int, std::string> *indexedValueTypes = nullptr) {
  const std::optional<TileAndBindMixedSlice> slice =
      ParseTileAndBindMixedSlice(operation);
  if (!slice || slice->prefixOperands.empty())
    return {};
  const std::map<int, std::string> ownedValueTypes =
      indexedValueTypes ? std::map<int, std::string>{} : ValueTypes(module);
  const std::map<int, std::string> &valueTypes =
      indexedValueTypes ? *indexedValueTypes : ownedValueTypes;
  // Helper.cpp::getOriginalType uses the source type for extract/subview, but
  // the destination type for tensor.insert_slice.  Comparing an insert's
  // sizes against its smaller source tensor hides exactly the dimensions in
  // which it is inserting into a larger destination.
  const size_t originalOperand =
      operation.name == "tensor.insert_slice" ? 1 : 0;
  if (originalOperand >= slice->prefixOperands.size())
    return {};
  auto originalType =
      valueTypes.find(slice->prefixOperands[originalOperand]);
  const std::optional<ShapedTypeModel> source =
      originalType == valueTypes.end()
          ? std::nullopt
          : ParseShapedTypeForDimensionAnalysis(originalType->second);
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
                                       const GenericOperation &sliceOperation,
                                       const std::map<int, std::string>
                                           *indexedValueTypes = nullptr,
                                       const std::map<int,
                                           const GenericOperation *>
                                           *indexedDefinitions = nullptr,
                                       const std::vector<int>
                                           *indexedDefinitionIds = nullptr) {
  const std::optional<TileAndBindMixedSlice> slice =
      ParseTileAndBindMixedSlice(sliceOperation);
  const std::set<size_t> dimensions =
      GetTileAndBindExtractOrInsertDims(module, sliceOperation,
                                        indexedValueTypes);
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

  const std::map<int, std::string> ownedValueTypes =
      indexedValueTypes ? std::map<int, std::string>{} : ValueTypes(module);
  const std::map<int, std::string> &valueTypes =
      indexedValueTypes ? *indexedValueTypes : ownedValueTypes;
  // Helper.cpp::createdByTiling forwards getOriginalType(op) to both its
  // size and offset checks.  For tensor.insert_slice that is the destination
  // tensor, not the smaller inserted source tensor.
  const size_t originalOperand =
      sliceOperation.name == "tensor.insert_slice" ? 1 : 0;
  if (originalOperand >= slice->prefixOperands.size())
    return false;
  auto originalType = valueTypes.find(slice->prefixOperands[originalOperand]);
  const std::optional<ShapedTypeModel> originalShape =
      originalType == valueTypes.end()
          ? std::nullopt
          : ParseShapedTypeForDimensionAnalysis(originalType->second);
  if (!originalShape || tilingDimension >= originalShape->shape.size() ||
      !originalShape->shape[tilingDimension])
    return false;
  for (size_t dimension = 0; dimension < originalShape->shape.size();
       ++dimension) {
    if (!originalShape->shape[dimension] ||
        dimension >= slice->sizes.size() ||
        (dimension != tilingDimension &&
         (!slice->sizes[dimension].constant ||
          *slice->sizes[dimension].constant !=
              *originalShape->shape[dimension])))
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

  const std::map<int, const GenericOperation *> ownedDefinitions =
      (indexedDefinitions || indexedDefinitionIds)
          ? std::map<int, const GenericOperation *>{}
          : DefiningOperations(module);
  const std::map<int, const GenericOperation *> &definitions =
      indexedDefinitions ? *indexedDefinitions : ownedDefinitions;
  const auto definitionFor = [&](int value) -> const GenericOperation * {
    if (indexedDefinitionIds) {
      if (value < 0 ||
          static_cast<size_t>(value) >= indexedDefinitionIds->size())
        return nullptr;
      const int operation =
          indexedDefinitionIds->at(static_cast<size_t>(value));
      return operation < 0
                 ? nullptr
                 : &module.operations.at(static_cast<size_t>(operation));
    }
    auto definition = definitions.find(value);
    return definition == definitions.end() ? nullptr : definition->second;
  };
  const auto constant = [&](int value) -> std::optional<int64_t> {
    const GenericOperation *definition = definitionFor(value);
    if (!definition || definition->name != "arith.constant")
      return std::nullopt;
    std::string text =
        FindDictionaryValue(definition->properties, "value");
    if (text.empty())
      text = FindDictionaryValue(definition->attributes, "value");
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
      *originalShape->shape[tilingDimension] % *upperBound != 0)
    return false;
  const int64_t tileSize =
      *originalShape->shape[tilingDimension] / *upperBound;
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
  const GenericOperation *offsetDefinition =
      definitionFor(slice->offsets[tilingDimension].value);
  if (!offsetDefinition || offsetDefinition->name != "affine.apply" ||
      offsetDefinition->operands.size() != 1)
    return false;

  const GenericRegion &loopRegion = module.regions.at(
      static_cast<size_t>(tilingLoop->regions.front()));
  if (loopRegion.blocks.empty())
    return false;
  const GenericBlock &loopBlock =
      module.blocks.at(static_cast<size_t>(loopRegion.blocks.front()));
  if (loopBlock.arguments.empty() ||
      offsetDefinition->operands.front() != loopBlock.arguments.front())
    return false;
  const std::optional<std::string> expression =
      ExistingAffineApplyExpression(*offsetDefinition);
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
inline bool ApplyHoistAffinePattern(
    GenericModule &module, int operationId,
    const std::map<int, const GenericOperation *> *indexedDefinitions =
        nullptr,
    const std::map<int, int> *indexedBlockArguments = nullptr,
    const std::vector<int> *indexedDefinitionIds = nullptr,
    const std::vector<int> *indexedBlockArgumentIds = nullptr,
    const std::vector<std::vector<uint8_t>> *indexedBlockDominance =
        nullptr) {
  const GenericOperation &currentOperation =
      module.operations.at(static_cast<size_t>(operationId));
  if ((currentOperation.name != "affine.apply" &&
       currentOperation.name != "affine.min" &&
       currentOperation.name != "affine.max") ||
      currentOperation.blockId < 0)
    return false;
  // Moving an operation never reallocates the operation table. Keep a stable
  // reference instead of copying strings and operand/result vectors for every
  // affine candidate in every fixed-point iteration.
  const GenericOperation &snapshot = currentOperation;

  std::optional<std::map<int, const GenericOperation *>> ownedDefinitions;
  if (!indexedDefinitions && !indexedDefinitionIds)
    ownedDefinitions = DefiningOperations(module);
  const auto definitionFor = [&](int value) -> const GenericOperation * {
    if (indexedDefinitionIds) {
      if (value < 0 ||
          static_cast<size_t>(value) >= indexedDefinitionIds->size())
        return nullptr;
      const int operation =
          indexedDefinitionIds->at(static_cast<size_t>(value));
      return operation < 0
                 ? nullptr
                 : &module.operations.at(static_cast<size_t>(operation));
    }
    const std::map<int, const GenericOperation *> &definitions =
        indexedDefinitions ? *indexedDefinitions : *ownedDefinitions;
    auto definition = definitions.find(value);
    return definition == definitions.end() ? nullptr : definition->second;
  };
  std::optional<std::map<int, int>> ownedBlockArguments;
  if (!indexedBlockArguments && !indexedBlockArgumentIds) {
    ownedBlockArguments.emplace();
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        (*ownedBlockArguments)[argument] = block.id;
  }
  const auto blockFor = [&](int value) -> int {
    if (indexedBlockArgumentIds)
      return value < 0 ||
                     static_cast<size_t>(value) >=
                         indexedBlockArgumentIds->size()
                 ? -1
                 : indexedBlockArgumentIds->at(static_cast<size_t>(value));
    const std::map<int, int> &blockArguments =
        indexedBlockArguments ? *indexedBlockArguments
                              : *ownedBlockArguments;
    auto block = blockArguments.find(value);
    return block == blockArguments.end() ? -1 : block->second;
  };

  auto blockDominates = [&](int candidateBlock, int operationBlock) {
    if (indexedBlockDominance && candidateBlock >= 0 && operationBlock >= 0 &&
        static_cast<size_t>(candidateBlock) < indexedBlockDominance->size() &&
        static_cast<size_t>(operationBlock) <
            indexedBlockDominance->at(static_cast<size_t>(candidateBlock))
                .size())
      return indexedBlockDominance->at(static_cast<size_t>(candidateBlock))
                 [static_cast<size_t>(operationBlock)] != 0;
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
  // These are live definitions from the current block lists. The rewriter
  // updates ordinals after every move, so same-block dominance can use the
  // ordinal directly instead of searching the complete block twice for every
  // affine operand.
  const auto operationDominates = [&](const GenericOperation &candidate,
                                      const GenericOperation &operation) {
    const GenericOperation *cursor = &operation;
    while (cursor) {
      if (candidate.blockId == cursor->blockId &&
          candidate.ordinal < cursor->ordinal)
        return true;
      if (cursor->regionId < 0)
        break;
      const int parent = module.regions.at(
          static_cast<size_t>(cursor->regionId)).parentOperation;
      if (parent < 0)
        break;
      cursor = &module.operations.at(static_cast<size_t>(parent));
    }
    return false;
  };

  const GenericOperation *lastDefiningOperation = nullptr;
  int lastDefiningValue = -1;
  int lastBlockArgument = -1;
  for (int operand : snapshot.operands) {
    const int blockArgument = blockFor(operand);
    if (blockArgument >= 0) {
      if (lastBlockArgument < 0 ||
          blockDominates(blockFor(lastBlockArgument), blockArgument))
        lastBlockArgument = operand;
      continue;
    }
    const GenericOperation *definition = definitionFor(operand);
    if (!definition)
      continue;
    if (!lastDefiningOperation ||
        operationDominates(*lastDefiningOperation, *definition)) {
      lastDefiningOperation = definition;
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
    insertionBlock = blockFor(lastBlockArgument);
    lastDefiningValue = lastBlockArgument;
  } else if (lastDefiningOperation && lastBlockArgument >= 0) {
    const int argumentBlock = blockFor(lastBlockArgument);
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

  // Hoisting only moves existing operations. Avoid rescanning every value in
  // the module merely to initialize the id allocator used by createOperation.
  GenericRewriter rewriter(
      module, nullptr,
      GenericRewriter::ValueInitialization::SkipExistingValues);
  if (insertionBlock != snapshot.blockId) {
    rewriter.removeFromBlock(snapshot.blockId, operationId);
    rewriter.insertToBlock(insertionBlock, insertionPosition, operationId);
    return true;
  }

  const std::vector<int> &operations =
      module.blocks.at(static_cast<size_t>(snapshot.blockId)).operations;
  if (snapshot.ordinal < 0 ||
      static_cast<size_t>(snapshot.ordinal) >= operations.size() ||
      operations[static_cast<size_t>(snapshot.ordinal)] != operationId)
    return false;
  const size_t currentPosition = static_cast<size_t>(snapshot.ordinal);
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
      operations.begin() + static_cast<std::ptrdiff_t>(currentPosition),
      [&](int betweenId) {
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
                                       int operationId,
                                       GenericMutableOperandUseIndex *uses =
                                           nullptr);

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

inline bool AttemptBindSubBlock(GenericModule &module, int functionId,
                                DebugTrace *trace = nullptr) {
  const auto fail = [&](const char *reason) {
    if (trace)
      trace->Pass("TileAndBind.Attempt.Failure", {{reason, 1}});
    return false;
  };
  GenericPipelineContext context(module, 0);
  MeasureStage(trace, "TileAndBind.Attempt.ReplicateOutEmpty", [&] {
    RunReplicateOutEmptyTensor(module, functionId, context);
  });
  PipelineMetadataCache &metadata = context.metadata();
  DimensionAnalyzer tiledAnalyzer(module);
  const bool analyzed =
      MeasureStage(trace, "TileAndBind.Attempt.Analyze", [&] {
        if (!tiledAnalyzer.initialize())
          return false;
        tiledAnalyzer.computeTilingDim(
            module.operations.at(static_cast<size_t>(functionId)));
        return true;
      });
  if (!analyzed)
    return fail("dimension_analysis");
  if (trace)
    trace->Artifact("TileAndBind.DimensionAnalysis", [&] {
      return tiledAnalyzer.serializeAnalysis();
    });
  const GenericOperation function =
      module.operations.at(static_cast<size_t>(functionId));
  if (TileAndSliceOpHasUnsupportedStore(module, function))
    return fail("unsupported_store");
  if (function.regions.size() != 1)
    return fail("function_region_count");
  const GenericRegion &functionRegion =
      module.regions.at(static_cast<size_t>(function.regions.front()));
  if (functionRegion.blocks.size() != 1)
    return fail("function_block_count");
  const int entryBlock = functionRegion.blocks.front();
  if (module.blocks.at(static_cast<size_t>(entryBlock)).operations.empty())
    return fail("empty_entry_block");

  const std::vector<int> originalOperations =
      module.blocks.at(static_cast<size_t>(entryBlock)).operations;
  int returnOperation = -1;
  for (int operationId : originalOperations)
    if (module.operations.at(static_cast<size_t>(operationId)).name ==
        "func.return")
      returnOperation = operationId;
  if (returnOperation < 0)
    return fail("missing_return");

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

  std::map<int, size_t> bubbleDims = CollectTileAndBindBubbleDims(
      module, function, tiledAnalyzer);
  std::map<int, std::string> valueTypes = ValueTypes(module);
  const std::map<int, std::string> originalValueTypes = valueTypes;
  struct TileAndSliceOffsetRequest {
    int operationId = -1;
    int value = -1;
    int64_t tileSize = 0;
    bool storeCopy = false;
    int replacedUserOperation = -1;
  };
  std::vector<TileAndSliceOffsetRequest> offsetRequests;
  std::vector<std::pair<int, int64_t>> storeCopyTiles;
  std::map<int, int> storeCopyStartValues;
  const std::map<int, const GenericOperation *> tileAndSliceDefinitions =
      DefiningOperations(module);
  int maximumUsedValue = -1;
  for (const GenericOperation &operation : module.operations)
    for (int operand : operation.operands)
      maximumUsedValue = std::max(maximumUsedValue, operand);
  std::vector<bool> valuesWithUsers(
      maximumUsedValue < 0 ? 0 : static_cast<size_t>(maximumUsedValue) + 1,
      false);
  for (const GenericOperation &operation : module.operations)
    for (int operand : operation.operands)
      if (operand >= 0)
        valuesWithUsers[static_cast<size_t>(operand)] = true;
  auto valueHasUser = [&](int value) {
    return value >= 0 && static_cast<size_t>(value) < valuesWithUsers.size() &&
           valuesWithUsers[static_cast<size_t>(value)];
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
      int replacedUserOperation = operation.id;
      auto sourceType = valueTypes.find(valueToSlice);
      std::optional<ShapedTypeModel> source =
          sourceType == valueTypes.end()
              ? std::nullopt
              : metadata.shapedType(sourceType->second);
      // TileAndSliceStoreCopyOp tries handleMaskedStore for every store before
      // falling back to the store's current source shape.  When both operands
      // are unit-stride views of equal static parents, it tiles those parents;
      // this is not limited to dynamically-shaped current views.
      if (operation.name == "hivm.hir.store" && source && axis >= 0 &&
          static_cast<size_t>(axis) < source->shape.size() &&
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
                  : metadata.shapedType(sourceParentType->second);
          const std::optional<ShapedTypeModel> destinationParentShape =
              destinationParentType == valueTypes.end()
                  ? std::nullopt
                  : metadata.shapedType(destinationParentType->second);
          const auto isStatic = [](const ShapedTypeModel &shape) {
            return std::all_of(shape.shape.begin(), shape.shape.end(),
                               [](const std::optional<int64_t> &extent) {
                                 return extent.has_value();
                               });
          };
          if (sourceParentShape && destinationParentShape &&
              isStatic(*sourceParentShape) &&
              isStatic(*destinationParentShape) &&
              sourceParentShape->shape == destinationParentShape->shape &&
              static_cast<size_t>(axis) < sourceParentShape->shape.size() &&
              sourceParentShape->shape[static_cast<size_t>(axis)] &&
              *sourceParentShape->shape[static_cast<size_t>(axis)] >= 2) {
            valueToSlice = sourceParent;
            source = sourceParentShape;
            // handleMaskedStore changes the existing extract/subview source
            // operand before rebuilding the dynamic store slices.  The new
            // marked slice therefore replaces this view's use of the parent,
            // not the store's use of the view result.
            replacedUserOperation = sourceView->second->id;
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
            {operationId, valueToSlice, tileSize, true,
             replacedUserOperation});
      }
    }

    if (operation.name != "scf.for" && operation.name != "scf.while" &&
        operation.name != "scf.if" && operation.name != "scope.scope")
      continue;
    for (size_t resultIndex = 0;
         resultIndex < operation.results.size() &&
         resultIndex < operation.resultTypes.size();
         ++resultIndex) {
      const int value = operation.results[resultIndex];
      const int64_t axis = tiledAnalyzer.getTilingDim(value);
      const auto type = metadata.shapedType(operation.resultTypes[resultIndex]);
      if (axis < 0 || valueHasUser(value) || !type || !type->tensor ||
          static_cast<size_t>(axis) >= type->shape.size() ||
          !type->shape[static_cast<size_t>(axis)] ||
          *type->shape[static_cast<size_t>(axis)] < 2)
        continue;
      offsetRequests.push_back(
          {operationId, value,
           (*type->shape[static_cast<size_t>(axis)] + 1) / 2, false, -1});
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
  std::vector<int> boundaryToTensor;
  std::vector<std::pair<int, int>> varangeOffsets;
  // Helper.cpp::calculateOffsetAtTilingDim materializes
  // `subblock_iv * tile_size` in the sub-block loop body.  Several marked
  // slices can reach a shared producer through different greedy worklist
  // paths, but each resulting slice must retain the offset corresponding to
  // its own static size.  Look up that already-materialized offset by the
  // same semantic expression instead of relying on the compact model's
  // value-keyed request map.
  const auto findSubBlockTileOffset =
      [&](int64_t size) -> std::optional<int> {
    const std::string induction =
        AffineValueExpression(inductionVariable);
    const std::string constant = "c(" + std::to_string(size) + ")";
    const std::string expected =
        MakeAffineBinaryExpression("mul", induction, constant);
    const std::string commuted =
        MakeAffineBinaryExpression("mul", constant, induction);
    const GenericBlock &subBlockBody =
        module.blocks.at(static_cast<size_t>(loopBlock));
    for (auto candidate = subBlockBody.operations.rbegin();
         candidate != subBlockBody.operations.rend(); ++candidate) {
      const GenericOperation &apply =
          module.operations.at(static_cast<size_t>(*candidate));
      if (apply.name != "affine.apply" || apply.results.size() != 1 ||
          apply.operands.size() != 1 ||
          apply.operands.front() != inductionVariable)
        continue;
      const std::optional<std::string> expression =
          ExistingAffineApplyExpression(apply);
      if (expression && (*expression == expected || *expression == commuted))
        return apply.results.front();
    }
    return std::nullopt;
  };
  const std::vector<int> bubbleRewriteOrder =
      GetTileAndBindGreedyRewriteOrder(module, function);

  struct BubbleUpRequest {
    int value = -1;
    int offset = -1;
    // The explicit extract_slice inserted by TileAndSlice replaces exactly
    // one use at every step of BubbleUpPattern.  Keep that use edge so the
    // generic projection can enforce MLIR's source.getUsers() precondition
    // instead of treating the value graph as a freely clonable tree.
    int replacedUserOperation = -1;
    // TileAndSlice changes one OpOperand in place, whereas
    // ElementwiseBubbleUpStrategy clones the structured operation and erases
    // the old one.  In the latter case every use edge owned by that operation
    // disappears before the newly-created operand slices are reconsidered.
    bool replacesWholeUserOperation = false;
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
        {request.value, offset,
         request.replacedUserOperation,
         false,
         order * (offsetRequests.size() + 1) + index});
  }
  std::stable_sort(initialRequests.begin(), initialRequests.end(),
                   [](const BubbleUpRequest &lhs,
                      const BubbleUpRequest &rhs) {
                     return lhs.initialOrder < rhs.initialOrder;
                   });
  // Bubble-up only appends affine.apply operations. Keep the definition
  // index in sync with those appends instead of rebuilding it for every
  // recursive request.
  int maximumDefinitionValue = -1;
  for (const auto &[value, unused] : initialDefinitions) {
    (void)unused;
    maximumDefinitionValue = std::max(maximumDefinitionValue, value);
  }
  for (const GenericBlock &block : module.blocks)
    for (int argument : block.arguments)
      maximumDefinitionValue = std::max(maximumDefinitionValue, argument);
  std::vector<int> bubbleDefinitionIds(
      maximumDefinitionValue < 0
          ? 0
          : static_cast<size_t>(maximumDefinitionValue) + 1,
      -1);
  for (const auto &[value, operation] : initialDefinitions)
    bubbleDefinitionIds[static_cast<size_t>(value)] = operation->id;
  std::vector<int> bubbleBlockArgumentIds(bubbleDefinitionIds.size(), -1);
  for (const GenericBlock &block : module.blocks)
    for (int argument : block.arguments)
      bubbleBlockArgumentIds[static_cast<size_t>(argument)] = block.id;
  const auto bubbleDefinition = [&](int value) -> const GenericOperation * {
    if (value < 0 || static_cast<size_t>(value) >= bubbleDefinitionIds.size())
      return nullptr;
    const int operationId = bubbleDefinitionIds[static_cast<size_t>(value)];
    return operationId < 0
               ? nullptr
               : &module.operations.at(static_cast<size_t>(operationId));
  };

  bool bubbleUpFailed = false;
  int bubbleUpFailedOperation = -1;
  std::set<int> loopBubbleRewrittenResults;
  // This is the live pre-rewrite use relation.  BubbleUpPattern replaces one
  // concrete use with its marked extract_slice, then requires every remaining
  // user to be annotation.mark.  Counting operand occurrences (rather than
  // unique owning operations) also preserves the behavior when one operation
  // consumes the same SSA value more than once.
  std::map<int, std::vector<int>> bubbleUsers;
  for (int operationId : GetTileAndBindDescendants(module, function)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int operand : operation.operands)
      bubbleUsers[operand].push_back(operation.id);
  }
  // TileAndSlice creates every initial marked extract_slice before the
  // BubbleUpExtractSlice greedy fixed point starts.  The compact evaluator
  // stores those extracts as requests instead of materializing them, so keep
  // the corresponding replaced use edges explicitly.  Otherwise two tiled
  // consumers of the same value see each other's original operations as
  // non-slice users and incorrectly reject a candidate that the real greedy
  // rewrite accepts.
  std::map<int, std::map<int, size_t>> initialMarkedSliceUses;
  for (const BubbleUpRequest &request : initialRequests)
    if (request.replacedUserOperation >= 0)
      ++initialMarkedSliceUses[request.value]
                              [request.replacedUserOperation];

  // TileAndSliceStoreCopyOp replaces the store/copy use of valueToSlice with
  // one marked extract_slice.  BubbleUpPattern then sees every other concrete
  // user of the source.  A surviving tensor.insert_slice is a non-slice user,
  // so an otherwise unsupported structured source cannot bubble and the
  // strict verifier rolls the complete TileAndBind transaction back.  Keep
  // this pre-rewrite use edge explicit: the compact evaluator does not
  // materialize the marked store operand and would otherwise erase the insert
  // chain while evaluating the fixed point.
  for (const auto &[storeCopyOperation, valueToSlice] :
       storeCopyStartValues) {
    const GenericOperation *source = bubbleDefinition(valueToSlice);
    if (!source || source->name == "bufferization.to_tensor" ||
        source->name == "tensor.empty" ||
        (source->name == "scf.for" &&
         HasSplitMixDictionaryEntry(source->attributes,
                                    "ExtractedLoadOrStore")) ||
        HasSplitMixDictionaryEntry(source->attributes, "tiled_op"))
      continue;
    auto users = bubbleUsers.find(valueToSlice);
    if (users == bubbleUsers.end())
      continue;
    for (int userId : users->second) {
      if (userId == storeCopyOperation)
        continue;
      const GenericOperation &user =
          module.operations.at(static_cast<size_t>(userId));
      if (user.name != "tensor.insert_slice")
        continue;
      bubbleUpFailed = true;
      bubbleUpFailedOperation = source->id;
      break;
    }
    if (bubbleUpFailed)
      break;
  }
  const auto removeBubbleUser = [&](int value, int userId,
                                    bool removeAllOccurrences) {
    if (userId < 0)
      return;
    auto users = bubbleUsers.find(value);
    if (users == bubbleUsers.end())
      return;
    auto &ids = users->second;
    if (removeAllOccurrences) {
      ids.erase(std::remove(ids.begin(), ids.end(), userId), ids.end());
      return;
    }
    auto position = std::find(ids.begin(), ids.end(), userId);
    if (position != ids.end())
      ids.erase(position);
  };
  const auto removeBubbleOperationUses = [&](const GenericOperation &operation) {
    for (int operand : operation.operands)
      removeBubbleUser(operand, operation.id, true);
  };
  const auto firstBlockingBubbleUser =
      [&](int value, int replacedUserOperation,
          bool replacesWholeUserOperation) {
        bool removedReplacedUse = false;
        std::map<int, size_t> markedSliceUses;
        auto initialMarked = initialMarkedSliceUses.find(value);
        if (initialMarked != initialMarkedSliceUses.end())
          markedSliceUses = initialMarked->second;
        auto users = bubbleUsers.find(value);
        if (users == bubbleUsers.end())
          return -1;
        for (int userId : users->second) {
          auto markedUse = markedSliceUses.find(userId);
          if (markedUse != markedSliceUses.end() && markedUse->second != 0) {
            --markedUse->second;
            if (userId == replacedUserOperation)
              removedReplacedUse = true;
            continue;
          }
          if (userId == replacedUserOperation &&
              (replacesWholeUserOperation || !removedReplacedUse)) {
            removedReplacedUse = true;
            continue;
          }
          const GenericOperation &user =
              module.operations.at(static_cast<size_t>(userId));
          if (user.name == "annotation.mark")
            continue;
          // The lightweight implementation evaluates all reachable marked
          // slices as one fixed-point batch.  Another extract_slice that is
          // itself in that batch is therefore a transient second slice user:
          // MLIR initially rejects this particular worklist visit, then
          // revisits it after the sibling slice has moved.  A non-slice user
          // (notably tensor.insert_slice) survives that fixed point and is the
          // real allAllowedOperationUsage blocker.
          if (user.name == "tensor.extract_slice" &&
              std::any_of(user.results.begin(), user.results.end(),
                          [&](int result) {
                            return bubbleOffsets.count(result) != 0;
                          }))
            continue;
          return userId;
        }
        return -1;
      };
  struct DeferredBubbleRequest {
    int value = -1;
    int offset = -1;
    int replacedUserOperation = -1;
    bool replacesWholeUserOperation = false;
    std::set<int> path;
  };
  std::vector<DeferredBubbleRequest> deferredBubbleRequests;
  std::set<int> unresolvedBubbleValues;
  size_t bubbleRewriteProgress = 0;
  // BufferizationBubbleUpStrategy is materialized in a later compact-model
  // phase, but production first subjects its marked extract_slice to the
  // ordinary BubbleUpPattern source-use gate.  Preserve that match result so
  // the later phase cannot rewrite a local to_tensor whose marker actually
  // remained for the strict verifier.
  std::set<int> acceptedBoundaryToTensorValues;
  std::set<int> rejectedBoundaryToTensorValues;
  // CSEExtractSlicePattern runs in the same greedy driver as BubbleUpPattern.
  // Multiple consumers can create equivalent marked slices of one source;
  // production merges those slices before the surviving request bubbles.
  // The compact evaluator represents slices as recursive requests, so retain
  // the equivalent CSE identity explicitly.  The affine expression includes
  // both the map and its SSA operands and is therefore the same equivalence
  // relation used by operation CSE, unlike the temporary result value id.
  std::set<std::pair<int, std::string>> bubbledSliceKeys;
  const auto bubbleSliceKey = [&](int value, int offset) {
    const GenericOperation *definition = bubbleDefinition(offset);
    const std::optional<std::string> expression =
        definition && definition->name == "affine.apply"
            ? ExistingAffineApplyExpression(*definition)
            : std::nullopt;
    return std::make_pair(
        value, expression.value_or("value:" + std::to_string(offset)));
  };
  struct NormalizedBubbleLoop {
    int operationId = -1;
    int bodyBlock = -1;
    int inductionVariable = -1;
    int normalizedInductionValue = -1;
  };
  std::map<int, NormalizedBubbleLoop> normalizedBubbleLoops;
  struct LoopRegionIterArgInsert {
    int regionIterArg = -1;
    int insertResult = -1;
    int insertOperation = -1;
  };
  std::vector<LoopRegionIterArgInsert> loopRegionIterArgInserts;
  const auto constantIndexValue = [&](int value) -> std::optional<int64_t> {
    const GenericOperation *definition = bubbleDefinition(value);
    if (!definition || definition->name != "arith.constant")
      return std::nullopt;
    std::string literal =
        FindDictionaryValue(definition->properties, "value");
    if (literal.empty())
      literal = FindDictionaryValue(definition->attributes, "value");
    const size_t type = literal.find(" : ");
    if (type != std::string::npos)
      literal = trim(literal.substr(0, type));
    try {
      size_t consumed = 0;
      const int64_t result = std::stoll(literal, &consumed, 0);
      return consumed == literal.size() ? std::optional<int64_t>(result)
                                        : std::nullopt;
    } catch (const std::exception &) {
      return std::nullopt;
    }
  };
  const auto insertSliceBubbleIsSupported =
      [&](const GenericOperation &insert, int resultValue,
          size_t childTilingDim) {
        if (insert.name != "tensor.insert_slice" ||
            insert.operands.size() < 2 || insert.results.size() != 1 ||
            HasSplitMixDictionaryEntry(insert.attributes,
                                       "to_be_bubbled_slice") ||
            HasSplitMixDictionaryEntry(
                insert.attributes,
                "to_be_canceled_out_insert_slice"))
          return false;
        const std::optional<TileAndBindMixedSlice> parentSlice =
            ParseTileAndBindMixedSlice(insert);
        if (!parentSlice || parentSlice->prefixOperands.size() < 2 ||
            std::any_of(parentSlice->strides.begin(),
                        parentSlice->strides.end(),
                        [](const TileAndBindFoldResult &stride) {
                          return !stride.constant || *stride.constant != 1;
                        }))
          return false;

        auto resultType = valueTypes.find(resultValue);
        auto sourceType = valueTypes.find(parentSlice->prefixOperands.front());
        const std::optional<ShapedTypeModel> resultShape =
            resultType == valueTypes.end()
                ? std::nullopt
                : metadata.shapedType(resultType->second);
        const std::optional<ShapedTypeModel> sourceShape =
            sourceType == valueTypes.end()
                ? std::nullopt
                : metadata.shapedType(sourceType->second);
        if (!resultShape || !sourceShape || !resultShape->tensor ||
            !sourceShape->tensor ||
            childTilingDim >= resultShape->shape.size() ||
            !resultShape->shape[childTilingDim] ||
            *resultShape->shape[childTilingDim] < 2)
          return false;

        // InsertSliceBubbleUpStrategy handles this rank-reduced form before
        // its ordinary same/different-dimension cases.
        const bool parentSizesStatic =
            std::all_of(parentSlice->sizes.begin(), parentSlice->sizes.end(),
                        [](const TileAndBindFoldResult &size) {
                          return size.constant.has_value();
                        });
        if (resultShape->shape.size() > sourceShape->shape.size() &&
            parentSizesStatic) {
          const int64_t childSize0 =
              childTilingDim == 0
                  ? (*resultShape->shape[0] + 1) / 2
                  : resultShape->shape[0].value_or(-1);
          return !parentSlice->sizes.empty() &&
                 parentSlice->sizes[0].constant &&
                 *parentSlice->sizes[0].constant == 1 &&
                 resultShape->shape.size() == sourceShape->shape.size() + 1 &&
                 resultShape->shape[0] &&
                 childSize0 == *resultShape->shape[0];
        }

        // The extract-insert-extract transaction is selected ahead of the
        // dimension intersection logic when the inserted source is a
        // single-use extract from a tensor of the full result type.
        const GenericOperation *sourceDefinition =
            bubbleDefinition(parentSlice->prefixOperands.front());
        if (sourceDefinition &&
            sourceDefinition->name == "tensor.extract_slice" &&
            !sourceDefinition->operands.empty() &&
            bubbleUsers[parentSlice->prefixOperands.front()].size() == 1) {
          auto extractedParentType =
              valueTypes.find(sourceDefinition->operands.front());
          if (extractedParentType != valueTypes.end() &&
              resultType != valueTypes.end() &&
              extractedParentType->second == resultType->second)
            return true;
        }

        const std::set<size_t> parentDimensions =
            GetTileAndBindExtractOrInsertDims(module, insert,
                                              &valueTypes);
        if (parentDimensions.count(childTilingDim) != 0) {
          // handleExtractOfInsertSameDimCase is deliberately conservative:
          // both slices must affect only the same dimension, the parent must
          // be static, and a non-tiling insert is accepted only for a unit
          // source extent on that dimension.
          if (parentDimensions.size() != 1 || !parentSizesStatic)
            return false;
          return CreatedByTileAndBindTiling(
                     module, insert, &valueTypes, nullptr,
                     &bubbleDefinitionIds) ||
                 (childTilingDim < sourceShape->shape.size() &&
                  sourceShape->shape[childTilingDim] &&
                  *sourceShape->shape[childTilingDim] == 1);
        }

        // handleExtractOfInsertDifferentDimCase supports only a static
        // inserted source.
        return std::all_of(sourceShape->shape.begin(),
                           sourceShape->shape.end(),
                           [](const std::optional<int64_t> &extent) {
                             return extent.has_value();
                           });
      };
  std::function<void(int, int, int, bool, std::set<int>)>
      runBubbleUpRequest =
      [&](int value, int offset, int replacedUserOperation,
          bool replacesWholeUserOperation, std::set<int> path) {
        if (bubbleUpFailed)
          return;
        const bool repeated = path.count(value) != 0;
        const bool hasDimension = bubbleDims.count(value) != 0;
        if (trace)
          trace->Pass("TileAndBind.Attempt.BubbleRequest",
                      {{"value", static_cast<uint64_t>(value)},
                       {"repeated", repeated ? 1U : 0U},
                       {"has_dimension", hasDimension ? 1U : 0U}});
        if (!path.insert(value).second || !hasDimension)
          return;
        const GenericOperation *definition = bubbleDefinition(value);
        if (!definition)
          return;
        const GenericOperation operationSnapshot = *definition;
        const std::pair<int, std::string> sliceKey =
            bubbleSliceKey(value, offset);
        if (bubbledSliceKeys.count(sliceKey) != 0)
          return;
        if (trace && (operationSnapshot.name == "scope.scope" ||
                      operationSnapshot.name == "scf.for"))
          trace->Pass("TileAndBind.Attempt.BubbleControl",
                      {{"value", static_cast<uint64_t>(value)},
                       {"operation",
                        static_cast<uint64_t>(operationSnapshot.id)}});
        if (operationSnapshot.name == "tensor.empty")
          return;

        // LoopBubbleUpStrategy::sliceRegionIterArg deliberately creates a
        // temporary insert_slice carrying this marker.  No bubble strategy is
        // allowed to move a marked insert; it must instead become dead after
        // all users of the full region iter_arg have been rewritten.  If it
        // remains live, the production verifier rejects the complete
        // TileAndBind candidate.
        if (operationSnapshot.name == "tensor.insert_slice" &&
            HasSplitMixDictionaryEntry(
                operationSnapshot.attributes,
                "to_be_canceled_out_insert_slice"))
          return;
        if (operationSnapshot.name == "tensor.insert_slice" &&
            !insertSliceBubbleIsSupported(operationSnapshot, value,
                                          bubbleDims.at(value))) {
          // No production strategy rewrites the conceptual marked extract.
          // Strict verification therefore rejects it and rolls the complete
          // transactional TileAndBind candidate back.
          bubbleUpFailed = true;
          bubbleUpFailedOperation = operationSnapshot.id;
          return;
        }

        // Exact BubbleUpPattern source-use gate.  The conceptual marked slice
        // is the one allowed extract_slice user; its insertion replaces the
        // recorded use edge.  Any other non-annotation use prevents every
        // strategy from running.  In strict mode an unsupported source left
        // inside the sub-block loop makes HIVMBubbleUpExtractSlice fail and
        // TileAndBindSubBlock rolls the whole candidate back.
        const int blockingUser =
            firstBlockingBubbleUser(value, replacedUserOperation,
                                    replacesWholeUserOperation);
        const bool sourceAlreadyAccepted =
            (operationSnapshot.name == "scf.for" &&
             HasSplitMixDictionaryEntry(operationSnapshot.attributes,
                                        "ExtractedLoadOrStore")) ||
            HasSplitMixDictionaryEntry(operationSnapshot.attributes,
                                       "tiled_op");
        if (blockingUser >= 0 || sourceAlreadyAccepted) {
          if (trace)
            trace->Pass(
                "TileAndBind.Attempt.BubbleUseGate",
                {{"operation", static_cast<uint64_t>(operationSnapshot.id)},
                 {"blocking_user", static_cast<uint64_t>(blockingUser)},
                 {"replaced_user",
                  static_cast<uint64_t>(replacedUserOperation)},
                 {"replaced_whole_user",
                  replacesWholeUserOperation ? 1U : 0U},
                 {"accepted_source", sourceAlreadyAccepted ? 1U : 0U}});
          if (blockingUser >= 0 && !sourceAlreadyAccepted &&
              operationSnapshot.name == "bufferization.to_tensor") {
            rejectedBoundaryToTensorValues.insert(value);
          } else if (blockingUser >= 0 && !sourceAlreadyAccepted) {
            // Pattern match failure is local to this greedy worklist visit.
            // Preserve the incoming path so another rewrite can remove the
            // blocking user before this request is revisited.
            path.erase(value);
            deferredBubbleRequests.push_back(
                {value, offset, replacedUserOperation,
                 replacesWholeUserOperation, std::move(path)});
          }
          return;
        }
        if (operationSnapshot.name == "bufferization.to_tensor") {
          acceptedBoundaryToTensorValues.insert(value);
          rejectedBoundaryToTensorValues.erase(value);
          return;
        }

        bubbledSliceKeys.insert(sliceKey);
        removeBubbleUser(value, replacedUserOperation,
                         replacesWholeUserOperation);
        ++bubbleRewriteProgress;

        if (operationSnapshot.name == "tensor.extract_slice" &&
            !operationSnapshot.operands.empty()) {
          const GenericOperation *parentExtract =
              bubbleDefinition(operationSnapshot.operands.front());
          if (parentExtract && parentExtract->name == "tensor.extract_slice") {
          const std::optional<TileAndBindMixedSlice> childSlice =
              ParseTileAndBindMixedSlice(operationSnapshot);
          const std::optional<TileAndBindMixedSlice> parentSlice =
              ParseTileAndBindMixedSlice(*parentExtract);
          const auto isStaticSlice = [](const auto &slice) {
            return slice &&
                   std::all_of(slice->sizes.begin(), slice->sizes.end(),
                               [](const TileAndBindFoldResult &size) {
                                 return size.constant.has_value();
                               });
          };
          const bool bothSlicesAreStatic =
              isStaticSlice(childSlice) && isStaticSlice(parentSlice);
          const std::set<size_t> parentDimensions =
              GetTileAndBindExtractOrInsertDims(module, *parentExtract,
                                                &originalValueTypes);
          const size_t childDimension = bubbleDims.at(value);
          std::optional<size_t> childDimensionInParent = childDimension;
          auto resultType = originalValueTypes.find(value);
          const std::optional<ShapedTypeModel> resultShape =
              resultType == originalValueTypes.end()
                  ? std::nullopt
                  : metadata.shapedType(resultType->second);
          if (childSlice && resultShape)
            childDimensionInParent = TileAndBindSliceAxisForResultAxis(
                childSlice->sizes, resultShape->shape, childDimension);
          auto parentResultType =
              originalValueTypes.find(operationSnapshot.operands.front());
          const std::optional<ShapedTypeModel> parentResultShape =
              parentResultType == originalValueTypes.end()
                  ? std::nullopt
                  : metadata.shapedType(parentResultType->second);
          if (childDimensionInParent && parentSlice && parentResultShape)
            childDimensionInParent = TileAndBindSliceAxisForResultAxis(
                parentSlice->sizes, parentResultShape->shape,
                *childDimensionInParent);
          if (bothSlicesAreStatic &&
              childDimensionInParent &&
              parentDimensions.count(*childDimensionInParent) != 0 &&
              !CreatedByTileAndBindTiling(module, *parentExtract,
                                          &originalValueTypes,
                                          nullptr,
                                          &bubbleDefinitionIds)) {
            if (trace)
              trace->Pass(
                  "TileAndBind.Attempt.BubbleIntersection",
                  {{"operation", static_cast<uint64_t>(operationSnapshot.id)},
                   {"child_dim", static_cast<uint64_t>(childDimension)},
                   {"parent_dim",
                    static_cast<uint64_t>(*childDimensionInParent)}});
            bubbleUpFailed = true;
            bubbleUpFailedOperation = operationSnapshot.id;
            return;
          }
          }
        }

        // ScopeBubbleUpStrategy moves a slice of a scope result to the
        // corresponding scope.return operand and narrows the scope result.
        // scope.scope has no ordinary operands, so the generic operand walk
        // below cannot discover this region-yield edge.
        if (operationSnapshot.name == "scope.scope") {
          auto result = std::find(operationSnapshot.results.begin(),
                                  operationSnapshot.results.end(), value);
          if (result == operationSnapshot.results.end() ||
              operationSnapshot.regions.size() != 1)
            return;
          const size_t resultIndex = static_cast<size_t>(
              std::distance(operationSnapshot.results.begin(), result));
          const size_t axis = bubbleDims.at(value);
          auto fullType = originalValueTypes.find(value);
          const std::optional<ShapedTypeModel> fullShape =
              fullType == originalValueTypes.end()
                  ? std::nullopt
                  : metadata.shapedType(fullType->second);
          if (!fullShape || !fullShape->tensor ||
              axis >= fullShape->shape.size() || !fullShape->shape[axis] ||
              *fullShape->shape[axis] < 2)
            return;
          const int64_t tileSize = (*fullShape->shape[axis] + 1) / 2;
          const std::string tileType = ReplaceTileAndBindShapeDimension(
              fullType->second, axis, tileSize);
          GenericOperation &updatedScope = module.operations.at(
              static_cast<size_t>(operationSnapshot.id));
          updatedScope.resultTypes[resultIndex] = tileType;
          valueTypes[value] = tileType;
          loopBubbleRewrittenResults.insert(value);

          const GenericRegion &region = module.regions.at(
              static_cast<size_t>(operationSnapshot.regions.front()));
          if (region.blocks.size() != 1)
            return;
          const GenericBlock &block = module.blocks.at(
              static_cast<size_t>(region.blocks.front()));
          if (block.operations.empty())
            return;
          GenericOperation &terminator = module.operations.at(
              static_cast<size_t>(block.operations.back()));
          if (terminator.name != "scope.return" ||
              resultIndex >= terminator.operands.size())
            return;
          if (resultIndex < terminator.operandTypes.size())
            terminator.operandTypes[resultIndex] = tileType;
          if (trace)
            trace->Pass("TileAndBind.Attempt.BubbleScopeReturn",
                        {{"scope_result", static_cast<uint64_t>(value)},
                         {"return_value", static_cast<uint64_t>(
                                              terminator.operands[resultIndex])}});
          removeBubbleUser(terminator.operands[resultIndex], terminator.id,
                           false);
          runBubbleUpRequest(terminator.operands[resultIndex], offset, -1,
                             false, path);
          return;
        }

        if (operationSnapshot.name == "scf.for") {
          if (HasSplitMixDictionaryEntry(operationSnapshot.attributes,
                                         "ExtractedLoadOrStore"))
            return;
          // LoopBubbleUpStrategy first normalizes a non-unit-step loop and
          // succeeds without moving the marked slice. The greedy driver then
          // revisits that slice. Preserve the same loop semantics here before
          // narrowing its iter_arg/result: update 0..ub step s to
          // 0..ceildiv(ub,s) step 1 and materialize iv*s for the old body
          // uses. Extract/insert strategies below will replace tiled slice
          // offsets with iv*newTileSize, leaving iv*s only for unrelated uses.
          if (normalizedBubbleLoops.count(operationSnapshot.id) == 0 &&
              operationSnapshot.operands.size() >= 3 &&
              !operationSnapshot.regions.empty()) {
            const std::optional<int64_t> lower =
                constantIndexValue(operationSnapshot.operands[0]);
            const std::optional<int64_t> upper =
                constantIndexValue(operationSnapshot.operands[1]);
            const std::optional<int64_t> step =
                constantIndexValue(operationSnapshot.operands[2]);
            const GenericOperation *stepDefinition =
                bubbleDefinition(operationSnapshot.operands[2]);
            const bool indexStep =
                stepDefinition && stepDefinition->resultTypes.size() == 1 &&
                stepDefinition->resultTypes.front() == "index";
            const GenericRegion &loopRegion = module.regions.at(
                static_cast<size_t>(operationSnapshot.regions.front()));
            if (lower && upper && step && *lower == 0 && *upper > 0 &&
                *step > 1 && indexStep && !loopRegion.blocks.empty()) {
              GenericBlock &body = module.blocks.at(
                  static_cast<size_t>(loopRegion.blocks.front()));
              if (!body.arguments.empty()) {
                const int64_t normalizedUpper =
                    (*upper + *step - 1) / *step;
                int upperValue = -1;
                if (normalizedUpper == 2)
                  upperValue = twoValue;
                else if (normalizedUpper == 1)
                  upperValue = oneValue;
                else if (std::optional<int> existing =
                             FindTileAndBindIndexConstant(
                                 module, loopBlock, normalizedUpper))
                  upperValue = *existing;
                else {
                  const int constant = rewriter.createOperation(
                      subBlockLoop,
                      module.blocks.at(static_cast<size_t>(loopBlock)).regionId,
                      loopBlock, "arith.constant",
                      {"index"}, {}, {},
                      "{value = " + std::to_string(normalizedUpper) +
                          " : index}");
                  rewriter.insertToBlock(loopBlock, 0, constant);
                  upperValue = module.operations.at(
                      static_cast<size_t>(constant)).results.front();
                }
                GenericOperation &loop = module.operations.at(
                    static_cast<size_t>(operationSnapshot.id));
                loop.operands[1] = upperValue;
                loop.operands[2] = oneValue;
                const int inductionVariable = body.arguments.front();
                const int apply = rewriter.createOperation(
                    operationSnapshot.id, loopRegion.id, body.id,
                    "affine.apply", {"index"}, {inductionVariable},
                    {"index"},
                    "{map = affine.apply(mul(v(" +
                        std::to_string(inductionVariable) + "),c(" +
                        std::to_string(*step) + ")))}");
                rewriter.insertToBlock(body.id, 0, apply);
                const int normalizedInduction = module.operations.at(
                    static_cast<size_t>(apply)).results.front();
                ReplaceTileAndBindValueExcept(
                    module, inductionVariable, normalizedInduction, apply);
                if (static_cast<size_t>(normalizedInduction) >=
                    bubbleDefinitionIds.size())
                  bubbleDefinitionIds.resize(
                      static_cast<size_t>(normalizedInduction) + 1, -1);
                bubbleDefinitionIds[static_cast<size_t>(normalizedInduction)] =
                    apply;
                normalizedBubbleLoops.emplace(
                    operationSnapshot.id,
                    NormalizedBubbleLoop{operationSnapshot.id, body.id,
                                         inductionVariable,
                                         normalizedInduction});
              }
            }
          }
          auto result = std::find(operationSnapshot.results.begin(),
                                  operationSnapshot.results.end(), value);
          if (result == operationSnapshot.results.end() ||
              operationSnapshot.regions.empty())
            return;
          const size_t resultIndex = static_cast<size_t>(
              std::distance(operationSnapshot.results.begin(), result));
          std::vector<std::pair<int, int>> newSlices;
          const size_t initBegin =
              operationSnapshot.operands.size() >=
                      operationSnapshot.results.size()
                  ? operationSnapshot.operands.size() -
                        operationSnapshot.results.size()
                  : operationSnapshot.operands.size();
          const size_t axis = bubbleDims.at(value);
          auto fullType = originalValueTypes.find(value);
          const std::optional<ShapedTypeModel> fullShape =
              fullType == originalValueTypes.end()
                  ? std::nullopt
                  : metadata.shapedType(fullType->second);
          if (!fullShape || !fullShape->tensor ||
              axis >= fullShape->shape.size() || !fullShape->shape[axis] ||
              *fullShape->shape[axis] < 2)
            return;
          const int64_t tileSize = (*fullShape->shape[axis] + 1) / 2;
          const std::string tileType = ReplaceTileAndBindShapeDimension(
              fullType->second, axis, tileSize);

          module.operations.at(static_cast<size_t>(operationSnapshot.id))
              .resultTypes[resultIndex] = tileType;
          valueTypes[value] = tileType;
          loopBubbleRewrittenResults.insert(value);

          if (initBegin + resultIndex < operationSnapshot.operands.size()) {
            const size_t initIndex = initBegin + resultIndex;
            const int init = operationSnapshot.operands[initIndex];
            const GenericOperation *initDefinition = bubbleDefinition(init);
            if (initDefinition && initDefinition->name == "tensor.empty") {
              const int tiledEmpty = rewriter.createOperation(
                  operationSnapshot.parentId, operationSnapshot.regionId,
                  operationSnapshot.blockId, "tensor.empty", {tileType});
              rewriter.insertToBlock(
                  operationSnapshot.blockId,
                  static_cast<size_t>(operationSnapshot.ordinal), tiledEmpty);
              const int tiledInit =
                  module.operations.at(static_cast<size_t>(tiledEmpty))
                      .results.front();
              GenericOperation &updatedLoop = module.operations.at(
                  static_cast<size_t>(operationSnapshot.id));
              updatedLoop.operands[initIndex] = tiledInit;
              updatedLoop.operandTypes[initIndex] = tileType;
              valueTypes[tiledInit] = tileType;
              if (static_cast<size_t>(tiledInit) >=
                  bubbleDefinitionIds.size())
                bubbleDefinitionIds.resize(static_cast<size_t>(tiledInit) + 1,
                                           -1);
              bubbleDefinitionIds[static_cast<size_t>(tiledInit)] =
                  tiledEmpty;
            } else {
              module.operations.at(static_cast<size_t>(operationSnapshot.id))
                  .operandTypes[initIndex] = tileType;
              newSlices.emplace_back(init, operationSnapshot.id);
            }
          }
          const GenericRegion &region = module.regions.at(
              static_cast<size_t>(operationSnapshot.regions.front()));
          if (!region.blocks.empty()) {
            GenericBlock &block =
                module.blocks.at(static_cast<size_t>(region.blocks.front()));
            if (resultIndex + 1 < block.arguments.size()) {
              const int regionIterArg = block.arguments[resultIndex + 1];
              block.argumentTypes[resultIndex + 1] = tileType;
              valueTypes[regionIterArg] = tileType;

              // Exact projection of
              // BubbleUpExtractSlice/Pattern.cpp::sliceRegionIterArg:
              // preserve a full-size view for the original body users while
              // changing the region iter_arg itself to the sliced type.  The
              // marked insert is transient only when every one of those
              // users is subsequently rewritten by the greedy bubble-up
              // fixed point.
              const int temporaryEmpty = rewriter.createOperation(
                  operationSnapshot.id, region.id, block.id, "tensor.empty",
                  {fullType->second});
              rewriter.insertToBlock(block.id, 0, temporaryEmpty);
              const int emptyResult =
                  module.operations.at(static_cast<size_t>(temporaryEmpty))
                      .results.front();
              const int argumentInsert = rewriter.createOperation(
                  operationSnapshot.id, region.id, block.id,
                  "tensor.insert_slice", {fullType->second},
                  {regionIterArg, emptyResult}, {tileType, fullType->second},
                  "", "{to_be_canceled_out_insert_slice}");
              rewriter.insertToBlock(block.id, 1, argumentInsert);
              GenericOperation &insert = module.operations.at(
                  static_cast<size_t>(argumentInsert));
              TileAndBindMixedSlice insertSlice;
              insertSlice.prefixOperands = {regionIterArg, emptyResult};
              insertSlice.prefixOperandTypes = {tileType, fullType->second};
              insertSlice.prefixSegments = {1, 1};
              insertSlice.offsets.resize(fullShape->shape.size());
              insertSlice.sizes.resize(fullShape->shape.size());
              insertSlice.strides.resize(fullShape->shape.size());
              for (size_t dimension = 0;
                   dimension < fullShape->shape.size(); ++dimension) {
                insertSlice.offsets[dimension] =
                    dimension == axis
                        ? TileAndBindFoldResult{std::nullopt, offset}
                        : TileAndBindFoldResult{int64_t{0}, -1};
                insertSlice.sizes[dimension] =
                    dimension == axis
                        ? TileAndBindFoldResult{tileSize, -1}
                        : TileAndBindFoldResult{fullShape->shape[dimension],
                                               -1};
                insertSlice.strides[dimension] = {int64_t{1}, -1};
              }
              SetTileAndBindMixedSlice(insert, insertSlice);
              const int insertResult = insert.results.front();

              // Keep the compact logical use list synchronized with
              // replaceAllUsesExcept(regionIterArg, insertResult, insert).
              // It preserves duplicate operand occurrences just like MLIR's
              // intrusive use-list.
              std::vector<int> replacedUsers = bubbleUsers[regionIterArg];
              ReplaceTileAndBindValueExcept(module, regionIterArg,
                                            insertResult, argumentInsert);
              bubbleUsers[regionIterArg] = {argumentInsert};
              bubbleUsers[insertResult] = std::move(replacedUsers);
              bubbleDims[insertResult] = axis;
              bubbleOffsets[insertResult] = offset;
              valueTypes[emptyResult] = fullType->second;
              valueTypes[insertResult] = fullType->second;
              const size_t requiredDefinitionSize =
                  static_cast<size_t>(std::max(emptyResult, insertResult)) + 1;
              if (bubbleDefinitionIds.size() < requiredDefinitionSize)
                bubbleDefinitionIds.resize(requiredDefinitionSize, -1);
              bubbleDefinitionIds[static_cast<size_t>(emptyResult)] =
                  temporaryEmpty;
              bubbleDefinitionIds[static_cast<size_t>(insertResult)] =
                  argumentInsert;
              loopRegionIterArgInserts.push_back(
                  {regionIterArg, insertResult, argumentInsert});
            }
            if (!block.operations.empty()) {
              const GenericOperation &terminator = module.operations.at(
                  static_cast<size_t>(block.operations.back()));
              if (terminator.name == "scf.yield" &&
                  resultIndex < terminator.operands.size())
                newSlices.emplace_back(terminator.operands[resultIndex],
                                       terminator.id);
            }
          }
          for (auto operand = newSlices.rbegin(); operand != newSlices.rend();
               ++operand) {
            removeBubbleUser(operand->first, operand->second, false);
            runBubbleUpRequest(operand->first, offset, -1, false, path);
          }
          return;
        }

        const bool elementwise =
            IsElementwiseNaryOp(operationSnapshot.name) ||
            operationSnapshot.name == "hivm.hir.load" ||
            operationSnapshot.name == "hivm.hir.store" ||
            operationSnapshot.name == "hivm.hir.copy";
        // Every non-control BubbleUpStrategy builds the replacement from
        // sliced operands and removes the original source operation. Reflect
        // that mutation in the logical use-list before visiting the new
        // operand slices, just as PatternRewriter does for the next greedy
        // worklist item.
        removeBubbleOperationUses(operationSnapshot);
        std::vector<std::pair<int, int>> insertedSlices;
        const GenericOperation *offsetDefinition = bubbleDefinition(offset);
        const std::optional<GenericOperation> sourceApply =
            offsetDefinition && offsetDefinition->name == "affine.apply"
                ? std::optional<GenericOperation>(*offsetDefinition)
                : std::nullopt;
        for (int operand : operationSnapshot.operands) {
          auto operandDim = bubbleDims.find(operand);
          auto operandType = valueTypes.find(operand);
          if (operandDim == bubbleDims.end() ||
              operandType == valueTypes.end())
            continue;
          const auto shaped =
              metadata.shapedType(operandType->second);
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
            if (static_cast<size_t>(composedOffset) >=
                bubbleDefinitionIds.size())
              bubbleDefinitionIds.resize(
                  static_cast<size_t>(composedOffset) + 1, -1);
            bubbleDefinitionIds[static_cast<size_t>(composedOffset)] =
                composedApply;
          }
          bubbleOffsets.emplace(operand, composedOffset);
          insertedSlices.emplace_back(operand, composedApply);
        }

        // computeAllSliceParameters creates every affine.apply in operand
        // order before makeTiledShapes materializes any extract_slice. The
        // structured op clone is inserted last. Consequently the LIFO greedy
        // worklist drains all slices in reverse operand order before it can
        // reach any of this level's affine.apply operations.
        // ElementwiseBubbleUpStrategy materializes one slice per tiled
        // OpOperand, then erases the original structured operation.  When the
        // same value appears in multiple equivalent operand positions, the
        // CSEExtractSlicePattern in the same greedy driver merges those
        // sibling slices before BubbleUpPattern can make progress.  Preserve
        // the first materialized offset (the later affine CSE representative)
        // and recurse once per value instead of bubbling an already-CSE'd
        // slice multiple times.
        std::vector<std::pair<int, int>> recursiveSlices;
        std::set<int> seenElementwiseOperands;
        for (const auto &inserted : insertedSlices) {
          if (elementwise && !seenElementwiseOperands.insert(inserted.first).second)
            continue;
          const int composedApply = inserted.second;
          const int composedOffset =
              elementwise
                  ? bubbleOffsets.at(inserted.first)
                  : composedApply < 0
                        ? offset
                        : module.operations.at(
                              static_cast<size_t>(composedApply))
                              .results.front();
          recursiveSlices.emplace_back(inserted.first, composedOffset);
        }
        for (auto inserted = recursiveSlices.rbegin();
             inserted != recursiveSlices.rend(); ++inserted)
          runBubbleUpRequest(inserted->first, inserted->second, -1, false,
                             path);
        for (auto inserted = insertedSlices.rbegin();
             inserted != insertedSlices.rend(); ++inserted)
          if (inserted->second >= 0) {
            ApplyHoistAffinePattern(
                module, inserted->second, nullptr, nullptr,
                &bubbleDefinitionIds, &bubbleBlockArgumentIds);
          }
        // CSE is intentionally deferred to the pass-level greedy sweep after
        // every recursive request has materialized its users. Running the
        // whole-block CSE after each inserted affine.apply repeatedly scans
        // the growing clone while producing the same final representative.
      };
  for (const BubbleUpRequest &request : initialRequests)
    runBubbleUpRequest(request.value, request.offset,
                       request.replacedUserOperation,
                       request.replacesWholeUserOperation, {});
  while (!deferredBubbleRequests.empty() && !bubbleUpFailed) {
    std::vector<DeferredBubbleRequest> retryRequests =
        std::move(deferredBubbleRequests);
    deferredBubbleRequests.clear();
    const size_t progressBeforeRetry = bubbleRewriteProgress;
    for (DeferredBubbleRequest &request : retryRequests)
      runBubbleUpRequest(request.value, request.offset,
                         request.replacedUserOperation,
                         request.replacesWholeUserOperation,
                         std::move(request.path));
    if (!deferredBubbleRequests.empty() &&
        bubbleRewriteProgress == progressBeforeRetry) {
      // Greedy pattern application reaching a fixed point is not a pass
      // failure. Remaining strict markers are classified by
      // VerifyTileAndBindMarkedSlicesAreBubbledUp after all rewrites and
      // canonicalization have been projected.
      for (const DeferredBubbleRequest &request : deferredBubbleRequests)
        unresolvedBubbleValues.insert(request.value);
      deferredBubbleRequests.clear();
    }
  }
  // The generic evaluator updates rewritten operations in place instead of
  // cloning and erasing them.  Remove the temporary full-size bridge from
  // those compact replacements exactly when the logical greedy use-list says
  // that every original full-size use disappeared.  A bridge with any
  // surviving logical use is intentionally left for the production-aligned
  // verifier below.
  for (const LoopRegionIterArgInsert &bridge : loopRegionIterArgInserts) {
    auto users = bubbleUsers.find(bridge.insertResult);
    if (users != bubbleUsers.end() && !users->second.empty())
      continue;
    ReplaceTileAndBindValueExcept(module, bridge.insertResult,
                                  bridge.regionIterArg,
                                  bridge.insertOperation);
  }
  unresolvedBubbleValues.insert(rejectedBoundaryToTensorValues.begin(),
                                rejectedBoundaryToTensorValues.end());
  if (!unresolvedBubbleValues.empty()) {
    const GenericModuleAnalysisSnapshot unresolvedAnalysis(
        module, kGenericAnalysisDefinitions |
                    kGenericAnalysisFunctionDescendants);
    for (int value : unresolvedBubbleValues) {
      if (IsTileAndBindBlockArgument(module, value))
        continue;
      const GenericOperation *source =
          unresolvedAnalysis.definingOperation(value);
      if (!source || !IsInsideTileAndBindSubBlockLoop(module, *source))
        continue;
      bool verifierRejects = false;
      if (source->name == "bufferization.to_tensor") {
        verifierRejects =
            !source->operands.empty() &&
            TileAndBindTracesToLocalAllocation(
                module, source->operands.front(), unresolvedAnalysis);
      } else if (source->name == "scf.while") {
        verifierRejects = true;
      } else {
        const bool acceptedSource =
            source->name == "tensor.empty" ||
            (source->name == "scf.for" &&
             HasSplitMixDictionaryEntry(source->attributes,
                                        "ExtractedLoadOrStore")) ||
            HasSplitMixDictionaryEntry(source->attributes, "tiled_op");
        verifierRejects = !acceptedSource;
      }
      if (!verifierRejects)
        continue;
      if (trace)
        trace->Pass(
            "TileAndBind.Attempt.UnresolvedBubbleVerifier",
            {{"operation", static_cast<uint64_t>(source->id)},
             {"value", static_cast<uint64_t>(value)}});
      bubbleUpFailed = true;
      bubbleUpFailedOperation = source->id;
      break;
    }
  }
  if (bubbleUpFailed) {
    if (trace)
      trace->Pass("TileAndBind.Attempt.BubbleFailure",
                  {{"operation", static_cast<uint64_t>(
                                     bubbleUpFailedOperation)}});
    return fail("bubble_up");
  }

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
  // The extract/insert eligibility test asks only whether a source has one
  // user.  Build that immutable snapshot once; the old helper rescanned every
  // operation for every insert_slice candidate.
  size_t bubbleValueCount = 0;
  for (const GenericOperation &operation : module.operations)
    for (int operand : operation.operands)
      if (operand >= 0)
        bubbleValueCount =
            std::max(bubbleValueCount, static_cast<size_t>(operand) + 1);
  std::vector<size_t> bubbleUserCounts(bubbleValueCount, 0);
  std::vector<int> bubbleUserLastSeen(bubbleValueCount, -1);
  for (const GenericOperation &operation : module.operations)
    for (int operand : operation.operands)
      if (operand >= 0 &&
          bubbleUserLastSeen[static_cast<size_t>(operand)] != operation.id) {
        bubbleUserLastSeen[static_cast<size_t>(operand)] = operation.id;
        ++bubbleUserCounts[static_cast<size_t>(operand)];
      }
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
        sourceDefinition->second->results.front() < 0 ||
        static_cast<size_t>(sourceDefinition->second->results.front()) >=
            bubbleUserCounts.size() ||
        bubbleUserCounts[static_cast<size_t>(
            sourceDefinition->second->results.front())] != 1)
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
        metadata.shapedType(insertSlice.resultTypes.front());
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
    const GenericOperation *definition = bubbleDefinition(value);
    if (!definition)
      return false;
    const GenericOperation &operation = *definition;
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
        metadata.shapedType(sourceType->second);
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
    const GenericOperation *sourceDefinition = bubbleDefinition(source);
    if (sourceDefinition && sourceDefinition->name == "scf.for" &&
        HasSplitMixDictionaryEntry(sourceDefinition->attributes,
                                   "ExtractedLoadOrStore")) {
      insertionParent = sourceDefinition->parentId;
      insertionRegion = sourceDefinition->regionId;
      insertionBlock = sourceDefinition->blockId;
      insertionPosition =
          static_cast<size_t>(sourceDefinition->ordinal + 1);
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
      return fail("extract_insert_extract");
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
      if (loopBubbleRewrittenResults.count(value) != 0)
        continue;
      const int64_t axis = static_cast<int64_t>(bubbleDim->second);
      auto shaped = metadata.shapedType(operation.resultTypes[index]);
      if (axis < 0 || !shaped || !shaped->tensor ||
          static_cast<size_t>(axis) >= shaped->shape.size() ||
          !shaped->shape[static_cast<size_t>(axis)] ||
          *shaped->shape[static_cast<size_t>(axis)] < 2)
        continue;
      const int64_t tileSize =
          (*shaped->shape[static_cast<size_t>(axis)] + 1) / 2;
      if (operation.name == "bufferization.to_tensor") {
        if (acceptedBoundaryToTensorValues.count(value) != 0 ||
            rejectedBoundaryToTensorValues.count(value) != 0)
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
      if (operation.name == "tensor.extract_slice" ||
          operation.name == "tensor.insert_slice") {
        const std::optional<TileAndBindMixedSlice> slice =
            ParseTileAndBindMixedSlice(operation);
        const std::optional<size_t> sliceAxis =
            slice ? TileAndBindSliceAxisForResultAxis(
                        slice->sizes, shaped->shape,
                        static_cast<size_t>(axis))
                  : std::nullopt;
        if (!slice || !sliceAxis) {
          if (trace)
            trace->Pass("TileAndBind.Attempt.RankReducedSliceFailure",
                        {{"operation", static_cast<uint64_t>(operation.id)},
                         {"result_axis", static_cast<uint64_t>(axis)}});
          if (trace)
            trace->Artifact("TileAndBind.RankReducedSliceFailure", [&] {
              return SerializeGenericModule(module);
            });
          return fail("rank_reduced_slice_axis");
        }
        TileAndBindMixedSlice updatedSlice = *slice;
        updatedSlice.sizes[*sliceAxis] = {tileSize, -1};
        SetTileAndBindMixedSlice(operation, updatedSlice);
      } else if (operation.name == "tensor.expand_shape") {
        const auto resultShape =
            metadata.shapedType(operation.resultTypes[index]);
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
        if (offset != bubbleOffsets.end()) {
          // A marked slice reaching varange must cover exactly one sub-block
          // tile of the original 1-D result.  The real greedy rewriter keeps
          // concurrent slices as distinct SSA operations; the compact model's
          // value-keyed offset map can otherwise retain a narrower offset from
          // another request that converged on the same producer. Recover the
          // matching calculateOffsetAtTilingDim result already materialized at
          // the start of the sub-block loop.
          const int tileOffset =
              findSubBlockTileOffset(tileSize).value_or(offset->second);
          bubbleOffsets[value] = tileOffset;
          varangeOffsets.emplace_back(operation.id, tileOffset);
        }
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

  // LoopBubbleUpStrategy can narrow an insert_slice source through its loop
  // iter_arg before the insert operation itself appears on the greedy
  // worklist. MLIR then rebuilds the mixed sizes from that narrowed source.
  // Keep the generic projection equivalent, including rank-reduced sources.
  for (GenericOperation &operation : module.operations) {
    if (operation.name != "tensor.insert_slice")
      continue;
    std::optional<TileAndBindMixedSlice> slice =
        ParseTileAndBindMixedSlice(operation);
    if (!slice || slice->prefixOperands.empty())
      continue;
    auto sourceType = valueTypes.find(slice->prefixOperands.front());
    const std::optional<ShapedTypeModel> source =
        sourceType == valueTypes.end()
            ? std::nullopt
            : metadata.shapedType(sourceType->second);
    if (!source || !source->tensor)
      continue;
    bool changed = false;
    for (size_t sourceAxis = 0; sourceAxis < source->shape.size();
         ++sourceAxis) {
      const std::optional<size_t> sliceAxis =
          TileAndBindSliceAxisForResultAxis(slice->sizes, source->shape,
                                            sourceAxis);
      if (!sliceAxis || !source->shape[sourceAxis])
        continue;
      TileAndBindFoldResult &size = slice->sizes[*sliceAxis];
      if (!size.constant || *size.constant == *source->shape[sourceAxis])
        continue;
      size = {*source->shape[sourceAxis], -1};
      changed = true;
    }
    if (changed)
      SetTileAndBindMixedSlice(operation, *slice);
  }

  // createNewChildOpAfterBubbledUp and
  // createNewInsertForExtractOfInsertSameDim rebuild slices inside a
  // normalized loop with `inner_iv * new_tile_size`. The normalization's
  // `inner_iv * old_step` remains only on operands that were not replaced by
  // those strategies. Materialize one representative per loop; the real
  // greedy affine CSE likewise shares it across the tensor slices.
  for (const auto &[loopId, normalized] : normalizedBubbleLoops) {
    (void)loopId;
    int64_t tileSize = std::numeric_limits<int64_t>::max();
    std::vector<std::pair<int, size_t>> tiledOffsets;
    const GenericOperation &loop = module.operations.at(
        static_cast<size_t>(normalized.operationId));
    for (int operationId : GetTileAndBindDescendants(module, loop)) {
      const GenericOperation &operation = module.operations.at(
          static_cast<size_t>(operationId));
      if (operation.name != "tensor.extract_slice" &&
          operation.name != "tensor.insert_slice")
        continue;
      const std::optional<TileAndBindMixedSlice> slice =
          ParseTileAndBindMixedSlice(operation);
      if (!slice)
        continue;
      for (size_t dimension = 0; dimension < slice->offsets.size();
           ++dimension) {
        if (slice->offsets[dimension].constant ||
            slice->offsets[dimension].value !=
                normalized.normalizedInductionValue ||
            dimension >= slice->sizes.size() ||
            !slice->sizes[dimension].constant ||
            *slice->sizes[dimension].constant <= 0)
          continue;
        tileSize = std::min(tileSize, *slice->sizes[dimension].constant);
        tiledOffsets.emplace_back(operationId, dimension);
      }
    }
    if (tiledOffsets.empty() || tileSize == std::numeric_limits<int64_t>::max())
      continue;
    const int apply = rewriter.createOperation(
        normalized.operationId,
        module.blocks.at(static_cast<size_t>(normalized.bodyBlock)).regionId,
        normalized.bodyBlock, "affine.apply", {"index"},
        {normalized.inductionVariable}, {"index"},
        "{map = affine.apply(mul(v(" +
            std::to_string(normalized.inductionVariable) + "),c(" +
            std::to_string(tileSize) + ")))}");
    rewriter.insertToBlock(normalized.bodyBlock, 0, apply);
    const int tileOffset =
        module.operations.at(static_cast<size_t>(apply)).results.front();
    for (const auto &[operationId, dimension] : tiledOffsets) {
      GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      std::optional<TileAndBindMixedSlice> slice =
          ParseTileAndBindMixedSlice(operation);
      if (!slice || dimension >= slice->offsets.size() ||
          dimension >= slice->sizes.size() ||
          !slice->sizes[dimension].constant ||
          *slice->sizes[dimension].constant != tileSize)
        continue;
      slice->offsets[dimension] = {std::nullopt, tileOffset};
      SetTileAndBindMixedSlice(operation, *slice);
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
    const bool requestAccepted =
        acceptedBoundaryToTensorValues.count(sourceValue) != 0;
    const size_t axis = bubbleDims.at(sourceValue);
    const auto full = metadata.shapedType(toTensor.resultTypes.front());
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

    // BubbleUpPattern invokes BufferizationBubbleUpStrategy on the marked
    // extract_slice at the concrete use site. A pre-existing unmarked slice
    // (notably a CVPipelining preload-workspace slice) consumes the newly
    // created tiled slice; it is not itself the marked bubble-up request.
    int existingMarkedSlice = -1;
    int directSliceConsumer = -1;
    for (const GenericOperation &user : module.operations) {
      if (user.name != "tensor.extract_slice" || user.operands.empty() ||
          user.operands.front() != sourceValue || user.results.size() != 1)
        continue;
      if (HasSplitMixDictionaryEntry(user.attributes,
                                     "to_be_bubbled_slice") ||
          HasSplitMixDictionaryEntry(user.properties,
                                     "to_be_bubbled_slice")) {
        existingMarkedSlice = user.id;
        break;
      }
      if (directSliceConsumer < 0)
        directSliceConsumer = user.id;
    }
    if (existingMarkedSlice >= 0) {
      if (requestAccepted && RunBufferizationBubbleUpStrategy(
              module, rewriter, operationId, existingMarkedSlice, axis,
              tileSize, tileOffset->second, valueTypes))
        tiledValues[sourceValue] = {axis, tileSize, tileOffset->second};
      continue;
    }

    int insertionBlock = toTensor.blockId;
    size_t insertionPosition = static_cast<size_t>(toTensor.ordinal + 1);
    int insertionParent = toTensor.parentId;
    int insertionRegion = toTensor.regionId;
    const auto definitions = DefiningOperations(module);
    if (directSliceConsumer >= 0) {
      const GenericOperation &consumer = module.operations.at(
          static_cast<size_t>(directSliceConsumer));
      insertionBlock = consumer.blockId;
      insertionPosition = static_cast<size_t>(consumer.ordinal);
      insertionParent = consumer.parentId;
      insertionRegion = consumer.regionId;
    } else {
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
    if (directSliceConsumer >= 0) {
      GenericOperation &consumer = module.operations.at(
          static_cast<size_t>(directSliceConsumer));
      consumer.operands.front() = sliceValue;
      if (!consumer.operandTypes.empty())
        consumer.operandTypes.front() = tileType;
    } else {
      ReplaceTileAndBindValueExcept(module, sourceValue, sliceValue, slice);
    }
    valueTypes[sliceValue] = tileType;
    tiledValues[sliceValue] = {axis, tileSize, tileOffset->second};
    if (requestAccepted && RunBufferizationBubbleUpStrategy(
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
    const auto destination = metadata.shapedType(destinationType);
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
              : metadata.shapedType(sourceParentType->second);
      const std::optional<ShapedTypeModel> destinationParent =
          destinationParentType == valueTypes.end()
              ? std::nullopt
              : metadata.shapedType(destinationParentType->second);
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

  // BubbleUpPattern constructs a fresh marked slice for every accepted
  // request.  The compact projection updates operations in place and stores
  // only one offset per producer value, so converging requests can otherwise
  // leave a slice with (for example) a 32-element size and a 16-element
  // sub-block offset.  Restore the exact calculateOffsetAtTilingDim invariant
  // before the affine/CSE rounds consume these slices.
  const std::map<int, const GenericOperation *> finalDefinitions =
      DefiningOperations(module);
  for (GenericOperation &operation : module.operations) {
    if ((operation.name != "tensor.extract_slice" &&
         operation.name != "memref.subview") ||
        !HasSplitMixDictionaryEntry(operation.attributes,
                                    "to_be_bubbled_slice"))
      continue;
    std::optional<TileAndBindMixedSlice> slice =
        ParseTileAndBindMixedSlice(operation);
    if (!slice)
      continue;
    bool changed = false;
    for (size_t dimension = 0;
         dimension < slice->offsets.size() &&
         dimension < slice->sizes.size();
         ++dimension) {
      if (slice->offsets[dimension].constant ||
          !slice->sizes[dimension].constant ||
          *slice->sizes[dimension].constant <= 0)
        continue;
      auto definition =
          finalDefinitions.find(slice->offsets[dimension].value);
      if (definition == finalDefinitions.end() ||
          definition->second->name != "affine.apply" ||
          definition->second->operands.size() != 1 ||
          definition->second->operands.front() != inductionVariable)
        continue;
      const std::optional<int> expected = findSubBlockTileOffset(
          *slice->sizes[dimension].constant);
      if (!expected || *expected == slice->offsets[dimension].value)
        continue;
      slice->offsets[dimension] = {std::nullopt, *expected};
      changed = true;
    }
    if (changed)
      SetTileAndBindMixedSlice(operation, *slice);
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
    const GenericOperation &operation =
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

  // The extended canonicalizer in
  // populateBindSubBlockBubbleUpPassManager folds arith.addi identities.  In
  // particular VarangeBubbleUpStrategy always builds
  // `original_offset + slice_offset`; when the original offset is zero the
  // AddIOp disappears before CSE.  Preserve that source behavior here rather
  // than retaining a model-only arithmetic node.
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  const auto isZero = [&](int value) {
    auto found = definitions.find(value);
    if (found == definitions.end() ||
        found->second->name != "arith.constant")
      return false;
    std::string literal =
        FindDictionaryValue(found->second->properties, "value");
    if (literal.empty())
      literal = FindDictionaryValue(found->second->attributes, "value");
    const size_t type = literal.find(" : ");
    if (type != std::string::npos)
      literal = trim(literal.substr(0, type));
    return literal == "0";
  };
  std::vector<int> identityAdds;
  for (int operationId : GetTileAndBindDescendants(module, function)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "arith.addi" || operation.blockId < 0 ||
        operation.operands.size() != 2 || operation.results.size() != 1)
      continue;
    int replacement = -1;
    if (isZero(operation.operands[0]))
      replacement = operation.operands[1];
    else if (isZero(operation.operands[1]))
      replacement = operation.operands[0];
    if (replacement < 0)
      continue;
    ReplaceSplitMixValue(module, operation.results.front(), replacement);
    identityAdds.push_back(operationId);
  }
  rewriter.removeManyFromBlocks(identityAdds);
}

struct TileAndBindStableValueIndexes {
  std::vector<int> definitions;
  std::vector<int> blockArguments;
  // Region/block ancestry does not change while affine operations move
  // between existing blocks. Cache the exact dominance predicate used by the
  // hoist pattern instead of walking parent regions for every operand in each
  // greedy iteration.
  std::vector<std::vector<uint8_t>> blockDominance;
};

inline TileAndBindStableValueIndexes
BuildTileAndBindStableValueIndexes(const GenericModule &module) {
  int maximumValue = -1;
  for (const GenericBlock &block : module.blocks)
    for (int argument : block.arguments)
      maximumValue = std::max(maximumValue, argument);
  for (const GenericOperation &operation : module.operations)
    for (int result : operation.results)
      maximumValue = std::max(maximumValue, result);
  const size_t valueCount = maximumValue < 0
                                ? 0
                                : static_cast<size_t>(maximumValue) + 1;
  TileAndBindStableValueIndexes result;
  result.definitions.assign(valueCount, -1);
  result.blockArguments.assign(valueCount, -1);
  result.blockDominance.assign(
      module.blocks.size(),
      std::vector<uint8_t>(module.blocks.size(), uint8_t{0}));
  for (const GenericOperation &operation : module.operations)
    for (int value : operation.results)
      result.definitions[static_cast<size_t>(value)] = operation.id;
  for (const GenericBlock &block : module.blocks)
    for (int value : block.arguments)
      result.blockArguments[static_cast<size_t>(value)] = block.id;
  for (const GenericBlock &operationBlock : module.blocks) {
    int cursorBlock = operationBlock.id;
    result.blockDominance[static_cast<size_t>(cursorBlock)]
                         [static_cast<size_t>(operationBlock.id)] = 1;
    while (cursorBlock >= 0) {
      const int regionId =
          module.blocks.at(static_cast<size_t>(cursorBlock)).regionId;
      if (regionId < 0)
        break;
      const int parent =
          module.regions.at(static_cast<size_t>(regionId)).parentOperation;
      if (parent < 0)
        break;
      cursorBlock =
          module.operations.at(static_cast<size_t>(parent)).blockId;
      if (cursorBlock >= 0)
        result.blockDominance[static_cast<size_t>(cursorBlock)]
                             [static_cast<size_t>(operationBlock.id)] = 1;
    }
  }
  return result;
}

inline bool RunTileAndBindHoistAffineIteration(
    GenericModule &module, int functionId,
    const TileAndBindStableValueIndexes &indexes,
    DebugTrace *trace = nullptr) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  const std::vector<int> worklist = MeasureStage(
      trace, "TileAndBind.Hoist.Worklist", [&] {
        return GetTileAndBindAffineGreedyRewriteOrder(module, function);
      });
  return MeasureStage(trace, "TileAndBind.Hoist.Apply", [&] {
    bool changed = false;
    for (int operationId : worklist)
      changed |= ApplyHoistAffinePattern(
          module, operationId, nullptr, nullptr, &indexes.definitions,
          &indexes.blockArguments, &indexes.blockDominance);
    return changed;
  });
}

inline void RunTileAndBindHoistAffine(GenericModule &module,
                                      int functionId,
                                      const TileAndBindStableValueIndexes
                                          &indexes,
                                      DebugTrace *trace = nullptr) {
  for (size_t iteration = 0; iteration < 50; ++iteration)
    if (!RunTileAndBindHoistAffineIteration(module, functionId, indexes,
                                             trace))
      return;
  throw std::runtime_error(
      "HIVMBubbleUpExtractSlice: HoistAffine exceeded max iterations");
}

inline void RunTileAndBindHoistAffine(GenericModule &module,
                                      int functionId) {
  const TileAndBindStableValueIndexes indexes =
      BuildTileAndBindStableValueIndexes(module);
  RunTileAndBindHoistAffine(module, functionId, indexes);
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
// GreedyPatternRewriteDriver owns the later trivially-dead removal. A caller
// can still hold a not-yet-materialized use of the sibling result while the
// recursive bubble-up worklist is being modeled.
inline bool ApplyCSEAffineApplyPattern(GenericModule &module,
                                       int operationId,
                                       GenericMutableOperandUseIndex *uses) {
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
  std::map<int, int> replacements;
  for (size_t index = 1; index < siblingGroup.size(); ++index) {
    const GenericOperation sibling =
        module.operations.at(static_cast<size_t>(siblingGroup[index]));
    for (size_t result = 0;
         result < sibling.results.size() &&
         result < representative.results.size();
         ++result)
      replacements[sibling.results[result]] = representative.results[result];
  }
  // Greedy CSE replaces every equivalent sibling with the same dominating
  // representative.  Applying those one value at a time rescans the complete
  // generic module for every sibling and becomes needlessly super-linear for
  // TileAndBindSubBlock's cloned affine trees.  All replacements are
  // independent, so perform the equivalent rewrite in one module walk.
  for (GenericOperation &operation : module.operations) {
    const auto replaceOperand = [&](int &operand) {
      auto found = replacements.find(operand);
      if (found != replacements.end()) {
        if (uses)
          uses->replaceUse(operand, found->second);
        operand = found->second;
      }
    };
    const auto replaceDerivedUse = [&](int &operand) {
      auto found = replacements.find(operand);
      if (found != replacements.end())
        operand = found->second;
    };
    for (int &operand : operation.operands)
      replaceOperand(operand);
    for (int &operand : operation.dpsInputs)
      replaceDerivedUse(operand);
    for (int &operand : operation.dpsInits)
      replaceDerivedUse(operand);
  }
  return true;
}

inline bool IsTriviallyDeadTileAndBindAffine(const GenericModule &module,
                                             int operationId,
                                             const GenericMutableOperandUseIndex
                                                 *uses = nullptr) {
  const GenericOperation &operation =
      module.operations.at(static_cast<size_t>(operationId));
  if (operation.name != "affine.apply" && operation.name != "affine.min" &&
      operation.name != "affine.max")
    return false;
  for (int result : operation.results) {
    if (uses) {
      if (uses->hasUsers(result))
        return false;
      continue;
    }
    for (const GenericOperation &user : module.operations)
      if (std::find(user.operands.begin(), user.operands.end(), result) !=
          user.operands.end())
        return false;
  }
  return true;
}

// BubbleUpSubviewFromTiling creates replacement parent/child subviews in the
// real PatternRewriter and erases the replaced operations.  The lightweight
// projection updates those two subviews in place to preserve stable generic
// IDs, so the old offset producers are not erased as a side effect of
// replaceOp.  Drop only the now-unused affine producers here.  Do not run the
// earlier affine CSE again: the production pass intentionally creates fresh
// outer and inner offsets in its post-bubble greedy rewrite.
inline void RunTileAndBindPostSubviewDCE(GenericModule &module,
                                         int functionId) {
  for (size_t iteration = 0; iteration < 50; ++iteration) {
    const GenericMutableOperandUseIndex uses = [&] {
      GenericMutableOperandUseIndex result;
      result.BuildActive(module);
      return result;
    }();
    const GenericOperation &function =
        module.operations.at(static_cast<size_t>(functionId));
    std::vector<int> dead;
    for (int operationId : GetTileAndBindGreedyRewriteOrder(module, function))
      if (IsTriviallyDeadTileAndBindAffine(module, operationId, &uses))
        dead.push_back(operationId);
    if (dead.empty())
      return;
    GenericRewriter(module).removeManyFromBlocks(dead);
  }
  throw std::runtime_error(
      "TileAndBindSubBlock: post-subview affine DCE exceeded max iterations");
}

inline void RunCSEAffineApplyPattern(
    GenericModule &module, int functionId,
    const TileAndBindStableValueIndexes &indexes) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  GenericMutableOperandUseIndex uses(module);
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
      if (IsTriviallyDeadTileAndBindAffine(module, operationId, &uses)) {
        rewriter.removeFromBlock(operation.blockId, operationId);
        changed = true;
        continue;
      }
      if (ApplyHoistAffinePattern(
              module, operationId, nullptr, nullptr, &indexes.definitions,
              &indexes.blockArguments, &indexes.blockDominance)) {
        changed = true;
        ApplyCSEAffineApplyPattern(module, operationId, &uses);
        continue;
      }
      changed |= ApplyCSEAffineApplyPattern(module, operationId, &uses);
    }
    if (!changed)
      return;
  }
  throw std::runtime_error(
      "HIVMBubbleUpExtractSlice: affine/CSE greedy rewrite exceeded max "
      "iterations");
}

inline void RunCSEAffineApplyPattern(GenericModule &module, int functionId) {
  const TileAndBindStableValueIndexes indexes =
      BuildTileAndBindStableValueIndexes(module);
  RunCSEAffineApplyPattern(module, functionId, indexes);
}

// populateBindSubBlockBubbleUpPassManager runs CSE after the bubble-up
// canonicalizer. This is intentionally local to the candidate function: the
// outer canonicalizationHIVMPipeline still owns module-wide CSE.
inline void RunTileAndBindBubbleUpCSE(GenericModule &module, int functionId) {
  const std::vector<int> descendants = GetTileAndBindDescendants(
      module, module.operations.at(static_cast<size_t>(functionId)));
  std::map<std::string, std::vector<int>> available;
  GenericRewriter rewriter(module);
  const GenericMutableOperandUseIndex uses(module);
  for (int operationId : descendants) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "tensor.empty" || operation.blockId < 0 ||
        std::any_of(operation.results.begin(), operation.results.end(),
                    [&](int result) { return uses.hasUsers(result); }))
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

inline std::string RemoveTileAndBindUnitAttribute(
    const std::string &dictionary, const std::string &name) {
  if (!HasSplitMixDictionaryEntry(dictionary, name) ||
      dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return dictionary;
  std::vector<std::string> kept;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    if (trim(entry.substr(0, equal)) != name)
      kept.push_back(trim(entry));
  }
  std::ostringstream output;
  output << '{';
  for (size_t index = 0; index < kept.size(); ++index)
    output << (index == 0 ? "" : ", ") << kept[index];
  return output.str() + '}';
}

// Mirrors the post-bubble BubbleUpSubviewFromTiling greedy rewrite. This is
// intentionally run after both bubble-up affine/CSE rounds: the real pass
// creates a fresh outer offset and a fresh inner offset here, so they are not
// merged with the equal tensor-side offsets produced earlier.
inline void RunTileAndBindSubviewFromTiling(GenericModule &module,
                                            int functionId) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  std::vector<int> candidates;
  for (int operationId : GetTileAndBindDescendants(module, function)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "memref.subview" &&
        HasSplitMixDictionaryEntry(operation.attributes,
                                   "to_be_bubbled_slice"))
      candidates.push_back(operationId);
  }
  if (candidates.empty())
    return;

  GenericRewriter rewriter(module);
  const std::map<int, std::string> valueTypes = ValueTypes(module);
  const auto constantIndex = [&](int value) -> std::optional<int64_t> {
    const auto definitions = DefiningOperations(module);
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return std::nullopt;
    const std::optional<ArithIntegerConstant> constant =
        ParseArithIntegerConstant(*definition->second);
    if (!constant)
      return std::nullopt;
    return static_cast<int64_t>(constant->bits);
  };
  for (int childId : candidates) {
    const GenericOperation childSnapshot =
        module.operations.at(static_cast<size_t>(childId));
    if (childSnapshot.operands.empty())
      continue;
    const auto definitions = DefiningOperations(module);
    auto parentDefinition = definitions.find(childSnapshot.operands.front());
    if (parentDefinition == definitions.end() ||
        parentDefinition->second->name != "memref.subview" ||
        !CreatedByTileAndBindTiling(module, *parentDefinition->second))
      continue;
    const GenericOperation parentSnapshot = *parentDefinition->second;
    const std::set<size_t> childDimensions =
        GetTileAndBindExtractOrInsertDims(module, childSnapshot);
    const std::set<size_t> parentDimensions =
        GetTileAndBindExtractOrInsertDims(module, parentSnapshot);
    if (childDimensions.size() != 1 ||
        parentDimensions != childDimensions)
      continue;
    const size_t axis = *childDimensions.begin();
    std::optional<TileAndBindMixedSlice> childSlice =
        ParseTileAndBindMixedSlice(childSnapshot);
    std::optional<TileAndBindMixedSlice> parentSlice =
        ParseTileAndBindMixedSlice(parentSnapshot);
    if (!childSlice || !parentSlice || axis >= childSlice->sizes.size() ||
        axis >= parentSlice->sizes.size() ||
        !childSlice->sizes[axis].constant ||
        !parentSlice->sizes[axis].constant)
      continue;

    const GenericOperation *innerLoop = nullptr;
    const GenericOperation *subBlockLoop = nullptr;
    int ancestor = childSnapshot.parentId;
    while (ancestor >= 0) {
      const GenericOperation &candidate =
          module.operations.at(static_cast<size_t>(ancestor));
      if (candidate.name == "scf.for") {
        if (!innerLoop)
          innerLoop = &candidate;
        if (candidate.attributes.find("#hivm.sub_block") !=
            std::string::npos) {
          subBlockLoop = &candidate;
          break;
        }
      }
      ancestor = candidate.parentId;
    }
    if (!innerLoop || !subBlockLoop || innerLoop == subBlockLoop ||
        innerLoop->regions.empty() || subBlockLoop->regions.empty())
      continue;
    const GenericOperation innerLoopSnapshot = *innerLoop;
    const GenericOperation subBlockLoopSnapshot = *subBlockLoop;
    const GenericRegion &innerRegion = module.regions.at(
        static_cast<size_t>(innerLoopSnapshot.regions.front()));
    const GenericRegion &outerRegion = module.regions.at(
        static_cast<size_t>(subBlockLoopSnapshot.regions.front()));
    if (innerRegion.blocks.empty() || outerRegion.blocks.empty())
      continue;
    const GenericBlock &innerBody = module.blocks.at(
        static_cast<size_t>(innerRegion.blocks.front()));
    const GenericBlock &outerBody = module.blocks.at(
        static_cast<size_t>(outerRegion.blocks.front()));
    if (innerBody.arguments.empty() || outerBody.arguments.empty())
      continue;

    if (parentSlice->prefixOperands.empty() ||
        parentSlice->prefixOperandTypes.empty() ||
        childSlice->prefixOperands.empty() ||
        childSlice->prefixOperandTypes.empty() ||
        subBlockLoopSnapshot.operands.size() < 2 ||
        innerLoopSnapshot.operands.size() < 2)
      continue;
    auto sourceType = valueTypes.find(parentSlice->prefixOperands.front());
    const std::optional<ShapedTypeModel> sourceShape =
        sourceType == valueTypes.end()
            ? std::nullopt
            : ParseShapedTypeForDimensionAnalysis(sourceType->second);
    const std::optional<int64_t> outerTileCount =
        constantIndex(subBlockLoopSnapshot.operands[1]);
    const std::optional<int64_t> innerTileCount =
        constantIndex(innerLoopSnapshot.operands[1]);
    if (!sourceShape || axis >= sourceShape->shape.size() ||
        !sourceShape->shape[axis] || !outerTileCount ||
        *outerTileCount <= 0 || !innerTileCount || *innerTileCount <= 0)
      continue;

    // BubbleUpSubviewFromTiling does not retain the sizes of the old nested
    // views. createNewParentOpAfterBubbledUp first tiles the original parent
    // source by the sub-block loop, then createNewChildOpAfterBubbledUp tiles
    // that new parent by the inner loop.  The lightweight projection updates
    // the two existing view records in place, so reproduce both size/type
    // changes explicitly instead of updating only their affine offsets.
    const int64_t sourceExtent = *sourceShape->shape[axis];
    if (sourceExtent % *outerTileCount != 0)
      continue;
    const int64_t parentSize = sourceExtent / *outerTileCount;
    if (parentSize < *innerTileCount ||
        parentSize % *innerTileCount != 0)
      continue;
    const int64_t childSize = parentSize / *innerTileCount;
    const auto createOffset = [&](const GenericOperation &loop,
                                  const GenericRegion &region,
                                  const GenericBlock &body, int64_t size) {
      const int induction = body.arguments.front();
      const int apply = rewriter.createOperation(
          loop.id, region.id, body.id, "affine.apply", {"index"},
          {induction}, {"index"},
          "{map = affine.apply(mul(v(" + std::to_string(induction) +
              "),c(" + std::to_string(size) + ")))}");
      rewriter.insertToBlock(body.id, 0, apply);
      return module.operations.at(static_cast<size_t>(apply)).results.front();
    };
    const int outerOffset =
        createOffset(subBlockLoopSnapshot, outerRegion, outerBody, parentSize);
    const int innerOffset =
        createOffset(innerLoopSnapshot, innerRegion, innerBody, childSize);
    parentSlice->offsets[axis] = {std::nullopt, outerOffset};
    parentSlice->sizes[axis] = {parentSize, -1};
    childSlice->offsets[axis] = {std::nullopt, innerOffset};
    childSlice->sizes[axis] = {childSize, -1};
    if (parentSnapshot.results.size() != 1 ||
        parentSnapshot.resultTypes.size() != 1 ||
        childSnapshot.results.size() != 1 ||
        childSnapshot.resultTypes.size() != 1)
      continue;
    // Preserve each subview's inferred strided layout.  Rebuilding a child
    // type from the new parent's type drops a dynamic memref offset that MLIR
    // keeps on the child view.
    const std::string parentResultType = ReplaceTileAndBindShapeDimension(
        parentSnapshot.resultTypes.front(), axis, parentSize);
    const std::string childResultType = ReplaceTileAndBindShapeDimension(
        childSnapshot.resultTypes.front(), axis, childSize);

    // Pattern.cpp creates a fresh parent view and rewires only the matched
    // child.  Mutating parentSnapshot in place is equivalent only when that
    // child is its sole user; preload pipelines can have additional users,
    // which must retain the old parent type and tiling offset.
    parentSlice->prefixOperandTypes.front() = sourceType->second;
    const int newParentId = rewriter.createOperation(
        childSnapshot.parentId, childSnapshot.regionId, childSnapshot.blockId,
        parentSnapshot.name, {parentResultType}, parentSnapshot.operands,
        parentSnapshot.operandTypes, parentSnapshot.properties,
        parentSnapshot.attributes);
    GenericOperation &newParent =
        module.operations.at(static_cast<size_t>(newParentId));
    SetTileAndBindMixedSlice(newParent, *parentSlice);
    newParent.attributes = AddTileAndBindUnitAttribute(
        newParent.attributes, "to_be_bubbled_slice");
    rewriter.insertToBlock(childSnapshot.blockId,
                           static_cast<size_t>(childSnapshot.ordinal),
                           newParentId);
    const int newParentValue = newParent.results.front();

    GenericOperation &child =
        module.operations.at(static_cast<size_t>(childId));
    child.resultTypes.front() = childResultType;
    childSlice->prefixOperands.front() = newParentValue;
    childSlice->prefixOperandTypes.front() = parentResultType;
    SetTileAndBindMixedSlice(child, *childSlice);
    child.attributes = RemoveTileAndBindUnitAttribute(
        child.attributes, "to_be_bubbled_slice");

    const std::vector<int> oldParentUsers =
        TileAndBindValueUsers(module, parentSnapshot.results.front());
    if (oldParentUsers.empty())
      rewriter.removeFromBlock(parentSnapshot.blockId, parentSnapshot.id);
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

  // applyPatternsGreedily visits the current IR tree in structural preorder;
  // GenericModule storage order is append history and can put an operation in
  // a nested loop before an earlier top-level store.  Preserve the greedy
  // walk order so the first generated guard dominates and is foldable for
  // later candidates exactly as in MLIR.
  const std::vector<int> preOrder = CanonicalizationOperationPreOrder(module);
  std::map<int, size_t> preOrderPosition;
  for (size_t index = 0; index < preOrder.size(); ++index)
    preOrderPosition[preOrder[index]] = index;
  std::stable_sort(candidates.begin(), candidates.end(),
                   [&](int lhs, int rhs) {
                     return preOrderPosition.at(lhs) <
                            preOrderPosition.at(rhs);
                   });

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

inline GenericModule RunTileAndBindSubBlock(GenericModule module,
                                             DebugTrace *trace = nullptr,
                                             bool enableTile = true) {
  module = MeasureStage(trace, "TileAndBind.EarlyPatterns", [&] {
    return RunTileAndBindSubBlockEarlyPatterns(std::move(module));
  });
  std::vector<int> aivFunctions;
  for (const GenericOperation &function : module.operations)
    if (IsMixAIVFunction(function))
      aivFunctions.push_back(function.id);
  if (aivFunctions.empty())
    return module;

  // The production cv2pm pipeline always runs TileAndBindSubBlock.  Turning
  // auto binding off disables only the tiling attempt: early canonicalization
  // above still runs, and every unsliced store/copy must be restricted to
  // sub-block 0.  The experimental suffix used to omit the whole pass here.
  if (!enableTile)
    return MeasureStage(trace, "TileAndBind.LimitUniqueSubBlock", [&] {
      return LimitUniqueSubBlockToStore(std::move(module));
    });

  const GenericModuleAnalysisSnapshot guardAnalysis(
      module, kGenericAnalysisDefinitions |
                  kGenericAnalysisFunctionDescendants);
  bool hasBatchMatmulLoop = false;
  bool hasImplicitTranspose = false;
  bool hasSameLoadStoreAddress = false;
  const bool guarded = MeasureStage(trace, "TileAndBind.Guards", [&] {
    hasBatchMatmulLoop =
        HasBatchMatmulLoopInAICFunctions(module, guardAnalysis);
    hasImplicitTranspose = HasImplicitTransposeWithLastAxisInAIVFunctions(
        module, guardAnalysis);
    hasSameLoadStoreAddress =
        AreLoadAndStoreSameAddress(module, guardAnalysis);
    return hasBatchMatmulLoop || hasImplicitTranspose ||
           hasSameLoadStoreAddress;
  });
  if (trace)
    trace->Pass("TileAndBind.Guards",
                {{"batch_matmul", hasBatchMatmulLoop},
                 {"implicit_transpose", hasImplicitTranspose},
                 {"same_load_store_address", hasSameLoadStoreAddress}});
  if (guarded)
    return MeasureStage(trace, "TileAndBind.LimitUniqueSubBlock", [&] {
      return LimitUniqueSubBlockToStore(std::move(module));
    });

  for (int functionId : aivFunctions) {
    // Dimension analysis is read-only.  Run it against the original module
    // and clone only when a transactional bind attempt is actually needed.
    DimensionAnalyzer analyzer(module);
    const bool analyzed = MeasureStage(trace, "TileAndBind.Analyze", [&] {
      if (!analyzer.initialize())
        return false;
      analyzer.computeTilingDim(
          module.operations.at(static_cast<size_t>(functionId)));
      return analyzer.hasSelectedTilingDim();
    });
    if (trace)
      trace->Pass("TileAndBind.Analyze", {{"selected", analyzed}});
    if (!analyzed)
      return LimitUniqueSubBlockToStore(std::move(module));
    GenericModule candidate = MeasureStage(
        trace, "TileAndBind.CloneCandidate", [&] { return module; });
    const bool bound =
        MeasureStage(trace, "TileAndBind.AttemptBindSubBlock", [&] {
          return AttemptBindSubBlock(candidate, functionId, trace);
        });
    if (bound)
      MeasureStage(trace, "TileAndBind.BubbleUpFoldTensorEmpty", [&] {
        RunFoldTensorEmptyPatternsInPlace(candidate);
      });
    bool reshapeTypesValid = false;
    bool sliceTypesValid = false;
    bool markedSlicesValid = false;
    int invalidSliceOperation = -1;
    int invalidMarkedSliceOperation = -1;
    const bool verified =
        bound && MeasureStage(trace, "TileAndBind.Verify", [&] {
          reshapeTypesValid = VerifyTileAndBindReshapeTypes(candidate);
          sliceTypesValid = VerifyTileAndBindSliceTypes(
              candidate, &invalidSliceOperation);
          markedSlicesValid = VerifyTileAndBindMarkedSlicesAreBubbledUp(
              candidate, functionId, /*strictMode=*/true,
              &invalidMarkedSliceOperation);
          return reshapeTypesValid && sliceTypesValid && markedSlicesValid;
        });
    if (trace)
      trace->Pass("TileAndBind.Attempt",
                  {{"bound", bound},
                   {"reshape_types", reshapeTypesValid},
                   {"slice_types", sliceTypesValid},
                   {"marked_slices", markedSlicesValid},
                   {"invalid_slice", static_cast<uint64_t>(
                                         invalidSliceOperation)},
                   {"invalid_marked_slice", static_cast<uint64_t>(
                                                invalidMarkedSliceOperation)},
                   {"verified", verified}});
    if (!bound || !verified) {
      if (trace && bound)
        trace->Artifact("TileAndBind.InvalidCandidate", [&] {
          return SerializeGenericModule(candidate);
        });
      return LimitUniqueSubBlockToStore(std::move(module));
    }
    const TileAndBindStableValueIndexes firstAffineIndexes =
        BuildTileAndBindStableValueIndexes(candidate);
    MeasureStage(trace, "TileAndBind.HoistAndCSE1", [&] {
      MeasureStage(trace, "TileAndBind.Hoist1", [&] {
        RunTileAndBindHoistAffine(candidate, functionId, firstAffineIndexes,
                                  trace);
      });
      MeasureStage(trace, "TileAndBind.CSE1", [&] {
        RunCSEAffineApplyPattern(candidate, functionId, firstAffineIndexes);
      });
    });
    MeasureStage(trace, "TileAndBind.OperationFolder", [&] {
      RunTileAndBindOperationFolder(candidate, functionId);
    });
    MeasureStage(trace, "TileAndBind.BubbleUpCSE", [&] {
      RunTileAndBindBubbleUpCSE(candidate, functionId);
    });
    const TileAndBindStableValueIndexes secondAffineIndexes =
        BuildTileAndBindStableValueIndexes(candidate);
    MeasureStage(trace, "TileAndBind.HoistAndCSE2", [&] {
      MeasureStage(trace, "TileAndBind.Hoist2", [&] {
        RunTileAndBindHoistAffine(candidate, functionId,
                                  secondAffineIndexes, trace);
      });
      MeasureStage(trace, "TileAndBind.CSE2", [&] {
        RunCSEAffineApplyPattern(candidate, functionId,
                                 secondAffineIndexes);
      });
    });
    MeasureStage(trace, "TileAndBind.BubbleUpSubviewFromTiling", [&] {
      RunTileAndBindSubviewFromTiling(candidate, functionId);
    });
    MeasureStage(trace, "TileAndBind.FoldAffineConstantOperands", [&] {
      // Bubble-up builds affine.min/apply operations with MLIR's composed
      // affine helpers. Those helpers run canonicalizeMapAndOperands at
      // construction time, so index constants are embedded in the map and
      // are not liveness operands of the new operation.
      FoldExistingAffineConstantOperands(candidate);
    });
    MeasureStage(trace, "TileAndBind.PostSubviewDCE", [&] {
      RunTileAndBindPostSubviewDCE(candidate, functionId);
    });
    module = std::move(candidate);
  }
  // The real pass applies limitUniqueSubBlockToStore to the successfully
  // tiled AIV functions as well.  It guards only operations that could not be
  // sliced (for example, a scalar workspace store), while operations marked
  // tiled_op continue to run independently on both sub-blocks.
  module = MeasureStage(trace, "TileAndBind.LimitUniqueSubBlock", [&] {
    return LimitUniqueSubBlockToStore(std::move(module));
  });
  ApplyOperationSemanticsToAll(module.operations);
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
