#ifndef CVPIPELINE_UB_MODEL_CPP_GENERIC_REWRITER_HPP
#define CVPIPELINE_UB_MODEL_CPP_GENERIC_REWRITER_HPP

#include "generic_ir.hpp"

namespace cvub {

class GenericRewriter {
public:
  explicit GenericRewriter(GenericModule &module) : module(module) {
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        nextValue = std::max(nextValue, argument + 1);
    for (const GenericOperation &operation : module.operations)
      for (int result : operation.results)
        nextValue = std::max(nextValue, result + 1);
  }

  int createOperation(int parent, int region, int block,
                      const std::string &name,
                      const std::vector<std::string> &resultTypes,
                      const std::vector<int> &operands = {},
                      const std::vector<std::string> &operandTypes = {},
                      const std::string &properties = "",
                      const std::string &attributes = "{}") {
    GenericOperation operation;
    operation.id = static_cast<int>(module.operations.size());
    operation.parentId = parent;
    operation.regionId = region;
    operation.blockId = block;
    operation.name = name;
    operation.operands = operands;
    operation.operandTypes = operandTypes;
    operation.resultTypes = resultTypes;
    operation.properties = properties;
    operation.attributes = attributes;
    for (size_t index = 0; index < resultTypes.size(); ++index)
      operation.results.push_back(nextValue++);
    module.operations.push_back(std::move(operation));
    return static_cast<int>(module.operations.size() - 1);
  }

  int createRegion(int parentOperation) {
    GenericRegion region;
    region.id = static_cast<int>(module.regions.size());
    region.parentOperation = parentOperation;
    module.regions.push_back(std::move(region));
    module.operations.at(static_cast<size_t>(parentOperation))
        .regions.push_back(static_cast<int>(module.regions.size() - 1));
    return static_cast<int>(module.regions.size() - 1);
  }

  int createBlock(int region, const std::vector<std::string> &argumentTypes) {
    GenericBlock block;
    block.id = static_cast<int>(module.blocks.size());
    block.regionId = region;
    block.argumentTypes = argumentTypes;
    for (size_t index = 0; index < argumentTypes.size(); ++index)
      block.arguments.push_back(nextValue++);
    module.blocks.push_back(std::move(block));
    module.regions.at(static_cast<size_t>(region))
        .blocks.push_back(static_cast<int>(module.blocks.size() - 1));
    return static_cast<int>(module.blocks.size() - 1);
  }

  void appendToBlock(int block, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    GenericOperation &op =
        module.operations.at(static_cast<size_t>(operation));
    op.ordinal = static_cast<int>(record.operations.size());
    record.operations.push_back(operation);
  }

  void insertToBlock(int block, size_t position, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    if (position > record.operations.size())
      position = record.operations.size();
    GenericOperation &op =
        module.operations.at(static_cast<size_t>(operation));
    op.blockId = block;
    op.regionId = record.regionId;
    op.parentId = module.regions.at(static_cast<size_t>(record.regionId))
                      .parentOperation;
    record.operations.insert(record.operations.begin() +
                                 static_cast<std::ptrdiff_t>(position),
                             operation);
    for (size_t index = 0; index < record.operations.size(); ++index)
      module.operations.at(static_cast<size_t>(record.operations[index]))
          .ordinal = static_cast<int>(index);
  }

  void removeFromBlock(int block, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    record.operations.erase(
        std::remove(record.operations.begin(), record.operations.end(),
                    operation),
        record.operations.end());
    for (size_t index = 0; index < record.operations.size(); ++index)
      module.operations.at(static_cast<size_t>(record.operations[index]))
          .ordinal = static_cast<int>(index);
  }

  int cloneOperation(int sourceId, int parent, int region, int block,
                     const std::map<int, int> &values) {
    const GenericOperation source =
        module.operations.at(static_cast<size_t>(sourceId));
    std::vector<int> operands = source.operands;
    for (int &operand : operands) {
      auto mapped = values.find(operand);
      if (mapped != values.end())
        operand = mapped->second;
    }
    const int clone = createOperation(
        parent, region, block, source.name, source.resultTypes, operands,
        source.operandTypes, source.properties, source.attributes);
    GenericOperation &result = module.operations.at(static_cast<size_t>(clone));
    result.effects = source.effects;
    result.dpsInputs = source.dpsInputs;
    result.dpsInits = source.dpsInits;
    for (int &value : result.dpsInputs) {
      auto mapped = values.find(value);
      if (mapped != values.end())
        value = mapped->second;
    }
    for (int &value : result.dpsInits) {
      auto mapped = values.find(value);
      if (mapped != values.end())
        value = mapped->second;
    }
    result.successors = source.successors;
    return clone;
  }

  int cloneOperationTree(int sourceId, int parent, int region, int block,
                         std::map<int, int> &values) {
    const GenericOperation source =
        module.operations.at(static_cast<size_t>(sourceId));
    const int clone = cloneOperation(sourceId, parent, region, block, values);
    GenericOperation &cloned =
        module.operations.at(static_cast<size_t>(clone));
    for (size_t index = 0;
         index < source.results.size() && index < cloned.results.size(); ++index)
      values[source.results[index]] = cloned.results[index];
    for (int sourceRegionId : source.regions) {
      const GenericRegion sourceRegion =
          module.regions.at(static_cast<size_t>(sourceRegionId));
      const int clonedRegion = createRegion(clone);
      for (int sourceBlockId : sourceRegion.blocks) {
        const GenericBlock sourceBlock =
            module.blocks.at(static_cast<size_t>(sourceBlockId));
        const int clonedBlock = createBlock(clonedRegion,
                                            sourceBlock.argumentTypes);
        const GenericBlock &newBlock =
            module.blocks.at(static_cast<size_t>(clonedBlock));
        for (size_t index = 0; index < sourceBlock.arguments.size(); ++index)
          values[sourceBlock.arguments[index]] = newBlock.arguments[index];
        for (int child : sourceBlock.operations) {
          const int clonedChild =
              cloneOperationTree(child, clone, clonedRegion, clonedBlock, values);
          appendToBlock(clonedBlock, clonedChild);
        }
      }
    }
    return clone;
  }

