#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TENSOR_EMPTY_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TENSOR_EMPTY_HPP

#include "canonicalization.hpp"
#include "ir_utils.hpp"
#include "types.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cvub {
namespace tensor_empty_detail {

inline bool IsAttachedOperation(const GenericModule &module,
                                const GenericOperation &operation) {
  if (operation.blockId < 0 ||
      static_cast<size_t>(operation.blockId) >= module.blocks.size())
    return false;
  const std::vector<int> &operations =
      module.blocks.at(static_cast<size_t>(operation.blockId)).operations;
  return std::find(operations.begin(), operations.end(), operation.id) !=
         operations.end();
}

inline const GenericOperation *AttachedDefinition(const GenericModule &module,
                                                  int value) {
  for (const GenericOperation &operation : module.operations)
    if (IsAttachedOperation(module, operation) &&
        std::find(operation.results.begin(), operation.results.end(), value) !=
            operation.results.end())
      return &operation;
  return nullptr;
}

inline bool IsTensorEmptyValue(const GenericModule &module, int value) {
  const GenericOperation *definition = AttachedDefinition(module, value);
  return definition != nullptr && definition->name == "tensor.empty" &&
         !definition->resultTypes.empty() &&
         startsWith(trim(definition->resultTypes.front()), "tensor<");
}

inline bool HasAttachedUse(const GenericModule &module, int value) {
  const auto contains = [value](const std::vector<int> &values) {
    return std::find(values.begin(), values.end(), value) != values.end();
  };
  for (const GenericOperation &operation : module.operations)
    if (IsAttachedOperation(module, operation) &&
        (contains(operation.operands) || contains(operation.dpsInputs) ||
         contains(operation.dpsInits)))
      return true;
  return false;
}

inline bool IsCloneStructuredOwner(const std::string &name) {
  static const std::set<std::string> fixed = {
      "hivm.hir.copy", "hivm.hir.load", "hivm.hir.store",
      "hivm.hir.fixpipe", "hivm.hir.mmadL1", "hivm.hir.Conv1dL1",
      "hivm.hir.Conv2dL1", "hivm.hir.Conv3dL1"};
  return fixed.count(name) != 0 || startsWith(name, "hivm.hir.v");
}

inline std::vector<size_t> DestinationOperandIndices(
    const GenericOperation &operation) {
  std::vector<size_t> indices;
  if (IsCloneStructuredOwner(operation.name)) {
    indices = DpsInitOperandIndices(operation.name, operation.operands.size(),
                                    operation.properties);
  } else if (operation.name == "scf.for" && operation.operands.size() > 3) {
    for (size_t index = 3; index < operation.operands.size(); ++index)
      indices.push_back(index);
  } else if (operation.name == "scf.while") {
    for (size_t index = 0; index < operation.operands.size(); ++index)
      indices.push_back(index);
  } else if (operation.name == "tensor.insert" &&
             operation.operands.size() > 1) {
    indices.push_back(1);
  }
  return indices;
}

inline int CloneEmptyBefore(GenericModule &module, int emptyId, int ownerId) {
  const GenericOperation source =
      module.operations.at(static_cast<size_t>(emptyId));
  const GenericOperation owner =
      module.operations.at(static_cast<size_t>(ownerId));
  GenericRewriter rewriter(module);
  const int clone = rewriter.cloneOperation(
      source.id, owner.parentId, owner.regionId, owner.blockId, {});
  const GenericBlock &block =
      module.blocks.at(static_cast<size_t>(owner.blockId));
  const auto position =
      std::find(block.operations.begin(), block.operations.end(), owner.id);
  rewriter.insertToBlock(
      owner.blockId,
      static_cast<size_t>(std::distance(block.operations.begin(), position)),
      clone);
  return clone;
}

inline void CloneBufferSizeMarks(GenericModule &module, int oldValue,
                                 int newValue, int ownerId) {
  std::vector<int> marks;
  for (const GenericOperation &operation : module.operations)
    if (IsAttachedOperation(module, operation) &&
        operation.name == "annotation.mark" &&
        !operation.operands.empty() && operation.operands.front() == oldValue &&
        !IRDictionaryValue(operation.attributes,
                           "buffer_size_in_byte").empty())
      marks.push_back(operation.id);
  for (int markId : marks) {
    const GenericOperation owner =
        module.operations.at(static_cast<size_t>(ownerId));
    GenericRewriter rewriter(module);
    const int clone = rewriter.cloneOperation(
        markId, owner.parentId, owner.regionId, owner.blockId,
        {{oldValue, newValue}});
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(owner.blockId));
    const auto position =
        std::find(block.operations.begin(), block.operations.end(), owner.id);
    rewriter.insertToBlock(
        owner.blockId,
        static_cast<size_t>(std::distance(block.operations.begin(), position)),
        clone);
  }
}

} // namespace tensor_empty_detail

