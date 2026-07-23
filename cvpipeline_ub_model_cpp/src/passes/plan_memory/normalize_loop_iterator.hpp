// PlanMemory NormalizeLoopIterator and greedy region simplification semantics.
#ifndef CVPIPELINE_UB_MODEL_CPP_NORMALIZE_LOOP_ITERATOR_HPP
#define CVPIPELINE_UB_MODEL_CPP_NORMALIZE_LOOP_ITERATOR_HPP

#include "../../ir/semantic_ir.hpp"

namespace cvub {

// PlanMemoryPass runs NormalizeIterUseAfterYieldInit before constructing
// MemLivenessAnalysis. Reproduce that rewrite on the lightweight operation
// stream: when a loop-carried input is read after the yielded allocation has
// been initialized, copy the yielded value into the iter arg and yield the
// iter arg itself.
inline std::vector<OperationRecord> NormalizeIterUseAfterYieldInit(
    std::vector<OperationRecord> input,
    const std::set<std::string> &allocNames,
    const std::map<std::string, std::string> &allocTypes) {
  struct LoopContext {
    int begin = -1;
    int end = -1;
    int yield = -1;
    std::vector<std::pair<std::string, std::string>> iterArgPairs;
    std::vector<std::string> yieldedValues;
  };

  std::vector<LoopContext> loops;
  std::vector<size_t> loopStack;
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i].opName == "scf.for") {
      LoopContext context;
      context.begin = static_cast<int>(i);
      context.iterArgPairs = extractIterArgPairs(input[i].text);
      loops.push_back(std::move(context));
      loopStack.push_back(loops.size() - 1);
    } else if (input[i].opName == "scf.yield" && !loopStack.empty()) {
      loops[loopStack.back()].yield = static_cast<int>(i);
      loops[loopStack.back()].yieldedValues = extractSSAs(input[i].text);
    } else if (input[i].opName == "scf.for.end" && !loopStack.empty()) {
      loops[loopStack.back()].end = static_cast<int>(i);
      loopStack.pop_back();
    }
  }
  // The rewrite is defined only for complete scf.for regions.  Most kernels
  // reaching PlanMemory do not contain such a loop; avoid the scope, alias,
  // read/write and reconstruction passes entirely for that common case.
  loops.erase(std::remove_if(loops.begin(), loops.end(),
                             [](const LoopContext &loop) {
                               return loop.begin < 0 || loop.yield < 0 ||
                                      loop.end < 0;
                             }),
              loops.end());
  if (loops.empty())
    return input;

  struct ScopeResult {
    int returnOperation = -1;
    std::string returnValue;
  };
  struct ScopeContext {
    std::vector<std::string> results;
    int returnOperation = -1;
    std::vector<std::string> returnedValues;
  };
  std::map<std::string, ScopeResult> scopeResult;
  std::map<int, std::vector<std::pair<std::string, std::string>>>
      scopeAliasesAtEnd;
  std::vector<ScopeContext> scopeStack;
  for (size_t i = 0; i < input.size(); ++i) {
    const OperationRecord &operation = input[i];
    if (operation.opName == "scope.scope") {
      scopeStack.push_back({operationResultNames(operation), -1, {}});
      continue;
    }
    if (operation.opName == "scope.return" && !scopeStack.empty()) {
      scopeStack.back().returnOperation = static_cast<int>(i);
      scopeStack.back().returnedValues = operationOperandNames(operation);
      continue;
    }
    if (operation.opName != "scope.scope.end" || scopeStack.empty())
      continue;
    ScopeContext context = std::move(scopeStack.back());
    scopeStack.pop_back();
    size_t count =
        std::min(context.results.size(), context.returnedValues.size());
    for (size_t resultIndex = 0; resultIndex < count; ++resultIndex) {
      scopeResult[context.results[resultIndex]] =
          {context.returnOperation, context.returnedValues[resultIndex]};
      scopeAliasesAtEnd[static_cast<int>(i)].push_back(
          {context.results[resultIndex], context.returnedValues[resultIndex]});
    }
  }

  // Keep only alias changes instead of copying the complete alias map before
  // every operation.  A change recorded at i + 1 has exactly the visibility
  // of the old aliasBefore[i + 1] snapshot.
  std::map<std::string, std::vector<std::pair<size_t, std::string>>>
      aliasHistory;
  auto recordAlias = [&](size_t position, const std::string &value,
                         const std::string &source) {
    aliasHistory[value].push_back({position, source});
  };
  auto aliasAt = [&](const std::string &value,
                     size_t position) -> std::optional<std::string> {
    auto history = aliasHistory.find(value);
    if (history == aliasHistory.end())
      return std::nullopt;
    const auto visible = std::upper_bound(
        history->second.begin(), history->second.end(), position,
        [](size_t query,
           const std::pair<size_t, std::string> &change) {
          return query < change.first;
        });
    if (visible == history->second.begin())
      return std::nullopt;
    return std::prev(visible)->second;
  };
  auto canonicalAt = [&](const std::string &value, size_t position) {
    std::set<std::string> seen;
    std::string current = value;
    while (!seen.count(current)) {
      seen.insert(current);
      const std::optional<std::string> next = aliasAt(current, position);
      if (!next || *next == current)
        break;
      current = *next;
    }
    return current;
  };
  auto canonicalAllocAt = [&](const std::string &value, size_t position)
      -> std::optional<std::string> {
    const std::string base = canonicalAt(value, position);
    return allocNames.count(base) ? std::optional<std::string>(base)
                                  : std::nullopt;
  };
  std::map<std::string, int> allocDef;
  for (const std::string &name : allocNames)
    recordAlias(0, name, name);
  for (size_t i = 0; i < input.size(); ++i) {
    std::string result = resultNameBeforeEqual(input[i].text);
    if (input[i].opName == "memref.alloc" && allocNames.count(result))
      allocDef[result] = static_cast<int>(i);
    if (isViewLikeMemrefOp(input[i].opName)) {
      for (const std::string &operand : operationOperandNames(input[i])) {
        if (operand == result)
          continue;
        recordAlias(i + 1, result, operand);
        break;
      }
    }
    if (input[i].opName == "scf.for") {
      for (const auto &pair : extractIterArgPairs(input[i].text))
        recordAlias(i + 1, pair.first, pair.second);
    }
    auto scopeAliases = scopeAliasesAtEnd.find(static_cast<int>(i));
    if (scopeAliases != scopeAliasesAtEnd.end())
      for (const auto &pair : scopeAliases->second)
        recordAlias(i + 1, pair.first, pair.second);
  }

  struct Rewrite {
    std::string source;
    std::string iterArg;
    std::string replacedValue;
    std::string memrefType;
  };
  std::map<int, std::vector<Rewrite>> rewrites;
  auto writtenValues = [](const OperationRecord &operation) {
    std::vector<std::string> values;
    if (IsDestinationStyleOp(operation.opName)) {
      values = extractGroupSSAs(operation.text, "outs");
      for (const std::string group : {"temp_buffer", "tmps"}) {
        std::vector<std::string> extra =
            extractGroupSSAs(operation.text, group);
        values.insert(values.end(), extra.begin(), extra.end());
      }
    } else if (operation.opName == "memref.store") {
      std::vector<std::string> operands = operationOperandNames(operation);
      if (operands.size() >= 2)
        values.push_back(operands[1]);
    }
    return values;
  };
  auto readValues = [](const OperationRecord &operation) {
    if (IsDestinationStyleOp(operation.opName))
      return extractGroupSSAs(operation.text, "ins");
    if (operation.opName == "memref.load" ||
        operation.opName == "hivm.hir.debug" ||
        operation.opName == "scf.yield")
      return operationOperandNames(operation);
    return std::vector<std::string>{};
  };
  std::vector<std::vector<std::string>> writtenByOperation;
  std::vector<std::vector<std::string>> readByOperation;
  writtenByOperation.reserve(input.size());
  readByOperation.reserve(input.size());
  for (const OperationRecord &operation : input) {
    writtenByOperation.push_back(writtenValues(operation));
    readByOperation.push_back(readValues(operation));
  }
  for (const LoopContext &loop : loops) {
    size_t count =
        std::min(loop.iterArgPairs.size(), loop.yieldedValues.size());
    for (size_t i = 0; i < count; ++i) {
      const std::string &iterArg = loop.iterArgPairs[i].first;
      const std::string &yielded = loop.yieldedValues[i];
      if (iterArg == yielded)
        continue;
      const size_t yieldIndex = static_cast<size_t>(loop.yield);
      auto yieldedBase = canonicalAllocAt(yielded, yieldIndex);
      if (!yieldedBase)
        continue;
      auto def = allocDef.find(*yieldedBase);
      if (def == allocDef.end() || def->second <= loop.begin ||
          def->second >= loop.yield)
        continue;

      int firstInitialization = -1;
      for (int opIndex = def->second + 1; opIndex < loop.yield; ++opIndex) {
        const size_t operationIndex = static_cast<size_t>(opIndex);
        for (const std::string &value :
             writtenByOperation[operationIndex]) {
          auto base = canonicalAllocAt(value, operationIndex);
          if (base && *base == *yieldedBase) {
            firstInitialization = opIndex;
            break;
          }
        }
        if (firstInitialization >= 0)
          break;
      }
      if (firstInitialization < 0)
        continue;

      bool iterArgReadAfterInitialization = false;
      for (int opIndex = firstInitialization + 1; opIndex < loop.yield;
           ++opIndex) {
        const size_t operationIndex = static_cast<size_t>(opIndex);
        const std::string iterRoot = canonicalAt(iterArg, operationIndex);
        for (const std::string &value : readByOperation[operationIndex]) {
          if (canonicalAt(value, operationIndex) == iterRoot) {
            iterArgReadAfterInitialization = true;
            break;
          }
        }
        if (iterArgReadAfterInitialization)
          break;
      }
      if (iterArgReadAfterInitialization) {
        auto type = allocTypes.find(*yieldedBase);
        if (type == allocTypes.end() || type->second.empty())
          throw std::runtime_error(
              "NormalizeIterUseAfterYieldInit: missing yielded memref type");
        auto scope = scopeResult.find(yielded);
        if (scope != scopeResult.end()) {
          rewrites[scope->second.returnOperation].push_back(
              {scope->second.returnValue, iterArg,
               scope->second.returnValue, type->second});
        } else {
          rewrites[loop.yield].push_back(
              {yielded, iterArg, yielded, type->second});
        }
      }
    }
  }

  // Preserve the original vector (including capacity and already assigned
  // indexes) when the semantic rewrite found nothing to change.
  if (rewrites.empty())
    return input;

  std::vector<OperationRecord> result;
  result.reserve(input.size() + rewrites.size());
  int nextOperationId = 0;
  for (const OperationRecord &op : input)
    nextOperationId = std::max(nextOperationId, op.operationId + 1);
  for (size_t i = 0; i < input.size(); ++i) {
    OperationRecord rewritten = std::move(input[i]);
    auto rewriteIt = rewrites.find(static_cast<int>(i));
    if (rewriteIt != rewrites.end()) {
      for (const Rewrite &rewrite : rewriteIt->second) {
        OperationRecord copy;
        copy.line = rewritten.line;
        copy.indent = rewritten.indent;
        copy.operationId = nextOperationId++;
        copy.regionPath = rewritten.regionPath;
        copy.blockId = rewritten.blockId;
        copy.blockLabel = rewritten.blockLabel;
        copy.blockArguments = rewritten.blockArguments;
        copy.opName = "hivm.hir.copy";
        copy.text = "hivm.hir.copy ins(" + rewrite.source + " : " +
                    rewrite.memrefType + ") outs(" + rewrite.iterArg +
                    " : " + rewrite.memrefType + ")";
        result.push_back(std::move(copy));
        rewritten.text =
            replaceSSAUse(rewritten.text, rewrite.replacedValue,
                          rewrite.iterArg);
      }
    }
    result.push_back(std::move(rewritten));
  }
  for (size_t i = 0; i < result.size(); ++i)
    result[i].index = static_cast<int>(i);
  return result;
}

