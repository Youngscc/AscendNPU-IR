#ifndef CVPIPELINE_UB_MODEL_CPP_MARK_REAL_CORE_TYPE_HPP
#define CVPIPELINE_UB_MODEL_CPP_MARK_REAL_CORE_TYPE_HPP

#include "canonicalization_hivm_pipeline.hpp"
#include "split_mix_kernel.hpp"

namespace cvub {

constexpr const char *kMarkRealCoreTypeInstructionMarker =
    "instruction-marker";

inline bool IsMarkRealCoreTypeOperation(const GenericOperation &operation) {
  static const std::set<std::string> scalarPipeOperations = {
      "memref.load",          "memref.store",
      "affine.load",          "affine.store",
      "tensor.extract",       "tensor.insert",
      "tensor.insert_slice",  "tensor.extract_slice"};
  if (scalarPipeOperations.count(operation.name) != 0)
    return true;
  if (operation.name == "hivm.hir.custom" ||
      operation.name == "hivm.hir.custom_macro")
    return false;

  static const std::set<std::string> inferCoreTypeOperations = {
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

inline std::optional<int> MarkRealCoreTypeInstructionMarker(
    const GenericOperation &operation) {
  const std::string value = FindDictionaryValue(
      operation.attributes, kMarkRealCoreTypeInstructionMarker);
  if (value.empty())
    return std::nullopt;
  const size_t end = value.find_first_not_of("0123456789");
  try {
    return std::stoi(value.substr(0, end));
  } catch (const std::exception &) {
    throw std::runtime_error(
        "MarkRealCoreType: malformed instruction-marker attribute");
  }
}

inline GenericModule AddMarkRealCoreTypeInstructionMarkers(
    GenericModule module) {
  for (GenericOperation &operation : module.operations) {
    if (operation.name == "builtin.module")
      continue;
    operation.attributes = SetCanonicalizationDictionaryValue(
        operation.attributes, kMarkRealCoreTypeInstructionMarker,
        std::to_string(operation.id) + " : index");
  }
  return module;
}

inline std::set<int> CollectMarkRealCoreTypeProjection(
    const GenericModule &module, SplitMixCoreType retainedCore) {
  GenericModule projection = AddMarkRealCoreTypeInstructionMarkers(module);
  projection =
      RunSplitMixKernelProjection(std::move(projection), retainedCore);
  projection = RunCanonicalizationHIVMPipeline(std::move(projection));

  std::set<int> retained;
  for (const GenericOperation &operation : projection.operations)
    if (const std::optional<int> marker =
            MarkRealCoreTypeInstructionMarker(operation))
      retained.insert(*marker);
  return retained;
}

inline GenericModule RunMarkRealCoreType(GenericModule module,
                                         bool removeCoreTypeAttrs = false) {
  if (removeCoreTypeAttrs) {
    for (GenericOperation &operation : module.operations) {
      if (!IsMarkRealCoreTypeOperation(operation))
        continue;
      operation.attributes = RemoveMarkRealCoreTypeDictionaryValue(
          operation.attributes, "hivm.tcore_type");
    }
    return module;
  }

  const std::set<int> cubeOperations = CollectMarkRealCoreTypeProjection(
      module, SplitMixCoreType::Cube);
  const std::set<int> vectorOperations = CollectMarkRealCoreTypeProjection(
      module, SplitMixCoreType::Vector);

  for (GenericOperation &operation : module.operations) {
    if (!IsMarkRealCoreTypeOperation(operation))
      continue;
    const bool inCube = cubeOperations.count(operation.id) != 0;
    const bool inVector = vectorOperations.count(operation.id) != 0;
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
