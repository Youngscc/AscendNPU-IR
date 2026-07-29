#ifndef UB_OVERFLOW_MODEL_CPP_SCF_FOR_LOOP_CANONICALIZATION_HPP
#define UB_OVERFLOW_MODEL_CPP_SCF_FOR_LOOP_CANONICALIZATION_HPP

#include "../ir/operation_folder.hpp"
#include "canonicalization_hivm_pipeline.hpp"

namespace cvub {

struct SCFLoopInterval {
  int64_t lower = 0;
  int64_t upper = 0;
};

inline bool SCFOperationIsAttached(const GenericModule &module,
                                   const GenericOperation &operation) {
  if (operation.blockId < 0 ||
      static_cast<size_t>(operation.blockId) >= module.blocks.size())
    return false;
  const std::vector<int> &operations =
      module.blocks.at(static_cast<size_t>(operation.blockId)).operations;
  return std::find(operations.begin(), operations.end(), operation.id) !=
         operations.end();
}

inline std::map<int, const GenericOperation *>
SCFDefiningOperations(const GenericModule &module) {
  std::map<int, const GenericOperation *> result;
  for (const GenericOperation &operation : module.operations)
    if (SCFOperationIsAttached(module, operation))
      for (int value : operation.results)
        result[value] = &operation;
  return result;
}

inline std::optional<int64_t> SCFIndexConstant(
    int value,
    const std::map<int, const GenericOperation *> &definitions) {
  const auto definition = definitions.find(value);
  if (definition == definitions.end())
    return std::nullopt;
  const std::optional<ArithIntegerConstant> constant =
      ParseArithIntegerConstant(*definition->second);
  if (!constant || constant->width != 64)
    return std::nullopt;
  return SignedArithInteger(*constant);
}

inline const GenericBlock *SCFSingleBody(const GenericModule &module,
                                         const GenericOperation &operation) {
  if (operation.regions.size() != 1)
    return nullptr;
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(operation.regions.front()));
  if (region.blocks.size() != 1)
    return nullptr;
  return &module.blocks.at(static_cast<size_t>(region.blocks.front()));
}

inline const GenericOperation *SCFBodyTerminator(
    const GenericModule &module, const GenericOperation &operation) {
  const GenericBlock *body = SCFSingleBody(module, operation);
  if (body == nullptr || body->operations.empty())
    return nullptr;
  return &module.operations.at(static_cast<size_t>(body->operations.back()));
}

// Exact projection of LoopCanonicalization.cpp::isShapePreserving for the
// scf.for/tensor.insert_slice subset. The native helper follows only the tied
// iter arg, tensor.insert_slice destinations and recursively shape-preserving
// nested scf.for results; every other producer fails the proof.
inline bool SCFForShapePreserving(
    const GenericModule &module, const GenericOperation &loop, size_t channel,
    const std::map<int, const GenericOperation *> &definitions,
    std::set<std::pair<int, size_t>> &active) {
  if (loop.name != "scf.for" || channel >= loop.results.size() ||
      loop.operands.size() <= channel + 3 ||
      !active.insert({loop.id, channel}).second)
    return false;
  const GenericBlock *body = SCFSingleBody(module, loop);
  const GenericOperation *yield = SCFBodyTerminator(module, loop);
  if (body == nullptr || yield == nullptr || yield->name != "scf.yield" ||
      body->arguments.size() <= channel + 1 ||
      yield->operands.size() <= channel) {
    active.erase({loop.id, channel});
    return false;
  }

  int value = yield->operands[channel];
  while (value != body->arguments[channel + 1]) {
    const auto definition = definitions.find(value);
    if (definition == definitions.end()) {
      active.erase({loop.id, channel});
      return false;
    }
    const GenericOperation &producer = *definition->second;
    if (producer.name == "tensor.insert_slice" &&
        producer.operands.size() >= 2) {
      value = producer.operands[1];
      continue;
    }
    if (producer.name != "scf.for") {
      active.erase({loop.id, channel});
      return false;
    }
    const auto result =
        std::find(producer.results.begin(), producer.results.end(), value);
    if (result == producer.results.end()) {
      active.erase({loop.id, channel});
      return false;
    }
    const size_t nestedChannel = static_cast<size_t>(
        std::distance(producer.results.begin(), result));
    if (!SCFForShapePreserving(module, producer, nestedChannel, definitions,
                               active) ||
        producer.operands.size() <= nestedChannel + 3) {
      active.erase({loop.id, channel});
      return false;
    }
    value = producer.operands[nestedChannel + 3];
  }
  active.erase({loop.id, channel});
  return true;
}