inline bool isKnownPureNormalizeOperation(const OperationRecord &operation) {
  return startsWith(operation.opName, "arith.") ||
         startsWith(operation.opName, "affine.") ||
         isViewLikeMemrefOp(operation.opName) ||
         operation.opName == "memref.dim";
}

inline bool isCommonPureNormalizeOperation(
    const OperationRecord &operation) {
  return isKnownPureNormalizeOperation(operation);
}

inline void EliminateCommonPureOperations(
    std::vector<OperationRecord> &operations) {
  using BlockKey = std::pair<std::vector<int>, int>;
  std::map<std::pair<BlockKey, std::string>,
           std::vector<std::string>> available;
  std::map<std::string, std::string> aliases;
  std::vector<OperationRecord> retained;
  retained.reserve(operations.size());
  for (OperationRecord &operation : operations) {
    operation.text = replaceSSAUsesInMapOrder(operation.text, aliases);
    if (!operation.normalizationKey.empty())
      operation.normalizationKey =
          replaceSSAUsesInMapOrder(operation.normalizationKey, aliases);
    for (BranchDestination &destination : operation.branchDestinations)
      for (std::string &operand : destination.operands)
        operand = resolveSSAUseInMapOrder(std::move(operand), aliases);
    const std::vector<std::string> results = operationResultNames(operation);
    const bool eligible = !results.empty() &&
                          operation.opName != "arith.constant" &&
                          isCommonPureNormalizeOperation(operation);
    if (!eligible) {
      retained.push_back(std::move(operation));
      continue;
    }
    const size_t equal = operation.text.find('=');
    const std::string rhs =
        equal == std::string::npos ? operation.text
                                   : trim(operation.text.substr(equal + 1));
    const BlockKey block{operation.regionPath, operation.blockId};
    const std::string &semanticRhs =
        operation.normalizationKey.empty() ? rhs : operation.normalizationKey;
    const auto key =
        std::make_pair(block, operation.opName + "\n" + semanticRhs);
    auto existing = available.find(key);
    if (existing == available.end() ||
        existing->second.size() != results.size()) {
      available[key] = results;
      retained.push_back(std::move(operation));
      continue;
    }
    for (size_t index = 0; index < results.size(); ++index)
      aliases[results[index]] = existing->second[index];
  }
  operations = std::move(retained);
}

