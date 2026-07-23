#ifndef CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_ANALYSIS_HPP
#define CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_ANALYSIS_HPP

#include "../passes/one_shot_bufferize.hpp"
#include "../ir/generic_analysis.hpp"


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
  // Built once at the immutable pre-bufferization boundary and moved through
  // BufferizedSemanticIR into every later planning projection.
  GenericModuleAnalysisIndexes analysis;
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
      : module(inputModule) {
    indexValuesAndUses();
  }

  std::vector<OneShotOpOperandDecision> Analyze() {
    initializeAliasSets();
    for (const GenericOperation &operation : module.operations)
      if (operation.name == "scf.yield" || operation.name == "scf.condition")
        for (size_t index = 0; index < operation.operandTypes.size(); ++index)
          if (IsTensorType(operation.operandTypes[index])) {
            setDecision(operation.id, index,
                        OneShotBufferizationDecision::InPlace);
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
        if (hasDecision(operation.id, index))
          continue;
        const bool outOfPlace =
            wouldCreateReadAfterWriteInterference(operation, index);
        setDecision(operation.id, index,
                    outOfPlace ? OneShotBufferizationDecision::OutOfPlace
                               : OneShotBufferizationDecision::InPlace);
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
                          decision(operation.id, index)});
      }
    return result;
  }