inline bool SCFForShapePreserving(
    const GenericModule &module, const GenericOperation &loop, size_t channel,
    const std::map<int, const GenericOperation *> &definitions) {
  std::set<std::pair<int, size_t>> active;
  return SCFForShapePreserving(module, loop, channel, definitions, active);
}

inline bool FoldSCFDimOfIterArgsAndResults(GenericModule &module) {
  const auto definitions = SCFDefiningOperations(module);
  bool changed = false;
  for (GenericOperation &dim : module.operations) {
    if ((dim.name != "tensor.dim" && dim.name != "memref.dim") ||
        dim.operands.empty() || !SCFOperationIsAttached(module, dim))
      continue;
    const int source = dim.operands.front();

    bool rewritten = false;
    for (const GenericBlock &block : module.blocks) {
      const auto argument =
          std::find(block.arguments.begin(), block.arguments.end(), source);
      if (argument == block.arguments.end())
        continue;
      const size_t argumentIndex = static_cast<size_t>(
          std::distance(block.arguments.begin(), argument));
      if (argumentIndex == 0 || block.regionId < 0)
        break;
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(block.regionId));
      if (region.parentOperation < 0)
        break;
      const GenericOperation &loop = module.operations.at(
          static_cast<size_t>(region.parentOperation));
      const size_t channel = argumentIndex - 1;
      if (loop.name != "scf.for" || loop.operands.size() <= channel + 3 ||
          !SCFForShapePreserving(module, loop, channel, definitions))
        break;
      dim.operands.front() = loop.operands[channel + 3];
      rewritten = true;
      break;
    }
    if (rewritten) {
      changed = true;
      continue;
    }

    const auto definition = definitions.find(source);
    if (definition == definitions.end() ||
        definition->second->name != "scf.for")
      continue;
    const GenericOperation &loop = *definition->second;
    const auto result =
        std::find(loop.results.begin(), loop.results.end(), source);
    if (result == loop.results.end())
      continue;
    const size_t channel =
        static_cast<size_t>(std::distance(loop.results.begin(), result));
    if (loop.operands.size() <= channel + 3 ||
        !SCFForShapePreserving(module, loop, channel, definitions))
      continue;
    dim.operands.front() = loop.operands[channel + 3];
    changed = true;
  }
  return changed;
}

inline std::vector<int64_t> ParseSCFIntegerArray(std::string text) {
  const size_t colon = text.find(':');
  const size_t close = text.rfind('>');
  if (colon == std::string::npos || close == std::string::npos ||
      colon >= close)
    return {};
  std::vector<int64_t> result;
  for (const std::string &item : split(text.substr(colon + 1,
                                                   close - colon - 1), ','))
    try {
      result.push_back(std::stoll(trim(item)));
    } catch (const std::exception &) {
      return {};
    }
  return result;
}

inline std::optional<SCFLoopInterval> MakeSCFLoopInterval(
    int64_t lower, int64_t upper, int64_t step) {
  if (step <= 0 || lower >= upper)
    return std::nullopt;
  const std::optional<int64_t> distance = CheckedSubInt64(upper, lower);
  if (!distance || *distance <= 0)
    return std::nullopt;
  const int64_t tripsMinusOne = (*distance - 1) / step;
  const std::optional<int64_t> last =
      CheckedMulAddInt64(step, tripsMinusOne, lower);
  if (!last)
    return std::nullopt;
  return SCFLoopInterval{lower, *last};
}

