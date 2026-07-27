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
    const bool wasDetached = detachedOperations.erase(operation) != 0;
    op.ordinal = static_cast<int>(record.operations.size());
    record.operations.push_back(operation);
    if (listener)
      listener->operationMoved(operation, wasDetached ? -1 : oldBlock, block);
  }

  void insertToBlock(int block, size_t position, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    if (position > record.operations.size())
      position = record.operations.size();
    GenericOperation &op =
        module.operations.at(static_cast<size_t>(operation));
    const int oldBlock = op.blockId;
    const bool wasDetached = detachedOperations.erase(operation) != 0;
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
      listener->operationMoved(operation, wasDetached ? -1 : oldBlock, block);
  }

  void removeFromBlock(int block, int operation) {
    GenericBlock &record = module.blocks.at(static_cast<size_t>(block));
    GenericOperation &target =
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
    const bool wasAttached = first != record.operations.end();
    if (wasAttached)
      record.operations.erase(first);
    for (size_t index = firstAffected; index < record.operations.size();
         ++index)
      module.operations.at(static_cast<size_t>(record.operations[index]))
          .ordinal = static_cast<int>(index);
    if (wasAttached)
      detachedOperations.insert(operation);
    if (listener && wasAttached)
      listener->operationMoved(operation, block, -1);
  }

  // Detach a rewrite wave in one linear block rebuild. Operation/value IDs
  // remain stable tombstones until CompactGenericModule runs, while each
  // surviving operation receives its final ordinal only once. This preserves
  // the observable block order of repeated removeFromBlock calls without the
  // quadratic suffix-renumbering cost.
  size_t removeManyFromBlocks(const std::vector<int> &operations) {
    if (operations.empty())
      return 0;
    std::vector<uint8_t> remove(module.operations.size(), uint8_t{0});
    for (int operation : operations)
      if (operation >= 0 &&
          static_cast<size_t>(operation) < module.operations.size())
        remove[static_cast<size_t>(operation)] = 1;

    size_t removed = 0;
    for (GenericBlock &block : module.blocks) {
      size_t write = 0;
      for (int operation : block.operations) {
        if (operation >= 0 &&
            static_cast<size_t>(operation) < remove.size() &&
            remove[static_cast<size_t>(operation)] != 0) {
          detachedOperations.insert(operation);
          if (listener)
            listener->operationMoved(operation, block.id, -1);
          ++removed;
          continue;
        }
        block.operations[write] = operation;
        module.operations.at(static_cast<size_t>(operation)).ordinal =
            static_cast<int>(write);
        ++write;
      }
      block.operations.resize(write);
    }
    return removed;
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
                         std::map<int, int> &values,
                         std::map<int, int> *blocks = nullptr) {
    // createOperation may grow the operation table and invalidate references
    // into it.  Preserve only the structural vectors needed after the clone;
    // copying the complete operation also copied attributes, properties,
    // types, effects and operand payloads a second time for every node in a
    // cloned tree.
    const GenericOperation &source =
        module.operations.at(static_cast<size_t>(sourceId));
    const std::vector<int> sourceResults = source.results;
    const std::vector<int> sourceRegions = source.regions;
    const int clone = cloneOperation(sourceId, parent, region, block, values);
    GenericOperation &cloned =
        module.operations.at(static_cast<size_t>(clone));
    for (size_t index = 0;
         index < sourceResults.size() && index < cloned.results.size(); ++index)
      values[sourceResults[index]] = cloned.results[index];
    for (int sourceRegionId : sourceRegions) {
      const std::vector<int> sourceBlocks =
          module.regions.at(static_cast<size_t>(sourceRegionId)).blocks;
      const int clonedRegion = createRegion(clone);
      for (int sourceBlockId : sourceBlocks) {
        const GenericBlock &sourceBlock =
            module.blocks.at(static_cast<size_t>(sourceBlockId));
        const std::vector<int> sourceArguments = sourceBlock.arguments;
        const std::vector<std::string> sourceArgumentTypes =
            sourceBlock.argumentTypes;
        const std::vector<int> sourceOperations = sourceBlock.operations;
        const int clonedBlock = createBlock(clonedRegion,
                                            sourceArgumentTypes);
        if (blocks)
          (*blocks)[sourceBlockId] = clonedBlock;
        const GenericBlock &newBlock =
            module.blocks.at(static_cast<size_t>(clonedBlock));
        for (size_t index = 0; index < sourceArguments.size(); ++index)
          values[sourceArguments[index]] = newBlock.arguments[index];
        for (int child : sourceOperations) {
          const int clonedChild =
              cloneOperationTree(child, clone, clonedRegion, clonedBlock,
                                 values, blocks);
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

  // Replace every SSA use while preserving GenericOperation's cached DPS
  // projections immediately.  With a PipelineAnalysisContext listener this
  // visits only the real users; standalone callers retain the exact legacy
  // behavior through the module-scan fallback.
  void replaceAllUses(int from, int to) {
    if (from == to)
      return;
    std::vector<int> users;
    bool hasIndexedUsers = false;
    if (listener) {
      if (const std::vector<int> *indexed = listener->replacementUsers(from)) {
        users = *indexed;
        hasIndexedUsers = true;
      }
    }
    if (!hasIndexedUsers) {
      users.reserve(module.operations.size());
      for (const GenericOperation &operation : module.operations)
        if (std::find(operation.operands.begin(), operation.operands.end(),
                      from) != operation.operands.end() ||
            std::find(operation.dpsInputs.begin(), operation.dpsInputs.end(),
                      from) != operation.dpsInputs.end() ||
            std::find(operation.dpsInits.begin(), operation.dpsInits.end(),
                      from) != operation.dpsInits.end())
          users.push_back(operation.id);
    }
    std::sort(users.begin(), users.end());
    users.erase(std::unique(users.begin(), users.end()), users.end());
    for (int operationId : users) {
      if (operationId < 0 ||
          static_cast<size_t>(operationId) >= module.operations.size())
        continue;
      GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      bool changed = false;
      for (size_t index = 0; index < operation.operands.size(); ++index) {
        if (operation.operands[index] != from)
          continue;
        replaceOperand(operationId, index, to);
        changed = true;
      }
      for (int &operand : operation.dpsInputs)
        if (operand == from) {
          operand = to;
          changed = true;
        }
      for (int &operand : operation.dpsInits)
        if (operand == from) {
          operand = to;
          changed = true;
        }
      if (changed)
        markDirty(operationId);
    }
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
  std::set<int> detachedOperations;
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
  const size_t operationCount = module.operations.size();
  const size_t regionCount = module.regions.size();
  const size_t blockCount = module.blocks.size();
  std::vector<uint8_t> reachableOperations(operationCount, uint8_t{0});
  std::vector<uint8_t> reachableRegions(regionCount, uint8_t{0});
  std::vector<uint8_t> reachableBlocks(blockCount, uint8_t{0});
  std::vector<int> operationOrder;
  std::vector<int> regionOrder;
  std::vector<int> blockOrder;
  std::vector<int> valueOrder;
  operationOrder.reserve(operationCount);
  regionOrder.reserve(regionCount);
  blockOrder.reserve(blockCount);

  std::vector<int> expectedOperationParent(operationCount, -2);
  std::vector<int> expectedOperationRegion(operationCount, -2);
  std::vector<int> expectedOperationBlock(operationCount, -2);
  std::vector<int> expectedOperationOrdinal(operationCount, -2);
  std::vector<int> expectedRegionParent(regionCount, -2);
  std::vector<int> expectedBlockRegion(blockCount, -2);

  std::function<void(int, int, int, int, int)> visitOperation;
  std::function<void(int, int)> visitRegion;
  visitRegion = [&](int regionId, int parent) {
    const size_t regionIndex = static_cast<size_t>(regionId);
    if (reachableRegions.at(regionIndex) != 0)
      return;
    reachableRegions[regionIndex] = 1;
    expectedRegionParent[regionIndex] = parent;
    regionOrder.push_back(regionId);
    const GenericRegion &region = module.regions.at(regionIndex);
    for (int blockId : region.blocks) {
      const size_t blockIndex = static_cast<size_t>(blockId);
      if (reachableBlocks.at(blockIndex) != 0)
        continue;
      reachableBlocks[blockIndex] = 1;
      expectedBlockRegion[blockIndex] = regionId;
      blockOrder.push_back(blockId);
      const GenericBlock &block = module.blocks.at(blockIndex);
      valueOrder.insert(valueOrder.end(), block.arguments.begin(),
                        block.arguments.end());
      for (size_t ordinal = 0; ordinal < block.operations.size(); ++ordinal)
        visitOperation(block.operations[ordinal], parent, regionId, blockId,
                       static_cast<int>(ordinal));
    }
  };
  visitOperation = [&](int operationId, int parent, int region, int block,
                       int ordinal) {
    const size_t operationIndex = static_cast<size_t>(operationId);
    if (reachableOperations.at(operationIndex) != 0)
      return;
    reachableOperations[operationIndex] = 1;
    expectedOperationParent[operationIndex] = parent;
    expectedOperationRegion[operationIndex] = region;
    expectedOperationBlock[operationIndex] = block;
    expectedOperationOrdinal[operationIndex] = ordinal;
    operationOrder.push_back(operationId);
    const GenericOperation &operation = module.operations.at(operationIndex);
    valueOrder.insert(valueOrder.end(), operation.results.begin(),
                      operation.results.end());
    for (int regionId : operation.regions)
      visitRegion(regionId, operationId);
  };
  visitOperation(0, -1, -1, -1, 0);

  int maximumValue = -1;
  auto observeValue = [&](int value) {
    if (value >= 0)
      maximumValue = std::max(maximumValue, value);
  };
  for (const GenericBlock &block : module.blocks)
    for (int argument : block.arguments)
      observeValue(argument);
  std::vector<int> valueDefinitions;
  for (const GenericOperation &operation : module.operations) {
    for (int result : operation.results)
      observeValue(result);
    for (int operand : operation.operands)
      observeValue(operand);
    for (int operand : operation.dpsInputs)
      observeValue(operand);
    for (int operand : operation.dpsInits)
      observeValue(operand);
  }
  const size_t valueCapacity =
      maximumValue < 0 ? 0 : static_cast<size_t>(maximumValue) + 1;
  std::vector<int> operationIds(operationCount, -1);
  std::vector<int> regionIds(regionCount, -1);
  std::vector<int> blockIds(blockCount, -1);
  std::vector<int> valueIds(valueCapacity, -1);
  valueDefinitions.assign(valueCapacity, -1);
  for (const GenericOperation &operation : module.operations)
    for (int result : operation.results)
      if (result >= 0)
        valueDefinitions[static_cast<size_t>(result)] = operation.id;
  for (size_t index = 0; index < operationOrder.size(); ++index)
    operationIds[static_cast<size_t>(operationOrder[index])] =
        static_cast<int>(index);
  for (size_t index = 0; index < regionOrder.size(); ++index)
    regionIds[static_cast<size_t>(regionOrder[index])] =
        static_cast<int>(index);
  for (size_t index = 0; index < blockOrder.size(); ++index)
    blockIds[static_cast<size_t>(blockOrder[index])] = static_cast<int>(index);
  for (size_t index = 0; index < valueOrder.size(); ++index) {
    const int value = valueOrder[index];
    if (value >= 0)
      valueIds[static_cast<size_t>(value)] = static_cast<int>(index);
  }

  auto mappedValue = [&](int value) {
    return value >= 0 && static_cast<size_t>(value) < valueIds.size()
               ? valueIds[static_cast<size_t>(value)]
               : -1;
  };
  for (int operationId : operationOrder) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    auto validateValue = [&](int value, const char *kind) {
      if (mappedValue(value) >= 0)
        return;
      const int definition =
          value >= 0 && static_cast<size_t>(value) < valueDefinitions.size()
              ? valueDefinitions[static_cast<size_t>(value)]
              : -1;
      std::string description = "unknown";
      if (definition >= 0) {
        const GenericOperation &defining =
            module.operations.at(static_cast<size_t>(definition));
        description = defining.name + "#" + std::to_string(definition) +
                      (reachableOperations[static_cast<size_t>(definition)] != 0
                           ? "(reachable)"
                           : "(erased)");
      }
      throw std::runtime_error(
          "CompactGenericModule: " + operation.name + " " + kind +
          " references unreachable SSA value " + std::to_string(value) +
          " defined by " + description);
    };
    for (int operand : operation.operands)
      validateValue(operand, "operand");
    for (int operand : operation.dpsInputs)
      validateValue(operand, "DPS input");
    for (int operand : operation.dpsInits)
      validateValue(operand, "DPS init");
  }

  bool identity = operationOrder.size() == operationCount &&
                  regionOrder.size() == regionCount &&
                  blockOrder.size() == blockCount;
  for (size_t index = 0; identity && index < operationCount; ++index) {
    const GenericOperation &operation = module.operations[index];
    identity = operation.id == static_cast<int>(index) &&
               operationIds[index] == static_cast<int>(index) &&
               operation.parentId == expectedOperationParent[index] &&
               operation.regionId == expectedOperationRegion[index] &&
               operation.blockId == expectedOperationBlock[index] &&
               operation.ordinal == expectedOperationOrdinal[index];
  }
  for (size_t index = 0; identity && index < regionCount; ++index)
    identity = module.regions[index].id == static_cast<int>(index) &&
               regionIds[index] == static_cast<int>(index) &&
               module.regions[index].parentOperation ==
                   expectedRegionParent[index];
  for (size_t index = 0; identity && index < blockCount; ++index)
    identity = module.blocks[index].id == static_cast<int>(index) &&
               blockIds[index] == static_cast<int>(index) &&
               module.blocks[index].regionId == expectedBlockRegion[index];
  for (size_t index = 0; identity && index < valueOrder.size(); ++index)
    identity = valueOrder[index] == static_cast<int>(index);
  if (identity)
    return module;

  GenericModule compact;
  auto growthCapacity = [](size_t size) {
    size_t capacity = 1;
    while (capacity < size)
      capacity *= 2;
    return capacity;
  };
  // Preserve the normal geometric headroom of the former push-built vectors.
  // Several rewrite passes append operations after compaction, so reserving
  // exactly size() would force an immediate reallocation on their first op.
  compact.operations.reserve(growthCapacity(operationOrder.size()));
  compact.regions.reserve(growthCapacity(regionOrder.size()));
  compact.blocks.reserve(growthCapacity(blockOrder.size()));
  std::function<int(int, int)> copyOperation;
  std::function<int(int, int)> copyRegion;
  copyRegion = [&](int oldRegion, int parent) {
    GenericRegion region =
        std::move(module.regions.at(static_cast<size_t>(oldRegion)));
    std::vector<int> oldBlocks = std::move(region.blocks);
    region.id = regionIds.at(static_cast<size_t>(oldRegion));
    region.parentOperation = parent;
    region.blocks.clear();
    compact.regions.push_back(std::move(region));
    GenericRegion &createdRegion = compact.regions.back();
    createdRegion.blocks.reserve(oldBlocks.size());
    for (int oldBlock : oldBlocks) {
      GenericBlock block =
          std::move(module.blocks.at(static_cast<size_t>(oldBlock)));
      std::vector<int> oldOperations = std::move(block.operations);
      block.id = blockIds.at(static_cast<size_t>(oldBlock));
      block.regionId = createdRegion.id;
      for (int &argument : block.arguments)
        argument = mappedValue(argument);
      block.operations.clear();
      compact.blocks.push_back(std::move(block));
      GenericBlock &createdBlock = compact.blocks.back();
      createdBlock.operations.reserve(oldOperations.size());
      createdRegion.blocks.push_back(createdBlock.id);
      for (int oldOperation : oldOperations)
        createdBlock.operations.push_back(
            copyOperation(oldOperation, createdBlock.id));
    }
    return createdRegion.id;
  };
  copyOperation = [&](int oldOperation, int block) {
    GenericOperation operation =
        std::move(module.operations.at(static_cast<size_t>(oldOperation)));
    std::vector<int> oldRegions = std::move(operation.regions);
    operation.id = operationIds.at(static_cast<size_t>(oldOperation));
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
    for (int &result : operation.results)
      result = mappedValue(result);
    compact.operations.push_back(std::move(operation));
    GenericOperation &createdOperation = compact.operations.back();
    createdOperation.regions.reserve(oldRegions.size());
    for (int oldRegion : oldRegions)
      createdOperation.regions.push_back(
          copyRegion(oldRegion, createdOperation.id));
    return createdOperation.id;
  };
  copyOperation(0, -1);

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
        const int mapped = mappedValue(oldValue);
        if (mapped < 0) {
          position = close + 1;
          continue;
        }
        const std::string replacement =
            "v(" + std::to_string(mapped) + ")";
        text.replace(position, close - position + 1, replacement);
        position += replacement.size();
      }
    };
    remapSemanticValueReferences(operation.properties);
    remapSemanticValueReferences(operation.attributes);
    auto remapValue = [&](int &value, const char *) {
      value = mappedValue(value);
    };
    for (int &operand : operation.operands)
      remapValue(operand, "operand");
    for (int &operand : operation.dpsInputs)
      remapValue(operand, "DPS input");
    for (int &operand : operation.dpsInits)
      remapValue(operand, "DPS init");
    for (int &successor : operation.successors)
      if (successor >= 0 && static_cast<size_t>(successor) < blockIds.size() &&
          blockIds[static_cast<size_t>(successor)] >= 0)
        successor = blockIds[static_cast<size_t>(successor)];
  }
  for (GenericBlock &block : compact.blocks)
    for (size_t index = 0; index < block.operations.size(); ++index)
      compact.operations.at(static_cast<size_t>(block.operations[index])).ordinal =
          static_cast<int>(index);
  return compact;
}

} // namespace cvub

#endif