// RegionUtils::dropRedundantArguments replaces a block argument when every
// predecessor forwards the same Value, then erases the corresponding branch
// operands. applyPatternsGreedily invokes this CFG simplification around the
// PlanMemory normalization pattern.
inline void DropRedundantArguments(std::vector<OperationRecord> &operations) {
  using BlockKey = std::pair<std::vector<int>, std::string>;
  bool changed = true;
  while (changed) {
    changed = false;
    std::map<BlockKey, std::vector<std::string>> argumentsByBlock;
    std::vector<BlockKey> blockOrder;
    for (const OperationRecord &operation : operations) {
      BlockKey key{operation.regionPath, operation.blockLabel};
      if (!argumentsByBlock.count(key))
        blockOrder.push_back(key);
      argumentsByBlock[key] = operation.blockArguments;
    }

    struct Incoming {
      size_t operation = 0;
      size_t destination = 0;
    };
    std::map<BlockKey, std::vector<Incoming>> incomingByBlock;
    for (size_t operationIndex = 0; operationIndex < operations.size();
         ++operationIndex) {
      OperationRecord &operation = operations[operationIndex];
      for (size_t destinationIndex = 0;
           destinationIndex < operation.branchDestinations.size();
           ++destinationIndex) {
        const BranchDestination &destination =
            operation.branchDestinations[destinationIndex];
        incomingByBlock[{operation.regionPath, destination.label}].push_back(
            {operationIndex, destinationIndex});
      }
    }

    for (const BlockKey &block : blockOrder) {
      const std::vector<std::string> &arguments = argumentsByBlock[block];
      auto incomingIt = incomingByBlock.find(block);
      if (arguments.empty() || incomingIt == incomingByBlock.end() ||
          incomingIt->second.empty())
        continue;
      for (size_t argumentIndex = 0; argumentIndex < arguments.size();
           ++argumentIndex) {
        std::optional<std::string> commonValue;
        bool sameValue = true;
        for (const Incoming &incoming : incomingIt->second) {
          const BranchDestination &destination =
              operations[incoming.operation]
                  .branchDestinations[incoming.destination];
          if (argumentIndex >= destination.operands.size()) {
            sameValue = false;
            break;
          }
          const std::string &operand = destination.operands[argumentIndex];
          if (!commonValue)
            commonValue = operand;
          else if (*commonValue != operand) {
            sameValue = false;
            break;
          }
        }
        if (!sameValue || !commonValue ||
            *commonValue == arguments[argumentIndex])
          continue;

        const std::string blockArgument = arguments[argumentIndex];
        for (OperationRecord &operation : operations) {
          if (operation.regionPath != block.first ||
              operation.blockLabel != block.second)
            continue;
          operation.text =
              replaceSSAUse(operation.text, blockArgument, *commonValue);
          for (BranchDestination &destination :
               operation.branchDestinations)
            for (std::string &operand : destination.operands)
              if (operand == blockArgument)
                operand = *commonValue;
          if (argumentIndex < operation.blockArguments.size())
            operation.blockArguments.erase(
                operation.blockArguments.begin() +
                static_cast<std::ptrdiff_t>(argumentIndex));
        }
        for (const Incoming &incoming : incomingIt->second) {
          std::vector<std::string> &operands =
              operations[incoming.operation]
                  .branchDestinations[incoming.destination]
                  .operands;
          operands.erase(operands.begin() +
                         static_cast<std::ptrdiff_t>(argumentIndex));
        }
        changed = true;
        break;
      }
      if (changed)
        break;
    }
  }
}

