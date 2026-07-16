#ifndef CVPIPELINE_UB_MODEL_CPP_AFFINE_MIN_MAX_CANONICALIZATION_HPP
#define CVPIPELINE_UB_MODEL_CPP_AFFINE_MIN_MAX_CANONICALIZATION_HPP

#include "../ir/generic_rewriter.hpp"
#include "convert_arith_to_affine.hpp"

namespace cvub {

inline bool AffineExpressionUsesValue(const std::string &expression,
                                      int value) {
  return expression.find(AffineValueExpression(value)) != std::string::npos;
}

inline std::string ReplaceAffineExpressionValue(
    const std::string &expression, int value,
    const std::string &replacement) {
  if (AffineValue(expression) == value)
    return replacement;
  for (const std::string &kind : {"mul", "add", "mod", "floordiv",
                                  "ceildiv"}) {
    const auto operands = AffineBinaryExpressionOperands(expression, kind);
    if (!operands)
      continue;
    return MakeAffineBinaryExpression(
        kind,
        ReplaceAffineExpressionValue(operands->first, value, replacement),
        ReplaceAffineExpressionValue(operands->second, value, replacement));
  }
  return expression;
}

inline std::optional<std::vector<std::string>>
ExistingAffineMinMaxExpressions(const GenericOperation &operation) {
  if (operation.name != "affine.min" && operation.name != "affine.max")
    return std::nullopt;
  std::string map = FindDictionaryValue(operation.properties, "map");
  if (map.empty())
    map = FindDictionaryValue(operation.attributes, "map");
  const std::string prefix = operation.name + "(";
  if (!startsWith(map, prefix) || map.back() != ')')
    return std::nullopt;
  const std::string body =
      map.substr(prefix.size(), map.size() - prefix.size() - 1);
  std::vector<std::string> expressions;
  size_t begin = 0;
  int depth = 0;
  for (size_t index = 0; index <= body.size(); ++index) {
    if (index < body.size() && body[index] == '(')
      ++depth;
    else if (index < body.size() && body[index] == ')')
      --depth;
    if (index != body.size() && (body[index] != ',' || depth != 0))
      continue;
    expressions.push_back(body.substr(begin, index - begin));
    begin = index + 1;
  }
  return expressions;
}

// The greedy canonicalizer visits affine.apply producers before their later
// min/max consumers. Record identity/constant folds first so a consumer sees
// the same producer value that MergeAffineMinMaxOp sees in MLIR.
inline void RunAffineApplyCanonicalization(
    const GenericModule &module, ConvertArithToAffineState &state) {
  std::map<int, int> valueAliases;
  auto resolveAlias = [&](int value) {
    std::set<int> visited;
    while (valueAliases.count(value) != 0 && visited.insert(value).second)
      value = valueAliases.at(value);
    return value;
  };
  std::function<std::string(const std::string &)> rewriteExpression;
  rewriteExpression = [&](const std::string &expression) -> std::string {
    if (const std::optional<int> value = AffineValue(expression))
      return AffineValueExpression(resolveAlias(*value));
    for (const std::string &kind : {"mul", "add", "mod", "floordiv",
                                    "ceildiv"}) {
      const auto operands = AffineBinaryExpressionOperands(expression, kind);
      if (!operands)
        continue;
      return MakeAffineBinaryExpression(kind,
                                        rewriteExpression(operands->first),
                                        rewriteExpression(operands->second));
    }
    return expression;
  };

  for (const GenericOperation &operation : module.operations) {
    auto name = state.replacementNames.find(operation.id);
    if (name == state.replacementNames.end())
      continue;
    std::vector<std::string> &expressions =
        state.replacementMapExpressions.at(operation.id);
    for (std::string &expression : expressions)
      expression = rewriteExpression(expression);
    std::vector<int> &operands = state.replacementOperands.at(operation.id);
    std::vector<int> canonicalOperands;
    for (int operand : operands) {
      operand = resolveAlias(operand);
      if (std::any_of(expressions.begin(), expressions.end(),
                      [&](const std::string &expression) {
                        return AffineExpressionUsesValue(expression, operand);
                      }))
        AppendUniqueAffineOperand(operand, canonicalOperands);
    }
    operands = std::move(canonicalOperands);

    if (name->second != "affine.apply" || expressions.size() != 1 ||
        operation.results.size() != 1)
      continue;
    if (const std::optional<int64_t> constant =
            AffineConstantValue(expressions.front())) {
      state.foldedConstants[operation.id] = *constant;
      continue;
    }
    const std::optional<int> value = AffineValue(expressions.front());
    if (!value)
      continue;
    const int resolved = resolveAlias(*value);
    state.foldedValues[operation.id] = resolved;
    valueAliases[operation.results.front()] = resolved;
  }
}

// SimplifyAffineOp composes affine.apply operands one at a time. MLIR marks
// the current operand null, appends the producer operands, and restarts its
// scan; canonicalizeMapAndOperands only prunes and deduplicates afterwards.
inline void RunAffineApplyCompositionForMinMax(
    const GenericModule &module, ConvertArithToAffineState &state) {
  std::map<int, const GenericOperation *> definitions;
  for (const GenericOperation &operation : module.operations)
    for (int result : operation.results)
      definitions[result] = &operation;

  for (const GenericOperation &operation : module.operations) {
    auto name = state.replacementNames.find(operation.id);
    if (name == state.replacementNames.end() ||
        (name->second != "affine.min" && name->second != "affine.max"))
      continue;
    std::vector<int> &operands = state.replacementOperands.at(operation.id);
    std::vector<std::string> &expressions =
        state.replacementMapExpressions.at(operation.id);
    while (true) {
      bool changed = false;
      for (size_t operandIndex = 0; operandIndex < operands.size();
           ++operandIndex) {
        const int operand = operands[operandIndex];
        auto definition = definitions.find(operand);
        if (definition == definitions.end())
          continue;
        const GenericOperation &producer = *definition->second;
        auto replacement = state.replacementNames.find(producer.id);
        const bool convertedApply =
            replacement != state.replacementNames.end() &&
            replacement->second == "affine.apply" &&
            state.foldedValues.count(producer.id) == 0 &&
            state.foldedConstants.count(producer.id) == 0;
        const std::optional<std::string> existingApply =
            ExistingAffineApplyExpression(producer);
        if (!convertedApply && !existingApply)
          continue;

        const std::string producerExpression =
            convertedApply
                ? state.replacementMapExpressions.at(producer.id).front()
                : *existingApply;
        const std::vector<int> &producerOperands =
            convertedApply ? state.replacementOperands.at(producer.id)
                           : producer.operands;
        operands.erase(operands.begin() +
                       static_cast<std::ptrdiff_t>(operandIndex));
        for (int producerOperand : producerOperands)
          operands.push_back(producerOperand);
        for (std::string &expression : expressions)
          expression = ReplaceAffineExpressionValue(
              expression, operand, producerExpression);
        changed = true;
        break;
      }
      if (!changed)
        break;
    }

    std::vector<int> canonicalOperands;
    for (int operand : operands)
      if (std::any_of(expressions.begin(), expressions.end(),
                      [&](const std::string &expression) {
                        return AffineExpressionUsesValue(expression, operand);
                      }))
        AppendUniqueAffineOperand(operand, canonicalOperands);
    operands = std::move(canonicalOperands);
  }
}

// UB planning only observes the surviving operation and SSA operand graph, not
// the affine map text. This projects MLIR's MergeAffineMinMaxOp pattern onto
// exactly that observable graph: a same-kind producer used as a standalone
// min/max expression is replaced by the producer's canonical map operands.
inline void RunAffineMinMaxCanonicalization(
    const GenericModule &module, ConvertArithToAffineState &state) {
  std::map<int, const GenericOperation *> definitions;
  for (const GenericOperation &operation : module.operations)
    for (int result : operation.results)
      definitions[result] = &operation;

  for (const GenericOperation &operation : module.operations) {
    auto name = state.replacementNames.find(operation.id);
    if (name == state.replacementNames.end() ||
        (name->second != "affine.min" && name->second != "affine.max"))
      continue;
    std::vector<int> mergedOperands =
        state.replacementOperands.at(operation.id);
    std::vector<std::string> mergedExpressions;
    const std::vector<std::string> &sourceExpressions =
        state.replacementMapExpressions.at(operation.id);
    for (size_t index = 0; index < sourceExpressions.size(); ++index) {
      const std::optional<int> standaloneValue =
          AffineValue(sourceExpressions[index]);
      auto definition = standaloneValue ? definitions.find(*standaloneValue)
                                        : definitions.end();
      auto producerName =
          definition == definitions.end()
              ? state.replacementNames.end()
              : state.replacementNames.find(definition->second->id);
      const bool convertedSameKind =
          producerName != state.replacementNames.end() &&
          producerName->second == name->second;
      const auto existingExpressions =
          definition == definitions.end()
              ? std::optional<std::vector<std::string>>()
              : ExistingAffineMinMaxExpressions(*definition->second);
      const bool existingSameKind =
          definition != definitions.end() &&
          definition->second->name == name->second && existingExpressions;
      if (standaloneValue && (convertedSameKind || existingSameKind)) {
        mergedOperands.erase(
            std::remove(mergedOperands.begin(), mergedOperands.end(),
                        *standaloneValue),
            mergedOperands.end());
        const std::vector<int> &producerOperands =
            convertedSameKind
                ? state.replacementOperands.at(definition->second->id)
                : definition->second->operands;
        for (int producerOperand : producerOperands)
          AppendUniqueAffineOperand(producerOperand, mergedOperands);
        const std::vector<std::string> &producerExpressions =
            convertedSameKind
                ? state.replacementMapExpressions.at(definition->second->id)
                : *existingExpressions;
        mergedExpressions.insert(mergedExpressions.end(),
                                 producerExpressions.begin(),
                                 producerExpressions.end());
      } else {
        mergedExpressions.push_back(sourceExpressions[index]);
      }
    }
    std::sort(mergedExpressions.begin(), mergedExpressions.end());
    mergedExpressions.erase(
        std::unique(mergedExpressions.begin(), mergedExpressions.end()),
        mergedExpressions.end());
    // MergeAffineMinMaxOp is followed by SimplifyAffineOp, whose
    // canonicalizeMapAndOperands step removes map operands that no surviving
    // expression references.
    mergedOperands.erase(
        std::remove_if(
            mergedOperands.begin(), mergedOperands.end(),
            [&](int operand) {
              return std::none_of(
                  mergedExpressions.begin(), mergedExpressions.end(),
                  [&](const std::string &expression) {
                    return AffineExpressionUsesValue(expression, operand);
                  });
            }),
        mergedOperands.end());
    state.replacementMapExpressions[operation.id] =
        std::move(mergedExpressions);
    state.replacementOperands[operation.id] = std::move(mergedOperands);
  }
}

// createCSEPass runs after affine conversion/canonicalization in
// canonicalizationHIVMPipeline. CSE only reuses an equivalent dominating op.
inline void RunAffineCanonicalizationCSE(
    const GenericModule &module, ConvertArithToAffineState &state) {
  std::map<std::string, std::vector<const GenericOperation *>> canonical;
  std::map<int, int> valueAliases;
  for (const GenericOperation &operation : module.operations) {
    auto folded = state.foldedValues.find(operation.id);
    if (folded != state.foldedValues.end() && operation.results.size() == 1)
      valueAliases[operation.results.front()] = folded->second;
  }
  auto resolveAlias = [&](int value) {
    std::set<int> visited;
    while (valueAliases.count(value) != 0 && visited.insert(value).second)
      value = valueAliases.at(value);
    return value;
  };
  std::function<std::string(const std::string &)> rewriteExpression;
  rewriteExpression = [&](const std::string &expression) -> std::string {
    if (const std::optional<int> value = AffineValue(expression))
      return AffineValueExpression(resolveAlias(*value));
    for (const std::string &kind : {"mul", "add", "mod", "floordiv",
                                    "ceildiv"}) {
      const auto operands = AffineBinaryExpressionOperands(expression, kind);
      if (!operands)
        continue;
      return MakeAffineBinaryExpression(kind,
                                        rewriteExpression(operands->first),
                                        rewriteExpression(operands->second));
    }
    return expression;
  };
  for (const GenericOperation &operation : module.operations) {
    if (operation.results.size() != 1)
      continue;

    const auto replacementName = state.replacementNames.find(operation.id);
    const bool converted = replacementName != state.replacementNames.end();
    std::string name;
    std::vector<std::string> canonicalExpressions;
    if (converted) {
      name = replacementName->second;
      canonicalExpressions =
          state.replacementMapExpressions.at(operation.id);
    } else if (const std::optional<std::string> expression =
                   ExistingAffineApplyExpression(operation)) {
      name = "affine.apply";
      canonicalExpressions.push_back(*expression);
    } else if (const auto expressions =
                   ExistingAffineMinMaxExpressions(operation)) {
      name = operation.name;
      canonicalExpressions = *expressions;
    } else {
      continue;
    }

    for (std::string &expression : canonicalExpressions)
      expression = rewriteExpression(expression);
    if (name == "affine.min" || name == "affine.max")
      std::sort(canonicalExpressions.begin(), canonicalExpressions.end());

    if (converted) {
      state.replacementMapExpressions.at(operation.id) = canonicalExpressions;
      // canonicalizeMapAndOperands runs after affine.apply composition as well
      // as after min/max merging. Composition can alias two symbols and
      // simplify them out of the map, so canonicalize the operand list again.
      std::vector<int> &operands = state.replacementOperands.at(operation.id);
      for (int &operand : operands)
        operand = resolveAlias(operand);
      std::vector<int> canonicalOperands;
      for (int operand : operands)
        if (std::any_of(canonicalExpressions.begin(),
                        canonicalExpressions.end(),
                        [&](const std::string &expression) {
                          return AffineExpressionUsesValue(expression, operand);
                        }))
          AppendUniqueAffineOperand(operand, canonicalOperands);
      operands = std::move(canonicalOperands);
    }

    if (converted && canonicalExpressions.size() == 1) {
      if (const std::optional<int64_t> constant =
              AffineConstantValue(canonicalExpressions.front()))
        state.foldedConstants[operation.id] = *constant;
      if (const std::optional<int> value =
              AffineValue(canonicalExpressions.front())) {
        state.foldedValues[operation.id] = *value;
        valueAliases[operation.results.front()] = *value;
      }
    }
    if (state.foldedConstants.count(operation.id) != 0 ||
        state.foldedValues.count(operation.id) != 0)
      continue;

    const std::string key =
        name + "(" + JoinDelimited(canonicalExpressions, ",") + ")";
    auto &candidates = canonical[key];
    const GenericOperation *dominating = nullptr;
    for (auto candidate = candidates.rbegin(); candidate != candidates.rend();
         ++candidate)
      if (GenericOperationDominates(module, **candidate, operation)) {
        dominating = *candidate;
        break;
      }
    if (!dominating) {
      candidates.push_back(&operation);
      continue;
    }
    state.cseValueAliases[operation.results.front()] =
        dominating->results.front();
    valueAliases[operation.results.front()] = dominating->results.front();
    state.erasedOperations.insert(operation.id);
  }
}

// Materialize the operation replacements produced by
// ArithToAffineConversionPass. The analysis above mirrors affine expression
// composition; this step mirrors the pass rewriter so subsequent CSE, LICM and
// OneShotBufferize observe affine operations instead of the replaced arith
// operations.
inline GenericModule MaterializeArithToAffineConversion(
    GenericModule module, ConvertArithToAffineState state) {

  std::map<int, int> valueAliases = state.cseValueAliases;
  for (const auto &[operationId, value] : state.foldedValues) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (!operation.results.empty())
      valueAliases[operation.results.front()] = value;
  }
  auto resolveAlias = [&](int value) {
    std::set<int> visited;
    while (valueAliases.count(value) != 0 && visited.insert(value).second)
      value = valueAliases.at(value);
    return value;
  };
  for (GenericOperation &operation : module.operations) {
    for (int &operand : operation.operands)
      operand = resolveAlias(operand);
    for (int &operand : operation.dpsInputs)
      operand = resolveAlias(operand);
    for (int &operand : operation.dpsInits)
      operand = resolveAlias(operand);
  }

