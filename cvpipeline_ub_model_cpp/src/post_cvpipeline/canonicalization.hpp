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
    if (operation.name == "scf.if" || operation.name == "scf.while")
      family = "SCF canonicalization";
    else if (operation.name == "memref.store")
      family = "memref dead-store elimination";
    else if (operation.name == "hivm.hir.vbrc" &&
             !operation.operandTypes.empty() && !operation.resultTypes.empty() &&
             operation.operandTypes.front() == operation.resultTypes.front())
      family = "HIVM one-to-one broadcast canonicalization";
    if (family.empty())
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"CanonicalizationBeforeSplit", "", operation.id, operation.name,
         "unmodeled applicable " + family + " pattern"});
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