inline std::map<int, SCFLoopInterval> CollectSCFLoopIntervals(
    const GenericModule &module,
    const std::map<int, const GenericOperation *> &definitions) {
  std::map<int, SCFLoopInterval> intervals;
  for (const GenericOperation &loop : module.operations) {
    const GenericBlock *body = SCFSingleBody(module, loop);
    if (body == nullptr)
      continue;
    if (loop.name == "scf.for") {
      if (body->arguments.empty() || loop.operands.size() < 3)
        continue;
      const auto lower = SCFIndexConstant(loop.operands[0], definitions);
      const auto upper = SCFIndexConstant(loop.operands[1], definitions);
      const auto step = SCFIndexConstant(loop.operands[2], definitions);
      if (!lower || !upper || !step)
        continue;
      if (const auto interval = MakeSCFLoopInterval(*lower, *upper, *step))
        intervals[body->arguments.front()] = *interval;
      continue;
    }
    if (loop.name == "scf.parallel") {
      std::vector<size_t> segments = OperandSegmentSizes(loop.properties);
      if (segments.size() != 4 || segments[0] != segments[1] ||
          segments[1] != segments[2] || body->arguments.size() < segments[0])
        continue;
      const size_t rank = segments[0];
      for (size_t index = 0; index < rank; ++index) {
        const auto lower = SCFIndexConstant(loop.operands[index], definitions);
        const auto upper =
            SCFIndexConstant(loop.operands[rank + index], definitions);
        const auto step =
            SCFIndexConstant(loop.operands[2 * rank + index], definitions);
        if (!lower || !upper || !step)
          continue;
        if (const auto interval = MakeSCFLoopInterval(*lower, *upper, *step))
          intervals[body->arguments[index]] = *interval;
      }
      continue;
    }
    if (loop.name != "scf.forall")
      continue;
    std::vector<size_t> segments = OperandSegmentSizes(loop.properties);
    const std::vector<int64_t> staticLower = ParseSCFIntegerArray(
        FindDictionaryValue(loop.properties, "staticLowerBound"));
    const std::vector<int64_t> staticUpper = ParseSCFIntegerArray(
        FindDictionaryValue(loop.properties, "staticUpperBound"));
    const std::vector<int64_t> staticStep = ParseSCFIntegerArray(
        FindDictionaryValue(loop.properties, "staticStep"));
    if (segments.size() != 4 || staticLower.size() != staticUpper.size() ||
        staticUpper.size() != staticStep.size() ||
        body->arguments.size() < staticLower.size())
      continue;
    const int64_t dynamic = std::numeric_limits<int64_t>::min();
    size_t lowerDynamic = 0;
    size_t upperDynamic = segments[0];
    size_t stepDynamic = segments[0] + segments[1];
    for (size_t index = 0; index < staticLower.size(); ++index) {
      const auto mixed = [&](int64_t value, size_t &cursor,
                             size_t end) -> std::optional<int64_t> {
        if (value != dynamic)
          return value;
        if (cursor >= end || cursor >= loop.operands.size())
          return std::nullopt;
        return SCFIndexConstant(loop.operands[cursor++], definitions);
      };
      const auto lower = mixed(staticLower[index], lowerDynamic, segments[0]);
      const auto upper = mixed(staticUpper[index], upperDynamic,
                               segments[0] + segments[1]);
      const auto step = mixed(staticStep[index], stepDynamic,
                              segments[0] + segments[1] + segments[2]);
      if (!lower || !upper || !step)
        continue;
      if (const auto interval = MakeSCFLoopInterval(*lower, *upper, *step))
        intervals[body->arguments[index]] = *interval;
    }
  }
  return intervals;
}

inline std::optional<SCFLoopInterval> EvaluateSCFAffineInterval(
    const AffineLinearForm &form,
    const std::map<int, SCFLoopInterval> &loopIntervals,
    const std::map<int, const GenericOperation *> &definitions) {
  int64_t lower = form.constant;
  int64_t upper = form.constant;
  for (const auto &[term, coefficient] : form.coefficients) {
    if (coefficient == 0)
      continue;
    const std::optional<int> value = AffineValue(term);
    if (!value)
      return std::nullopt;
    SCFLoopInterval interval;
    const auto loopInterval = loopIntervals.find(*value);
    if (loopInterval != loopIntervals.end())
      interval = loopInterval->second;
    else if (const auto constant = SCFIndexConstant(*value, definitions))
      interval = SCFLoopInterval{*constant, *constant};
    else
      return std::nullopt;
    const int64_t lowValue = coefficient >= 0 ? interval.lower : interval.upper;
    const int64_t highValue = coefficient >= 0 ? interval.upper : interval.lower;
    const auto newLower = CheckedMulAddInt64(coefficient, lowValue, lower);
    const auto newUpper = CheckedMulAddInt64(coefficient, highValue, upper);
    if (!newLower || !newUpper)
      return std::nullopt;
    lower = *newLower;
    upper = *newUpper;
  }
  return SCFLoopInterval{lower, upper};
}

