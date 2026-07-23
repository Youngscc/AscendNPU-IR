#ifndef CVPIPELINE_UB_MODEL_CPP_MARK_REAL_CORE_TYPE_HPP
#define CVPIPELINE_UB_MODEL_CPP_MARK_REAL_CORE_TYPE_HPP

#include "canonicalization_hivm_pipeline.hpp"
#include "split_mix_kernel.hpp"

namespace cvub {

inline bool IsMarkRealCoreTypeOperation(const GenericOperation &operation) {
  static const std::unordered_set<std::string> scalarPipeOperations = {
      "memref.load",          "memref.store",
      "affine.load",          "affine.store",
      "tensor.extract",       "tensor.insert",
      "tensor.insert_slice",  "tensor.extract_slice"};
  if (scalarPipeOperations.count(operation.name) != 0)
    return true;
  if (operation.name == "hivm.hir.custom" ||
      operation.name == "hivm.hir.custom_macro")
    return false;

  static const std::unordered_set<std::string> inferCoreTypeOperations = {
      "hivm.hir.load",          "hivm.hir.store",
      "hivm.hir.copy",          "hivm.hir.atomic_rmw",
      "hivm.hir.vbrc",          "hivm.hir.convert_layout",
      "hivm.hir.debug",         "hivm.hir.matmul",
      "hivm.hir.mix_matmul",    "hivm.hir.mix_group_matmul",
      "hivm.hir.sync_block",    "hivm.hir.sync_block_set",
      "hivm.hir.sync_block_wait", "hivm.hir.pipe_barrier"};
  return inferCoreTypeOperations.count(operation.name) != 0;
}

inline std::string RemoveMarkRealCoreTypeDictionaryValue(
    const std::string &dictionary, const std::string &name) {
  // The cleanup MarkRealCoreType pass visits every potentially annotated
  // operation, while only a small subset normally carries tcore_type.  Avoid
  // tokenizing and reconstructing an unchanged attribute dictionary.
  if (dictionary.find(name) == std::string::npos)
    return dictionary;
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return dictionary;
  std::vector<std::string> kept;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    if (trim(entry.substr(0, equal)) != name)
      kept.push_back(trim(entry));
  }
  std::ostringstream output;
  output << '{';
  for (size_t index = 0; index < kept.size(); ++index)
    output << (index == 0 ? "" : ", ") << kept[index];
  return output.str() + '}';
}

inline GenericModule AddMarkRealCoreTypeInstructionMarkers(
    GenericModule module) {
  for (GenericOperation &operation : module.operations) {
    if (operation.name == "builtin.module")
      continue;
    // The suffix uses a temporary IR attribute because MLIR operations do not
    // have model-side provenance.  Carry the same identity in analysis-only
    // metadata instead: projection copies and compaction preserve it without
    // rebuilding and reparsing every attribute dictionary twice.
    operation.projectionSourceId = operation.id;
  }
  return module;
}

inline std::vector<uint8_t> CollectMarkedRealCoreTypeProjection(
    GenericModule projection, SplitMixCoreType retainedCore,
    size_t sourceOperationCount) {
  projection =
      RunSplitMixKernelProjection(std::move(projection), retainedCore);
  projection = RunCanonicalizationHIVMPipeline(std::move(projection));

  std::vector<uint8_t> retained(sourceOperationCount, uint8_t{0});
  for (const GenericOperation &operation : projection.operations)
    if (operation.projectionSourceId >= 0 &&
        static_cast<size_t>(operation.projectionSourceId) < retained.size())
      retained[static_cast<size_t>(operation.projectionSourceId)] = 1;
  return retained;
}

inline std::vector<uint8_t> CollectMarkRealCoreTypeProjection(
    const GenericModule &module, SplitMixCoreType retainedCore) {
  return CollectMarkedRealCoreTypeProjection(
      AddMarkRealCoreTypeInstructionMarkers(module), retainedCore,
      module.operations.size());
}

struct MarkRealCoreTypeProjectionSets {
  std::vector<uint8_t> cube;
  std::vector<uint8_t> vector;
};