inline StageResult RunFoldTensorEmpty(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);
  bool changed = true;
  while (changed) {
    changed = false;
    for (const GenericOperation &operation : result.module.operations) {
      if (operation.blockId < 0 || operation.name != "tensor.insert_slice" ||
          operation.operands.size() < 2 || operation.results.size() != 1 ||
          !tensor_empty_detail::IsTensorEmptyValue(result.module,
                                                   operation.operands[0]))
        continue;
      const int source = operation.operands[0];
      const int destination = operation.operands[1];
      ReplaceAllUses(result.module, operation.results.front(), destination);
      GenericRewriter(result.module).removeFromBlock(operation.blockId,
                                                     operation.id);
      const GenericOperation *sourceDefinition =
          tensor_empty_detail::AttachedDefinition(result.module, source);
      if (sourceDefinition != nullptr &&
          !tensor_empty_detail::HasAttachedUse(result.module, source))
        GenericRewriter(result.module)
            .removeFromBlock(sourceDefinition->blockId, sourceDefinition->id);
      result.module = CompactGenericModule(std::move(result.module));
      changed = true;
      break;
    }
  }
  ValidateGenericModule(result.module);
  return result;
}

inline StageResult RunPostSplitCanonicalization(GenericModule module) {
  StageResult result = RunPreSplitCanonicalization(std::move(module));
  for (PostCVPipelineDiagnostic &diagnostic : result.diagnostics)
    if (diagnostic.pipelineStage == "CanonicalizationBeforeSplit")
      diagnostic.pipelineStage = "CanonicalizationAfterSplit";
  return result;
}

inline StageResult RunCloneTensorEmpty(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);
  const size_t originalOperationCount = result.module.operations.size();
  for (size_t id = 0; id < originalOperationCount; ++id) {
    if (id >= result.module.operations.size())
      break;
    const GenericOperation owner = result.module.operations[id];
    if (owner.blockId < 0)
      continue;
    for (size_t operandIndex :
         tensor_empty_detail::DestinationOperandIndices(owner)) {
      if (operandIndex >= result.module.operations[id].operands.size())
        continue;
      const int ownerId = static_cast<int>(id);
      const int oldValue = result.module.operations[id].operands[operandIndex];
      const GenericOperation *empty =
          tensor_empty_detail::AttachedDefinition(result.module, oldValue);
      if (empty == nullptr || empty->name != "tensor.empty" ||
          empty->results.size() != 1 || empty->resultTypes.size() != 1 ||
          !startsWith(trim(empty->resultTypes.front()), "tensor<"))
        continue;
      const int clone = tensor_empty_detail::CloneEmptyBefore(
          result.module, empty->id, ownerId);
      const int newValue = result.module.operations.at(
          static_cast<size_t>(clone)).results.front();
      GenericOperation &mutableOwner = result.module.operations[id];
      mutableOwner.operands[operandIndex] = newValue;
      if (!mutableOwner.dpsInits.empty()) {
        mutableOwner.dpsInits.clear();
        for (size_t initIndex : tensor_empty_detail::DestinationOperandIndices(
                 mutableOwner))
          mutableOwner.dpsInits.push_back(mutableOwner.operands[initIndex]);
      }
      if (tensor_empty_detail::IsCloneStructuredOwner(mutableOwner.name))
        tensor_empty_detail::CloneBufferSizeMarks(
            result.module, oldValue, newValue, ownerId);
    }
  }
  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