inline std::optional<AffineLinearForm> SubtractSCFAffineForms(
    AffineLinearForm lhs, const AffineLinearForm &rhs) {
  if (!AccumulateAffineCoefficient(rhs.constant, -1, lhs.constant))
    return std::nullopt;
  for (const auto &[term, coefficient] : rhs.coefficients)
    if (!AccumulateAffineCoefficient(coefficient, -1,
                                     lhs.coefficients[term]))
      return std::nullopt;
  return lhs;
}

inline size_t SCFAffineDimensionCount(const GenericOperation &operation) {
  std::string map = FindDictionaryValue(operation.properties, "map");
  if (map.empty())
    map = FindDictionaryValue(operation.attributes, "map");
  std::string compact;
  for (char character : map)
    if (!std::isspace(static_cast<unsigned char>(character)))
      compact.push_back(character);
  const std::string prefix = "affine_map<(";
  if (!startsWith(compact, prefix))
    return 0;
  const size_t close = compact.find(')', prefix.size());
  if (close == std::string::npos || close == prefix.size())
    return 0;
  return splitTopLevel(compact.substr(prefix.size(),
                                      close - prefix.size())).size();
}

inline std::optional<std::string> SCFNativePureAffineMapText(
    const GenericOperation &operation, const AffineLinearForm &form,
    std::vector<int> &operands) {
  const size_t oldDimensionCount = SCFAffineDimensionCount(operation);
  std::vector<int> dimensions;
  std::vector<int> symbols;
  for (size_t index = 0; index < operation.operands.size(); ++index) {
    const int operand = operation.operands[index];
    if (form.coefficients.count(AffineValueExpression(operand)) == 0)
      continue;
    (index < oldDimensionCount ? dimensions : symbols).push_back(operand);
  }
  operands = dimensions;
  operands.insert(operands.end(), symbols.begin(), symbols.end());

  struct Term {
    size_t group = 0;
    size_t position = 0;
    int64_t coefficient = 0;
  };
  std::vector<Term> terms;
  for (const auto &[expression, coefficient] : form.coefficients) {
    if (coefficient == 0)
      continue;
    const std::optional<int> value = AffineValue(expression);
    if (!value)
      return std::nullopt;
    auto dimension = std::find(dimensions.begin(), dimensions.end(), *value);
    if (dimension != dimensions.end()) {
      terms.push_back(Term{0, static_cast<size_t>(
                                  std::distance(dimensions.begin(), dimension)),
                           coefficient});
      continue;
    }
    auto symbol = std::find(symbols.begin(), symbols.end(), *value);
    if (symbol == symbols.end())
      return std::nullopt;
    terms.push_back(Term{1, static_cast<size_t>(
                                std::distance(symbols.begin(), symbol)),
                         coefficient});
  }
  std::sort(terms.begin(), terms.end(), [](const Term &lhs, const Term &rhs) {
    return std::tie(lhs.group, lhs.position) <
           std::tie(rhs.group, rhs.position);
  });

  std::string expression;
  for (const Term &term : terms) {
    const bool negative = term.coefficient < 0;
    const uint64_t magnitude = NativeAffineMagnitude(term.coefficient);
    std::string text =
        std::string(term.group == 0 ? "d" : "s") +
        std::to_string(term.position);
    if (magnitude != 1)
      text += " * " + std::to_string(magnitude);
    if (expression.empty())
      expression = negative ? "-" + text : text;
    else
      expression += negative ? " - " + text : " + " + text;
  }
  if (form.constant != 0 || expression.empty()) {
    if (expression.empty())
      expression = std::to_string(form.constant);
    else if (form.constant < 0)
      expression += " - " +
                    std::to_string(NativeAffineMagnitude(form.constant));
    else
      expression += " + " + std::to_string(form.constant);
  }

  std::ostringstream map;
  map << "affine_map<(";
  for (size_t index = 0; index < dimensions.size(); ++index) {
    if (index != 0)
      map << ", ";
    map << 'd' << index;
  }
  map << ')';
  if (!symbols.empty()) {
    map << '[';
    for (size_t index = 0; index < symbols.size(); ++index) {
      if (index != 0)
        map << ", ";
      map << 's' << index;
    }
    map << ']';
  }
  map << " -> (" << expression << ")>";
  return map.str();
}

