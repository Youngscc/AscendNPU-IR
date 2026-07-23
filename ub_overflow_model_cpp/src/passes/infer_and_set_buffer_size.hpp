#ifndef CVPIPELINE_UB_MODEL_CPP_INFER_AND_SET_BUFFER_SIZE_HPP
#define CVPIPELINE_UB_MODEL_CPP_INFER_AND_SET_BUFFER_SIZE_HPP

#include "../ir/generic_rewriter.hpp"
#include "global_workspace_plan.hpp"
#include "split_mix_kernel.hpp"

namespace cvub {

inline std::optional<int64_t>
ParseBufferSizeInByte(const GenericOperation &operation) {
  std::string value =
      FindDictionaryValue(operation.attributes, "buffer_size_in_byte");
  if (value.empty())
    value = FindDictionaryValue(operation.properties, "buffer_size_in_byte");
  if (value.empty())
    return std::nullopt;
  const size_t typed = value.find(" : ");
  if (typed != std::string::npos)
    value.resize(typed);
  return std::stoll(trim(value));
}

inline std::string RemoveBufferSizeInByte(std::string dictionary) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return dictionary;
  std::vector<std::string> kept;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    if (trim(entry.substr(0, equal)) != "buffer_size_in_byte")
      kept.push_back(entry);
  }
  std::string result = "{";
  for (size_t index = 0; index < kept.size(); ++index) {
    if (index)
      result += ", ";
    result += kept[index];
  }
  result += '}';
  return result;
}

inline bool HasNoDiscardableAttributes(const GenericOperation &operation) {
  if (operation.attributes.empty() || operation.attributes == "{}")
    return true;

  std::set<std::string> inherentProperties;
  if (operation.properties.size() >= 2) {
    for (const std::string &entry : splitTopLevel(operation.properties.substr(
             1, operation.properties.size() - 2)))
      inherentProperties.insert(trim(entry));
  }
  for (const std::string &entry : splitTopLevel(operation.attributes.substr(
           1, operation.attributes.size() - 2)))
    if (inherentProperties.count(trim(entry)) == 0)
      return false;
  return true;
}

inline std::string StaticByteBufferType(const std::string &type,
                                        int64_t bytes) {
  const size_t open = type.find('<');
  const size_t close = type.rfind('>');
  if (open == std::string::npos || close == std::string::npos || close <= open)
    throw std::runtime_error("SetBufferSize: malformed memref type");
  const std::string body = type.substr(open + 1, close - open - 1);
  const std::vector<std::string> fields = splitTopLevel(body);
  std::string result = "memref<" + std::to_string(bytes) + "xi8";
  for (size_t index = 1; index < fields.size(); ++index)
    result += ", " + fields[index];
  return result + ">";
}

inline std::vector<int> BufferSizeFunctionDescendants(
    const GenericModule &module, const GenericOperation &function) {
  std::vector<int> descendants;
  std::function<void(int)> visit = [&](int operationId) {
    descendants.push_back(operationId);
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          visit(child);
  };
  visit(function.id);
  return descendants;
}

inline int TraceBufferSizeAllocation(
    int value, const std::map<int, const GenericOperation *> &definitions) {
  std::set<int> visited;
  while (visited.insert(value).second) {
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return -1;
    const GenericOperation &operation = *definition->second;
    if (operation.name == "memref.alloc" || operation.name == "memref.alloca" ||
        operation.name == "memref_ext.alloc_workspace")
      return operation.id;
    if ((operation.name == "memref.view" ||
         operation.name == "memref.subview" ||
         operation.name == "memref.reinterpret_cast" ||
         operation.name == "memref.expand_shape" ||
         operation.name == "memref.collapse_shape") &&
        !operation.operands.empty()) {
      value = operation.operands.front();
      continue;
    }
    return -1;
  }
  return -1;
}

