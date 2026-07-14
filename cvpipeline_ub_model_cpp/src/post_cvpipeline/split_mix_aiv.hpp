#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_SPLIT_MIX_AIV_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_SPLIT_MIX_AIV_HPP

#include "core_type.hpp"
#include "types.hpp"

#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cvub {

inline std::string FunctionCoreType(const GenericOperation &function) {
  return trim(IRDictionaryValue(function.attributes, "hivm.func_core_type"));
}

inline std::vector<int> LoopInitialValues(const GenericModule &,
                                          const GenericOperation &operation) {
  if (operation.results.size() > operation.operands.size())
    throw std::runtime_error("MIX projection: loop has fewer initial values than results");
  return std::vector<int>(operation.operands.end() -
                              static_cast<std::ptrdiff_t>(operation.results.size()),
                          operation.operands.end());
}

inline std::vector<int>
YieldedOrExternalValues(const GenericModule &module,
                        const GenericOperation &operation) {
  std::vector<int> replacements;
  for (int regionId : operation.regions) {
    const GenericRegion &region = module.regions.at(static_cast<size_t>(regionId));
    if (region.blocks.size() != 1)
      throw std::runtime_error("MIX projection: unsupported multi-block yielded region");
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    if (block.operations.empty())
      throw std::runtime_error("MIX projection: yielded region has no terminator");
    const GenericOperation &terminator = module.operations.at(
        static_cast<size_t>(block.operations.back()));
    if (terminator.operands.size() != operation.results.size())
      throw std::runtime_error("MIX projection: yield/result count mismatch");
    if (replacements.empty())
      replacements = terminator.operands;
    else if (replacements != terminator.operands)
      throw std::runtime_error(
          "MIX projection: branch yields do not share external replacement values");
  }
  return replacements;
}

inline std::vector<int> ReplacementValues(const GenericModule &module,
                                          const GenericOperation &operation) {
  if (!operation.dpsInits.empty())
    return operation.dpsInits;
  if (operation.name == "scf.for" || operation.name == "scf.while")
    return LoopInitialValues(module, operation);
  if (operation.name == "scf.if" || operation.name == "scope.scope")
    return YieldedOrExternalValues(module, operation);
  if (operation.results.empty())
    return {};
  throw std::runtime_error("MIX projection: operation " + operation.name +
                           " has no result replacement rule");
}

inline std::vector<int> PostOrderOperations(const GenericModule &module,
                                            int operationId) {
  std::vector<int> result;
  std::function<void(int)> visit = [&](int id) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(id));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
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
    ReplaceAllUses(module, results[index], replacements[index]);
}

inline void RemoveCubeOnlyMarks(GenericModule &module,
                                const GenericOperation &cubeOperation) {
  const std::set<int> cubeResults(cubeOperation.results.begin(),
                                  cubeOperation.results.end());
  std::vector<int> marks;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "annotation.mark" || operation.blockId < 0)
      continue;
    const bool referencesCube =
        std::any_of(operation.operands.begin(), operation.operands.end(),
                    [&](int value) { return cubeResults.count(value) != 0; });
    if (referencesCube)
      marks.push_back(operation.id);
  }
  for (int mark : marks)
    EraseOperationTree(module, mark);
}

inline void RemoveDeadCubeSupportOperations(GenericModule &module,
                                            int functionId) {
  static const std::set<std::string> removable = {
      "tensor.empty", "tensor.extract", "tensor.extract_slice",
      "tensor.collapse_shape", "tensor.expand_shape"};
  bool changed = true;
  while (changed) {
    changed = false;
    const std::vector<int> postOrder = PostOrderOperations(module, functionId);
    for (int operationId : postOrder) {
      if (operationId == functionId)
        continue;
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (removable.count(operation.name) == 0 || operation.results.empty() ||
          std::any_of(operation.results.begin(), operation.results.end(),
                      [&](int value) {
                        return ModuleReferencesValue(module, value);
                      }))
        continue;
      EraseOperationTree(module, operationId);
      changed = true;
    }
  }
}

inline ProjectedAIVModule
ProjectMixFunction(GenericModule module, int sourceFunctionId,
                   std::vector<PostCVPipelineDiagnostic> &diagnostics,
                   Precision &precision) {
  const std::string sourceName = FunctionSymbolName(
      module.operations.at(static_cast<size_t>(sourceFunctionId)));
  const std::string projectedName = sourceName + "_mix_aiv";
  const int projectedFunction = sourceFunctionId;
  GenericOperation &function =
      module.operations.at(static_cast<size_t>(projectedFunction));
  const std::string quotedProjectedName = "\"" + projectedName + "\"";
  function.properties = SetDictionaryValue(function.properties, "sym_name",
                                            quotedProjectedName);
  function.attributes = SetDictionaryValue(function.attributes, "sym_name",
                                            quotedProjectedName);
  function.attributes = SetDictionaryValue(
      function.attributes, "hivm.func_core_type", "#hivm.func_core_type<AIV>");
  function.attributes =
      SetDictionaryValue(function.attributes, "hivm.part_of_mix", "true");
  function.attributes =
      SetDictionaryValue(function.attributes, "mix_mode", "\"aiv\"");

  const std::vector<int> postOrder = PostOrderOperations(module, projectedFunction);
  for (int operationId : postOrder) {
    if (operationId == projectedFunction)
      continue;
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    const CoreClassification classification =
        ClassifyCoreDetailed(module, snapshot);
    if (classification.kind == CoreKind::Unknown) {
      precision = Precision::Incomplete;
      diagnostics.push_back({"SplitMixKernelAIVProjection", sourceName,
                             operationId, snapshot.name,
                             classification.reason});
      continue;
    }
    if (classification.kind != CoreKind::Cube)
      continue;
    const std::vector<int> replacements = ReplacementValues(module, snapshot);
    RemoveCubeOnlyMarks(module, snapshot);
    ReplaceOperationResults(module, operationId, replacements);
    EraseOperationTree(module, operationId);
  }

  RemoveDeadCubeSupportOperations(module, projectedFunction);
  module = CompactGenericModule(std::move(module));
  GenericModule isolated = ExtractFunctionModule(module, projectedName);
  ValidateGenericModule(isolated);
  return {sourceName, projectedName, std::move(isolated)};
}

inline PostCVPipelineResult ProjectMixFunctionsToAIV(GenericModule module) {
  ValidateGenericModule(module);
  PostCVPipelineResult result;
  std::vector<int> mixFunctions;
  std::vector<std::string> aivFunctions;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "func.func" || operation.regions.empty() ||
        trim(IRDictionaryValue(operation.attributes, "hacc.function_kind")) !=
            "#hacc.function_kind<DEVICE>")
      continue;
    const std::string core = FunctionCoreType(operation);
    if (core == "#hivm.func_core_type<MIX>")
      mixFunctions.push_back(operation.id);
    else if (core == "#hivm.func_core_type<AIV>")
      aivFunctions.push_back(FunctionSymbolName(operation));
  }

  for (int functionId : mixFunctions)
    result.functions.push_back(ProjectMixFunction(
        module, functionId, result.diagnostics, result.precision));
  for (const std::string &functionName : aivFunctions) {
    GenericModule isolated = ExtractFunctionModule(module, functionName);
    ValidateGenericModule(isolated);
    result.functions.push_back(
        {functionName, functionName, std::move(isolated)});
  }
  return result;
}

} // namespace cvub

#endif
