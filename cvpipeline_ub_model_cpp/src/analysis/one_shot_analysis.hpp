#ifndef CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_ANALYSIS_HPP
#define CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_ANALYSIS_HPP

#include "../passes/one_shot_bufferize.hpp"


namespace cvub {

enum class OneShotBufferizationDecision { InPlace, OutOfPlace };

struct OneShotOpOperandDecision {
  int operationId = -1;
  int operandNumber = -1;
  OneShotBufferizationDecision decision =
      OneShotBufferizationDecision::InPlace;
};

struct OneShotBufferizationResult {
  std::vector<OneShotOpOperandDecision> decisions;
  std::vector<BufferAllocation> allocations;
  PreBufferizationCSEState preBufferizationCSE;
};

inline std::vector<std::string> ParseStringArray(std::string value) {
  value = trim(std::move(value));
  if (value.size() < 2 || value.front() != '[' || value.back() != ']')
    return {};
  std::vector<std::string> result;
  for (std::string item : splitTopLevel(value.substr(1, value.size() - 2))) {
    item = trim(std::move(item));
    if (item.size() >= 2 && item.front() == '"' && item.back() == '"')
      item = item.substr(1, item.size() - 2);
    result.push_back(std::move(item));
  }
  return result;
}

inline bool IsTensorType(const std::string &type) {
  return startsWith(type, "tensor<") || startsWith(type, "tensor<*");
}

inline std::vector<OneShotOpOperandDecision>
CollectOneShotAnalysisOracle(const GenericModule &module) {
  std::vector<OneShotOpOperandDecision> result;
  for (const GenericOperation &operation : module.operations) {
    const std::vector<std::string> decisions = ParseStringArray(
        FindDictionaryValue(operation.attributes, "__inplace_operands_attr__"));
    for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
      if (!IsTensorType(operation.operandTypes[index]))
        continue;
      if (index >= decisions.size() ||
          (decisions[index] != "true" && decisions[index] != "false"))
        throw std::runtime_error("OneShotAnalysis oracle: missing decision for " +
                                 operation.name);
      result.push_back(
          {operation.id, static_cast<int>(index),
           decisions[index] == "true"
               ? OneShotBufferizationDecision::InPlace
               : OneShotBufferizationDecision::OutOfPlace});
    }
  }
  return result;
}

class OneShotAnalysisModel {
public:
  explicit OneShotAnalysisModel(const GenericModule &inputModule)
      : module(inputModule), definitions(DefiningOperations(inputModule)) {
    indexBlockArguments();
    indexUses();
  }

  std::vector<OneShotOpOperandDecision> Analyze() {
    initializeAliasSets();
    std::map<std::pair<int, int>, OneShotBufferizationDecision> decisions;
    for (const GenericOperation &operation : module.operations)
      if (operation.name == "scf.yield" || operation.name == "scf.condition")
        for (size_t index = 0; index < operation.operandTypes.size(); ++index)
          if (IsTensorType(operation.operandTypes[index])) {
            decisions[{operation.id, static_cast<int>(index)}] =
                OneShotBufferizationDecision::InPlace;
            inplaceDecisions[{operation.id, static_cast<int>(index)}] = true;
            if (operation.name == "scf.yield" && operation.parentId >= 0 &&
                module.operations.at(
                    static_cast<size_t>(operation.parentId)).name == "scf.if")
              bufferizeInPlace(operation, index);
          }

    for (auto iterator = module.operations.rbegin();
         iterator != module.operations.rend(); ++iterator) {
      const GenericOperation &operation = *iterator;
      for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
        if (!IsTensorType(operation.operandTypes[index]))
          continue;
        const auto key = std::pair<int, int>{operation.id,
                                             static_cast<int>(index)};
        if (decisions.count(key) != 0)
          continue;
        const bool outOfPlace =
            wouldCreateReadAfterWriteInterference(operation, index);
        decisions[key] = outOfPlace
                             ? OneShotBufferizationDecision::OutOfPlace
                             : OneShotBufferizationDecision::InPlace;
        inplaceDecisions[key] = !outOfPlace;
        if (!outOfPlace)
          bufferizeInPlace(operation, index);
      }
    }
    std::vector<OneShotOpOperandDecision> result;
    for (const GenericOperation &operation : module.operations)
      for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
        if (!IsTensorType(operation.operandTypes[index]))
          continue;
        result.push_back({operation.id, static_cast<int>(index),
                          decisions.at({operation.id,
                                        static_cast<int>(index)})});
      }
    return result;
  }

