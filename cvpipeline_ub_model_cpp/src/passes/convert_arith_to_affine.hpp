#ifndef CVPIPELINE_UB_MODEL_CPP_CONVERT_ARITH_TO_AFFINE_HPP
#define CVPIPELINE_UB_MODEL_CPP_CONVERT_ARITH_TO_AFFINE_HPP

#include "../pipeline/buffer_topology.hpp"

namespace cvub {

struct ConvertArithToAffineState {
  std::map<int, std::string> replacementNames;
  std::map<int, std::vector<int>> replacementOperands;
  std::map<int, std::vector<std::string>> replacementMapExpressions;
  std::map<int, int64_t> foldedConstants;
  std::map<int, int> foldedValues;
  std::map<int, int> cseValueAliases;
  std::set<int> composedApplyOperations;
  std::set<int> erasedOperations;
};

inline bool IsArithToAffineCandidate(const GenericOperation &operation) {
  static const std::set<std::string> candidates = {
      "arith.addi", "arith.subi", "arith.muli", "arith.ceildivsi",
      "arith.divsi", "arith.remsi", "arith.maxsi", "arith.maxui",
      "arith.minsi", "arith.minui"};
  return candidates.count(operation.name) != 0 &&
         operation.operands.size() == 2 && operation.operandTypes.size() == 2 &&
         operation.operandTypes[0] == "index" &&
         operation.operandTypes[1] == "index";
}

inline std::string ArithToAffineReplacementName(
    const GenericOperation &operation) {
  if (!IsArithToAffineCandidate(operation))
    return {};
  if (operation.name == "arith.maxsi" || operation.name == "arith.maxui")
    return "affine.max";
  if (operation.name == "arith.minsi" || operation.name == "arith.minui")
    return "affine.min";
  return "affine.apply";
}

inline bool IsIndexConstant(const GenericOperation &operation) {
  return operation.name == "arith.constant" && operation.results.size() == 1 &&
         operation.resultTypes.size() == 1 &&
         operation.resultTypes.front() == "index";
}

inline void AppendUniqueAffineOperand(int value, std::vector<int> &operands) {
  if (std::find(operands.begin(), operands.end(), value) == operands.end())
    operands.push_back(value);
}

inline std::string IndexConstantExpression(
    const GenericOperation &operation) {
  std::string literal = FindDictionaryValue(operation.properties, "value");
  if (literal.empty())
    literal = FindDictionaryValue(operation.attributes, "value");
  const size_t typedSuffix = literal.find(" : ");
  if (typedSuffix != std::string::npos)
    literal = trim(literal.substr(0, typedSuffix));
  return "c(" + literal + ")";
}

inline std::string AffineValueExpression(int value) {
  return "v(" + std::to_string(value) + ")";
}

inline std::optional<std::string>
ExistingAffineApplyExpression(const GenericOperation &operation) {
  if (operation.name != "affine.apply")
    return std::nullopt;
  std::string map = FindDictionaryValue(operation.properties, "map");
  if (map.empty())
    map = FindDictionaryValue(operation.attributes, "map");
  const std::string prefix = "affine.apply(";
  if (!startsWith(map, prefix) || map.back() != ')')
    return std::nullopt;
  return map.substr(prefix.size(), map.size() - prefix.size() - 1);
}

inline std::optional<int64_t> AffineConstantValue(
    const std::string &expression) {
  if (!startsWith(expression, "c(") || expression.back() != ')')
    return std::nullopt;
  try {
    size_t consumed = 0;
    const std::string literal =
        expression.substr(2, expression.size() - 3);
    const int64_t value = std::stoll(literal, &consumed, 0);
    return consumed == literal.size() ? std::optional<int64_t>(value)
                                      : std::nullopt;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline std::optional<std::pair<std::string, std::string>>
AffineBinaryExpressionOperands(const std::string &expression,
                               const std::string &kind) {
  const std::string prefix = kind + "(";
  if (!startsWith(expression, prefix) || expression.back() != ')')
    return std::nullopt;
  const size_t begin = prefix.size();
  int depth = 0;
  for (size_t index = begin; index + 1 < expression.size(); ++index) {
    if (expression[index] == '(')
      ++depth;
    else if (expression[index] == ')')
      --depth;
    else if (expression[index] == ',' && depth == 0)
      return std::make_pair(expression.substr(begin, index - begin),
                            expression.substr(index + 1,
                                              expression.size() - index - 2));
  }
  return std::nullopt;
}

// Mirrors AffineExpr::getLargestKnownDivisor for the expression subset
// produced by ConvertArithToAffine.
inline int64_t AffineLargestKnownDivisor(const std::string &expression) {
  if (const std::optional<int64_t> constant =
          AffineConstantValue(expression))
    return *constant == std::numeric_limits<int64_t>::min()
               ? std::numeric_limits<int64_t>::max()
               : std::abs(*constant);
  if (startsWith(expression, "v("))
    return 1;
  for (const std::string &kind : {"mul", "add", "mod", "floordiv",
                                  "ceildiv"}) {
    const auto operands = AffineBinaryExpressionOperands(expression, kind);
    if (!operands)
      continue;
    const int64_t lhs = AffineLargestKnownDivisor(operands->first);
    const int64_t rhs = AffineLargestKnownDivisor(operands->second);
    if (kind == "mul") {
      const __int128 product = static_cast<__int128>(lhs) * rhs;
      return product > std::numeric_limits<int64_t>::max()
                 ? 1
                 : static_cast<int64_t>(product);
    }
    if (kind == "add" || kind == "mod")
      return static_cast<int64_t>(
          std::gcd(static_cast<uint64_t>(lhs), static_cast<uint64_t>(rhs)));
    const std::optional<int64_t> divisor =
        AffineConstantValue(operands->second);
    if (divisor && *divisor != 0 && lhs % *divisor == 0)
      return std::abs(lhs / *divisor);
    return 1;
  }
  return 1;
}

inline std::optional<int64_t> CheckedAffineArithmetic(
    const std::string &kind, int64_t lhs, int64_t rhs) {
  if (kind == "add" || kind == "mul") {
    const __int128 value = kind == "add"
                               ? static_cast<__int128>(lhs) + rhs
                               : static_cast<__int128>(lhs) * rhs;
    if (value < std::numeric_limits<int64_t>::min() ||
        value > std::numeric_limits<int64_t>::max())
      return std::nullopt;
    return static_cast<int64_t>(value);
  }
  if (rhs <= 0)
    return std::nullopt;
  int64_t quotient = lhs / rhs;
  const int64_t remainder = lhs % rhs;
  if (kind == "mod")
    return remainder < 0 ? remainder + rhs : remainder;
  if (kind == "floordiv")
    return remainder < 0 ? quotient - 1 : quotient;
  if (kind == "ceildiv")
    return remainder > 0 ? quotient + 1 : quotient;
  return std::nullopt;
}

inline std::optional<int> AffineValue(const std::string &expression) {
  if (!startsWith(expression, "v(") || expression.back() != ')')
    return std::nullopt;
  try {
    size_t consumed = 0;
    const std::string literal =
        expression.substr(2, expression.size() - 3);
    const int value = std::stoi(literal, &consumed);
    return consumed == literal.size() ? std::optional<int>(value)
                                      : std::nullopt;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

struct AffineLinearForm {
  std::map<std::string, int64_t> coefficients;
  int64_t constant = 0;
};

inline bool AccumulateAffineCoefficient(int64_t value, int64_t scale,
                                        int64_t &target) {
  const __int128 result = static_cast<__int128>(value) * scale + target;
  if (result < std::numeric_limits<int64_t>::min() ||
      result > std::numeric_limits<int64_t>::max())
    return false;
  target = static_cast<int64_t>(result);
  return true;
}

// Projection of simplifyAffineExpr's flatten-and-reconstruct behavior for the
// add and constant-multiply expressions emitted by ArithToAffineConversionPass.
// Non-linear subexpressions remain opaque terms, while equal terms still share
// one coefficient just as the affine flattener shares one local expression.
inline std::optional<AffineLinearForm>
FlattenAffineLinearExpression(const std::string &expression) {
  if (const std::optional<int64_t> constant =
          AffineConstantValue(expression))
    return AffineLinearForm{{}, *constant};

  if (const auto add = AffineBinaryExpressionOperands(expression, "add")) {
    auto lhs = FlattenAffineLinearExpression(add->first);
    auto rhs = FlattenAffineLinearExpression(add->second);
    if (!lhs || !rhs ||
        !AccumulateAffineCoefficient(rhs->constant, 1, lhs->constant))
      return std::nullopt;
    for (const auto &[term, coefficient] : rhs->coefficients)
      if (!AccumulateAffineCoefficient(coefficient, 1,
                                       lhs->coefficients[term]))
        return std::nullopt;
    return lhs;
  }

  if (const auto multiply =
          AffineBinaryExpressionOperands(expression, "mul")) {
    std::optional<int64_t> scale = AffineConstantValue(multiply->first);
    std::string operand = multiply->second;
    if (!scale) {
      scale = AffineConstantValue(multiply->second);
      operand = multiply->first;
    }
    if (scale) {
      auto form = FlattenAffineLinearExpression(operand);
      if (!form)
        return std::nullopt;
      int64_t scaledConstant = 0;
      if (!AccumulateAffineCoefficient(form->constant, *scale,
                                       scaledConstant))
        return std::nullopt;
      form->constant = scaledConstant;
      for (auto &[term, coefficient] : form->coefficients) {
        int64_t scaledCoefficient = 0;
        if (!AccumulateAffineCoefficient(coefficient, *scale,
                                         scaledCoefficient))
          return std::nullopt;
        coefficient = scaledCoefficient;
      }
      return form;
    }
  }

  return AffineLinearForm{{{expression, 1}}, 0};
}

inline std::string ReconstructAffineLinearExpression(AffineLinearForm form) {
  std::string result;
  for (const auto &[term, coefficient] : form.coefficients) {
    if (coefficient == 0)
      continue;
    const std::string scaled =
        coefficient == 1
            ? term
            : "mul(" + term + ",c(" + std::to_string(coefficient) + "))";
    result = result.empty() ? scaled : "add(" + result + "," + scaled + ")";
  }
  if (form.constant != 0 || result.empty()) {
    const std::string constant = "c(" + std::to_string(form.constant) + ")";
    result = result.empty() ? constant
                            : "add(" + result + "," + constant + ")";
  }
  return result;
}

inline std::string MakeAffineBinaryExpression(
    const std::string &kind, std::string lhs, std::string rhs);

inline std::optional<std::string>
DivideKnownMultiple(const std::string &expression, int64_t divisor) {
  if (divisor <= 0)
    return std::nullopt;
  const auto multiply = AffineBinaryExpressionOperands(expression, "mul");
  if (!multiply)
    return std::nullopt;
  for (const auto &[factor, other] :
       {std::pair<std::string, std::string>{multiply->first, multiply->second},
        {multiply->second, multiply->first}}) {
    const std::optional<int64_t> constant = AffineConstantValue(factor);
    if (!constant || *constant % divisor != 0)
      continue;
    return MakeAffineBinaryExpression(
        "mul", "c(" + std::to_string(*constant / divisor) + ")", other);
  }
  return std::nullopt;
}

inline std::string MakeAffineBinaryExpression(
    const std::string &kind, std::string lhs, std::string rhs) {
  const std::optional<int64_t> lhsConstant = AffineConstantValue(lhs);
  const std::optional<int64_t> rhsConstant = AffineConstantValue(rhs);
  if (lhsConstant && rhsConstant)
    if (const std::optional<int64_t> folded =
            CheckedAffineArithmetic(kind, *lhsConstant, *rhsConstant))
      return "c(" + std::to_string(*folded) + ")";
  if (kind == "add" || kind == "mul") {
    const std::string expression = kind + "(" + lhs + "," + rhs + ")";
    if (auto linear = FlattenAffineLinearExpression(expression))
      return ReconstructAffineLinearExpression(std::move(*linear));
  }
  if (kind == "add") {
    if (lhsConstant && *lhsConstant == 0)
      return rhs;
    if (rhsConstant && *rhsConstant == 0)
      return lhs;
    if (rhs < lhs)
      std::swap(lhs, rhs);
  } else if (kind == "mul") {
    if ((lhsConstant && *lhsConstant == 0) ||
        (rhsConstant && *rhsConstant == 0))
      return "c(0)";
    if (lhsConstant && *lhsConstant == 1)
      return rhs;
    if (rhsConstant && *rhsConstant == 1)
      return lhs;
    if (rhs < lhs)
      std::swap(lhs, rhs);
  } else if (kind == "mod" && rhsConstant && *rhsConstant > 0 &&
             AffineLargestKnownDivisor(lhs) % *rhsConstant == 0) {
    // AffineExpr::simplifyMod folds a known multiple of the modulus to zero.
    return "c(0)";
  } else if (kind == "mod" && rhsConstant && *rhsConstant > 0) {
    // Mirror AffineExpr::simplifyMod: discard an additive term that is known
    // to be a multiple of the positive modulus.
    if (const auto add = AffineBinaryExpressionOperands(lhs, "add")) {
      if (AffineLargestKnownDivisor(add->first) % *rhsConstant == 0)
        return MakeAffineBinaryExpression("mod", add->second, rhs);
      if (AffineLargestKnownDivisor(add->second) % *rhsConstant == 0)
        return MakeAffineBinaryExpression("mod", add->first, rhs);
    }

    // Mirror (e mod a) mod b -> e mod b when b evenly divides a.
    if (const auto innerMod = AffineBinaryExpressionOperands(lhs, "mod")) {
      const std::optional<int64_t> innerModulus =
          AffineConstantValue(innerMod->second);
      if (innerModulus && *innerModulus >= 1 &&
          *innerModulus % *rhsConstant == 0)
        return MakeAffineBinaryExpression("mod", innerMod->first, rhs);
    }
  } else if ((kind == "floordiv" || kind == "ceildiv") && rhsConstant) {
    if (const std::optional<std::string> divided =
            DivideKnownMultiple(lhs, *rhsConstant))
      return *divided;
    // AffineExpr::simplifyFloorDiv distributes floor division over an add
    // when either term is known to be a multiple of the positive divisor.
    // The recursive calls then fold that term and any constant remainder.
    if (kind == "floordiv" && *rhsConstant > 0) {
      if (const auto add = AffineBinaryExpressionOperands(lhs, "add")) {
        const int64_t lhsDivisor =
            AffineLargestKnownDivisor(add->first);
        const int64_t rhsDivisor =
            AffineLargestKnownDivisor(add->second);
        if (lhsDivisor % *rhsConstant == 0 ||
            rhsDivisor % *rhsConstant == 0)
          return MakeAffineBinaryExpression(
              "add",
              MakeAffineBinaryExpression("floordiv", add->first, rhs),
              MakeAffineBinaryExpression("floordiv", add->second, rhs));
      }
    }
  }
  return kind + "(" + lhs + "," + rhs + ")";
}

inline std::optional<int64_t> FoldAffineReplacement(
    const std::string &name, const std::vector<std::string> &expressions) {
  if (expressions.empty())
    return std::nullopt;
  std::optional<int64_t> folded;
  for (const std::string &expression : expressions) {
    const std::optional<int64_t> value = AffineConstantValue(expression);
    if (!value)
      return std::nullopt;
    if (!folded)
      folded = value;
    else if (name == "affine.min")
      folded = std::min(*folded, *value);
    else if (name == "affine.max")
      folded = std::max(*folded, *value);
    else
      return std::nullopt;
  }
  return folded;
}

inline std::string AffineBinaryOperationExpression(
    const GenericOperation &operation, const std::string &lhs,
    const std::string &rhs) {
  if (operation.name == "arith.addi")
    return MakeAffineBinaryExpression("add", lhs, rhs);
  if (operation.name == "arith.subi")
    return MakeAffineBinaryExpression(
        "add", lhs, MakeAffineBinaryExpression("mul", "c(-1)", rhs));
  if (operation.name == "arith.muli")
    return MakeAffineBinaryExpression("mul", lhs, rhs);
  if (operation.name == "arith.ceildivsi")
    return MakeAffineBinaryExpression("ceildiv", lhs, rhs);
  if (operation.name == "arith.divsi")
    return MakeAffineBinaryExpression("floordiv", lhs, rhs);
  if (operation.name == "arith.remsi")
    return MakeAffineBinaryExpression("mod", lhs, rhs);
  throw std::runtime_error(
      "ConvertArithToAffine: unsupported affine.apply expression");
}

inline std::string AffineMapSemanticKey(
    const GenericOperation &operation,
    const ConvertArithToAffineState &state) {
  auto expressions = state.replacementMapExpressions.find(operation.id);
  if (expressions == state.replacementMapExpressions.end())
    throw std::runtime_error(
        "ConvertArithToAffine: missing replacement map expressions");
  std::vector<std::string> canonical = expressions->second;
  const std::string &name = state.replacementNames.at(operation.id);
  if (name == "affine.min" || name == "affine.max")
    std::sort(canonical.begin(), canonical.end());
  return name + "(" + JoinDelimited(canonical, ",") + ")";
}

inline ConvertArithToAffineState
RunConvertArithToAffine(const GenericModule &module,
                        bool composeConversionProducers = true) {
  ConvertArithToAffineState result;
  std::map<int, const GenericOperation *> definitions;
  std::map<int, std::set<int>> users;
  for (const GenericOperation &operation : module.operations) {
    if (const std::string name = ArithToAffineReplacementName(operation);
        !name.empty())
      result.replacementNames[operation.id] = name;
    for (int value : operation.results)
      definitions[value] = &operation;
    for (int operand : operation.operands)
      users[operand].insert(operation.id);
  }

  std::map<int, std::vector<std::string>> expressionCache;
  std::set<int> expressionStack;
  std::function<const std::vector<std::string> &(const GenericOperation &)>
      operationExpressions;
  std::function<std::string(int)> operandExpression;
  operandExpression = [&](int value) {
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return AffineValueExpression(value);
    if (IsIndexConstant(*definition->second))
      return IndexConstantExpression(*definition->second);
    if (const std::optional<std::string> expression =
            ExistingAffineApplyExpression(*definition->second))
      return *expression;
    auto replacement = result.replacementNames.find(definition->second->id);
    if (composeConversionProducers &&
        replacement != result.replacementNames.end()) {
      const std::vector<std::string> &expressions =
          operationExpressions(*definition->second);
      auto folded = result.foldedConstants.find(definition->second->id);
      if (folded != result.foldedConstants.end())
        return "c(" + std::to_string(folded->second) + ")";
      auto foldedValue = result.foldedValues.find(definition->second->id);
      if (foldedValue != result.foldedValues.end())
        return AffineValueExpression(foldedValue->second);
      if (replacement->second == "affine.apply")
        return expressions.front();
    }
    return AffineValueExpression(value);
  };
  operationExpressions = [&](const GenericOperation &operation)
      -> const std::vector<std::string> & {
    auto cached = expressionCache.find(operation.id);
    if (cached != expressionCache.end())
      return cached->second;
    if (!expressionStack.insert(operation.id).second)
      throw std::runtime_error(
          "ConvertArithToAffine: cyclic affine expression dependency");
    const std::string lhs = operandExpression(operation.operands[0]);
    const std::string rhs = operandExpression(operation.operands[1]);
    std::vector<std::string> expressions;
    const std::string &name = result.replacementNames.at(operation.id);
    if (name == "affine.min" || name == "affine.max")
      expressions = {lhs, rhs};
    else
      expressions = {AffineBinaryOperationExpression(operation, lhs, rhs)};
    // BinaryArithOpToAffineApply uses makeComposedAffineApply, which always
    // materializes an affine.apply even when its simplified map is constant
    // or an identity. Only the folded min/max builders may return an existing
    // value or attribute instead of creating an affine operation.
    if (composeConversionProducers || name == "affine.min" ||
        name == "affine.max") {
      if (const std::optional<int64_t> folded =
              FoldAffineReplacement(name, expressions))
        result.foldedConstants[operation.id] = *folded;
      if (expressions.size() == 1)
        if (const std::optional<int> value = AffineValue(expressions.front()))
          result.foldedValues[operation.id] = *value;
    }
    expressionStack.erase(operation.id);
    return expressionCache.emplace(operation.id, std::move(expressions))
        .first->second;
  };

  for (const GenericOperation &operation : module.operations)
    if (result.replacementNames.count(operation.id) != 0)
      result.replacementMapExpressions[operation.id] =
          operationExpressions(operation);

  std::set<int> operandStack;
  std::function<const std::vector<int> &(const GenericOperation &)>
      operationOperands;
  operationOperands = [&](const GenericOperation &operation)
      -> const std::vector<int> & {
    auto cached = result.replacementOperands.find(operation.id);
    if (cached != result.replacementOperands.end())
      return cached->second;
    if (!operandStack.insert(operation.id).second)
      throw std::runtime_error(
          "ConvertArithToAffine: cyclic affine operand dependency");

    auto composedApplyDefinition = [&](int value)
        -> const GenericOperation * {
      auto definition = definitions.find(value);
      if (definition == definitions.end())
        return nullptr;
      auto replacement = result.replacementNames.find(definition->second->id);
      if (composeConversionProducers &&
          replacement != result.replacementNames.end() &&
          replacement->second == "affine.apply" &&
          result.foldedConstants.count(definition->second->id) == 0)
        return definition->second;
      return ExistingAffineApplyExpression(*definition->second)
                     ? definition->second
                     : nullptr;
    };

    std::vector<int> operands;
    for (int operand : operation.operands) {
      auto definition = definitions.find(operand);
      if (definition != definitions.end() &&
          (IsIndexConstant(*definition->second) ||
           result.foldedConstants.count(definition->second->id) != 0 ||
           result.foldedValues.count(definition->second->id) != 0))
        continue;
      if (!composedApplyDefinition(operand))
        AppendUniqueAffineOperand(operand, operands);
    }
    for (int operand : operation.operands) {
      const GenericOperation *producer = composedApplyDefinition(operand);
      if (!producer)
        continue;
      const auto replacement = result.replacementNames.find(producer->id);
      const std::vector<int> &producerOperands =
          composeConversionProducers &&
                  replacement != result.replacementNames.end()
              ? operationOperands(*producer)
              : producer->operands;
      for (int producerOperand : producerOperands)
        AppendUniqueAffineOperand(producerOperand, operands);
    }
    operandStack.erase(operation.id);
    return result.replacementOperands
        .emplace(operation.id, std::move(operands))
        .first->second;
  };

  // composeAffineMapAndOperands keeps uncomposed inputs in their current
  // order, then appends each affine.apply producer's already-canonicalized
  // operands. canonicalizeMapAndOperands removes constants and duplicates
  // while preserving that dimension/symbol order.
  for (const GenericOperation &operation : module.operations)
    if (result.replacementNames.count(operation.id) != 0)
      operationOperands(operation);

  if (composeConversionProducers) {
    for (const GenericOperation &operation : module.operations) {
      auto replacement = result.replacementNames.find(operation.id);
      if (replacement == result.replacementNames.end() ||
          replacement->second != "affine.apply" ||
          operation.results.size() != 1 ||
          result.foldedConstants.count(operation.id) != 0 ||
          result.foldedValues.count(operation.id) != 0)
        continue;
      auto resultUsers = users.find(operation.results.front());
      if (resultUsers != users.end() && !resultUsers->second.empty() &&
          std::all_of(resultUsers->second.begin(), resultUsers->second.end(),
                      [&](int userId) {
                        return result.replacementNames.count(userId) != 0;
                      })) {
        result.composedApplyOperations.insert(operation.id);
        result.erasedOperations.insert(operation.id);
      }
    }
    for (const GenericOperation &constant : module.operations) {
      if (!IsIndexConstant(constant))
        continue;
      auto resultUsers = users.find(constant.results.front());
      if (resultUsers != users.end() && !resultUsers->second.empty() &&
          std::all_of(resultUsers->second.begin(), resultUsers->second.end(),
                      [&](int userId) {
                        return result.replacementNames.count(userId) != 0;
                      }))
        result.erasedOperations.insert(constant.id);
    }
  }

  return result;
}

inline std::string SerializeConvertArithToAffineState(
    const GenericModule &module, const ConvertArithToAffineState &state) {
  std::ostringstream output;
  output << "CONVERT_ARITH_TO_AFFINE\t1\n";
  for (const GenericOperation &operation : module.operations) {
    auto replacement = state.replacementNames.find(operation.id);
    if (replacement == state.replacementNames.end())
      continue;
    output << "OP\t" << operation.id << '\t' << replacement->second << '\t';
    auto operands = state.replacementOperands.find(operation.id);
    if (operands != state.replacementOperands.end()) {
      for (size_t index = 0; index < operands->second.size(); ++index) {
        if (index)
          output << ',';
        output << operands->second[index];
      }
    }
    output << '\t' << AffineMapSemanticKey(operation, state) << '\t'
           << (state.composedApplyOperations.count(operation.id) != 0) << '\t'
           << (state.erasedOperations.count(operation.id) != 0) << '\n';
  }
  return output.str();
}

} // namespace cvub

#endif
