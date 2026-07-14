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

inline bool IsStaticTensorType(const std::string &type) {
  return startsWith(trim(type), "tensor<") && type.find('?') == std::string::npos;
}

inline bool AnyEmptyOperand(const GenericModule &module,
                            const GenericOperation &operation) {
  return std::any_of(operation.operands.begin(), operation.operands.end(),
                     [&](int value) { return IsTensorEmptyValue(module, value); });
}

inline bool AllOperandsEmpty(const GenericModule &module,
                             const GenericOperation &operation) {
  return !operation.operands.empty() &&
         std::all_of(operation.operands.begin(), operation.operands.end(),
                     [&](int value) { return IsTensorEmptyValue(module, value); });
}

inline std::string UnsupportedFoldReason(const GenericModule &module,
                                         const GenericOperation &operation) {
  if (operation.blockId < 0 || operation.results.size() != 1 ||
      operation.resultTypes.size() != 1)
    return "";
  const bool sourceEmpty = !operation.operands.empty() &&
                           IsTensorEmptyValue(module, operation.operands.front());
  if ((operation.name == "tensor.extract_slice" ||
       operation.name == "tensor.expand_shape" ||
       operation.name == "tensor.collapse_shape") &&
      sourceEmpty && !IsStaticTensorType(operation.resultTypes.front()))
    return operation.name +
           " empty fold has dynamic result operands not modeled exactly";
  if (operation.name == "tensor.concat" && AnyEmptyOperand(module, operation) &&
      (!AllOperandsEmpty(module, operation) ||
       !IsStaticTensorType(operation.resultTypes.front())))
    return "tensor.concat empty pattern is applicable but not modeled exactly";
  if ((operation.name == "tensor.pack" || operation.name == "tensor.unpack") &&
      sourceEmpty && operation.operands.size() > 1 &&
      IsTensorEmptyValue(module, operation.operands[1]))
    return operation.name +
           " empty source/destination pattern is applicable but not modeled exactly";
  return "";
}

inline void EraseDeadEmpty(GenericModule &module, int value) {
  const GenericOperation *definition = AttachedDefinition(module, value);
  if (definition != nullptr && definition->name == "tensor.empty" &&
      !HasAttachedUse(module, value))
    GenericRewriter(module).removeFromBlock(definition->blockId, definition->id);
}

inline bool FoldEmptyProducer(GenericModule &module,
                              GenericOperation operation) {
  if (operation.blockId < 0 || operation.results.size() != 1 ||
      operation.resultTypes.size() != 1 || operation.operands.empty() ||
      !IsTensorEmptyValue(module, operation.operands.front()))
    return false;
  if (operation.name == "tensor.insert_slice" &&
      operation.operands.size() >= 2) {
    const int source = operation.operands.front();
    ReplaceAllUses(module, operation.results.front(), operation.operands[1]);
    GenericRewriter(module).removeFromBlock(operation.blockId, operation.id);
    EraseDeadEmpty(module, source);
    return true;
  }
  const bool reshapeOrSlice =
      operation.name == "tensor.extract_slice" ||
      operation.name == "tensor.expand_shape" ||
      operation.name == "tensor.collapse_shape";
  const bool concat = operation.name == "tensor.concat" &&
                      AllOperandsEmpty(module, operation);
  if ((!reshapeOrSlice && !concat) ||
      !IsStaticTensorType(operation.resultTypes.front()))
    return false;
  const std::vector<int> sources = operation.operands;
  GenericRewriter rewriter(module);
  const int empty = rewriter.createOperation(
      operation.parentId, operation.regionId, operation.blockId,
      "tensor.empty", operation.resultTypes);
  const GenericBlock &block =
      module.blocks.at(static_cast<size_t>(operation.blockId));
  const auto position =
      std::find(block.operations.begin(), block.operations.end(), operation.id);
  rewriter.insertToBlock(
      operation.blockId,
      static_cast<size_t>(std::distance(block.operations.begin(), position)),
      empty);
  ReplaceAllUses(module, operation.results.front(),
                 module.operations.at(static_cast<size_t>(empty)).results.front());
  rewriter.removeFromBlock(operation.blockId, operation.id);
  for (int source : sources)
    EraseDeadEmpty(module, source);
  return true;
}

inline bool IsCloneStructuredOwner(const std::string &name) {
  static const std::set<std::string> fixed = {
      "hivm.hir.copy", "hivm.hir.load", "hivm.hir.store",
      "hivm.hir.fixpipe", "hivm.hir.mmadL1", "hivm.hir.Conv1dL1",
      "hivm.hir.Conv2dL1", "hivm.hir.Conv3dL1"};
  // The real pass registers the generated HIVM vector op list, not arbitrary
  // operations sharing a spelling prefix.  IsDestinationStyleOp is the
  // generated-name registry used by this model.
  return fixed.count(name) != 0 ||
         (startsWith(name, "hivm.hir.v") && IsDestinationStyleOp(name));
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
  for (const GenericOperation &operation : result.module.operations) {
    const std::string reason =
        tensor_empty_detail::UnsupportedFoldReason(result.module, operation);
    if (reason.empty())
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"FoldTensorEmpty", "", operation.id, operation.name, reason});
  }
  if (result.precision == Precision::Incomplete)
    return result;
  bool changed = true;
  while (changed) {
    changed = false;
    for (const GenericOperation &operation : result.module.operations) {
      if (!tensor_empty_detail::FoldEmptyProducer(result.module, operation))
        continue;
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
    std::map<int, int> replacements;
    for (size_t operandIndex :
         tensor_empty_detail::DestinationOperandIndices(owner)) {
      if (operandIndex >= result.module.operations[id].operands.size())
        continue;
      const int ownerId = static_cast<int>(id);
      const int oldValue = result.module.operations[id].operands[operandIndex];
      if (replacements.count(oldValue) != 0)
        continue;
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
      replacements[oldValue] = newValue;
      GenericOperation &mutableOwner = result.module.operations[id];
      RemapValues(mutableOwner.operands, {{oldValue, newValue}});
      RemapValues(mutableOwner.dpsInputs, {{oldValue, newValue}});
      RemapValues(mutableOwner.dpsInits, {{oldValue, newValue}});
      if (tensor_empty_detail::IsCloneStructuredOwner(mutableOwner.name))
        tensor_empty_detail::CloneBufferSizeMarks(
            result.module, oldValue, newValue, ownerId);
    }
    for (const auto &[oldValue, newValue] : replacements) {
      (void)newValue;
      tensor_empty_detail::EraseDeadEmpty(result.module, oldValue);
    }
  }
  result.module = CompactGenericModule(std::move(result.module));
  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