// Mirrors AutoInferBufferSizePass. It is intentionally limited to
// memref.alloc because the real pass does not infer alloc_workspace sizes.
inline GenericModule RunAutoInferBufferSize(GenericModule module) {
  GenericRewriter rewriter(module);
  std::vector<int> functions;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      functions.push_back(operation.id);
  for (int functionId : functions) {
    const GenericOperation functionSnapshot =
        module.operations.at(static_cast<size_t>(functionId));
    if (functionSnapshot.name != "func.func" ||
        functionSnapshot.attributes.find("enable_auto_mark_buffer_size") ==
            std::string::npos)
      continue;
    const std::vector<int> descendants =
        BufferSizeFunctionDescendants(module, functionSnapshot);
    std::optional<int64_t> elementCount;
    std::set<int> annotatedValues;
    for (int operationId : descendants) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "annotation.mark" || operation.operands.empty())
        continue;
      const std::optional<int64_t> bytes = ParseBufferSizeInByte(operation);
      if (!bytes)
        continue;
      annotatedValues.insert(operation.operands.front());
      if (elementCount || operation.operandTypes.empty())
        continue;
      const auto type = ParseMemRefType(operation.operandTypes.front());
      if (type)
        elementCount = *bytes * 8 /
                       static_cast<int64_t>(
                           getElementTypeBitWidth(type->elementType));
    }
    if (!elementCount)
      continue;

    for (int operationId : descendants) {
      const GenericOperation allocation =
          module.operations.at(static_cast<size_t>(operationId));
      if (allocation.name != "memref.alloc" || allocation.results.size() != 1 ||
          allocation.resultTypes.size() != 1 ||
          annotatedValues.count(allocation.results.front()) != 0)
        continue;
      const auto type = ParseMemRefType(allocation.resultTypes.front());
      if (!type || std::all_of(type->shape.begin(), type->shape.end(),
                               [](const auto &extent) { return extent.has_value(); }))
        continue;
      const int64_t bytes =
          *elementCount *
          static_cast<int64_t>(getElementTypeBitWidth(type->elementType)) / 8;
      const int mark = rewriter.createOperation(
          allocation.parentId, allocation.regionId, allocation.blockId,
          "annotation.mark", {}, {allocation.results.front()},
          {allocation.resultTypes.front()}, "",
          "{buffer_size_in_byte = " + std::to_string(bytes) + " : i64}");
      rewriter.insertToBlock(allocation.blockId,
                             static_cast<size_t>(allocation.ordinal + 1), mark);
    }
  }
  return CompactGenericModule(std::move(module));
}

// ConstantizeBufferSizePass only rewrites dynamic memref.alloc/alloca values
// whose SSA dimensions have a proven constant upper bound. OneShotBufferize
// corpus has no such allocation; keeping this boundary explicit prevents the
// alloc_workspace SetBufferSize rule below from being attributed to it.
inline GenericModule RunConstantizeBufferSize(GenericModule module) {
  return module;
}