private:
  void initializeAliasSets() {
    for (const auto &[value, operation] : definitions) {
      (void)operation;
      aliasParents[value] = value;
    }
    for (const auto &[value, block] : blockArgumentOwners) {
      (void)block;
      aliasParents[value] = value;
    }
  }

  int findAlias(int value) {
    auto found = aliasParents.find(value);
    if (found == aliasParents.end())
      return aliasParents[value] = value;
    if (found->second != value)
      found->second = findAlias(found->second);
    return found->second;
  }

  void unionAliases(int lhs, int rhs) {
    int lhsRoot = findAlias(lhs);
    int rhsRoot = findAlias(rhs);
    if (lhsRoot != rhsRoot)
      aliasParents[rhsRoot] = lhsRoot;
  }

  std::vector<int> getAliasingValues(const GenericOperation &operation,
                                     size_t operand) const {
    std::vector<int> result;
    const std::vector<size_t> inits = DpsInitOperandIndices(
        operation.name, operation.operands.size(), operation.properties);
    auto init = std::find(inits.begin(), inits.end(), operand);
    if (init != inits.end()) {
      size_t resultNumber = static_cast<size_t>(init - inits.begin());
      if (resultNumber < operation.results.size())
        result.push_back(operation.results[resultNumber]);
      return result;
    }
    static const std::set<std::string> aliasingOps = {
        "tensor.cast", "tensor.collapse_shape", "tensor.expand_shape",
        "tensor.extract_slice", "hivm.hir.bitcast"};
    if (operand == 0 && aliasingOps.count(operation.name) != 0 &&
        !operation.results.empty())
      result.push_back(operation.results.front());
    if (operation.name == "arith.select" && operand > 0 &&
        !operation.results.empty())
      result.push_back(operation.results.front());
    if (operation.name == "scf.for" && operand >= 3 &&
        operand - 3 < operation.results.size())
      result.push_back(operation.results[operand - 3]);
    if (operation.name == "scf.yield" && operation.parentId >= 0) {
      const GenericOperation &parent =
          module.operations.at(static_cast<size_t>(operation.parentId));
      if (parent.name == "scf.if" && operand < parent.results.size())
        result.push_back(parent.results[operand]);
    }
    return result;
  }

  void bufferizeInPlace(const GenericOperation &operation, size_t operand) {
    if (operand >= operation.operands.size())
      return;
    for (int alias : getAliasingValues(operation, operand))
      unionAliases(operation.operands[operand], alias);
  }

  void indexBlockArguments() {
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        blockArgumentOwners[argument] = block.id;
  }

  void indexUses() {
    for (const GenericOperation &operation : module.operations)
      for (size_t index = 0; index < operation.operands.size(); ++index)
        uses[operation.operands[index]].insert(
            {operation.id, static_cast<int>(index)});
  }

  const GenericOperation *enclosingRepetitiveOp(
      const GenericOperation &operation) const {
    int parent = operation.parentId;
    while (parent >= 0) {
      const GenericOperation &owner =
          module.operations.at(static_cast<size_t>(parent));
      if (owner.name == "scf.for" || owner.name == "scf.while")
        return &owner;
      parent = owner.parentId;
    }
    return nullptr;
  }

  bool isProperAncestor(const GenericOperation &ancestor,
                        const GenericOperation &operation) const {
    int parent = operation.parentId;
    while (parent >= 0) {
      if (parent == ancestor.id)
        return true;
      parent = module.operations.at(static_cast<size_t>(parent)).parentId;
    }
    return false;
  }

  bool bufferizesToMemoryWrite(const GenericOperation &operation,
                               size_t operand) const {
    const std::vector<size_t> initIndices = DpsInitOperandIndices(
        operation.name, operation.operands.size(), operation.properties);
    if (std::find(initIndices.begin(), initIndices.end(), operand) !=
        initIndices.end())
      return true;
    if (operation.name == "scf.for")
      return operand >= 3 && IsTensorType(operation.operandTypes[operand]);
    if (operation.name == "scf.while")
      return IsTensorType(operation.operandTypes[operand]);
    return false;
  }

  bool bufferizesToMemoryRead(const GenericOperation &operation,
                              size_t operand) {
    if (operation.name == "scf.yield" ||
        operation.name == "scf.condition")
      return true;
    if (operation.name == "arith.select" || operation.name == "tensor.cast" ||
        operation.name == "tensor.expand_shape" ||
        operation.name == "tensor.extract_slice" ||
        operation.name == "hivm.hir.bitcast")
      return false;
    if (operation.name == "tensor.collapse_shape")
      return true;
    if (operation.name == "scf.for" && operand >= 3) {
      if (operation.regions.empty())
        return true;
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(operation.regions.front()));
      if (region.blocks.empty())
        return true;
      const GenericBlock &block =
          module.blocks.at(static_cast<size_t>(region.blocks.front()));
      const size_t argument = operand - 2;
      return argument >= block.arguments.size() ||
             isValueRead(block.arguments[argument]);
    }
    const std::vector<size_t> inits = DpsInitOperandIndices(
        operation.name, operation.operands.size(), operation.properties);
    const bool isInit =
        std::find(inits.begin(), inits.end(), operand) != inits.end();
    if (startsWith(operation.name, "hivm.hir.v"))
      return !isInit;
    if (IsDestinationStyleOp(operation.name) ||
        operation.name == "tensor.insert" ||
        operation.name == "tensor.insert_slice")
      return true;
    return true;
  }

  bool bufferizesToAliasOnly(const GenericOperation &operation,
                              size_t operand) {
    return !bufferizesToMemoryRead(operation, operand) &&
           !bufferizesToMemoryWrite(operation, operand) &&
           !getAliasingValues(operation, operand).empty();
  }

  bool hasDuplicateTiedInit(const GenericOperation &operation,
                            size_t operand) const {
    if (!bufferizesToMemoryWrite(operation, operand))
      return false;
    for (size_t init : DpsInitOperandIndices(
             operation.name, operation.operands.size(), operation.properties)) {
      if (init >= operation.operands.size() || init == operand)
        continue;
      if (operation.operands[init] == operation.operands[operand] &&
          init < operand)
        return true;
    }
    return false;
  }

  using OpOperand = std::pair<int, int>;

  std::vector<OpOperand> getAliasingOpOperands(int value) const {
    std::vector<OpOperand> result;
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return result;
    const GenericOperation &operation = *definition->second;
    if (operation.name == "scf.if") {
      const auto resultIt =
          std::find(operation.results.begin(), operation.results.end(), value);
      if (resultIt == operation.results.end())
        return result;
      const size_t resultNumber =
          static_cast<size_t>(resultIt - operation.results.begin());
      for (int regionId : operation.regions) {
        const GenericRegion &region =
            module.regions.at(static_cast<size_t>(regionId));
        for (int blockId : region.blocks) {
          const GenericBlock &block =
              module.blocks.at(static_cast<size_t>(blockId));
          if (block.operations.empty())
            continue;
          const GenericOperation &terminator = module.operations.at(
              static_cast<size_t>(block.operations.back()));
          if (terminator.name == "scf.yield" &&
              resultNumber < terminator.operands.size())
            result.push_back(
                {terminator.id, static_cast<int>(resultNumber)});
        }
      }
      return result;
    }
    for (size_t index = 0; index < operation.operands.size(); ++index)
      for (int alias : getAliasingValues(operation, index))
        if (alias == value)
          result.push_back({operation.id, static_cast<int>(index)});
    return result;
  }

  bool resultBufferizesToMemoryWrite(int value,
                                      std::set<int> &active) {
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return true;
    const GenericOperation &operation = *definition->second;
    if (operation.name == "tensor.empty" ||
        operation.name == "bufferization.alloc_tensor")
      return false;
    if (!active.insert(value).second)
      return false;
    const std::vector<OpOperand> aliases = getAliasingOpOperands(value);
    if (aliases.empty()) {
      active.erase(value);
      return true;
    }
    for (const OpOperand &alias : aliases) {
      const GenericOperation &owner =
          module.operations.at(static_cast<size_t>(alias.first));
      if (bufferizesToMemoryWrite(owner, static_cast<size_t>(alias.second))) {
        active.erase(value);
        return true;
      }
    }
    for (const OpOperand &alias : aliases)
      for (int candidate : findValueInReverseUseDefChain(
               module.operations.at(static_cast<size_t>(alias.first))
                   .operands.at(static_cast<size_t>(alias.second)),
               [&](int nested) {
                 auto nestedDefinition = definitions.find(nested);
                 return nestedDefinition != definitions.end() &&
                        isProperAncestor(operation, *nestedDefinition->second) &&
                        resultBufferizesToMemoryWrite(nested, active);
               }, false)) {
        (void)candidate;
        active.erase(value);
        return true;
      }
    active.erase(value);
    return false;
  }

  bool bufferizesToMemoryWrite(int value) {
    if (definitions.count(value) == 0)
      return true;
    std::set<int> active;
    return resultBufferizesToMemoryWrite(value, active);
  }

  template <typename Condition>
  std::vector<int> findValueInReverseUseDefChain(int value,
                                                  Condition condition,
                                                  bool alwaysIncludeLeaves =
                                                      false) {
    std::set<int> visited;
    std::vector<int> result;
    std::vector<int> worklist{value};
    while (!worklist.empty()) {
      const int current = worklist.back();
      worklist.pop_back();
      if (!visited.insert(current).second)
        continue;
      if (condition(current)) {
        result.push_back(current);
        continue;
      }
      const std::vector<OpOperand> aliases = getAliasingOpOperands(current);
      if (aliases.empty()) {
        if (alwaysIncludeLeaves)
          result.push_back(current);
        continue;
      }
      for (const OpOperand &alias : aliases)
        worklist.push_back(
            module.operations.at(static_cast<size_t>(alias.first))
                .operands.at(static_cast<size_t>(alias.second)));
    }
    return result;
  }

  std::vector<int> findDefinitions(int value) {
    return findValueInReverseUseDefChain(
        value, [&](int candidate) {
          return bufferizesToMemoryWrite(candidate);
        });
  }

  bool isValueRead(int value) {
    std::set<OpOperand> visited;
    std::vector<OpOperand> worklist;
    auto found = uses.find(value);
    if (found != uses.end())
      worklist.assign(found->second.begin(), found->second.end());
    while (!worklist.empty()) {
      const OpOperand use = worklist.back();
      worklist.pop_back();
      if (!visited.insert(use).second)
        continue;
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(use.first));
      const size_t operand = static_cast<size_t>(use.second);
      if (bufferizesToAliasOnly(operation, operand))
        for (int alias : getAliasingValues(operation, operand)) {
          auto aliasUses = uses.find(alias);
          if (aliasUses != uses.end())
            worklist.insert(worklist.end(), aliasUses->second.begin(),
                            aliasUses->second.end());
        }
      if (bufferizesToMemoryRead(operation, operand))
        return true;
    }
    return false;
  }

  void getAliasingInplaceWrites(std::set<OpOperand> &result, int value) {
    const int root = findAlias(value);
    for (auto &[alias, parent] : aliasParents) {
      (void)parent;
      if (findAlias(alias) != root)
        continue;
      auto aliasUses = uses.find(alias);
      if (aliasUses == uses.end())
        continue;
      for (const OpOperand &use : aliasUses->second) {
        auto decision = inplaceDecisions.find(use);
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(use.first));
        if (decision != inplaceDecisions.end() && decision->second &&
            bufferizesToMemoryWrite(operation,
                                     static_cast<size_t>(use.second)))
          result.insert(use);
      }
    }
  }

  void getAliasingReads(std::set<OpOperand> &result, int value) {
    const int root = findAlias(value);
    for (auto &[alias, parent] : aliasParents) {
      (void)parent;
      if (findAlias(alias) != root)
        continue;
      auto aliasUses = uses.find(alias);
      if (aliasUses == uses.end())
        continue;
      for (const OpOperand &use : aliasUses->second) {
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(use.first));
        const size_t operand = static_cast<size_t>(use.second);
        if (bufferizesToMemoryRead(operation, operand)) {
          result.insert(use);
          continue;
        }
        if (!bufferizesToMemoryWrite(operation, operand))
          for (int valueAlias : getAliasingValues(operation, operand))
            if (isValueRead(valueAlias)) {
              result.insert(use);
              break;
            }
      }
    }
  }

  bool happensBefore(const GenericOperation &lhs,
                     const GenericOperation &rhs) const {
    if (isProperAncestor(lhs, rhs))
      return false;
    return lhs.id < rhs.id;
  }

  std::map<int, int>
  enclosingIfRegions(const GenericOperation &operation) const {
    std::map<int, int> result;
    const GenericOperation *current = &operation;
    while (current->regionId >= 0) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(current->regionId));
      if (region.parentOperation < 0)
        break;
      const GenericOperation &owner = module.operations.at(
          static_cast<size_t>(region.parentOperation));
      if (owner.name == "scf.if")
        result[owner.id] = region.id;
      current = &owner;
    }
    return result;
  }

  bool insideMutuallyExclusiveRegions(
      const GenericOperation &lhs,
      const GenericOperation &rhs) const {
    const std::map<int, int> lhsRegions = enclosingIfRegions(lhs);
    const std::map<int, int> rhsRegions = enclosingIfRegions(rhs);
    for (const auto &[owner, region] : lhsRegions) {
      auto found = rhsRegions.find(owner);
      if (found != rhsRegions.end() && found->second != region)
        return true;
    }
    return false;
  }

  std::string subsetSignature(const GenericOperation &operation) const {
    std::ostringstream signature;
    for (const char *key : {"static_offsets", "static_sizes",
                            "static_strides"})
      signature << key << '='
                << FindDictionaryValue(operation.properties, key) << ';';
    const std::vector<size_t> segments =
        OperandSegmentSizes(operation.properties);
    if (!segments.empty()) {
      const size_t firstSubsetSegment =
          operation.name == "tensor.insert_slice" ? 2 : 1;
      size_t begin = 0;
      for (size_t index = 0; index < firstSubsetSegment; ++index)
        begin += segments.at(index);
      for (size_t index = firstSubsetSegment; index < segments.size(); ++index)
        for (size_t count = 0; count < segments[index]; ++count)
          signature << operation.operands.at(begin++) << ',';
    }
    return signature.str();
  }

  bool matchesInsertDestination(int value,
                                const GenericOperation &insertSlice) {
    const std::string signature = subsetSignature(insertSlice);
    const int destination = insertSlice.operands.at(1);
    return !findValueInReverseUseDefChain(
                value,
                [&](int candidate) {
                  auto definition = definitions.find(candidate);
                  if (definition == definitions.end() ||
                      definition->second->name != "tensor.extract_slice" ||
                      definition->second->operands.empty())
                    return false;
                  return findAlias(definition->second->operands.front()) ==
                             findAlias(destination) &&
                         subsetSignature(*definition->second) == signature;
                })
                .empty();
  }

  bool areNonConflictingSubsets(const OpOperand &read,
                                const OpOperand &write) {
    const GenericOperation &readingOperation =
        module.operations.at(static_cast<size_t>(read.first));
    if (readingOperation.name == "tensor.insert_slice" && read.second == 1) {
      const GenericOperation &writingOperation =
          module.operations.at(static_cast<size_t>(write.first));
      return matchesInsertDestination(
          writingOperation.operands.at(static_cast<size_t>(write.second)),
          readingOperation);
    }
    return false;
  }

  const GenericOperation *getEnclosingRepetitiveRegion(
      const GenericOperation &operation) const {
    return enclosingRepetitiveOp(operation);
  }

  bool canUseOpDominance(const OpOperand &read, const OpOperand &write,
                         const std::vector<int> &valueDefinitions) const {
    const GenericOperation &readOperation =
        module.operations.at(static_cast<size_t>(read.first));
    const GenericOperation &writeOperation =
        module.operations.at(static_cast<size_t>(write.first));
    for (int value : valueDefinitions) {
      const GenericOperation *readRegion =
          getEnclosingRepetitiveRegion(readOperation);
      const GenericOperation *definitionRegion = nullptr;
      auto definition = definitions.find(value);
      if (definition != definitions.end())
        definitionRegion = getEnclosingRepetitiveRegion(*definition->second);
      else {
        auto blockOwner = blockArgumentOwners.find(value);
        if (blockOwner != blockArgumentOwners.end()) {
          const GenericBlock &block =
              module.blocks.at(static_cast<size_t>(blockOwner->second));
          const GenericRegion &region =
              module.regions.at(static_cast<size_t>(block.regionId));
          const GenericOperation &owner = module.operations.at(
              static_cast<size_t>(region.parentOperation));
          if (owner.name == "scf.for" || owner.name == "scf.while")
            definitionRegion = &owner;
          else
            definitionRegion = getEnclosingRepetitiveRegion(owner);
        }
      }
      if (readRegion == definitionRegion)
        continue;
      const GenericOperation *candidate = readRegion;
      while (candidate && candidate != definitionRegion) {
        const GenericOperation *next = getEnclosingRepetitiveRegion(*candidate);
        if (next == definitionRegion)
          break;
        candidate = next;
      }
      if (candidate && isProperAncestor(*candidate, writeOperation))
        return false;
    }
    return true;
  }

  bool hasReadAfterWriteInterference(const std::set<OpOperand> &reads,
                                     const std::set<OpOperand> &writes) {
    for (const OpOperand &read : reads) {
      const GenericOperation &readingOperation =
          module.operations.at(static_cast<size_t>(read.first));
      const int readValue = readingOperation.operands.at(
          static_cast<size_t>(read.second));
      const std::vector<int> valueDefinitions = findDefinitions(readValue);
      if (valueDefinitions.empty())
        continue;
      for (const OpOperand &write : writes) {
        const GenericOperation &writingOperation =
            module.operations.at(static_cast<size_t>(write.first));
        const bool useDominance =
            canUseOpDominance(read, write, valueDefinitions);
        if (useDominance && happensBefore(readingOperation, writingOperation))
          continue;
        if (useDominance && read == write)
          continue;
        if (useDominance && insideMutuallyExclusiveRegions(
                                readingOperation, writingOperation))
          continue;
        if (useDominance && readingOperation.id == writingOperation.id &&
            startsWith(readingOperation.name, "hivm.hir.v") &&
            readingOperation.operands.at(static_cast<size_t>(read.second)) ==
                writingOperation.operands.at(
                    static_cast<size_t>(write.second)))
          continue;
        if (areNonConflictingSubsets(read, write))
          continue;
        for (int definitionValue : valueDefinitions) {
          auto definition = definitions.find(definitionValue);
          if (definition != definitions.end()) {
            if (happensBefore(writingOperation, *definition->second) ||
                isProperAncestor(*definition->second, writingOperation))
              continue;
          } else {
            auto owner = blockArgumentOwners.find(definitionValue);
            if (owner != blockArgumentOwners.end()) {
              const GenericBlock &block =
                  module.blocks.at(static_cast<size_t>(owner->second));
              bool writeInsideBlock = writingOperation.blockId == block.id;
              for (int operationId : block.operations)
                if (operationId == writingOperation.id ||
                    isProperAncestor(module.operations.at(
                                         static_cast<size_t>(operationId)),
                                     writingOperation))
                  writeInsideBlock = true;
              if (!writeInsideBlock)
                continue;
            }
          }
          const std::vector<int> aliases = getAliasingValues(
              writingOperation, static_cast<size_t>(write.second));
          if (aliases.size() == 1 && aliases.front() == definitionValue)
            continue;
          return true;
        }
      }
    }
    return false;
  }

  bool wouldCreateReadAfterWriteInterference(
      const GenericOperation &operation, size_t operand) {
    if (hasDuplicateTiedInit(operation, operand))
      return true;
    std::set<OpOperand> reads;
    std::set<OpOperand> writes;
    getAliasingReads(reads, operation.operands.at(operand));
    getAliasingInplaceWrites(writes, operation.operands.at(operand));
    for (int alias : getAliasingValues(operation, operand)) {
      getAliasingReads(reads, alias);
      getAliasingInplaceWrites(writes, alias);
    }
    if (bufferizesToMemoryWrite(operation, operand))
      writes.insert({operation.id, static_cast<int>(operand)});
    return hasReadAfterWriteInterference(reads, writes);
  }

  const GenericModule &module;
  std::map<int, const GenericOperation *> definitions;
  std::map<int, int> blockArgumentOwners;
  std::map<int, int> aliasParents;
  std::map<int, std::set<OpOperand>> uses;
  std::map<OpOperand, bool> inplaceDecisions;
};

