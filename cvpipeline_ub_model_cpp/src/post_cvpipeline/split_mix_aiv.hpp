#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_SPLIT_MIX_AIV_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_SPLIT_MIX_AIV_HPP

#include "core_type.hpp"
#include "types.hpp"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cvub {

inline std::string FunctionCoreType(const GenericOperation &function) {
  return trim(IRDictionaryValue(function.attributes, "hivm.func_core_type"));
}

inline std::string SetUnitDictionaryAttribute(const std::string &dictionary,
                                              const std::string &name) {
  std::vector<std::string> entries;
  if (dictionary.size() >= 2 && dictionary.front() == '{' &&
      dictionary.back() == '}')
    entries = splitTopLevel(dictionary.substr(1, dictionary.size() - 2));
  entries.erase(std::remove_if(entries.begin(), entries.end(),
                               [&](const std::string &entry) {
                                 const size_t equal = entry.find('=');
                                 return trim(entry.substr(0, equal)) == name;
                               }),
                entries.end());
  entries.push_back(name);
  std::string result = "{";
  for (size_t index = 0; index < entries.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += trim(entries[index]);
  }
  return result + "}";
}

struct ReplacementPlan {
  bool safe = false;
  std::vector<int> values;
  std::string reason;
};

inline bool ResultIsUsed(const GenericModule &module, int value) {
  return ModuleReferencesValue(module, value);
}

inline std::set<int> RegionDefinedValues(const GenericModule &module,
                                         const GenericOperation &operation) {
  std::set<int> values;
  std::function<void(int)> visit = [&](int operationId) {
    const GenericOperation &nested =
        module.operations.at(static_cast<size_t>(operationId));
    values.insert(nested.results.begin(), nested.results.end());
    for (int regionId : nested.regions)
      for (int blockId : module.regions.at(static_cast<size_t>(regionId)).blocks) {
        const GenericBlock &block =
            module.blocks.at(static_cast<size_t>(blockId));
        values.insert(block.arguments.begin(), block.arguments.end());
        for (int child : block.operations)
          visit(child);
      }
  };
  for (int regionId : operation.regions)
    for (int blockId : module.regions.at(static_cast<size_t>(regionId)).blocks) {
      const GenericBlock &block = module.blocks.at(static_cast<size_t>(blockId));
      values.insert(block.arguments.begin(), block.arguments.end());
      for (int child : block.operations)
        visit(child);
    }
  return values;
}

