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
template <typename AllocationNameSet, typename AllocationTypeMap>
inline std::vector<OperationRecord> NormalizeIterUseAfterYieldInit(
    std::vector<OperationRecord> input, const AllocationNameSet &allocNames,
    const AllocationTypeMap &allocTypes) {
  // Both normalization patterns repeatedly query the same SSA lists. Parse
  // an untyped text input once up front; the production bridge reaches here
  // with these lists already populated, so this is a no-op on the hot path.
  MaterializeOperationValueLists(input);
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
      context.iterArgPairs = OperationIterArgPairs(input[i]);
      loops.push_back(std::move(context));
      loopStack.push_back(loops.size() - 1);
    } else if (input[i].opName == "scf.yield" && !loopStack.empty()) {
      loops[loopStack.back()].yield = static_cast<int>(i);
      loops[loopStack.back()].yieldedValues = input[i].materializedOperands;
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
      scopeStack.push_back({operation.materializedResults, -1, {}});
      continue;
    }
    if (operation.opName == "scope.return" && !scopeStack.empty()) {
      scopeStack.back().returnOperation = static_cast<int>(i);
      scopeStack.back().returnedValues = operation.materializedOperands;
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
    const std::string result = input[i].materializedResults.empty()
                                   ? std::string()
                                   : input[i].materializedResults.front();
    if (input[i].opName == "memref.alloc" && allocNames.count(result))
      allocDef[result] = static_cast<int>(i);
    if (isViewLikeMemrefOp(input[i].opName)) {
      for (const std::string &operand : input[i].materializedOperands) {
        if (operand == result)
          continue;
        recordAlias(i + 1, result, operand);
        break;
      }
    }
    if (input[i].opName == "scf.for") {
      for (const auto &pair : OperationIterArgPairs(input[i]))
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
      values = OperationGroupNames(operation, "outs");
      for (const std::string group : {"temp_buffer", "tmps"}) {
        std::vector<std::string> extra =
            OperationGroupNames(operation, group);
        values.insert(values.end(), extra.begin(), extra.end());
      }
    } else if (operation.opName == "memref.store") {
      if (operation.materializedOperands.size() >= 2)
        values.push_back(operation.materializedOperands[1]);
    }
    return values;
  };
  auto readValues = [](const OperationRecord &operation) {
    if (IsDestinationStyleOp(operation.opName))
      return OperationGroupNames(operation, "ins");
    if (operation.opName == "memref.load" ||
        operation.opName == "hivm.hir.debug" ||
        operation.opName == "scf.yield")
      return operation.materializedOperands;
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
        copy.materializedValueLists = true;
        copy.materializedOperands = {rewrite.source, rewrite.iterArg};
        result.push_back(std::move(copy));
        ReplaceOperationSSAUse(rewritten, rewrite.replacedValue,
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

// OperationRecord is the bridge's post-bufferization projection, so it no
// longer carries GenericOperation::regions.  Recover the same direct region
// ownership used by GenericOperationDominates from the pre-order stream: the
// last operation in the parent path before the first child is the operation
// owning that child region.
inline std::vector<std::optional<size_t>> NormalizeParentOperations(
    const std::vector<OperationRecord> &operations) {
  using RegionKey = std::pair<std::vector<int>, int>;
  std::map<std::vector<int>, size_t> lastOperationAtPath;
  std::map<RegionKey, std::optional<size_t>> parentByRegion;
  std::vector<std::optional<size_t>> parents(operations.size());
  for (size_t index = 0; index < operations.size(); ++index) {
    const OperationRecord &operation = operations[index];
    if (!operation.regionPath.empty()) {
      std::vector<int> parentPath(operation.regionPath.begin(),
                                  std::prev(operation.regionPath.end()));
      const RegionKey key{parentPath, operation.regionPath.back()};
      auto parent = parentByRegion.find(key);
      if (parent == parentByRegion.end()) {
        const auto owner = lastOperationAtPath.find(parentPath);
        parent = parentByRegion
                     .emplace(key, owner == lastOperationAtPath.end()
                                       ? std::nullopt
                                       : std::optional<size_t>(owner->second))
                     .first;
      }
      parents[index] = parent->second;
    }
    lastOperationAtPath[operation.regionPath] = index;
  }
  return parents;
}

// This is the OperationRecord counterpart of GenericOperationDominates used
// by RunCanonicalizationHIVMAfterArithToAffine.  It intentionally remains
// conservative for sibling CFG blocks: a definition is reusable only when it
// precedes the use in the same block, or precedes an enclosing region owner.
inline bool NormalizeOperationDominates(
    const std::vector<OperationRecord> &operations,
    const std::vector<std::optional<size_t>> &parents, size_t candidate,
    size_t operation) {
  size_t cursor = operation;
  while (true) {
    if (operations[candidate].blockId == operations[cursor].blockId &&
        candidate < cursor)
      return true;
    if (!parents[cursor])
      return false;
    cursor = *parents[cursor];
  }
}

inline void EliminateCommonPureOperations(
    std::vector<OperationRecord> &operations) {
  // canonicalizationHIVMPipeline runs MLIR's CSE after FlattenOps.  CSE is not
  // block-local: a collapse_shape before scf.for/scf.if dominates equivalent
  // collapses in its nested regions.  OperationRecord does not retain the
  // complete side-effect and region metadata needed to widen this rule safely
  // for every MLIR operation, so all other pure bridge records keep the former
  // exact-block rule.
  const std::vector<std::optional<size_t>> parents =
      NormalizeParentOperations(operations);
  std::map<std::string, std::vector<size_t>> available;
  std::map<std::string, std::string> aliases;
  std::vector<bool> erased(operations.size(), false);
  for (size_t operationIndex = 0; operationIndex < operations.size();
       ++operationIndex) {
    OperationRecord &operation = operations[operationIndex];
    ReplaceOperationSSAUsesInMapOrder(operation, aliases);
    if (!operation.normalizationKey.empty())
      operation.normalizationKey =
          replaceSSAUsesInMapOrder(operation.normalizationKey, aliases);
    for (BranchDestination &destination : operation.branchDestinations)
      for (std::string &operand : destination.operands)
        operand = resolveSSAUseInMapOrder(std::move(operand), aliases);
    const std::vector<std::string> &results = operation.materializedResults;
    const bool eligible = !results.empty() &&
                          operation.opName != "arith.constant" &&
                          isCommonPureNormalizeOperation(operation);
    if (!eligible)
      continue;
    const size_t equal = operation.text.find('=');
    const std::string rhs =
        equal == std::string::npos ? operation.text
                                   : trim(operation.text.substr(equal + 1));
    const std::string &semanticRhs =
        operation.normalizationKey.empty() ? rhs : operation.normalizationKey;
    const std::string key = operation.opName + "\n" + semanticRhs;
    size_t dominating = operations.size();
    auto existing = available.find(key);
    if (existing != available.end()) {
      for (auto candidate = existing->second.rbegin();
           candidate != existing->second.rend(); ++candidate) {
        const OperationRecord &candidateOperation = operations[*candidate];
        const bool visible =
            operation.opName == "memref.collapse_shape"
                ? NormalizeOperationDominates(operations, parents, *candidate,
                                              operationIndex)
                : candidateOperation.regionPath == operation.regionPath &&
                      candidateOperation.blockId == operation.blockId &&
                      *candidate < operationIndex;
        if (candidateOperation.materializedResults.size() == results.size() &&
            visible) {
          dominating = *candidate;
          break;
        }
      }
    }
    if (dominating == operations.size()) {
      available[key].push_back(operationIndex);
      continue;
    }
    const std::vector<std::string> &dominatingResults =
        operations[dominating].materializedResults;
    for (size_t index = 0; index < results.size(); ++index)
      aliases[results[index]] = dominatingResults[index];
    erased[operationIndex] = true;
  }
  std::vector<OperationRecord> retained;
  retained.reserve(operations.size());
  for (size_t index = 0; index < operations.size(); ++index)
    if (!erased[index])
      retained.push_back(std::move(operations[index]));
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
          ReplaceOperationSSAUse(operation, blockArgument, *commonValue);
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
          OperationRecord &branch = operations[incoming.operation];
          std::vector<std::string> &operands =
              branch.branchDestinations[incoming.destination].operands;
          operands.erase(operands.begin() +
                         static_cast<std::ptrdiff_t>(argumentIndex));
          RematerializeOperationValueLists(branch);
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
  MaterializeOperationValueLists(operations);
  DropRedundantArguments(operations);
  EliminateCommonPureOperations(operations);
  // Greedy DCE reaches the same fixed point with a use-count worklist.  The
  // old round-based form reparsed every operation and rebuilt the complete
  // use map after each newly exposed dead layer, which was costly for long
  // affine/view chains emitted by the PlanMemory bridge.
  std::unordered_map<std::string, size_t> useCounts;
  std::unordered_map<std::string, size_t> definingOperations;
  useCounts.reserve(operations.size() * 2);
  definingOperations.reserve(operations.size() * 2);
  for (size_t index = 0; index < operations.size(); ++index) {
    for (const std::string &operand :
         operations[index].materializedOperands)
      ++useCounts[operand];
    for (const std::string &result :
         operations[index].materializedResults)
      definingOperations[result] = index;
  }
  const auto isDead = [&](size_t index) {
    const std::vector<std::string> &results =
        operations[index].materializedResults;
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
    for (const std::string &operand :
         operations[index].materializedOperands) {
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
