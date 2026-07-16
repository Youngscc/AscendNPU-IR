#ifndef CVPIPELINE_UB_MODEL_CPP_FOLD_TENSOR_EMPTY_HPP
#define CVPIPELINE_UB_MODEL_CPP_FOLD_TENSOR_EMPTY_HPP

#include "canonicalization_hivm_pipeline.hpp"

namespace cvub {

inline std::map<int, const GenericOperation *>
FoldTensorEmptyDefinitions(const GenericModule &module) {
  std::map<int, const GenericOperation *> result;
  for (const GenericOperation &operation : module.operations)
    for (int value : operation.results)
      result[value] = &operation;
  return result;
}

inline bool IsDefinedByTensorEmpty(
    int value, const std::map<int, const GenericOperation *> &definitions) {
  const auto definition = definitions.find(value);
  return definition != definitions.end() &&
         definition->second->name == "tensor.empty";
}

inline void ReplaceWithTensorEmpty(GenericOperation &operation,
                                   std::vector<int> dynamicSizes = {}) {
  operation.name = "tensor.empty";
  operation.operands = std::move(dynamicSizes);
  operation.operandTypes.assign(operation.operands.size(), "index");
  operation.properties.clear();
  operation.attributes = "{}";
  operation.effects = "none";
  operation.dpsInputs.clear();
  operation.dpsInits.clear();
}

inline std::vector<int>
ExtractSliceDynamicSizes(const GenericOperation &operation) {
  const std::vector<size_t> segments =
      OperandSegmentSizes(operation.properties);
  if (segments.size() != 4)
    return {};
  const size_t begin = segments[0] + segments[1];
  if (begin + segments[2] > operation.operands.size())
    throw std::runtime_error(
        "FoldTensorEmpty: malformed tensor.extract_slice operands");
  return std::vector<int>(operation.operands.begin() +
                              static_cast<std::ptrdiff_t>(begin),
                          operation.operands.begin() +
                              static_cast<std::ptrdiff_t>(begin + segments[2]));
}

// Mirrors tensor::populateFoldTensorEmptyPatterns. Only the SSA and shaped
// type information observed by OneShotBufferize is retained.
inline GenericModule RunFoldTensorEmpty(GenericModule module) {
  bool changed = true;
  while (changed) {
    changed = false;
    const auto definitions = FoldTensorEmptyDefinitions(module);
    for (GenericOperation &operation : module.operations) {
      if (operation.results.empty())
        continue;
      if (operation.name == "tensor.extract_slice" &&
          !operation.operands.empty() &&
          IsDefinedByTensorEmpty(operation.operands.front(), definitions)) {
        ReplaceWithTensorEmpty(operation,
                               ExtractSliceDynamicSizes(operation));
        changed = true;
        continue;
      }
      if ((operation.name == "tensor.expand_shape" ||
           operation.name == "tensor.collapse_shape") &&
          !operation.operands.empty() &&
          IsDefinedByTensorEmpty(operation.operands.front(), definitions)) {
        std::vector<int> dynamicSizes(operation.operands.begin() + 1,
                                      operation.operands.end());
        ReplaceWithTensorEmpty(operation, std::move(dynamicSizes));
        changed = true;
        continue;
      }
      if (operation.name == "tensor.concat" &&
          !operation.operands.empty() &&
          std::all_of(operation.operands.begin(), operation.operands.end(),
                      [&](int value) {
                        return IsDefinedByTensorEmpty(value, definitions);
                      })) {
        ReplaceWithTensorEmpty(operation);
        changed = true;
        continue;
      }
      if ((operation.name == "tensor.pack" ||
           operation.name == "tensor.unpack") &&
          operation.operands.size() >= 2 &&
          IsDefinedByTensorEmpty(operation.operands.front(), definitions)) {
        const int destination = operation.operands[1];
        for (int result : operation.results)
          ReplaceCanonicalizedValue(module, result, destination);
        GenericRewriter(module).removeFromBlock(operation.blockId,
                                                 operation.id);
        changed = true;
        break;
      }
    }
  }
  while (EliminateCanonicalizationDeadCode(module)) {
  }
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