  GenericRewriter rewriter(module);
  for (GenericOperation &operation : module.operations) {
    auto replacement = state.replacementNames.find(operation.id);
    if (replacement == state.replacementNames.end())
      continue;
    if (state.foldedValues.count(operation.id) != 0 ||
        (!operation.results.empty() &&
         state.cseValueAliases.count(operation.results.front()) != 0))
      continue;
    auto foldedConstant = state.foldedConstants.find(operation.id);
    if (foldedConstant != state.foldedConstants.end()) {
      operation.name = "arith.constant";
      operation.operands.clear();
      operation.operandTypes.clear();
      operation.properties =
          "{value = " + std::to_string(foldedConstant->second) +
          " : index}";
      operation.attributes = operation.properties;
      operation.effects = "none";
      continue;
    }
    operation.name = replacement->second;
    operation.operands = state.replacementOperands.at(operation.id);
    for (int &operand : operation.operands)
      operand = resolveAlias(operand);
    operation.operandTypes.assign(operation.operands.size(), "index");
    operation.properties =
        "{map = " + AffineMapSemanticKey(operation, state) + "}";
    operation.attributes = operation.properties;
    operation.effects = "none";
  }

  std::set<int> erased = state.erasedOperations;
  for (const auto &[operation, value] : state.foldedValues) {
    (void)value;
    erased.insert(operation);
  }
  for (int operationId : erased) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.blockId >= 0)
      rewriter.removeFromBlock(operation.blockId, operationId);
  }
  return CompactGenericModule(std::move(module));
}

inline GenericModule RunArithToAffineConversionPass(GenericModule module) {
  ConvertArithToAffineState state = RunConvertArithToAffine(module, false);
  return MaterializeArithToAffineConversion(std::move(module),
                                             std::move(state));
}

inline GenericModule
RunArithToAffineConversionWithAffineCanonicalization(GenericModule module) {
  ConvertArithToAffineState state = RunConvertArithToAffine(module, false);
  RunAffineApplyCanonicalization(module, state);
  RunAffineMinMaxCanonicalization(module, state);
  RunAffineApplyCompositionForMinMax(module, state);
  RunAffineMinMaxCanonicalization(module, state);
  RunAffineCanonicalizationCSE(module, state);
  return MaterializeArithToAffineConversion(std::move(module),
                                             std::move(state));
}

inline GenericModule RunArithToAffineConversion(GenericModule module) {
  ConvertArithToAffineState state = RunConvertArithToAffine(module);
  RunAffineMinMaxCanonicalization(module, state);
  RunAffineCanonicalizationCSE(module, state);
  return MaterializeArithToAffineConversion(std::move(module),
                                             std::move(state));
}

} // namespace cvub

#endif