inline std::vector<OperationRecord>
ApplyPlanMemoryNormalizePatterns(std::vector<OperationRecord> operations) {
  DropRedundantArguments(operations);
  EliminateCommonPureOperations(operations);
  // Greedy DCE reaches the same fixed point with a use-count worklist.  The
  // old round-based form reparsed every operation and rebuilt the complete
  // use map after each newly exposed dead layer, which was costly for long
  // affine/view chains emitted by the PlanMemory bridge.
  std::vector<std::vector<std::string>> operandsByOperation;
  std::vector<std::vector<std::string>> resultsByOperation;
  operandsByOperation.reserve(operations.size());
  resultsByOperation.reserve(operations.size());
  std::unordered_map<std::string, size_t> useCounts;
  std::unordered_map<std::string, size_t> definingOperations;
  useCounts.reserve(operations.size() * 2);
  definingOperations.reserve(operations.size() * 2);
  for (size_t index = 0; index < operations.size(); ++index) {
    operandsByOperation.push_back(operationOperandNames(operations[index]));
    resultsByOperation.push_back(operationResultNames(operations[index]));
    for (const std::string &operand : operandsByOperation.back())
      ++useCounts[operand];
    for (const std::string &result : resultsByOperation.back())
      definingOperations[result] = index;
  }
  const auto isDead = [&](size_t index) {
    const std::vector<std::string> &results = resultsByOperation[index];
    return !results.empty() &&
           isKnownPureNormalizeOperation(operations[index]) &&
           std::all_of(results.begin(), results.end(),
                       [&](const std::string &result) {
                         auto count = useCounts.find(result);
                         return count == useCounts.end() || count->second == 0;
                       });
  };
  std::vector<size_t> worklist;
  for (size_t index = 0; index < operations.size(); ++index)
    if (isDead(index))
      worklist.push_back(index);
  std::vector<bool> erased(operations.size(), false);
  while (!worklist.empty()) {
    const size_t index = worklist.back();
    worklist.pop_back();
    if (erased[index] || !isDead(index))
      continue;
    erased[index] = true;
    for (const std::string &operand : operandsByOperation[index]) {
      auto count = useCounts.find(operand);
      if (count == useCounts.end() || count->second == 0)
        continue;
      --count->second;
      if (count->second != 0)
        continue;
      auto definition = definingOperations.find(operand);
      if (definition != definingOperations.end() &&
          !erased[definition->second] && isDead(definition->second))
        worklist.push_back(definition->second);
    }
  }
  if (std::any_of(erased.begin(), erased.end(), [](bool value) {
        return value;
      })) {
    std::vector<OperationRecord> retained;
    retained.reserve(operations.size());
    for (size_t index = 0; index < operations.size(); ++index)
      if (!erased[index])
        retained.push_back(std::move(operations[index]));
    operations = std::move(retained);
  }

  using BlockKey = std::pair<std::vector<int>, int>;
  std::vector<BlockKey> blockOrder;
  std::map<BlockKey, std::vector<OperationRecord>> constantsByBlock;
  std::vector<OperationRecord> nonConstants;
  nonConstants.reserve(operations.size());
  for (OperationRecord &operation : operations) {
    if (operation.opName != "arith.constant") {
      nonConstants.push_back(std::move(operation));
      continue;
    }
    BlockKey key{operation.regionPath, operation.blockId};
    if (!constantsByBlock.count(key))
      blockOrder.push_back(key);
    constantsByBlock[key].push_back(std::move(operation));
  }
  // Rebuild once instead of inserting every block's constants into the
  // middle of an ever-growing vector.  Constants still precede the first
  // non-constant in their block; constant-only blocks retain blockOrder at
  // the end, exactly matching the former repeated-insert implementation.
  std::vector<OperationRecord> reordered;
  reordered.reserve(operations.size());
  std::set<BlockKey> emittedConstantBlocks;
  for (OperationRecord &operation : nonConstants) {
    const BlockKey key{operation.regionPath, operation.blockId};
    auto constants = constantsByBlock.find(key);
    if (constants != constantsByBlock.end() &&
        emittedConstantBlocks.insert(key).second)
      for (OperationRecord &constant : constants->second)
        reordered.push_back(std::move(constant));
    reordered.push_back(std::move(operation));
  }
  for (const BlockKey &key : blockOrder) {
    if (!emittedConstantBlocks.insert(key).second)
      continue;
    for (OperationRecord &constant : constantsByBlock[key])
      reordered.push_back(std::move(constant));
  }
  operations = std::move(reordered);
  for (size_t i = 0; i < operations.size(); ++i)
    operations[i].index = static_cast<int>(i);
  return operations;
}


} // namespace cvub

#endif