private:
  using OpOperand = std::pair<int, int>;

  const std::vector<size_t> &
  dpsInitOperandIndices(const GenericOperation &operation) const {
    return dpsInitOperands.at(static_cast<size_t>(operation.id));
  }

  void initializeAliasSets() {
    aliasParents.assign(definitions.size(), -1);
    aliasMembers.clear();
    aliasMembers.resize(definitions.size());
    for (size_t value = 0; value < definitions.size(); ++value)
      if (definitions[value] || blockArgumentOwners[value] >= 0) {
        aliasParents[value] = static_cast<int>(value);
        aliasMembers[value].push_back(static_cast<int>(value));
      }
  }

  int findAlias(int value) {
    if (value < 0)
      return value;
    const size_t ordinal = static_cast<size_t>(value);
    if (ordinal >= aliasParents.size()) {
      aliasParents.resize(ordinal + 1, -1);
      aliasMembers.resize(ordinal + 1);
    }
    if (aliasParents[ordinal] < 0) {
      aliasParents[ordinal] = value;
      aliasMembers[ordinal].push_back(value);
      return value;
    }
    if (aliasParents[ordinal] != value)
      aliasParents[ordinal] = findAlias(aliasParents[ordinal]);
    return aliasParents[ordinal];
  }

  void unionAliases(int lhs, int rhs) {
    int lhsRoot = findAlias(lhs);
    int rhsRoot = findAlias(rhs);
    if (lhsRoot != rhsRoot) {
      aliasParents[static_cast<size_t>(rhsRoot)] = lhsRoot;
      std::vector<int> &lhsMembers =
          aliasMembers[static_cast<size_t>(lhsRoot)];
      std::vector<int> &rhsMembers =
          aliasMembers[static_cast<size_t>(rhsRoot)];
      lhsMembers.insert(lhsMembers.end(), rhsMembers.begin(), rhsMembers.end());
      rhsMembers.clear();
    }
  }

  std::vector<int> getAliasingValues(const GenericOperation &operation,
                                     size_t operand) const {
    std::vector<int> result;
    const std::vector<size_t> &inits = dpsInitOperandIndices(operation);
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

  void indexValuesAndUses() {
    int maximumValue = -1;
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        maximumValue = std::max(maximumValue, argument);
    for (const GenericOperation &operation : module.operations) {
      for (int result : operation.results)
        maximumValue = std::max(maximumValue, result);
      for (int operand : operation.operands)
        maximumValue = std::max(maximumValue, operand);
    }
    const size_t valueCount = maximumValue < 0
                                  ? 0
                                  : static_cast<size_t>(maximumValue) + 1;
    definitions.assign(valueCount, nullptr);
    blockArgumentOwners.assign(valueCount, -1);
    uses.resize(valueCount);
    operandDecisions.resize(module.operations.size());
    dpsInitOperands.resize(module.operations.size());
    aliasingOperandCache.resize(valueCount);
    aliasingOperandReady.assign(valueCount, false);
    memoryWriteCache.assign(valueCount, -1);
    definitionsCache.resize(valueCount);
    definitionsCacheReady.assign(valueCount, false);
    valueReadCache.assign(valueCount, -1);
    enclosingIfRegionCache.resize(module.operations.size());
    enclosingIfRegionReady.assign(module.operations.size(), false);
    subsetSignatureCache.resize(module.operations.size());
    subsetSignatureReady.assign(module.operations.size(), false);
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        blockArgumentOwners.at(static_cast<size_t>(argument)) = block.id;
    for (const GenericOperation &operation : module.operations) {
      dpsInitOperands.at(static_cast<size_t>(operation.id)) =
          DpsInitOperandIndices(operation.name, operation.operands.size(),
                                operation.properties);
      for (int result : operation.results)
        definitions.at(static_cast<size_t>(result)) = &operation;
      for (size_t index = 0; index < operation.operands.size(); ++index)
        uses.at(static_cast<size_t>(operation.operands[index]))
            .push_back({operation.id, static_cast<int>(index)});
      operandDecisions.at(static_cast<size_t>(operation.id))
          .assign(operation.operandTypes.size(), -1);
    }
  }

  const GenericOperation *definition(int value) const {
    return value < 0 || static_cast<size_t>(value) >= definitions.size()
               ? nullptr
               : definitions[static_cast<size_t>(value)];
  }

  int blockArgumentOwner(int value) const {
    return value < 0 ||
                   static_cast<size_t>(value) >= blockArgumentOwners.size()
               ? -1
               : blockArgumentOwners[static_cast<size_t>(value)];
  }

  bool hasDecision(int operation, size_t operand) const {
    return operation >= 0 &&
           static_cast<size_t>(operation) < operandDecisions.size() &&
           operand < operandDecisions[static_cast<size_t>(operation)].size() &&
           operandDecisions[static_cast<size_t>(operation)][operand] >= 0;
  }

  void setDecision(int operation, size_t operand,
                   OneShotBufferizationDecision value) {
    operandDecisions.at(static_cast<size_t>(operation)).at(operand) =
        value == OneShotBufferizationDecision::InPlace ? 0 : 1;
  }

  OneShotBufferizationDecision decision(int operation, size_t operand) const {
    const int8_t value =
        operandDecisions.at(static_cast<size_t>(operation)).at(operand);
    if (value < 0)
      throw std::runtime_error("OneShotAnalysis: missing operand decision");
    return value == 0 ? OneShotBufferizationDecision::InPlace
                      : OneShotBufferizationDecision::OutOfPlace;
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
    const std::vector<size_t> &initIndices =
        dpsInitOperandIndices(operation);
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
    const std::vector<size_t> &inits = dpsInitOperandIndices(operation);
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
    for (size_t init : dpsInitOperandIndices(operation)) {
      if (init >= operation.operands.size() || init == operand)
        continue;
      if (operation.operands[init] == operation.operands[operand] &&
          init < operand)
        return true;
    }
    return false;
  }

  const std::vector<OpOperand> &getAliasingOpOperands(int value) const {
    static const std::vector<OpOperand> empty;
    if (value < 0 || static_cast<size_t>(value) >=
                         aliasingOperandCache.size())
      return empty;
    const size_t ordinal = static_cast<size_t>(value);
    if (aliasingOperandReady[ordinal])
      return aliasingOperandCache[ordinal];
    aliasingOperandReady[ordinal] = true;
    std::vector<OpOperand> &result = aliasingOperandCache[ordinal];
    const GenericOperation *definingOperation = definition(value);
    if (!definingOperation)
      return result;
    const GenericOperation &operation = *definingOperation;
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
    const GenericOperation *definingOperation = definition(value);
    if (!definingOperation)
      return true;
    const GenericOperation &operation = *definingOperation;
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
                 const GenericOperation *nestedDefinition =
                     definition(nested);
                 return nestedDefinition &&
                        isProperAncestor(operation, *nestedDefinition) &&
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
    if (!definition(value))
      return true;
    const size_t ordinal = static_cast<size_t>(value);
    if (memoryWriteCache[ordinal] >= 0)
      return memoryWriteCache[ordinal] != 0;
    std::set<int> active;
    const bool result = resultBufferizesToMemoryWrite(value, active);
    memoryWriteCache[ordinal] = result ? 1 : 0;
    return result;
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
      const std::vector<OpOperand> &aliases =
          getAliasingOpOperands(current);
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

  const std::vector<int> &findDefinitions(int value) {
    static const std::vector<int> empty;
    if (value < 0 || static_cast<size_t>(value) >= definitionsCache.size())
      return empty;
    const size_t ordinal = static_cast<size_t>(value);
    if (!definitionsCacheReady[ordinal]) {
      definitionsCache[ordinal] = findValueInReverseUseDefChain(
          value, [&](int candidate) {
            return bufferizesToMemoryWrite(candidate);
          });
      definitionsCacheReady[ordinal] = true;
    }
    return definitionsCache[ordinal];
  }

  bool isValueRead(int value) {
    if (value < 0 || static_cast<size_t>(value) >= valueReadCache.size())
      return false;
    const size_t ordinal = static_cast<size_t>(value);
    if (valueReadCache[ordinal] >= 0)
      return valueReadCache[ordinal] != 0;
    std::set<OpOperand> visited;
    std::vector<OpOperand> worklist;
    if (value >= 0 && static_cast<size_t>(value) < uses.size())
      worklist = uses[static_cast<size_t>(value)];
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
          if (alias >= 0 && static_cast<size_t>(alias) < uses.size()) {
            const std::vector<OpOperand> &aliasUses =
                uses[static_cast<size_t>(alias)];
            worklist.insert(worklist.end(), aliasUses.begin(),
                            aliasUses.end());
          }
        }
      if (bufferizesToMemoryRead(operation, operand)) {
        valueReadCache[ordinal] = 1;
        return true;
      }
    }
    valueReadCache[ordinal] = 0;
    return false;
  }

  void getAliasingInplaceWrites(std::set<OpOperand> &result, int value) {
    const int root = findAlias(value);
    if (root < 0 || static_cast<size_t>(root) >= aliasMembers.size())
      return;
    for (int alias : aliasMembers[static_cast<size_t>(root)]) {
      if (alias < 0 || static_cast<size_t>(alias) >= uses.size())
        continue;
      for (const OpOperand &use : uses[static_cast<size_t>(alias)]) {
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(use.first));
        const size_t operand = static_cast<size_t>(use.second);
        if (hasDecision(use.first, operand) &&
            decision(use.first, operand) ==
                OneShotBufferizationDecision::InPlace &&
            bufferizesToMemoryWrite(operation,
                                     operand))
          result.insert(use);
      }
    }
  }

  void getAliasingReads(std::set<OpOperand> &result, int value) {
    const int root = findAlias(value);
    if (root < 0 || static_cast<size_t>(root) >= aliasMembers.size())
      return;
    for (int alias : aliasMembers[static_cast<size_t>(root)]) {
      if (alias < 0 || static_cast<size_t>(alias) >= uses.size())
        continue;
      for (const OpOperand &use : uses[static_cast<size_t>(alias)]) {
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

  const std::map<int, int> &
  enclosingIfRegions(const GenericOperation &operation) const {
    const size_t operationId = static_cast<size_t>(operation.id);
    if (enclosingIfRegionReady.at(operationId))
      return enclosingIfRegionCache.at(operationId);
    std::map<int, int> &result = enclosingIfRegionCache.at(operationId);
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
    enclosingIfRegionReady.at(operationId) = true;
    return result;
  }

  bool insideMutuallyExclusiveRegions(
      const GenericOperation &lhs,
      const GenericOperation &rhs) const {
    const std::map<int, int> &lhsRegions = enclosingIfRegions(lhs);
    const std::map<int, int> &rhsRegions = enclosingIfRegions(rhs);
    for (const auto &[owner, region] : lhsRegions) {
      auto found = rhsRegions.find(owner);
      if (found != rhsRegions.end() && found->second != region)
        return true;
    }
    return false;
  }

  const std::string &subsetSignature(const GenericOperation &operation) const {
    const size_t operationId = static_cast<size_t>(operation.id);
    if (subsetSignatureReady.at(operationId))
      return subsetSignatureCache.at(operationId);
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
    subsetSignatureCache.at(operationId) = signature.str();
    subsetSignatureReady.at(operationId) = true;
    return subsetSignatureCache.at(operationId);
  }

  bool matchesInsertDestination(int value,
                                const GenericOperation &insertSlice) {
    const std::string &signature = subsetSignature(insertSlice);
    const int destination = insertSlice.operands.at(1);
    return !findValueInReverseUseDefChain(
                value,
                [&](int candidate) {
                  const GenericOperation *definingOperation =
                      definition(candidate);
                  if (!definingOperation ||
                      definingOperation->name != "tensor.extract_slice" ||
                      definingOperation->operands.empty())
                    return false;
                  return findAlias(definingOperation->operands.front()) ==
                             findAlias(destination) &&
                         subsetSignature(*definingOperation) == signature;
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
      const GenericOperation *definingOperation = definition(value);
      if (definingOperation)
        definitionRegion = getEnclosingRepetitiveRegion(*definingOperation);
      else {
        const int blockOwner = blockArgumentOwner(value);
        if (blockOwner >= 0) {
          const GenericBlock &block =
              module.blocks.at(static_cast<size_t>(blockOwner));
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
      const std::vector<int> &valueDefinitions = findDefinitions(readValue);
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
          const GenericOperation *definingOperation =
              definition(definitionValue);
          if (definingOperation) {
            if (happensBefore(writingOperation, *definingOperation) ||
                isProperAncestor(*definingOperation, writingOperation))
              continue;
          } else {
            const int owner = blockArgumentOwner(definitionValue);
            if (owner >= 0) {
              const GenericBlock &block =
                  module.blocks.at(static_cast<size_t>(owner));
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
  std::vector<const GenericOperation *> definitions;
  std::vector<int> blockArgumentOwners;
  std::vector<int> aliasParents;
  std::vector<std::vector<int>> aliasMembers;
  std::vector<std::vector<OpOperand>> uses;
  std::vector<std::vector<size_t>> dpsInitOperands;
  mutable std::vector<std::vector<OpOperand>> aliasingOperandCache;
  mutable std::vector<bool> aliasingOperandReady;
  std::vector<int8_t> memoryWriteCache;
  std::vector<std::vector<int>> definitionsCache;
  std::vector<bool> definitionsCacheReady;
  std::vector<int8_t> valueReadCache;
  mutable std::vector<std::map<int, int>> enclosingIfRegionCache;
  mutable std::vector<bool> enclosingIfRegionReady;
  mutable std::vector<std::string> subsetSignatureCache;
  mutable std::vector<bool> subsetSignatureReady;
  // -1 = not analyzed yet, 0 = in-place, 1 = out-of-place.
  std::vector<std::vector<int8_t>> operandDecisions;
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
    const PreBufferizationCSEState &preBufferizationCSE,
    const GenericModuleAnalysisIndexes &analysis) {
  struct OrderedAllocation {
    int operationKey = -1;
    int orderKey = 0;
    size_t sequence = 0;
    BufferAllocation allocation;
  };
  analysis.ensureCompatible(module);
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
      const int definitionId =
          analysis.definingOperationId(operation.operands[operand]);
      const GenericOperation *definition =
          definitionId < 0
              ? nullptr
              : &module.operations.at(static_cast<size_t>(definitionId));
      const size_t useCount =
          analysis.users(operation.operands[operand]).size();
      // Replacing the sole use of an empty destination changes allocation
      // ownership but does not add another live allocation.
      if (definition && definition->name == "tensor.empty" && useCount == 1)
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
ModelOneShotBufferizeAllocationsExact(
    const GenericModule &module,
    const std::vector<OneShotOpOperandDecision> &decisions,
    const PreBufferizationCSEState &preBufferizationCSE =
        PreBufferizationCSEState{}) {
  const GenericModuleAnalysisIndexes analysis(
      module, kGenericAnalysisDefinitions | kGenericAnalysisUsers);
  return ModelOneShotBufferizeAllocationsExact(
      module, decisions, preBufferizationCSE, analysis);
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
  result.analysis.Build(
      module, kGenericAnalysisDefinitions | kGenericAnalysisUsers |
                  kGenericAnalysisValueTypes |
                  kGenericAnalysisEnclosingFunctions);
  result.allocations = ModelOneShotBufferizeAllocationsExact(
      module, result.decisions, result.preBufferizationCSE, result.analysis);
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