inline const GenericOperation *SingleBlockTerminator(
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

inline int CreateScopeStub(GenericModule &module,
                           const GenericOperation &scope,
                           const std::string &type) {
  std::string name;
  std::string attributes = "{}";
  if (startsWith(trim(type), "tensor<")) {
    name = "tensor.empty";
  } else if (startsWith(trim(type), "i") || trim(type) == "index") {
    name = "arith.constant";
    attributes = "{value = 0 : " + trim(type) + "}";
  } else if (startsWith(trim(type), "f") || startsWith(trim(type), "bf")) {
    name = "arith.constant";
    attributes = "{value = 0.0 : " + trim(type) + "}";
  } else {
    return -1;
  }
  GenericRewriter rewriter(module);
  const int stub = rewriter.createOperation(scope.parentId, scope.regionId,
                                            scope.blockId, name, {type}, {}, {},
                                            "", attributes);
  rewriter.insertToBlock(scope.blockId, static_cast<size_t>(scope.ordinal), stub);
  return module.operations.at(static_cast<size_t>(stub)).results.front();
}

inline ReplacementPlan LoopReplacementPlan(
    const GenericModule &, const GenericOperation &operation) {
  if (operation.results.size() > operation.operands.size())
    return {false, {}, "loop has fewer initial values than results"};
  return {true,
          std::vector<int>(operation.operands.end() -
                               static_cast<std::ptrdiff_t>(operation.results.size()),
                           operation.operands.end()),
          ""};
}

inline ReplacementPlan ScopeReplacementPlan(GenericModule &module,
                                              const GenericOperation &scope) {
  if (scope.regions.size() != 1)
    return {false, {}, "scope does not have exactly one region"};
  const GenericOperation *terminator =
      SingleBlockTerminator(module, scope.regions.front());
  if (terminator == nullptr)
    return {false, {}, "scope has no supported single-block terminator"};
  if (terminator->operands.size() != scope.results.size())
    return {false, {}, "scope yield/result count mismatch"};

  const std::vector<int> yieldedValues = terminator->operands;
  const std::set<int> internal = RegionDefinedValues(module, scope);
  ReplacementPlan plan{true, {}, ""};
  plan.values.reserve(scope.results.size());
  for (size_t index = 0; index < scope.results.size(); ++index) {
    if (!ResultIsUsed(module, scope.results[index])) {
      plan.values.push_back(-1);
      continue;
    }
    const int yielded = yieldedValues[index];
    if (internal.count(yielded) == 0) {
      plan.values.push_back(yielded);
      continue;
    }
    const int stub = CreateScopeStub(module, scope, scope.resultTypes[index]);
    if (stub < 0)
      return {false, {}, "scope result type cannot be safely stubbed: " +
                             scope.resultTypes[index]};
    plan.values.push_back(stub);
  }
  return plan;
}

inline ReplacementPlan IfReplacementPlan(const GenericModule &module,
                                          const GenericOperation &operation) {
  if (operation.regions.empty())
    return {false, {}, "if has no branches"};
  std::vector<const GenericOperation *> terminators;
  for (int regionId : operation.regions) {
    const GenericOperation *terminator =
        SingleBlockTerminator(module, regionId);
    if (terminator == nullptr ||
        terminator->operands.size() != operation.results.size())
      return {false, {}, "if branch yield/result count mismatch"};
    terminators.push_back(terminator);
  }
  const std::set<int> internal = RegionDefinedValues(module, operation);
  ReplacementPlan plan{true, {}, ""};
  for (size_t index = 0; index < operation.results.size(); ++index) {
    if (!ResultIsUsed(module, operation.results[index])) {
      plan.values.push_back(-1);
      continue;
    }
    const int candidate = terminators.front()->operands[index];
    if (internal.count(candidate) != 0 ||
        std::any_of(terminators.begin() + 1, terminators.end(),
                    [&](const GenericOperation *terminator) {
                      return terminator->operands[index] != candidate;
                    }))
      return {false, {},
              "if branch yields do not share a safe external replacement"};
    plan.values.push_back(candidate);
  }
  return plan;
}

inline ReplacementPlan ReplacementValues(GenericModule &module,
                                          const GenericOperation &operation) {
  if (!operation.dpsInits.empty()) {
    if (operation.dpsInits.size() != operation.results.size())
      return {false, {}, "DPS init/result count mismatch"};
    return {true, operation.dpsInits, ""};
  }
  if (operation.name == "scf.for" || operation.name == "scf.while")
    return LoopReplacementPlan(module, operation);
  if (operation.name == "scope.scope")
    return ScopeReplacementPlan(module, operation);
  if (operation.name == "scf.if")
    return IfReplacementPlan(module, operation);
  if (operation.results.empty())
    return {true, {}, ""};
  return {false, {}, "operation has no safe result replacement rule"};
}

inline std::vector<int> PostOrderOperations(const GenericModule &module,
                                            int operationId) {
  std::vector<int> result;
  std::function<void(int)> visit = [&](int id) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(id));
    for (int regionId : operation.regions)
      for (int blockId : module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child : module.blocks.at(static_cast<size_t>(blockId)).operations)
          visit(child);
    result.push_back(id);
  };
  visit(operationId);
  return result;
}

inline void ReplaceOperationResults(GenericModule &module, int operationId,
                                    const std::vector<int> &replacements) {
  const std::vector<int> results =
      module.operations.at(static_cast<size_t>(operationId)).results;
  if (results.size() != replacements.size())
    throw std::runtime_error("MIX projection: replacement/result count mismatch for " +
                             module.operations.at(static_cast<size_t>(operationId)).name);
  for (size_t index = 0; index < results.size(); ++index)
    if (replacements[index] >= 0)
      ReplaceAllUses(module, results[index], replacements[index]);
}

inline bool HasAttribute(const GenericOperation &operation,
                         const std::string &name) {
  const auto dictionaryHas = [&](const std::string &dictionary) {
    if (dictionary.size() < 2 || dictionary.front() != '{' ||
        dictionary.back() != '}')
      return false;
    for (const std::string &entry :
         splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
      const size_t equal = entry.find('=');
      std::string key = trim(entry.substr(0, equal));
      if (key.size() >= 2 && key.front() == '"' && key.back() == '"')
        key = key.substr(1, key.size() - 2);
      if (key == name)
        return true;
    }
    return false;
  };
  return dictionaryHas(operation.attributes) ||
         dictionaryHas(operation.properties);
}

inline void PostProcessVectorFunction(GenericModule &module, int functionId) {
  static const std::string replacementLabel =
      "DuplicateTensorExtractForCube::replacementLabel";
  static const std::string newExtractLabel =
      "DuplicateTensorExtractForCube::newExtractLabel";
  std::vector<int> marks;
  std::vector<int> extracts;
  for (int operationId : PostOrderOperations(module, functionId)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "annotation.mark" &&
        HasAttribute(operation, replacementLabel))
      marks.push_back(operationId);
    else if (operation.name == "tensor.extract" &&
             HasAttribute(operation, newExtractLabel))
      extracts.push_back(operationId);
  }
  for (int operationId : marks)
    EraseOperationTree(module, operationId);
  for (int operationId : extracts)
    EraseOperationTree(module, operationId);
}

