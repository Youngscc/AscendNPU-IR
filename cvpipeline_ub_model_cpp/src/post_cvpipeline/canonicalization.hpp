#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_CANONICALIZATION_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_CANONICALIZATION_HPP

#include "buffer_size.hpp"

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace cvub {

inline bool HasAttachedUsers(const GenericModule &module, int value,
                             int exceptOperation = -1) {
  for (const GenericBlock &block : module.blocks)
    for (int operationId : block.operations) {
      if (operationId == exceptOperation)
        continue;
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (std::find(operation.operands.begin(), operation.operands.end(), value) !=
              operation.operands.end() ||
          std::find(operation.dpsInputs.begin(), operation.dpsInputs.end(), value) !=
              operation.dpsInputs.end() ||
          std::find(operation.dpsInits.begin(), operation.dpsInits.end(), value) !=
              operation.dpsInits.end())
        return true;
    }
  return false;
}

inline std::vector<int64_t> CanonicalizationI64Array(std::string text) {
  text = trim(std::move(text));
  if (startsWith(text, "array<i64:"))
    text = "[" + text.substr(10, text.size() - 11) + "]";
  if (text.size() < 2 || text.front() != '[' || text.back() != ']')
    return {};
  std::vector<int64_t> values;
  try {
    for (const std::string &entry :
         splitTopLevel(text.substr(1, text.size() - 2)))
      values.push_back(std::stoll(trim(entry)));
  } catch (const std::exception &) {
    return {};
  }
  return values;
}

inline std::vector<int64_t> CanonicalizationStaticShape(
    const std::string &type) {
  const size_t open = type.find('<');
  const size_t close = type.rfind('>');
  if (open == std::string::npos || close == std::string::npos || close <= open)
    return {};
  const std::string shaped = splitTopLevel(
      type.substr(open + 1, close - open - 1)).front();
  static const std::regex element(R"((?:bf16|[fiu][0-9]+)$)");
  const std::string dimensions = std::regex_replace(shaped, element, "");
  if (dimensions.empty())
    return {};
  std::vector<int64_t> shape;
  try {
    for (std::string dimension : split(dimensions, 'x')) {
      dimension = trim(std::move(dimension));
      if (dimension.empty())
        continue;
      if (dimension == "?")
        shape.push_back(-1);
      else
        shape.push_back(std::stoll(dimension));
    }
  } catch (const std::exception &) {
    return {};
  }
  return shape;
}

inline std::string CanonicalizationProperty(const GenericOperation &operation,
                                            const std::string &name) {
  std::string value = IRDictionaryValue(operation.properties, name);
  return value.empty() ? IRDictionaryValue(operation.attributes, name) : value;
}

inline bool HasUnusedResults(const GenericModule &module,
                             const GenericOperation &operation) {
  return !operation.results.empty() &&
         std::all_of(operation.results.begin(), operation.results.end(),
                     [&](int value) { return !HasAttachedUsers(module, value); });
}

inline const std::vector<int> &CanonicalizationInputs(
    const GenericOperation &operation) {
  return operation.dpsInputs.empty() ? operation.operands
                                     : operation.dpsInputs;
}

inline bool DimensionListDropsUnitExtent(const GenericOperation &operation,
                                         const std::string &property) {
  if (operation.operandTypes.empty())
    return false;
  const std::vector<int64_t> shape =
      CanonicalizationStaticShape(operation.operandTypes.front());
  const std::vector<int64_t> dimensions = CanonicalizationI64Array(
      CanonicalizationProperty(operation, property));
  for (int64_t dimension : dimensions)
    if (dimension >= 0 && static_cast<size_t>(dimension) < shape.size() &&
        shape[static_cast<size_t>(dimension)] == 1)
      return true;
  return false;
}

inline bool ApplicableHIVMCanonicalization(const GenericModule &module,
                                           const GenericOperation &operation) {
  const std::vector<int> &inputs = CanonicalizationInputs(operation);
  if (operation.name == "hivm.hir.vpow") {
    if (inputs.size() < 2)
      return false;
    const GenericOperation *exponent =
        DefinitionForBufferSize(module, inputs[1]);
    if (exponent == nullptr || exponent->name != "arith.constant")
      return false;
    if (exponent->resultTypes.empty() ||
        (exponent->resultTypes.front() != "f16" &&
         exponent->resultTypes.front() != "f32" &&
         exponent->resultTypes.front() != "f64" &&
         exponent->resultTypes.front() != "bf16"))
      return false;
    std::string value = IRDictionaryValue(exponent->attributes, "value");
    const size_t colon = value.find(':');
    if (colon != std::string::npos)
      value = value.substr(0, colon);
    try {
      const double number = std::stod(trim(value));
      return number == 0.0 || number == 0.5 || number == 1.0 ||
             number == 2.0 || number == 3.0;
    } catch (const std::exception &) {
      return false;
    }
  }
  if (operation.name == "hivm.hir.vreduce") {
    if (DimensionListDropsUnitExtent(operation, "reduce_dims"))
      return true;
    for (int init : operation.dpsInits) {
      const GenericOperation *definition = DefinitionForBufferSize(module, init);
      if (definition != nullptr && definition->name == "hivm.hir.vbrc" &&
          !CanonicalizationInputs(*definition).empty()) {
        const GenericOperation *constant = DefinitionForBufferSize(
            module, CanonicalizationInputs(*definition).front());
        if (constant != nullptr && constant->name == "arith.constant")
          return true;
      }
    }
    return false;
  }
  if (operation.name == "hivm.hir.vcumsum" ||
      operation.name == "hivm.hir.vcumprod")
    return DimensionListDropsUnitExtent(operation, "cum_dims");
  if (operation.name == "hivm.hir.vtranspose") {
    if (HasUnusedResults(module, operation))
      return true;
    const std::vector<int64_t> permutation = CanonicalizationI64Array(
        CanonicalizationProperty(operation, "permutation"));
    bool identity = !permutation.empty();
    for (size_t index = 0; index < permutation.size(); ++index)
      identity = identity && permutation[index] == static_cast<int64_t>(index);
    if (identity)
      return true;
    if (inputs.empty() || permutation.empty())
      return false;
    std::vector<int64_t> simulated(permutation.size());
    for (size_t index = 0; index < simulated.size(); ++index)
      simulated[index] = static_cast<int64_t>(index);
    auto applyPermutation = [](const std::vector<int64_t> &input,
                               const std::vector<int64_t> &current) {
      if (input.size() != current.size())
        return std::vector<int64_t>{};
      std::vector<int64_t> output(current.size(), -1);
      for (size_t index = 0; index < current.size(); ++index) {
        if (current[index] < 0 ||
            static_cast<size_t>(current[index]) >= output.size())
          return std::vector<int64_t>{};
        output[static_cast<size_t>(current[index])] = input[index];
      }
      return output;
    };
    const std::vector<int64_t> base = simulated;
    const GenericOperation *current = &operation;
    std::set<int> visited;
    while (current != nullptr && current->name == "hivm.hir.vtranspose" &&
           visited.insert(current->id).second) {
      const std::vector<int64_t> currentPermutation = CanonicalizationI64Array(
          CanonicalizationProperty(*current, "permutation"));
      simulated = applyPermutation(simulated, currentPermutation);
      if (simulated.empty())
        return false;
      if (simulated == base)
        return true;
      const std::vector<int> &currentInputs = CanonicalizationInputs(*current);
      current = currentInputs.empty()
                    ? nullptr
                    : DefinitionForBufferSize(module, currentInputs.front());
    }
    return false;
  }
  if (operation.name == "hivm.hir.vpad") {
    if (inputs.empty() || operation.resultTypes.empty())
      return false;
    const GenericOperation *source = DefinitionForBufferSize(module, inputs.front());
    return source != nullptr && source->name == "hivm.hir.load" &&
           CanonicalizationProperty(*source, "pad_mode").empty() &&
           CanonicalizationStaticShape(operation.resultTypes.front()).size() == 1;
  }
  if (operation.name == "hivm.hir.debug") {
    if (CanonicalizationProperty(operation, "debugtype") != "\"assert\"" ||
        inputs.empty())
      return false;
    const GenericOperation *condition = DefinitionForBufferSize(module, inputs.front());
    return condition != nullptr && condition->name == "arith.constant" &&
           !condition->resultTypes.empty() && condition->resultTypes.front() == "i1" &&
           ParseIntegerAttribute(
               IRDictionaryValue(condition->attributes, "value")) == 1;
  }
  if (operation.name == "hivm.hir.vbrc")
    return DimensionListDropsUnitExtent(operation, "broadcast_dims");
  return false;
}

inline bool ApplicableInlineBroadcastCanonicalization(
    const GenericOperation &operation) {
  static const std::set<std::string> broadcastable = {
      "hivm.hir.vexp", "hivm.hir.vabs", "hivm.hir.vln",
      "hivm.hir.vrelu", "hivm.hir.vrsqrt", "hivm.hir.vsqrt",
      "hivm.hir.vrec", "hivm.hir.vnot", "hivm.hir.vcast",
      "hivm.hir.vadd", "hivm.hir.vmul", "hivm.hir.vsub",
      "hivm.hir.vdiv", "hivm.hir.vmax", "hivm.hir.vmin",
      "hivm.hir.vor", "hivm.hir.vand", "hivm.hir.vshl",
      "hivm.hir.vshr", "hivm.hir.vsel"};
  if (broadcastable.count(operation.name) == 0)
    return false;
  std::string raw = CanonicalizationProperty(operation, "broadcast_dims");
  if (raw.empty())
    raw = CanonicalizationProperty(operation, "broadcast");
  const std::vector<int64_t> dimensions = CanonicalizationI64Array(raw);
  if (dimensions.empty())
    return false;
  std::vector<std::string> types = operation.operandTypes;
  types.insert(types.end(), operation.resultTypes.begin(),
               operation.resultTypes.end());
  for (int64_t dimension : dimensions) {
    std::optional<int64_t> extent;
    bool allSame = true;
    for (const std::string &type : types) {
      const std::vector<int64_t> shape = CanonicalizationStaticShape(type);
      if (dimension < 0 || static_cast<size_t>(dimension) >= shape.size())
        continue;
      const int64_t current = shape[static_cast<size_t>(dimension)];
      if (current < 0 || (extent && *extent != current)) {
        allSame = false;
        break;
      }
      extent = current;
    }
    if (allSame && extent.has_value())
      return true;
  }
  return false;
}

inline bool ApplicableHIVMMemRefCastFold(const GenericModule &module,
                                         const GenericOperation &operation) {
  if (operation.name != "hivm.hir.copy" &&
      operation.name != "hivm.hir.load" &&
      operation.name != "hivm.hir.store")
    return false;
  for (int operand : operation.operands) {
    const GenericOperation *definition = DefinitionForBufferSize(module, operand);
    if (definition != nullptr && definition->name == "memref.cast")
      return true;
  }
  return false;
}

inline bool ApplicableConvertLayoutCanonicalization(
    const GenericModule &module, const GenericOperation &operation) {
  if (operation.name != "hivm.hir.convert_layout" || operation.operands.empty())
    return false;
  const GenericOperation *source =
      DefinitionForBufferSize(module, operation.operands.front());
  if (source == nullptr)
    return false;
  if (source->name == "tensor.empty" || source->name == "tensor.cast")
    return true;
  if (source->name != "hivm.hir.convert_layout")
    return false;
  return CanonicalizationProperty(*source, "src_layout") ==
             CanonicalizationProperty(operation, "dst_layout") &&
         CanonicalizationProperty(*source, "dst_layout") ==
             CanonicalizationProperty(operation, "src_layout") &&
         !HasAttachedUsers(module, source->results.front(), operation.id);
}

inline bool ApplicableTensorReshapeCanonicalization(
    const GenericModule &module, const GenericOperation &operation) {
  if (operation.operands.empty())
    return false;
  const GenericOperation *source =
      DefinitionForBufferSize(module, operation.operands.front());
  if (source == nullptr)
    return false;
  if (operation.name == "tensor.collapse_shape")
    return source->name == "tensor.expand_shape";
  if (operation.name == "tensor.expand_shape")
    return source->name == "tensor.collapse_shape";
  if (operation.name == "tensor.extract_slice")
    return source->name == "tensor.insert_slice";
  return false;
}

inline bool ApplicableMemRefReshapeCanonicalization(
    const GenericModule &module, const GenericOperation &operation) {
  if (operation.operands.empty())
    return false;
  const GenericOperation *source =
      DefinitionForBufferSize(module, operation.operands.front());
  if (source == nullptr)
    return false;
  if (operation.name == "memref.collapse_shape")
    return source->name == "memref.expand_shape";
  if (operation.name == "memref.expand_shape")
    return source->name == "memref.collapse_shape";
  if (operation.name == "memref.reinterpret_cast")
    return !operation.operandTypes.empty() && !operation.resultTypes.empty() &&
           operation.operandTypes.front() == operation.resultTypes.front();
  return false;
}

inline bool IsNestedUnderOperation(const GenericModule &module, int operationId,
                                   int ancestorId) {
  int current = operationId;
  std::set<int> visited;
  while (current >= 0 && visited.insert(current).second) {
    if (current == ancestorId)
      return true;
    current = module.operations.at(static_cast<size_t>(current)).parentId;
  }
  return false;
}

inline std::optional<int64_t> ConstantIntegerValue(
    const GenericModule &module, int value) {
  const GenericOperation *definition = DefinitionForBufferSize(module, value);
  if (definition == nullptr || definition->name != "arith.constant")
    return std::nullopt;
  return ParseIntegerAttribute(
      IRDictionaryValue(definition->attributes, "value"));
}

inline bool FoldOneIntegerOperation(GenericModule &module,
                                    GenericOperation &operation) {
  if (operation.name == "arith.index_cast" && operation.operands.size() == 1 &&
      operation.results.size() == 1 && operation.resultTypes.size() == 1) {
    const auto value = ConstantIntegerValue(module, operation.operands[0]);
    if (!value)
      return false;
    operation.name = "arith.constant";
    operation.operands.clear();
    operation.operandTypes.clear();
    operation.attributes = "{value = " + std::to_string(*value) + " : " +
                           operation.resultTypes.front() + "}";
    ApplyOperationSemantics(operation);
    return true;
  }
  if (operation.name == "affine.apply" && operation.operands.empty() &&
      operation.results.size() == 1 && operation.resultTypes.size() == 1) {
    const std::string map = IRDictionaryValue(operation.properties, "map");
    static const std::regex constantMap(
        R"(^affine_map<\(\) -> \((-?[0-9]+)\)>$)");
    std::smatch match;
    const std::string spelling = trim(map);
    if (!std::regex_match(spelling, match, constantMap))
      return false;
    operation.name = "arith.constant";
    operation.properties.clear();
    operation.attributes = "{value = " + match[1].str() + " : " +
                           operation.resultTypes.front() + "}";
    ApplyOperationSemantics(operation);
    return true;
  }
  static const std::set<std::string> binary = {
      "arith.addi", "arith.subi", "arith.muli", "arith.minsi",
      "arith.maxsi"};
  if (binary.count(operation.name) == 0 || operation.operands.size() != 2 ||
      operation.results.size() != 1 || operation.resultTypes.size() != 1)
    return false;
  const auto left = ConstantIntegerValue(module, operation.operands[0]);
  const auto right = ConstantIntegerValue(module, operation.operands[1]);
  if (!left || !right)
    return false;
  int64_t value = 0;
  if (operation.name == "arith.addi") {
    if (__builtin_add_overflow(*left, *right, &value))
      return false;
  } else if (operation.name == "arith.subi") {
    if (__builtin_sub_overflow(*left, *right, &value))
      return false;
  } else if (operation.name == "arith.muli") {
    if (__builtin_mul_overflow(*left, *right, &value))
      return false;
  } else if (operation.name == "arith.minsi") {
    value = std::min(*left, *right);
  } else {
    value = std::max(*left, *right);
  }
  operation.name = "arith.constant";
  operation.operands.clear();
  operation.operandTypes.clear();
  operation.dpsInputs.clear();
  operation.dpsInits.clear();
  operation.attributes = "{value = " + std::to_string(value) + " : " +
                         operation.resultTypes.front() + "}";
  ApplyOperationSemantics(operation);
  return true;
}

inline const GenericOperation *CanonicalSingleBlockTerminator(
    const GenericModule &module, int regionId) {
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(regionId));
  if (region.blocks.size() != 1)
    return nullptr;
  const GenericBlock &block =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));
  if (block.operations.empty())
    return nullptr;
  return &module.operations.at(static_cast<size_t>(block.operations.back()));
}

