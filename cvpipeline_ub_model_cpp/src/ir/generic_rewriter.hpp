#ifndef CVPIPELINE_UB_MODEL_CPP_GENERIC_REWRITER_HPP
#define CVPIPELINE_UB_MODEL_CPP_GENERIC_REWRITER_HPP

#include "generic_ir.hpp"

namespace cvub {

class GenericRewriter {
public:
  enum class ValueInitialization { ScanExistingValues, SkipExistingValues };

  explicit GenericRewriter(GenericModule &inputModule,
                           GenericMutationListener *mutationListener = nullptr,
                           ValueInitialization valueInitialization =
                               ValueInitialization::ScanExistingValues)
      : module(inputModule), listener(mutationListener) {
    if (valueInitialization == ValueInitialization::SkipExistingValues)
      return;
    for (const GenericBlock &block : inputModule.blocks)
      for (int argument : block.arguments)
        nextValue = std::max(nextValue, argument + 1);
    for (const GenericOperation &operation : inputModule.operations)
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
    const int operationId = static_cast<int>(module.operations.size() - 1);
    markDirty(operationId);
    if (listener)
      listener->operationCreated(
          module.operations.at(static_cast<size_t>(operationId)));
    return operationId;
  }

  int createRegion(int parentOperation) {
    GenericRegion region;
    region.id = static_cast<int>(module.regions.size());
    region.parentOperation = parentOperation;
    region.ordinal = static_cast<int>(
        module.operations.at(static_cast<size_t>(parentOperation))
            .regions.size());
    module.regions.push_back(std::move(region));
    module.operations.at(static_cast<size_t>(parentOperation))
        .regions.push_back(static_cast<int>(module.regions.size() - 1));
    if (listener)
      listener->regionCreated(module.regions.back());
    return static_cast<int>(module.regions.size() - 1);
  }

  int createBlock(int region, const std::vector<std::string> &argumentTypes) {
    GenericBlock block;
    block.id = static_cast<int>(module.blocks.size());
    block.regionId = region;
    block.ordinal = static_cast<int>(
        module.regions.at(static_cast<size_t>(region)).blocks.size());
    block.argumentTypes = argumentTypes;
    for (size_t index = 0; index < argumentTypes.size(); ++index)
      block.arguments.push_back(nextValue++);
    module.blocks.push_back(std::move(block));
    module.regions.at(static_cast<size_t>(region))
        .blocks.push_back(static_cast<int>(module.blocks.size() - 1));
    if (listener)
      listener->blockCreated(module.blocks.back());
    return static_cast<int>(module.blocks.size() - 1);
  }

  void appendToBlock(int block, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    GenericOperation &op =
        module.operations.at(static_cast<size_t>(operation));
    const int oldBlock = op.blockId;
    op.ordinal = static_cast<int>(record.operations.size());
    record.operations.push_back(operation);
    if (listener)
      listener->operationMoved(operation, oldBlock, block);
  }

  void insertToBlock(int block, size_t position, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    if (position > record.operations.size())
      position = record.operations.size();
    GenericOperation &op =
        module.operations.at(static_cast<size_t>(operation));
    const int oldBlock = op.blockId;
    op.blockId = block;
    op.regionId = record.regionId;
    op.parentId = module.regions.at(static_cast<size_t>(record.regionId))
                      .parentOperation;
    record.operations.insert(record.operations.begin() +
                                 static_cast<std::ptrdiff_t>(position),
                             operation);
    for (size_t index = position; index < record.operations.size(); ++index)
      module.operations.at(static_cast<size_t>(record.operations[index]))
          .ordinal = static_cast<int>(index);
    if (listener)
      listener->operationMoved(operation, oldBlock, block);
  }

  void removeFromBlock(int block, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    const GenericOperation &target =
        module.operations.at(static_cast<size_t>(operation));
    auto first = record.operations.end();
    if (target.ordinal >= 0 &&
        static_cast<size_t>(target.ordinal) < record.operations.size() &&
        record.operations[static_cast<size_t>(target.ordinal)] == operation)
      first = record.operations.begin() + target.ordinal;
    else
      first = std::find(record.operations.begin(), record.operations.end(),
                        operation);
    const size_t firstAffected =
        first == record.operations.end()
            ? record.operations.size()
            : static_cast<size_t>(
                  std::distance(record.operations.begin(), first));
    if (first != record.operations.end())
      record.operations.erase(first);
    for (size_t index = firstAffected; index < record.operations.size();
         ++index)
      module.operations.at(static_cast<size_t>(record.operations[index]))
          .ordinal = static_cast<int>(index);
    if (listener)
      listener->operationMoved(operation, block, -1);
  }