// Mirrors SetBufferSizePass and createAllocWithSettingBufferSize: a dynamic
// alloc-like value with a byte-size annotation becomes a static i8 backing
// allocation viewed through its original dynamic type.
inline GenericModule RunSetBufferSize(GenericModule module) {
  const auto definitions = DefiningOperations(module);
  std::map<int, int64_t> allocationSizes;
  std::vector<int> marks;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "annotation.mark" || operation.operands.empty())
      continue;
    const std::optional<int64_t> bytes = ParseBufferSizeInByte(operation);
    if (!bytes)
      continue;
    const int allocation =
        TraceBufferSizeAllocation(operation.operands.front(), definitions);
    if (allocation < 0)
      continue;
    const GenericOperation &root =
        module.operations.at(static_cast<size_t>(allocation));
    if (root.resultTypes.empty())
      continue;
    const auto type = ParseMemRefType(root.resultTypes.front());
    if (!type)
      continue;
    marks.push_back(operation.id);
    if (std::all_of(type->shape.begin(), type->shape.end(),
                    [](const auto &extent) { return extent.has_value(); }))
      continue;
    auto inserted = allocationSizes.emplace(allocation, *bytes);
    if (!inserted.second && inserted.first->second != *bytes)
      throw std::runtime_error(
          "SetBufferSize: conflicting sizes on the same allocation");
  }

  GenericRewriter rewriter(module);
  for (const auto &[allocationId, bytes] : allocationSizes) {
    const GenericOperation allocation =
        module.operations.at(static_cast<size_t>(allocationId));
    if (allocation.results.size() != 1 || allocation.resultTypes.size() != 1 ||
        allocation.blockId < 0)
      throw std::runtime_error("SetBufferSize: malformed allocation");
    const std::vector<size_t> segments =
        OperandSegmentSizes(allocation.properties);
    std::vector<int> dynamicSizes;
    std::vector<std::string> dynamicSizeTypes;
    if (allocation.name == "memref_ext.alloc_workspace") {
      if (segments.size() != 3 || segments.front() != 1)
        throw std::runtime_error(
            "SetBufferSize: malformed alloc_workspace operands");
      for (size_t index = 0; index < segments[1]; ++index) {
        dynamicSizes.push_back(allocation.operands.at(1 + index));
        dynamicSizeTypes.push_back(allocation.operandTypes.at(1 + index));
      }
    } else {
      dynamicSizes = allocation.operands;
      dynamicSizeTypes = allocation.operandTypes;
    }

    const std::string backingType =
        StaticByteBufferType(allocation.resultTypes.front(), bytes);
    const int zero = rewriter.createOperation(
        allocation.parentId, allocation.regionId, allocation.blockId,
        "arith.constant", {"index"}, {}, {}, "{value = 0 : index}");
    std::vector<int> backingOperands;
    std::vector<std::string> backingOperandTypes;
    std::string backingProperties = allocation.properties;
    if (allocation.name == "memref_ext.alloc_workspace") {
      backingOperands = {allocation.operands.front()};
      backingOperandTypes = {allocation.operandTypes.front()};
      backingProperties = SetOperandSegmentSizes(allocation.properties,
                                                 {1, 0, 0});
    }
    const int backing = rewriter.createOperation(
        allocation.parentId, allocation.regionId, allocation.blockId,
        allocation.name, {backingType}, backingOperands,
        backingOperandTypes, backingProperties, allocation.attributes);
    std::vector<int> viewOperands = {
        module.operations.at(static_cast<size_t>(backing)).results.front(),
        module.operations.at(static_cast<size_t>(zero)).results.front()};
    viewOperands.insert(viewOperands.end(), dynamicSizes.begin(),
                        dynamicSizes.end());
    std::vector<std::string> viewOperandTypes = {backingType, "index"};
    viewOperandTypes.insert(viewOperandTypes.end(), dynamicSizeTypes.begin(),
                            dynamicSizeTypes.end());
    const int view = rewriter.createOperation(
        allocation.parentId, allocation.regionId, allocation.blockId,
        "memref.view", allocation.resultTypes, viewOperands,
        viewOperandTypes);
    const int viewValue =
        module.operations.at(static_cast<size_t>(view)).results.front();
    ReplaceSplitMixValue(module, allocation.results.front(), viewValue);

    const size_t position = static_cast<size_t>(allocation.ordinal);
    rewriter.removeFromBlock(allocation.blockId, allocationId);
    rewriter.insertToBlock(allocation.blockId, position, zero);
    rewriter.insertToBlock(allocation.blockId, position + 1, backing);
    rewriter.insertToBlock(allocation.blockId, position + 2, view);
  }

  for (int markId : marks) {
    GenericOperation &mark = module.operations.at(static_cast<size_t>(markId));
    mark.attributes = RemoveBufferSizeInByte(mark.attributes);
    mark.properties = RemoveBufferSizeInByte(mark.properties);
    // GenericIR keeps inherent properties in both `properties` and the merged
    // attribute dictionary. MLIR's annotation::MarkOp::isAttrEmpty() ignores
    // those properties and only observes remaining discardable attributes.
    if (HasNoDiscardableAttributes(mark) && mark.blockId >= 0)
      rewriter.removeFromBlock(mark.blockId, markId);
  }
  ApplyOperationSemanticsToAll(module.operations);
  return CompactGenericModule(std::move(module));
}

inline GenericModule RunInferAndSetBufferSizePipeline(GenericModule module) {
  module = RunAutoInferBufferSize(std::move(module));
  module = RunConstantizeBufferSize(std::move(module));
  return RunSetBufferSize(std::move(module));
}

} // namespace cvub

#endif
