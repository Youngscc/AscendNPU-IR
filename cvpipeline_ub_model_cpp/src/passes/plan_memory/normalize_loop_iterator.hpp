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
    const std::vector<OperationRecord> &input,
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

  std::map<std::string, std::string> alias;
  std::map<std::string, int> allocDef;
  std::vector<std::map<std::string, std::string>> aliasBefore(input.size());
  for (const std::string &name : allocNames)
    alias[name] = name;
  for (size_t i = 0; i < input.size(); ++i) {
    aliasBefore[i] = alias;
    std::string result = resultNameBeforeEqual(input[i].text);
    if (input[i].opName == "memref.alloc" && allocNames.count(result))
      allocDef[result] = static_cast<int>(i);
    if (isViewLikeMemrefOp(input[i].opName)) {
      for (const std::string &operand : operationOperandNames(input[i])) {
        if (operand == result)
          continue;
        alias[result] = operand;
        break;
      }
    }
    if (input[i].opName == "scf.for") {
      for (const auto &pair : extractIterArgPairs(input[i].text))
        alias[pair.first] = pair.second;
    }
    auto scopeAliases = scopeAliasesAtEnd.find(static_cast<int>(i));
    if (scopeAliases != scopeAliasesAtEnd.end())
      for (const auto &pair : scopeAliases->second)
        alias[pair.first] = pair.second;
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
      for (const std::string &group : {"temp_buffer", "tmps"}) {
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
  for (const LoopContext &loop : loops) {
    if (loop.begin < 0 || loop.yield < 0 || loop.end < 0)
      continue;
    size_t count =
        std::min(loop.iterArgPairs.size(), loop.yieldedValues.size());
    for (size_t i = 0; i < count; ++i) {
      const std::string &iterArg = loop.iterArgPairs[i].first;
      const std::string &yielded = loop.yieldedValues[i];
      if (iterArg == yielded)
        continue;
      const size_t yieldIndex = static_cast<size_t>(loop.yield);
      auto yieldedBase =
          canonicalAlloc(aliasBefore[yieldIndex], allocNames, yielded);
      if (!yieldedBase)
        continue;
      auto def = allocDef.find(*yieldedBase);
      if (def == allocDef.end() || def->second <= loop.begin ||
          def->second >= loop.yield)
        continue;

      int firstInitialization = -1;
      for (int opIndex = def->second + 1; opIndex < loop.yield; ++opIndex) {
        const size_t operationIndex = static_cast<size_t>(opIndex);
        std::vector<std::string> written =
            writtenValues(input[operationIndex]);
        for (const std::string &value : written) {
          auto base =
              canonicalAlloc(aliasBefore[operationIndex], allocNames, value);
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
        const std::string iterRoot =
            canonical(aliasBefore[operationIndex], iterArg);
        for (const std::string &value : readValues(input[operationIndex])) {
          if (canonical(aliasBefore[operationIndex], value) == iterRoot) {
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

  std::vector<OperationRecord> result;
  int nextOperationId = 0;
  for (const OperationRecord &op : input)
    nextOperationId = std::max(nextOperationId, op.operationId + 1);
  for (size_t i = 0; i < input.size(); ++i) {
    OperationRecord rewritten = input[i];
    auto rewriteIt = rewrites.find(static_cast<int>(i));
    if (rewriteIt != rewrites.end()) {
      for (const Rewrite &rewrite : rewriteIt->second) {
        OperationRecord copy;
        copy.line = input[i].line;
        copy.indent = input[i].indent;
        copy.operationId = nextOperationId++;
        copy.regionPath = input[i].regionPath;
        copy.blockId = input[i].blockId;
        copy.blockLabel = input[i].blockLabel;
        copy.blockArguments = input[i].blockArguments;
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
    for (const auto &[from, to] : aliases) {
      operation.text = replaceSSAUse(operation.text, from, to);
      if (!operation.normalizationKey.empty())
        operation.normalizationKey =
            replaceSSAUse(operation.normalizationKey, from, to);
      for (BranchDestination &destination : operation.branchDestinations)
        for (std::string &operand : destination.operands)
          if (operand == from)
            operand = to;
    }
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
  bool changed = true;
  while (changed) {
    changed = false;
    std::map<std::string, size_t> useCounts;
    for (const OperationRecord &operation : operations)
      for (const std::string &operand : operationOperandNames(operation))
        ++useCounts[operand];

    std::vector<OperationRecord> retained;
    retained.reserve(operations.size());
    for (OperationRecord &operation : operations) {
      std::vector<std::string> results = operationResultNames(operation);
      bool unused = !results.empty() &&
                    std::all_of(results.begin(), results.end(),
                                [&](const std::string &result) {
                                  return useCounts[result] == 0;
                                });
      if (unused && isKnownPureNormalizeOperation(operation)) {
        changed = true;
        continue;
      }
      retained.push_back(std::move(operation));
    }
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
  operations = std::move(nonConstants);
  for (const BlockKey &key : blockOrder) {
    auto insertion = std::find_if(
        operations.begin(), operations.end(), [&](const OperationRecord &op) {
          return op.regionPath == key.first && op.blockId == key.second;
        });
    std::vector<OperationRecord> &constants = constantsByBlock[key];
    operations.insert(insertion,
                      std::make_move_iterator(constants.begin()),
                      std::make_move_iterator(constants.end()));
  }
  for (size_t i = 0; i < operations.size(); ++i)
    operations[i].index = static_cast<int>(i);
  return operations;
}


} // namespace cvub

#endif