inline bool SimplifyOneTrivialLoop(GenericModule &module,
                                   GenericOperation &loop) {
  if (loop.name != "scf.for" || loop.regions.size() != 1 ||
      loop.operands.size() < 3 || loop.results.size() + 3 > loop.operands.size())
    return false;
  const GenericOperation *yield =
      CanonicalSingleBlockTerminator(module, loop.regions[0]);
  if (yield == nullptr || yield->name != "scf.yield" ||
      yield->operands.size() != loop.results.size())
    return false;
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(loop.regions[0]));
  if (region.blocks.size() != 1)
    return false;
  const GenericBlock &body =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));
  if (body.arguments.size() < 1 + loop.results.size())
    return false;
  const auto lower = ConstantIntegerValue(module, loop.operands[0]);
  const auto upper = ConstantIntegerValue(module, loop.operands[1]);
  const auto step = ConstantIntegerValue(module, loop.operands[2]);
  if (lower && upper && step && *step > 0 && *upper > *lower &&
      (*upper - *lower + *step - 1) / *step == 1) {
    const std::vector<int> initValues(loop.operands.begin() + 3,
                                      loop.operands.end());
    ReplaceAllUses(module, body.arguments.front(), loop.operands[0]);
    for (size_t index = 0; index < loop.results.size(); ++index)
      ReplaceAllUses(module, body.arguments[1 + index], initValues[index]);
    const GenericOperation &updatedYield = module.operations.at(
        static_cast<size_t>(body.operations.back()));
    const std::vector<int> yielded = updatedYield.operands;
    for (size_t index = 0; index < loop.results.size(); ++index)
      ReplaceAllUses(module, loop.results[index], yielded[index]);
    const std::vector<int> children = body.operations;
    for (int child : children)
      if (child != updatedYield.id)
        MoveOperationBefore(module, child, loop.id);
    GenericRewriter rewriter(module);
    rewriter.removeFromBlock(body.id, updatedYield.id);
    rewriter.removeFromBlock(loop.blockId, loop.id);
    return true;
  }

  std::vector<size_t> passthrough;
  for (size_t index = 0; index < loop.results.size(); ++index) {
    const int init = loop.operands[3 + index];
    const int iterArg = body.arguments[1 + index];
    if (yield->operands[index] == init || yield->operands[index] == iterArg)
      passthrough.push_back(index);
  }
  if (passthrough.empty())
    return false;
  GenericBlock &mutableBody =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));
  GenericOperation &mutableYield = module.operations.at(
      static_cast<size_t>(mutableBody.operations.back()));
  for (auto iterator = passthrough.rbegin(); iterator != passthrough.rend();
       ++iterator) {
    const size_t index = *iterator;
    const int init = loop.operands[3 + index];
    ReplaceAllUses(module, loop.results[index], init);
    ReplaceAllUses(module, mutableBody.arguments[1 + index], init);
    loop.results.erase(loop.results.begin() + static_cast<std::ptrdiff_t>(index));
    loop.resultTypes.erase(loop.resultTypes.begin() +
                           static_cast<std::ptrdiff_t>(index));
    loop.operands.erase(loop.operands.begin() +
                        static_cast<std::ptrdiff_t>(3 + index));
    loop.operandTypes.erase(loop.operandTypes.begin() +
                           static_cast<std::ptrdiff_t>(3 + index));
    mutableBody.arguments.erase(
        mutableBody.arguments.begin() + static_cast<std::ptrdiff_t>(1 + index));
    mutableBody.argumentTypes.erase(mutableBody.argumentTypes.begin() +
                                    static_cast<std::ptrdiff_t>(1 + index));
    mutableYield.operands.erase(mutableYield.operands.begin() +
                                static_cast<std::ptrdiff_t>(index));
    mutableYield.operandTypes.erase(mutableYield.operandTypes.begin() +
                                    static_cast<std::ptrdiff_t>(index));
  }
  if (loop.results.empty() && mutableBody.operations.size() == 1)
    GenericRewriter(module).removeFromBlock(loop.blockId, loop.id);
  return true;
}

