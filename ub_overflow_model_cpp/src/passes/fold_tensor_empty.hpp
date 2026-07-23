#ifndef CVPIPELINE_UB_MODEL_CPP_FOLD_TENSOR_EMPTY_HPP
#define CVPIPELINE_UB_MODEL_CPP_FOLD_TENSOR_EMPTY_HPP

#include "canonicalization_hivm_pipeline.hpp"

namespace cvub {

inline std::unordered_map<int, const GenericOperation *>
FoldTensorEmptyDefinitions(const GenericModule &module) {
  std::unordered_map<int, const GenericOperation *> result;
  result.reserve(module.operations.size());
  for (const GenericOperation &operation : module.operations)
    for (int value : operation.results)
      result[value] = &operation;
  return result;
}

inline bool IsDefinedByTensorEmpty(
    int value,
    const std::unordered_map<int, const GenericOperation *> &definitions) {
  const auto definition = definitions.find(value);
  return definition != definitions.end() &&
         definition->second->name == "tensor.empty";
}

// Reach the same side-effect-free DCE fixed point as repeated
// EliminateCanonicalizationDeadCode calls, but maintain use counts in a
// worklist.  The old implementation rebuilt the complete ordered use map
// after removing every single dead operation.
inline bool EliminateFoldTensorEmptyDeadCode(GenericModule &module) {
  std::vector<bool> active(module.operations.size(), false);
  for (const GenericBlock &block : module.blocks)
    for (int operation : block.operations)
      if (operation >= 0 &&
          static_cast<size_t>(operation) < active.size())
        active[static_cast<size_t>(operation)] = true;

  std::unordered_map<int, size_t> uses;
  std::unordered_map<int, int> definitions;
  uses.reserve(module.operations.size() * 2);
  definitions.reserve(module.operations.size());
  for (const GenericOperation &operation : module.operations) {
    if (operation.blockId < 0 || !active[static_cast<size_t>(operation.id)])
      continue;
    for (int operand : operation.operands)
      ++uses[operand];
    for (int result : operation.results)
      definitions[result] = operation.id;
  }

  const auto isDead = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (!active[static_cast<size_t>(operationId)] ||
        operation.blockId < 0 || operation.results.empty() ||
        !operation.regions.empty() ||
        IsCanonicalizationTerminator(operation))
      return false;
    if (!operation.effects.empty() && operation.effects != "none" &&
        operation.name != "tensor.empty")
      return false;
    return std::all_of(operation.results.begin(), operation.results.end(),
                       [&](int result) {
                         auto found = uses.find(result);
                         return found == uses.end() || found->second == 0;
                       });
  };

  std::vector<int> worklist;
  worklist.reserve(module.operations.size());
  for (auto iterator = module.operations.rbegin();
       iterator != module.operations.rend(); ++iterator)
    if (iterator->id >= 0 && isDead(iterator->id))
      worklist.push_back(iterator->id);

  GenericRewriter rewriter(module);
  bool changed = false;
  while (!worklist.empty()) {
    const int operationId = worklist.back();
    worklist.pop_back();
    if (!isDead(operationId))
      continue;
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    active[static_cast<size_t>(operationId)] = false;
    rewriter.removeFromBlock(operation.blockId, operationId);
    changed = true;
    for (int operand : operation.operands) {
      auto count = uses.find(operand);
      if (count == uses.end() || count->second == 0)
        continue;
      --count->second;
      if (count->second != 0)
        continue;
      auto definition = definitions.find(operand);
      if (definition != definitions.end() && isDead(definition->second))
        worklist.push_back(definition->second);
    }
  }
  return changed;
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
  bool anyChange = false;
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
        changed = anyChange = true;
        continue;
      }
      if ((operation.name == "tensor.expand_shape" ||
           operation.name == "tensor.collapse_shape") &&
          !operation.operands.empty() &&
          IsDefinedByTensorEmpty(operation.operands.front(), definitions)) {
        std::vector<int> dynamicSizes(operation.operands.begin() + 1,
                                      operation.operands.end());
        ReplaceWithTensorEmpty(operation, std::move(dynamicSizes));
        changed = anyChange = true;
        continue;
      }
      if (operation.name == "tensor.concat" &&
          !operation.operands.empty() &&
          std::all_of(operation.operands.begin(), operation.operands.end(),
                      [&](int value) {
                        return IsDefinedByTensorEmpty(value, definitions);
                      })) {
        ReplaceWithTensorEmpty(operation);
        changed = anyChange = true;
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
        changed = anyChange = true;
        break;
      }
    }
  }
  anyChange |= EliminateFoldTensorEmptyDeadCode(module);
  if (!anyChange)
    return module;
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