  int cloneOperation(int sourceId, int parent, int region, int block,
                     const std::map<int, int> &values) {
    const GenericOperation &source =
        module.operations.at(static_cast<size_t>(sourceId));
    std::vector<int> operands = source.operands;
    for (int &operand : operands) {
      auto mapped = values.find(operand);
      if (mapped != values.end())
        operand = mapped->second;
    }
    // Preserve only the supplemental fields needed after createOperation.
    // Copying the complete source also copied large textual/CFG payloads for
    // every clone even though createOperation intentionally rebuilds the
    // structural fields.
    const std::string effects = source.effects;
    const int projectionSourceId = source.projectionSourceId;
    std::vector<int> dpsInputs = source.dpsInputs;
    std::vector<int> dpsInits = source.dpsInits;
    const std::vector<int> successors = source.successors;
    const int clone = createOperation(
        parent, region, block, source.name, source.resultTypes, operands,
        source.operandTypes, source.properties, source.attributes);
    GenericOperation &result = module.operations.at(static_cast<size_t>(clone));
    result.projectionSourceId = projectionSourceId;
    result.effects = effects;
    result.dpsInputs = std::move(dpsInputs);
    result.dpsInits = std::move(dpsInits);
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
    result.successors = successors;
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

  GenericOperation &modifyOperation(int operation) {
    if (listener)
      listener->operationWillModify(
          module.operations.at(static_cast<size_t>(operation)));
    markDirty(operation);
    return module.operations.at(static_cast<size_t>(operation));
  }

  void replaceOperand(int operation, size_t operand, int value) {
    GenericOperation &record =
        module.operations.at(static_cast<size_t>(operation));
    if (operand >= record.operands.size())
      throw std::runtime_error("GenericRewriter: invalid operand replacement");
    const int oldValue = record.operands[operand];
    markDirty(operation);
    record.operands[operand] = value;
    if (listener)
      listener->operandReplaced(operation, operand, oldValue, value);
  }

  void markDirty(int operation) {
    if (operation < 0 ||
        static_cast<size_t>(operation) >= module.operations.size())
      throw std::runtime_error("GenericRewriter: invalid dirty operation");
    if (dirtyFlags.size() < module.operations.size())
      dirtyFlags.resize(module.operations.size(), false);
    if (dirtyFlags[static_cast<size_t>(operation)])
      return;
    dirtyFlags[static_cast<size_t>(operation)] = true;
    dirtyOperations.push_back(operation);
  }

  void applyDirtyOperationSemantics() {
    for (int operation : dirtyOperations)
      ApplyOperationSemantics(
          module.operations.at(static_cast<size_t>(operation)));
    for (int operation : dirtyOperations)
      dirtyFlags[static_cast<size_t>(operation)] = false;
    dirtyOperations.clear();
  }

private:
  GenericModule &module;
  GenericMutationListener *listener = nullptr;
  int nextValue = 0;
  std::vector<int> dirtyOperations;
  std::vector<bool> dirtyFlags;
};

inline bool GenericOperationDominates(const GenericModule &module,
                                      const GenericOperation &candidate,
                                      const GenericOperation &operation) {
  auto precedesInBlock = [&](int blockId, int lhs, int rhs) {
    if (blockId < 0)
      return false;
    const std::vector<int> &operations =
        module.blocks.at(static_cast<size_t>(blockId)).operations;
    const auto lhsPosition =
        std::find(operations.begin(), operations.end(), lhs);
    const auto rhsPosition =
        std::find(operations.begin(), operations.end(), rhs);
    return lhsPosition != operations.end() && rhsPosition != operations.end() &&
           lhsPosition < rhsPosition;
  };

  const GenericOperation *cursor = &operation;
  while (cursor) {
    if (candidate.blockId == cursor->blockId &&
        precedesInBlock(candidate.blockId, candidate.id, cursor->id))
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
}

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
    auto remapSemanticValueReferences = [&](std::string &text) {
      size_t position = 0;
      while ((position = text.find("v(", position)) != std::string::npos) {
        const size_t digitsBegin = position + 2;
        const size_t close = text.find(')', digitsBegin);
        if (close == std::string::npos)
          break;
        const std::string token = text.substr(digitsBegin, close - digitsBegin);
        if (token.empty() ||
            !std::all_of(token.begin(), token.end(), [](unsigned char value) {
              return std::isdigit(value) != 0;
            })) {
          position = close + 1;
          continue;
        }
        const int oldValue = std::stoi(token);
        auto mapped = valueIds.find(oldValue);
        if (mapped == valueIds.end()) {
          position = close + 1;
          continue;
        }
        const std::string replacement =
            "v(" + std::to_string(mapped->second) + ")";
        text.replace(position, close - position + 1, replacement);
        position += replacement.size();
      }
    };
    remapSemanticValueReferences(operation.properties);
    remapSemanticValueReferences(operation.attributes);
    auto remapValue = [&](int &value, const char *kind) {
      auto mapped = valueIds.find(value);
      if (mapped == valueIds.end()) {
        std::string definition = "unknown";
        for (const GenericOperation &candidate : module.operations)
          if (std::find(candidate.results.begin(), candidate.results.end(),
                        value) != candidate.results.end()) {
            definition = candidate.name + "#" +
                         std::to_string(candidate.id) +
                         (reachableOperations.count(candidate.id) != 0
                              ? "(reachable)"
                              : "(erased)");
            break;
          }
        throw std::runtime_error(
            "CompactGenericModule: " + operation.name + " " + kind +
            " references unreachable SSA value " + std::to_string(value) +
            " defined by " + definition);
      }
      value = mapped->second;
    };
    for (int &operand : operation.operands)
      remapValue(operand, "operand");
    for (int &operand : operation.dpsInputs)
      remapValue(operand, "DPS input");
    for (int &operand : operation.dpsInits)
      remapValue(operand, "DPS init");
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
