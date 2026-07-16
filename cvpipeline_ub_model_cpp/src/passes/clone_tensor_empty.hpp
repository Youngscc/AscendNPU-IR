#ifndef CVPIPELINE_UB_MODEL_CPP_CLONE_TENSOR_EMPTY_HPP
#define CVPIPELINE_UB_MODEL_CPP_CLONE_TENSOR_EMPTY_HPP

#include "../ir/generic_rewriter.hpp"

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

inline GenericModule RunCloneTensorEmpty(GenericModule module) {
  ApplyOperationSemanticsToAll(module.operations);
  std::map<int, int> definitions;
  std::map<int, std::vector<int>> users;
  for (const GenericOperation &operation : module.operations) {
    for (int result : operation.results)
      definitions[result] = operation.id;
    for (int operand : operation.operands)
      users[operand].push_back(operation.id);
  }

  std::vector<int> consumers;
  consumers.reserve(module.operations.size());
  for (const GenericOperation &operation : module.operations)
    consumers.push_back(operation.id);

  GenericRewriter rewriter(module);
  for (int consumerId : consumers) {
    const GenericOperation consumerSnapshot =
        module.operations.at(static_cast<size_t>(consumerId));
    const std::vector<size_t> operands =
        CloneTensorEmptyOperandIndices(consumerSnapshot);
    size_t insertion = static_cast<size_t>(consumerSnapshot.ordinal);
    for (size_t operandIndex : operands) {
      GenericOperation &consumer =
          module.operations.at(static_cast<size_t>(consumerId));
      if (operandIndex >= consumer.operands.size() ||
          operandIndex >= consumer.operandTypes.size() ||
          !GenericIsTensorType(consumer.operandTypes[operandIndex]))
        continue;
      const int sourceValue = consumer.operands[operandIndex];
      auto definition = definitions.find(sourceValue);
      if (definition == definitions.end())
        continue;
      const GenericOperation empty =
          module.operations.at(static_cast<size_t>(definition->second));
      if (empty.name != "tensor.empty" || empty.results.size() != 1)
        continue;

      const int cloneId = rewriter.cloneOperation(
          empty.id, consumerSnapshot.parentId, consumerSnapshot.regionId,
          consumerSnapshot.blockId, {});
      rewriter.insertToBlock(consumerSnapshot.blockId, insertion++, cloneId);
      const int clonedValue =
          module.operations.at(static_cast<size_t>(cloneId)).results.front();
      module.operations.at(static_cast<size_t>(consumerId))
          .operands[operandIndex] = clonedValue;

      for (int userId : users[sourceValue]) {
        const GenericOperation mark =
            module.operations.at(static_cast<size_t>(userId));
        if (!HasBufferSizeInByteAnnotation(mark) || mark.operands.empty() ||
            mark.operands.front() != sourceValue)
          continue;
        const int markClone = rewriter.cloneOperation(
            mark.id, consumerSnapshot.parentId, consumerSnapshot.regionId,
            consumerSnapshot.blockId, {{sourceValue, clonedValue}});
        rewriter.insertToBlock(consumerSnapshot.blockId, insertion++,
                               markClone);
      }
    }
  }
  bool erased = true;
  while (erased) {
    erased = false;
    std::map<int, size_t> userCounts;
    for (const GenericBlock &block : module.blocks)
      for (int operationId : block.operations)
        for (int operand :
             module.operations.at(static_cast<size_t>(operationId)).operands)
          ++userCounts[operand];
    for (auto operation = module.operations.rbegin();
         operation != module.operations.rend(); ++operation) {
      if (operation->name != "tensor.empty" || operation->results.empty() ||
          operation->blockId < 0 ||
          std::find(module.blocks.at(static_cast<size_t>(operation->blockId))
                        .operations.begin(),
                    module.blocks.at(static_cast<size_t>(operation->blockId))
                        .operations.end(),
                    operation->id) ==
              module.blocks.at(static_cast<size_t>(operation->blockId))
                  .operations.end() ||
          std::any_of(operation->results.begin(), operation->results.end(),
                      [&](int result) { return userCounts[result] != 0; }))
        continue;
      rewriter.removeFromBlock(operation->blockId, operation->id);
      erased = true;
      break;
    }
  }
  ApplyOperationSemanticsToAll(module.operations);
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