inline std::string ExactCSEKey(const GenericOperation &operation) {
  std::ostringstream key;
  key << operation.name << '|' << joinIds(operation.operands) << '|';
  for (const std::string &type : operation.operandTypes)
    key << type << ',';
  key << '|' << operation.properties << '|' << operation.attributes << '|';
  for (const std::string &type : operation.resultTypes)
    key << type << ',';
  key << '|' << operation.effects;
  return key.str();
}

inline bool IsCanonicalizablePure(const GenericOperation &operation) {
  if (!HasModeledOperationSemantics(operation.name) ||
      (!operation.effects.empty() && operation.effects != "none") ||
      !operation.regions.empty() ||
      operation.results.empty())
    return false;
  static const std::set<std::string> neverErase = {
      "func.return", "scf.yield", "cf.br", "cf.cond_br"};
  return neverErase.count(operation.name) == 0;
}

inline bool RunOneCanonicalizationIteration(GenericModule &module) {
  bool changed = false;
  for (GenericOperation &operation : module.operations)
    if (operation.blockId >= 0)
      changed = FoldOneIntegerOperation(module, operation) || changed;

  for (GenericOperation &operation : module.operations) {
    if (operation.blockId < 0)
      continue;
    if (SimplifyOneTrivialLoop(module, operation)) {
      changed = true;
      break;
    }
  }
  if (changed) {
    module = CompactGenericModule(std::move(module));
    return true;
  }

  for (GenericBlock &block : module.blocks) {
    for (int operationId : block.operations) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "annotation.mark" ||
          (!IRDictionaryValue(operation.attributes, "buffer_size_in_byte").empty()) ||
          operation.attributes != "{}")
        continue;
      GenericRewriter(module).removeFromBlock(block.id, operation.id);
      module = CompactGenericModule(std::move(module));
      return true;
    }
  }

  std::map<std::pair<int, std::string>, int> representatives;
  for (GenericBlock &block : module.blocks) {
    for (int operationId : block.operations) {
      GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (!IsCanonicalizablePure(operation))
        continue;
      const auto key = std::make_pair(block.id, ExactCSEKey(operation));
      const auto found = representatives.find(key);
      if (found == representatives.end()) {
        representatives.emplace(key, operation.id);
        continue;
      }
      const GenericOperation &representative =
          module.operations.at(static_cast<size_t>(found->second));
      if (representative.results.size() != operation.results.size())
        continue;
      for (size_t index = 0; index < operation.results.size(); ++index)
        ReplaceAllUses(module, operation.results[index],
                       representative.results[index]);
      GenericRewriter(module).removeFromBlock(block.id, operation.id);
      changed = true;
      break;
    }
    if (changed)
      break;
  }
  if (changed) {
    module = CompactGenericModule(std::move(module));
    return true;
  }

  for (GenericBlock &block : module.blocks) {
    for (auto iterator = block.operations.rbegin();
         iterator != block.operations.rend(); ++iterator) {
      GenericOperation &operation =
          module.operations.at(static_cast<size_t>(*iterator));
      if (!IsCanonicalizablePure(operation))
        continue;
      const bool unused =
          std::all_of(operation.results.begin(), operation.results.end(),
                      [&](int value) { return !HasAttachedUsers(module, value); });
      if (!unused)
        continue;
      GenericRewriter(module).removeFromBlock(block.id, operation.id);
      changed = true;
      break;
    }
    if (changed)
      break;
  }
  if (changed)
    module = CompactGenericModule(std::move(module));
  return changed;
}

