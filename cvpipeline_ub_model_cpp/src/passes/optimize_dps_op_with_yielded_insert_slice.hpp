#ifndef CVPIPELINE_UB_MODEL_CPP_OPTIMIZE_DPS_OP_WITH_YIELDED_INSERT_SLICE_HPP
#define CVPIPELINE_UB_MODEL_CPP_OPTIMIZE_DPS_OP_WITH_YIELDED_INSERT_SLICE_HPP

#include "one_shot_bufferize.hpp"

namespace cvub {

inline std::string ExtractSlicePropertiesFromInsertSlice(
    const std::string &properties) {
  const std::vector<size_t> segments = OperandSegmentSizes(properties);
  if (segments.size() != 5)
    throw std::runtime_error(
        "OptimizeDpsOpWithYieldedInsertSlice: malformed segment sizes");
  const std::string marker = "operandSegmentSizes = array<i32:";
  const size_t begin = properties.find(marker);
  const size_t end = properties.find('>', begin + marker.size());
  if (begin == std::string::npos || end == std::string::npos)
    throw std::runtime_error(
        "OptimizeDpsOpWithYieldedInsertSlice: missing segment sizes");
  std::ostringstream replacement;
  replacement << marker << " 1, " << segments[2] << ", " << segments[3]
              << ", " << segments[4] << '>';
  return properties.substr(0, begin) + replacement.str() +
         properties.substr(end + 1);
}

inline bool IsGenericBlockArgument(const GenericModule &module, int value) {
  return std::any_of(module.blocks.begin(), module.blocks.end(),
                     [&](const GenericBlock &block) {
                       return std::find(block.arguments.begin(),
                                        block.arguments.end(), value) !=
                              block.arguments.end();
                     });
}

// Mirrors ModifyDpsInitToSlicedIterArg. This pass runs immediately before
// OneShotBufferize in the Triton UB planning path.
inline GenericModule
RunOptimizeDpsOpWithYieldedInsertSlice(GenericModule module) {
  bool changed = true;
  while (changed) {
    changed = false;
    const auto definitions = DefiningOperations(module);
    for (const GenericOperation &insertSnapshot : module.operations) {
      if (insertSnapshot.name != "tensor.insert_slice" ||
          insertSnapshot.operands.size() < 2)
        continue;
      const auto sourceDefinition =
          definitions.find(insertSnapshot.operands.front());
      if (sourceDefinition == definitions.end())
        continue;
      const GenericOperation producerSnapshot = *sourceDefinition->second;
      const auto result = std::find(producerSnapshot.results.begin(),
                                    producerSnapshot.results.end(),
                                    insertSnapshot.operands.front());
      if (result == producerSnapshot.results.end())
        continue;
      const size_t resultNumber = static_cast<size_t>(
          result - producerSnapshot.results.begin());
      if (resultNumber >= producerSnapshot.dpsInits.size())
        continue;
      const int tyingInit = producerSnapshot.dpsInits[resultNumber];
      const auto initDefinition = definitions.find(tyingInit);
      if (initDefinition == definitions.end() ||
          initDefinition->second->name != "tensor.empty")
        continue;

      bool dominates = true;
      for (int operand : insertSnapshot.operands) {
        if (IsGenericBlockArgument(module, operand))
          continue;
        const auto definition = definitions.find(operand);
        if (definition == definitions.end() ||
            (definition->second->id != producerSnapshot.id &&
             !GenericOperationDominates(module, *definition->second,
                                        producerSnapshot))) {
          dominates = false;
          break;
        }
      }
      if (!dominates || producerSnapshot.blockId < 0)
        continue;

      std::vector<int> operands = {insertSnapshot.operands[1]};
      operands.insert(operands.end(), insertSnapshot.operands.begin() + 2,
                      insertSnapshot.operands.end());
      std::vector<std::string> operandTypes = {
          insertSnapshot.operandTypes.at(1)};
      operandTypes.insert(operandTypes.end(),
                          insertSnapshot.operandTypes.begin() + 2,
                          insertSnapshot.operandTypes.end());
      GenericRewriter rewriter(module);
      const int extract = rewriter.createOperation(
          producerSnapshot.parentId, producerSnapshot.regionId,
          producerSnapshot.blockId, "tensor.extract_slice",
          {producerSnapshot.resultTypes.at(resultNumber)}, operands,
          operandTypes,
          ExtractSlicePropertiesFromInsertSlice(insertSnapshot.properties),
          insertSnapshot.attributes);
      ApplyOperationSemantics(
          module.operations.at(static_cast<size_t>(extract)));
      const GenericBlock &block = module.blocks.at(
          static_cast<size_t>(producerSnapshot.blockId));
      const auto position = std::find(block.operations.begin(),
                                      block.operations.end(),
                                      producerSnapshot.id);
      if (position == block.operations.end())
        throw std::runtime_error(
            "OptimizeDpsOpWithYieldedInsertSlice: producer is not active");
      rewriter.insertToBlock(
          producerSnapshot.blockId,
          static_cast<size_t>(position - block.operations.begin()), extract);
      const int slicedInit =
          module.operations.at(static_cast<size_t>(extract)).results.front();
      GenericOperation &producer =
          module.operations.at(static_cast<size_t>(producerSnapshot.id));
      for (int &operand : producer.operands)
        if (operand == tyingInit)
          operand = slicedInit;
      producer.dpsInits[resultNumber] = slicedInit;
      changed = true;
      break;
    }
  }
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