inline bool CanonicalizeSCFAffineMinMax(GenericModule &module) {
  const auto definitions = SCFDefiningOperations(module);
  const auto loopIntervals = CollectSCFLoopIntervals(module, definitions);
  GenericRewriter rewriter(module);
  bool changed = false;
  for (GenericOperation &operation : module.operations) {
    if ((operation.name != "affine.min" && operation.name != "affine.max") ||
        operation.results.size() != 1 || !SCFOperationIsAttached(module, operation))
      continue;
    const auto expressions = ExistingAffineMinMaxExpressions(operation);
    if (!expressions || expressions->empty())
      continue;
    std::vector<std::optional<AffineLinearForm>> forms;
    for (const std::string &expression : *expressions)
      forms.push_back(FlattenAffineLinearExpression(expression));

    std::optional<size_t> selected;
    for (size_t candidate = 0; candidate < forms.size() && !selected;
         ++candidate) {
      if (!forms[candidate])
        continue;
      bool provesAll = true;
      for (size_t other = 0; other < forms.size(); ++other) {
        if (other == candidate)
          continue;
        if (!forms[other]) {
          provesAll = false;
          break;
        }
        const auto difference =
            SubtractSCFAffineForms(*forms[candidate], *forms[other]);
        const auto interval =
            difference ? EvaluateSCFAffineInterval(*difference, loopIntervals,
                                                   definitions)
                       : std::nullopt;
        const bool proven = interval &&
                            (operation.name == "affine.min"
                                 ? interval->upper <= 0
                                 : interval->lower >= 0);
        if (!proven) {
          provesAll = false;
          break;
        }
      }
      if (provesAll)
        selected = candidate;
    }
    if (!selected)
      continue;

    const std::string expression = expressions->at(*selected);
    if (const auto constant = AffineConstantValue(expression)) {
      operation.name = "arith.constant";
      operation.operands.clear();
      operation.operandTypes.clear();
      operation.properties =
          "{value = " + std::to_string(*constant) + " : index}";
      operation.attributes = operation.properties;
      operation.effects.clear();
      changed = true;
      continue;
    }
    if (const auto value = AffineValue(expression)) {
      rewriter.replaceAllUses(operation.results.front(), *value);
      rewriter.removeFromBlock(operation.blockId, operation.id);
      changed = true;
      continue;
    }

    if (!forms[*selected])
      continue;
    std::vector<int> operands;
    const std::optional<std::string> map = SCFNativePureAffineMapText(
        operation, *forms[*selected], operands);
    if (!map)
      continue;
    operation.name = "affine.apply";
    operation.operands = operands;
    operation.operandTypes.assign(operands.size(), "index");
    operation.properties = "{map = " + *map + "}";
    operation.attributes = operation.properties;
    operation.effects.clear();
    changed = true;
  }
  return changed;
}

inline GenericModule RunSCFForLoopCanonicalization(GenericModule module) {
  bool changed = false;
  while (FoldSCFDimOfIterArgsAndResults(module))
    changed = true;
  const bool affineChanged = CanonicalizeSCFAffineMinMax(module);
  if (affineChanged) {
    std::vector<int> functions;
    for (const GenericOperation &operation : module.operations)
      if (operation.name == "func.func")
        functions.push_back(operation.id);
    for (int functionId : functions)
      RunGreedyOperationFolder(module, functionId);
    RunExistingAffineDeadCodeElimination(module);
    changed = true;
  }
  if (changed) {
    PipelineAnalysisContext useLists(
        module, kGenericAnalysisDefinitions | kGenericAnalysisUsers);
    while (EliminateCanonicalizationDeadCode(module, useLists)) {
    }
  }
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
