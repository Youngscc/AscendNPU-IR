#ifndef CVPIPELINE_UB_MODEL_CPP_CLONE_TENSOR_EMPTY_HPP
#define CVPIPELINE_UB_MODEL_CPP_CLONE_TENSOR_EMPTY_HPP

#include "../ir/generic_analysis.hpp"
#include "../ir/generic_rewriter.hpp"
#include "../support/debug_trace.hpp"

namespace cvub {

inline bool HasBufferSizeInByteAnnotation(const GenericOperation &operation) {
  return operation.name == "annotation.mark" &&
         (operation.attributes.find("buffer_size_in_byte") !=
              std::string::npos ||
          operation.properties.find("buffer_size_in_byte") !=
              std::string::npos);
}

inline std::vector<size_t>
CloneTensorEmptyOperandIndices(const GenericOperation &operation) {
  auto range = [](size_t begin, size_t end) {
    std::vector<size_t> result;
    for (size_t index = begin; index < end; ++index)
      result.push_back(index);
    return result;
  };
  if (IsHIVMStructuredOp(operation.name))
    return DpsInitOperandIndices(operation.name, operation.operands.size(),
                                 operation.properties);
  if (operation.name == "tensor.insert")
    return operation.operands.size() > 1 ? std::vector<size_t>{1}
                                         : std::vector<size_t>{};
  if (operation.name == "scf.for") {
    const size_t initCount = operation.results.size();
    return operation.operands.size() >= 3 + initCount
               ? range(operation.operands.size() - initCount,
                       operation.operands.size())
               : std::vector<size_t>{};
  }
  if (operation.name == "scf.while")
    return range(0, operation.operands.size());
  return {};
}

inline GenericModule RunCloneTensorEmpty(GenericModule module,
                                         DebugTrace *trace = nullptr) {
  MeasureStage(trace, "CloneTensorEmpty.ApplySemantics", [&] {
    ApplyOperationSemanticsToAll(module.operations);
  });
  int maximumValue = -1;
  MeasureStage(trace, "CloneTensorEmpty.Index", [&] {
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        maximumValue = std::max(maximumValue, argument);
    for (const GenericOperation &operation : module.operations) {
      for (int result : operation.results)
        maximumValue = std::max(maximumValue, result);
      for (int operand : operation.operands)
        maximumValue = std::max(maximumValue, operand);
    }
  });
  const size_t valueCount = maximumValue < 0
                                ? 0
                                : static_cast<size_t>(maximumValue) + 1;
  std::vector<int> definitions(valueCount, -1);
  std::vector<std::vector<int>> users(valueCount);
  MeasureStage(trace, "CloneTensorEmpty.Index", [&] {
    for (const GenericOperation &operation : module.operations) {
      for (int result : operation.results)
        definitions[static_cast<size_t>(result)] = operation.id;
      for (int operand : operation.operands)
        users[static_cast<size_t>(operand)].push_back(operation.id);
    }
  });
  GenericMutableOperandUseIndex useCounts = MeasureStage(
      trace, "CloneTensorEmpty.Index",
      [&] { return GenericMutableOperandUseIndex(module); });

  std::vector<int> consumers;
  consumers.reserve(module.operations.size());
  for (const GenericOperation &operation : module.operations)
    consumers.push_back(operation.id);
  const size_t originalOperationCount = module.operations.size();
  std::vector<std::vector<int>> clonesBefore(originalOperationCount);
  std::vector<std::vector<size_t>> cloneOperandIndices(originalOperationCount);
  size_t cloneUpperBound = 0;
  MeasureStage(trace, "CloneTensorEmpty.PrepareOperands", [&] {
    for (int consumerId : consumers) {
      const GenericOperation &consumer =
          module.operations.at(static_cast<size_t>(consumerId));
      std::vector<size_t> &indices =
          cloneOperandIndices[static_cast<size_t>(consumerId)];
      indices = CloneTensorEmptyOperandIndices(consumer);
      for (size_t operandIndex : indices) {
        if (operandIndex >= consumer.operands.size() ||
            operandIndex >= consumer.operandTypes.size() ||
            !GenericIsTensorType(consumer.operandTypes[operandIndex]))
          continue;
        const int source = consumer.operands[operandIndex];
        if (source < 0 || static_cast<size_t>(source) >= users.size() ||
            static_cast<size_t>(source) >= definitions.size())
          continue;
        const int definition = definitions[static_cast<size_t>(source)];
        if (definition < 0 ||
            module.operations.at(static_cast<size_t>(definition)).name !=
                "tensor.empty")
          continue;
        ++cloneUpperBound;
        for (int userId : users[static_cast<size_t>(source)])
          if (HasBufferSizeInByteAnnotation(
                  module.operations.at(static_cast<size_t>(userId))))
            ++cloneUpperBound;
      }
    }
  });
  module.operations.reserve(originalOperationCount + cloneUpperBound);

  GenericRewriter rewriter(module);
  MeasureStage(trace, "CloneTensorEmpty.Clone", [&] {
    for (int consumerId : consumers) {
    const GenericOperation &consumerSnapshot =
        module.operations.at(static_cast<size_t>(consumerId));
    const std::vector<size_t> &operands =
        cloneOperandIndices[static_cast<size_t>(consumerId)];
    for (size_t operandIndex : operands) {
      GenericOperation &consumer =
          module.operations.at(static_cast<size_t>(consumerId));
      if (operandIndex >= consumer.operands.size() ||
          operandIndex >= consumer.operandTypes.size() ||
          !GenericIsTensorType(consumer.operandTypes[operandIndex]))
        continue;
      const int sourceValue = consumer.operands[operandIndex];
      if (sourceValue < 0 ||
          static_cast<size_t>(sourceValue) >= definitions.size() ||
          definitions[static_cast<size_t>(sourceValue)] < 0)
        continue;
      const GenericOperation &empty =
          module.operations.at(static_cast<size_t>(
              definitions[static_cast<size_t>(sourceValue)]));
      if (empty.name != "tensor.empty" || empty.results.size() != 1)
        continue;

      const int cloneId = rewriter.cloneOperation(
          empty.id, consumerSnapshot.parentId, consumerSnapshot.regionId,
          consumerSnapshot.blockId, {});
      clonesBefore[static_cast<size_t>(consumerId)].push_back(cloneId);
      const int clonedValue =
          module.operations.at(static_cast<size_t>(cloneId)).results.front();
      for (int operand :
           module.operations.at(static_cast<size_t>(cloneId)).operands)
        useCounts.addUse(operand);
      rewriter.replaceOperand(consumerId, operandIndex, clonedValue);
      useCounts.replaceUse(sourceValue, clonedValue);

      for (int userId : users[static_cast<size_t>(sourceValue)]) {
        const GenericOperation &mark =
            module.operations.at(static_cast<size_t>(userId));
        if (!HasBufferSizeInByteAnnotation(mark) || mark.operands.empty() ||
            mark.operands.front() != sourceValue)
          continue;
        const int markClone = rewriter.cloneOperation(
            mark.id, consumerSnapshot.parentId, consumerSnapshot.regionId,
            consumerSnapshot.blockId, {{sourceValue, clonedValue}});
        clonesBefore[static_cast<size_t>(consumerId)].push_back(markClone);
        for (int operand :
             module.operations.at(static_cast<size_t>(markClone)).operands)
          useCounts.addUse(operand);
      }
      }
    }
  });
  // Inserting every clone into the middle of a block shifts the remaining
  // operation IDs and rewrites all trailing ordinals.  Clone IDs, SSA IDs and
  // the final block order do not depend on those intermediate shifts, so keep
  // the exact per-consumer clone sequence above and materialize each block in
  // one linear pass.
  MeasureStage(trace, "CloneTensorEmpty.RebuildBlocks", [&] {
    for (GenericBlock &block : module.blocks) {
    size_t cloneCount = 0;
    for (int operationId : block.operations)
      if (operationId >= 0 &&
          static_cast<size_t>(operationId) < originalOperationCount)
        cloneCount += clonesBefore[static_cast<size_t>(operationId)].size();
    if (cloneCount == 0)
      continue;
    std::vector<int> rebuilt;
    rebuilt.reserve(block.operations.size() + cloneCount);
    for (int operationId : block.operations) {
      if (operationId >= 0 &&
          static_cast<size_t>(operationId) < originalOperationCount) {
        const std::vector<int> &clones =
            clonesBefore[static_cast<size_t>(operationId)];
        rebuilt.insert(rebuilt.end(), clones.begin(), clones.end());
      }
      rebuilt.push_back(operationId);
    }
    block.operations.swap(rebuilt);
    for (size_t ordinal = 0; ordinal < block.operations.size(); ++ordinal) {
      GenericOperation &operation = module.operations.at(
          static_cast<size_t>(block.operations[ordinal]));
      operation.blockId = block.id;
      operation.regionId = block.regionId;
      operation.parentId =
          module.regions.at(static_cast<size_t>(block.regionId))
              .parentOperation;
      operation.ordinal = static_cast<int>(ordinal);
      }
    }
  });
  std::vector<bool> active(module.operations.size(), false);
  for (const GenericBlock &block : module.blocks)
    for (int operationId : block.operations)
      active[static_cast<size_t>(operationId)] = true;
  auto isDeadEmpty = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    return active[static_cast<size_t>(operationId)] &&
           operation.name == "tensor.empty" && !operation.results.empty() &&
           std::none_of(operation.results.begin(), operation.results.end(),
                        [&](int result) { return useCounts.hasUsers(result); });
  };
  std::vector<int> deadWorklist;
  deadWorklist.reserve(module.operations.size());
  for (const GenericOperation &operation : module.operations)
    if (isDeadEmpty(operation.id))
      deadWorklist.push_back(operation.id);
  MeasureStage(trace, "CloneTensorEmpty.EraseDead", [&] {
    while (!deadWorklist.empty()) {
    const int operationId = deadWorklist.back();
    deadWorklist.pop_back();
    if (!isDeadEmpty(operationId))
      continue;
    const GenericOperation operation =
        module.operations.at(static_cast<size_t>(operationId));
    rewriter.removeFromBlock(operation.blockId, operationId);
    active[static_cast<size_t>(operationId)] = false;
    for (int operand : operation.operands) {
      useCounts.removeUse(operand);
      if (operand < 0 || static_cast<size_t>(operand) >= definitions.size())
        continue;
      const int definingOperation = definitions[static_cast<size_t>(operand)];
      if (definingOperation >= 0 && isDeadEmpty(definingOperation))
        deadWorklist.push_back(definingOperation);
      }
    }
  });
  MeasureStage(trace, "CloneTensorEmpty.ApplyDirtySemantics",
               [&] { rewriter.applyDirtyOperationSemantics(); });
  return MeasureStage(trace, "CloneTensorEmpty.Compact", [&] {
    return CompactGenericModule(std::move(module));
  });
}

} // namespace cvub

#endif