inline std::vector<OneShotOpOperandDecision>
ModelOneShotAnalysis(const GenericModule &module) {
  return OneShotAnalysisModel(module).Analyze();
}

inline size_t CountDynamicTensorExtents(const std::string &type) {
  if (!startsWith(type, "tensor<"))
    return 0;
  const size_t elementType = type.rfind('x');
  const std::string shape = type.substr(7, elementType - 7);
  return static_cast<size_t>(std::count(shape.begin(), shape.end(), '?'));
}

inline std::vector<BufferAllocation>
ModelOneShotBufferizeAllocationsExact(
    const GenericModule &module,
    const std::vector<OneShotOpOperandDecision> &decisions,
    const PreBufferizationCSEState &preBufferizationCSE =
        PreBufferizationCSEState{}) {
  struct OrderedAllocation {
    int operationKey = -1;
    int orderKey = 0;
    size_t sequence = 0;
    BufferAllocation allocation;
  };
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  const std::map<int, size_t> useCounts = ValueUseCounts(module);
  std::map<int, std::pair<int, size_t>> soleUses;
  for (const GenericOperation &operation : module.operations) {
    if (preBufferizationCSE.erasedOperations.count(operation.id) != 0)
      continue;
    for (size_t operand = 0; operand < operation.operands.size(); ++operand)
      if (useCounts.at(operation.operands[operand]) == 1)
        soleUses[operation.operands[operand]] = {operation.id, operand};
  }
  std::map<std::pair<int, int>, OneShotBufferizationDecision>
      decisionByOperand;
  for (const OneShotOpOperandDecision &decision : decisions)
    decisionByOperand[{decision.operationId, decision.operandNumber}] =
        decision.decision;

  std::vector<OrderedAllocation> ordered;
  size_t sequence = 0;
  auto append = [&](int operationKey, int orderKey,
                    BufferAllocation allocation) {
    ordered.push_back({operationKey, orderKey, sequence++,
                       std::move(allocation)});
  };
  for (const GenericOperation &operation : module.operations) {
    if (preBufferizationCSE.erasedOperations.count(operation.id) != 0)
      continue;
    for (size_t operand = 0; operand < operation.operandTypes.size(); ++operand) {
      auto decision = decisionByOperand.find(
          {operation.id, static_cast<int>(operand)});
      if (decision == decisionByOperand.end() ||
          decision->second != OneShotBufferizationDecision::OutOfPlace)
        continue;
      auto definition = definitions.find(operation.operands[operand]);
      const auto uses = useCounts.find(operation.operands[operand]);
      // Replacing the sole use of an empty destination changes allocation
      // ownership but does not add another live allocation.
      if (definition != definitions.end() &&
          definition->second->name == "tensor.empty" &&
          uses != useCounts.end() && uses->second == 1)
        continue;
      std::string tensorType = operation.operandTypes[operand];
      if (operation.name == "tensor.extract_slice" &&
          !operation.resultTypes.empty())
        tensorType = operation.resultTypes.front();
      const std::string allocationSource =
          operation.name == "tensor.extract_slice" &&
                  !operation.results.empty()
              ? "result:" + std::to_string(operation.id) + ":0"
              : "operand:" + std::to_string(operation.id) + ":" +
                    std::to_string(operand);
      const std::vector<int> dynamicExtentValues(
          CountDynamicTensorExtents(tensorType), operation.operands[operand]);
      append(operation.id, static_cast<int>(operand),
             {ConvertTensorToMemRefType(tensorType), "64 : i64",
              CountDynamicTensorExtents(tensorType), allocationSource,
              operation.id,
              dynamicExtentValues});
    }
    if (operation.name == "memref.alloc") {
      for (size_t index = 0; index < operation.resultTypes.size(); ++index) {
        const std::string &type = operation.resultTypes[index];
        if (IsMemRefType(type))
          append(operation.id, static_cast<int>(index),
                 {type, FindDictionaryValue(operation.attributes, "alignment"),
                  operation.operands.size(),
                  "result:" + std::to_string(operation.id) + ":" +
                      std::to_string(index),
                  operation.id,
                  operation.operands});
      }
    } else if (operation.name == "tensor.empty" ||
               operation.name == "tensor.from_elements") {
      for (size_t index = 0; index < operation.resultTypes.size(); ++index) {
        if (index < operation.results.size() &&
            preBufferizationCSE.elidedTensorEmptyResults.count(
                operation.results[index]) != 0)
          continue;
        const std::string &type = operation.resultTypes[index];
        if (!IsTensorType(type))
          continue;
        append(operation.id, static_cast<int>(index),
               {ConvertTensorToMemRefType(type), "64 : i64",
                CountDynamicTensorExtents(type),
                "result:" + std::to_string(operation.id) + ":" +
                    std::to_string(index),
                operation.id,
                operation.operands});
      }
    }
  }
  std::stable_sort(ordered.begin(), ordered.end(),
                   [](const OrderedAllocation &lhs,
                      const OrderedAllocation &rhs) {
                     return std::tie(lhs.operationKey, lhs.orderKey,
                                     lhs.sequence) <
                            std::tie(rhs.operationKey, rhs.orderKey,
                                     rhs.sequence);
                   });
  std::vector<BufferAllocation> result;
  result.reserve(ordered.size());
  for (OrderedAllocation &allocation : ordered)
    result.push_back(std::move(allocation.allocation));
  return result;
}

inline std::vector<BufferAllocation>
ModelOneShotBufferizeAllocationsExact(const GenericModule &module) {
  return ModelOneShotBufferizeAllocationsExact(
      module, ModelOneShotAnalysis(module), ModelPreBufferizationCSE(module));
}

inline OneShotBufferizationResult
RunOneShotBufferize(const GenericModule &module) {
  OneShotBufferizationResult result;
  result.decisions = ModelOneShotAnalysis(module);
  result.allocations = ModelOneShotBufferizeAllocationsExact(
      module, result.decisions, result.preBufferizationCSE);
  return result;
}

inline std::string SerializeOneShotAnalysis(
    const std::vector<OneShotOpOperandDecision> &decisions) {
  std::ostringstream output;
  output << "ONE_SHOT_ANALYSIS\t1\n";
  for (const OneShotOpOperandDecision &decision : decisions)
    output << "OPERAND\t" << decision.operationId << '\t'
           << decision.operandNumber << '\t'
           << (decision.decision == OneShotBufferizationDecision::InPlace
                   ? "in_place"
                   : "out_of_place")
           << '\n';
  return output.str();
}

} // namespace cvub

#endif