inline StageResult RunPreSplitCanonicalization(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  // These passes are larger than the targeted model below.  Fail closed when
  // a real canonicalization family can successfully change UB-observable IR
  // and no equivalent rewrite is implemented here.
  for (const GenericOperation &operation : result.module.operations) {
    std::string family;
    std::string detail;
    if (operation.name == "scf.if" || operation.name == "scf.while")
      family = "SCF canonicalization";
    else if (operation.name == "scf.for" && operation.operands.size() >= 3) {
      const auto lower = ConstantIntegerValue(result.module,
                                               operation.operands[0]);
      const auto upper = ConstantIntegerValue(result.module,
                                               operation.operands[1]);
      const auto step = ConstantIntegerValue(result.module,
                                              operation.operands[2]);
      if (lower && upper && step && *step > 0 && *lower >= *upper) {
        family = "SCF canonicalization";
        detail = "zero-trip ";
      } else if (operation.regions.size() == 1) {
        const GenericOperation *yield = CanonicalSingleBlockTerminator(
            result.module, operation.regions.front());
        const GenericRegion &region = result.module.regions.at(
            static_cast<size_t>(operation.regions.front()));
        if (yield != nullptr && yield->name == "scf.yield" &&
            region.blocks.size() == 1) {
          const GenericBlock &body = result.module.blocks.at(
              static_cast<size_t>(region.blocks.front()));
          for (size_t index = 0; index < operation.results.size(); ++index) {
            if (index + 3 >= operation.operands.size() ||
                index + 1 >= body.arguments.size() ||
                index >= yield->operands.size())
              continue;
            const int yielded = yield->operands[index];
            const bool modeledPassthrough =
                yielded == operation.operands[3 + index] ||
                yielded == body.arguments[1 + index];
            const GenericOperation *yieldedDefinition =
                DefinitionForBufferSize(result.module, yielded);
            const bool nestedEquivalenceCandidate =
                yieldedDefinition != nullptr &&
                (yieldedDefinition->name == "scf.if" ||
                 yieldedDefinition->name == "scf.for" ||
                 yieldedDefinition->name == "scf.while") &&
                IsNestedUnderOperation(result.module, yieldedDefinition->id,
                                       operation.id);
            if (!modeledPassthrough &&
                (!HasAttachedUsers(result.module, operation.results[index]) ||
                 nestedEquivalenceCandidate)) {
              family = "SCF canonicalization";
              detail = nestedEquivalenceCandidate ? "iter-arg equivalence "
                                                   : "unused-result fold ";
              break;
            }
          }
        }
      }
    } else if (ApplicableTensorReshapeCanonicalization(result.module,
                                                        operation)) {
      family = "tensor slice/reshape canonicalization";
    } else if (ApplicableMemRefReshapeCanonicalization(result.module,
                                                        operation)) {
      family = "extended memref reshape canonicalization";
    }
    else if (ApplicableHIVMCanonicalization(result.module, operation))
      family = "HIVM canonicalization";
    else if (ApplicableInlineBroadcastCanonicalization(operation))
      family = "HIVM inline-broadcast canonicalization";
    else if (ApplicableHIVMMemRefCastFold(result.module, operation))
      family = "HIVM memref-cast fold";
    else if (ApplicableConvertLayoutCanonicalization(result.module, operation))
      family = "HIVM convert-layout canonicalization";
    if (family.empty())
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"CanonicalizationBeforeSplit", "", operation.id, operation.name,
         "unmodeled applicable " + detail + family + " pattern"});
  }
  if (result.precision == Precision::Incomplete)
    return result;
  constexpr size_t kMaxIterations = 10000;
  size_t iterations = 0;
  while (RunOneCanonicalizationIteration(result.module)) {
    ++iterations;
    if (iterations == kMaxIterations) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"CanonicalizationBeforeSplit", "", -1, "",
           "canonicalization did not reach a fixed point"});
      break;
    }
  }
  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
