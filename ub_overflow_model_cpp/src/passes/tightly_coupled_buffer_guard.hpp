#ifndef CVPIPELINE_UB_MODEL_CPP_TIGHTLY_COUPLED_BUFFER_GUARD_HPP
#define CVPIPELINE_UB_MODEL_CPP_TIGHTLY_COUPLED_BUFFER_GUARD_HPP

#include "../pipeline/modeling_result.hpp"

namespace cvub {

inline bool IsAscend950Target(const GenericModule &module) {
  if (module.operations.empty())
    return false;
  const GenericOperation &root = module.operations.front();
  const std::string text = root.attributes + " " + root.properties;
  if (text.find("hacc.target") == std::string::npos)
    return false;
  return text.find("Ascend950") != std::string::npos ||
         text.find("Ascend910_9599") != std::string::npos;
}

inline bool IsTightlyCoupledAllocationCandidate(
    const GenericOperation &operation) {
  if (operation.name != "memref.alloc" || operation.resultTypes.empty())
    return false;
  std::string type = operation.resultTypes.front();
  std::transform(type.begin(), type.end(), type.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  return type.find("#hivm.address_space<ub>") != std::string::npos ||
         type.find("#hivm.address_space<cbuf>") != std::string::npos;
}

// The real pipeline marks and may hoist L1/UB allocations on Ascend950 before
// SplitMixKernel. Until the model has oracle-proven anchor/rotation semantics,
// reject only modules where the real passes are applicable. Other targets are
// a proven no-op in the real pass implementation.
inline StageResult GuardTightlyCoupledBufferPasses(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  if (!IsAscend950Target(result.module))
    return result;
  for (const GenericOperation &operation : result.module.operations) {
    if (!IsTightlyCoupledAllocationCandidate(operation))
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"MarkTightlyCoupledBuffer;HoistTightlyCoupledAlloc", "",
         operation.id, operation.name,
         "Ascend950 L1/UB allocation marking and hoisting is not modeled "
         "exactly"});
    return result;
  }
  return result;
}

} // namespace cvub

#endif