  int newValue() { return nextValue++; }

private:
  GenericModule &module;
  int nextValue = 0;
};

inline GenericModule CompactGenericModule(GenericModule module) {
  if (module.operations.empty())
    return module;
  std::set<int> reachableOperations;
  std::set<int> reachableRegions;
  std::set<int> reachableBlocks;
  std::function<void(int)> visitOperation;
  std::function<void(int)> visitRegion;
  visitRegion = [&](int regionId) {
    if (!reachableRegions.insert(regionId).second)
      return;
    for (int blockId : module.regions.at(static_cast<size_t>(regionId)).blocks) {
      if (!reachableBlocks.insert(blockId).second)
        continue;
      for (int operationId :
           module.blocks.at(static_cast<size_t>(blockId)).operations)
        visitOperation(operationId);
    }
  };
  visitOperation = [&](int operationId) {
    if (!reachableOperations.insert(operationId).second)
      return;
    for (int regionId :
         module.operations.at(static_cast<size_t>(operationId)).regions)
      visitRegion(regionId);
  };
  visitOperation(0);

  std::map<int, int> operationIds;
  std::map<int, int> regionIds;
  std::map<int, int> blockIds;
  std::map<int, int> valueIds;
  GenericModule compact;
  std::function<int(int, int)> copyOperation;
  std::function<int(int, int)> copyRegion;
  copyRegion = [&](int oldRegion, int parent) {
    const GenericRegion &source =
        module.regions.at(static_cast<size_t>(oldRegion));
    GenericRegion region;
    region.id = static_cast<int>(compact.regions.size());
    region.parentOperation = parent;
    region.ordinal = source.ordinal;
    regionIds[oldRegion] = region.id;
    compact.regions.push_back(region);
    for (int oldBlock : source.blocks) {
      const GenericBlock &sourceBlock =
          module.blocks.at(static_cast<size_t>(oldBlock));
      GenericBlock block;
      block.id = static_cast<int>(compact.blocks.size());
      block.regionId = region.id;
      block.ordinal = sourceBlock.ordinal;
      block.argumentTypes = sourceBlock.argumentTypes;
      for (int argument : sourceBlock.arguments) {
        const int mapped = static_cast<int>(valueIds.size());
        valueIds[argument] = mapped;
        block.arguments.push_back(mapped);
      }
      blockIds[oldBlock] = block.id;
      compact.blocks.push_back(block);
      compact.regions[static_cast<size_t>(region.id)].blocks.push_back(block.id);
      for (int oldOperation : sourceBlock.operations) {
        const int mapped = copyOperation(oldOperation, block.id);
        compact.blocks[static_cast<size_t>(block.id)].operations.push_back(mapped);
      }
    }
    return region.id;
  };
  copyOperation = [&](int oldOperation, int block) {
    GenericOperation operation =
        module.operations.at(static_cast<size_t>(oldOperation));
    operation.id = static_cast<int>(compact.operations.size());
    operationIds[oldOperation] = operation.id;
    operation.blockId = block;
    operation.regionId = block >= 0
                             ? compact.blocks.at(static_cast<size_t>(block)).regionId
                             : -1;
    operation.parentId = operation.regionId >= 0
                             ? compact.regions.at(
                                   static_cast<size_t>(operation.regionId))
                                   .parentOperation
                             : -1;
    operation.regions.clear();
    for (int &result : operation.results) {
      const int mapped = static_cast<int>(valueIds.size());
      valueIds[result] = mapped;
      result = mapped;
    }
    compact.operations.push_back(operation);
    for (int oldRegion :
         module.operations.at(static_cast<size_t>(oldOperation)).regions) {
      const int mapped = copyRegion(oldRegion, operation.id);
      compact.operations[static_cast<size_t>(operation.id)].regions.push_back(mapped);
    }
    return operation.id;
  };

  GenericOperation root = module.operations.front();
  root.id = 0;
  root.parentId = -1;
  root.regionId = -1;
  root.blockId = -1;
  root.regions.clear();
  for (int &result : root.results) {
    const int mapped = static_cast<int>(valueIds.size());
    valueIds[result] = mapped;
    result = mapped;
  }
  compact.operations.push_back(root);
  operationIds[0] = 0;
  for (int oldRegion : module.operations.front().regions) {
    const int mapped = copyRegion(oldRegion, 0);
    compact.operations.front().regions.push_back(mapped);
  }

  for (GenericOperation &operation : compact.operations) {
    for (int &operand : operation.operands)
      if (valueIds.count(operand))
        operand = valueIds.at(operand);
    for (int &successor : operation.successors)
      if (blockIds.count(successor))
        successor = blockIds.at(successor);
  }
  for (GenericBlock &block : compact.blocks)
    for (size_t index = 0; index < block.operations.size(); ++index)
      compact.operations.at(static_cast<size_t>(block.operations[index])).ordinal =
          static_cast<int>(index);
  return compact;
}

} // namespace cvub

#endif