inline void AddIncompleteDiagnostic(
    std::vector<PostCVPipelineDiagnostic> &diagnostics, Precision &precision,
    const std::string &functionName, const GenericOperation &operation,
    const std::string &reason) {
  precision = Precision::Incomplete;
  diagnostics.push_back({"SplitMixKernelAIVProjection", functionName,
                         operation.id, operation.name, reason});
}

inline void ScanUnsupportedOperations(
    const GenericModule &module, int functionId, const std::string &functionName,
    std::vector<PostCVPipelineDiagnostic> &diagnostics, Precision &precision) {
  for (int operationId : PostOrderOperations(module, functionId)) {
    if (operationId == functionId)
      continue;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    const CoreClassification classification =
        ClassifyCoreDetailed(module, operation);
    if (classification.kind == CoreKind::Unknown)
      AddIncompleteDiagnostic(diagnostics, precision, functionName, operation,
                              classification.reason);
    else if (classification.kind == CoreKind::Cube)
      AddIncompleteDiagnostic(
          diagnostics, precision, functionName, operation,
          "already-AIV function contains Cube-classified operation");
  }
}

inline ProjectedAIVModule ProjectMixFunction(
    GenericModule module, int sourceFunctionId,
    std::vector<PostCVPipelineDiagnostic> &diagnostics, Precision &precision) {
  const std::string sourceName = FunctionSymbolName(
      module.operations.at(static_cast<size_t>(sourceFunctionId)));
  const std::string projectedName = sourceName + "_mix_aiv";
  GenericOperation &function =
      module.operations.at(static_cast<size_t>(sourceFunctionId));
  const std::string quotedProjectedName = "\"" + projectedName + "\"";
  function.properties =
      SetDictionaryValue(function.properties, "sym_name", quotedProjectedName);
  function.attributes =
      SetDictionaryValue(function.attributes, "sym_name", quotedProjectedName);
  function.attributes = SetDictionaryValue(
      function.attributes, "hivm.func_core_type", "#hivm.func_core_type<AIV>");
  function.attributes =
      SetUnitDictionaryAttribute(function.attributes, "hivm.part_of_mix");

  const std::vector<int> postOrder =
      PostOrderOperations(module, sourceFunctionId);
  for (int operationId : postOrder) {
    if (operationId == sourceFunctionId)
      continue;
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    const CoreClassification classification =
        ClassifyCoreDetailed(module, snapshot);
    if (classification.kind == CoreKind::Unknown) {
      AddIncompleteDiagnostic(diagnostics, precision, sourceName, snapshot,
                              classification.reason);
      continue;
    }
    if (classification.kind != CoreKind::Cube)
      continue;
    const ReplacementPlan replacements = ReplacementValues(module, snapshot);
    if (!replacements.safe) {
      AddIncompleteDiagnostic(diagnostics, precision, sourceName, snapshot,
                              replacements.reason);
      continue;
    }
    ReplaceOperationResults(module, operationId, replacements.values);
    EraseOperationTree(module, operationId);
  }

  PostProcessVectorFunction(module, sourceFunctionId);
  module = CompactGenericModule(std::move(module));
  GenericModule isolated = ExtractFunctionModule(module, projectedName);
  ValidateGenericModule(isolated);
  return {sourceName, projectedName, std::move(isolated)};
}

inline PostCVPipelineResult ProjectMixFunctionsToAIV(GenericModule module) {
  ValidateGenericModule(module);
  PostCVPipelineResult result;
  std::vector<int> mixFunctions;
  std::vector<int> aivFunctions;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "func.func" || operation.regions.empty() ||
        trim(IRDictionaryValue(operation.attributes, "hacc.function_kind")) !=
            "#hacc.function_kind<DEVICE>")
      continue;
    const std::string core = FunctionCoreType(operation);
    if (core == "#hivm.func_core_type<MIX>")
      mixFunctions.push_back(operation.id);
    else if (core == "#hivm.func_core_type<AIV>")
      aivFunctions.push_back(operation.id);
  }

  for (int functionId : mixFunctions)
    result.functions.push_back(ProjectMixFunction(
        module, functionId, result.diagnostics, result.precision));
  for (int functionId : aivFunctions) {
    const std::string functionName = FunctionSymbolName(
        module.operations.at(static_cast<size_t>(functionId)));
    ScanUnsupportedOperations(module, functionId, functionName,
                              result.diagnostics, result.precision);
    GenericModule isolated = ExtractFunctionModule(module, functionName);
    ValidateGenericModule(isolated);
    result.functions.push_back(
        {functionName, functionName, std::move(isolated)});
  }
  return result;
}

} // namespace cvub

#endif