inline MarkRealCoreTypeProjectionSets
CollectMarkedRealCoreTypeCombinedProjection(GenericModule projection,
                                            size_t sourceOperationCount) {
  projection = RunSplitMixKernelCombinedProjection(std::move(projection));
  projection = RunCanonicalizationHIVMPipeline(std::move(projection));

  MarkRealCoreTypeProjectionSets retained{
      std::vector<uint8_t>(sourceOperationCount, uint8_t{0}),
      std::vector<uint8_t>(sourceOperationCount, uint8_t{0})};
  const GenericModuleAnalysisSnapshot analysis(
      projection, kGenericAnalysisFunctionDescendants);
  auto retainOperation = [&](int operationId, bool inCube, bool inVector) {
    const GenericOperation &operation =
        projection.operations.at(static_cast<size_t>(operationId));
    if (operation.projectionSourceId < 0 ||
        static_cast<size_t>(operation.projectionSourceId) >=
            sourceOperationCount)
      return;
    const size_t source =
        static_cast<size_t>(operation.projectionSourceId);
    if (inCube)
      retained.cube[source] = 1;
    if (inVector)
      retained.vector[source] = 1;
  };
  for (const GenericOperation &function : projection.operations) {
    if (function.name != "func.func")
      continue;
    const bool partOfMix =
        HasSplitMixDictionaryEntry(function.attributes, "hivm.part_of_mix");
    const std::string core = SplitMixEnumValue(FindDictionaryValue(
        function.attributes, "hivm.func_core_type"));
    const bool inCube = !partOfMix || core == "AIC";
    const bool inVector = !partOfMix || core == "AIV";
    retainOperation(function.id, inCube, inVector);
    for (int operationId : analysis.descendants(function))
      retainOperation(operationId, inCube, inVector);
  }
  return retained;
}

inline GenericModule RunMarkRealCoreType(GenericModule module,
                                         bool removeCoreTypeAttrs = false,
                                         bool inputCanonicalized = false) {
  if (removeCoreTypeAttrs) {
    for (GenericOperation &operation : module.operations) {
      if (operation.attributes.find("hivm.tcore_type") == std::string::npos)
        continue;
      if (!IsMarkRealCoreTypeOperation(operation))
        continue;
      operation.attributes = RemoveMarkRealCoreTypeDictionaryValue(
          operation.attributes, "hivm.tcore_type");
    }
    return module;
  }

  // MIX inputs still require independent Cube and Vector projections.  Both
  // start from the same instruction-marked snapshot so marker construction is
  // shared while their projection/canonicalization semantics remain exact.
  const size_t sourceOperationCount = module.operations.size();
  const bool hasMixFunction = std::any_of(
      module.operations.begin(), module.operations.end(),
      [](const GenericOperation &operation) {
        return IsSplitMixFunction(operation);
      });
  std::vector<uint8_t> cubeOperations;
  std::vector<uint8_t> vectorOperations;
  const bool allOperationsInBothProjections =
      !hasMixFunction && inputCanonicalized;
  // The enclosing suffix/model pipeline has just reached the fixed point of
  // this same canonicalizer.  With no MIX function the SplitMix projection is
  // the identity for both cores, so every current operation survives both
  // projections.  Standalone callers stay conservative through the explicit
  // inputCanonicalized contract.
  if (!allOperationsInBothProjections) {
    GenericModule marked = AddMarkRealCoreTypeInstructionMarkers(module);
    if (!hasMixFunction) {
      cubeOperations = CollectMarkedRealCoreTypeProjection(
          std::move(marked), SplitMixCoreType::Vector, sourceOperationCount);
      vectorOperations = cubeOperations;
    } else {
      MarkRealCoreTypeProjectionSets retained =
          CollectMarkedRealCoreTypeCombinedProjection(std::move(marked),
                                                      sourceOperationCount);
      cubeOperations = std::move(retained.cube);
      vectorOperations = std::move(retained.vector);
    }
  }

  for (GenericOperation &operation : module.operations) {
    if (!IsMarkRealCoreTypeOperation(operation))
      continue;
    const size_t operationId = static_cast<size_t>(operation.id);
    const bool inCube =
        allOperationsInBothProjections ||
        (operationId < cubeOperations.size() && cubeOperations[operationId]);
    const bool inVector =
        allOperationsInBothProjections ||
        (operationId < vectorOperations.size() &&
         vectorOperations[operationId]);
    if (!inCube && !inVector)
      continue;
    const char *coreType = inCube && inVector
                               ? "CUBE_AND_VECTOR"
                               : (inCube ? "CUBE" : "VECTOR");
    operation.attributes = SetCanonicalizationDictionaryValue(
        operation.attributes, "hivm.tcore_type",
        std::string("#hivm.tcore_type<") + coreType + ">");
  }
  return module;
}

} // namespace cvub

#endif
