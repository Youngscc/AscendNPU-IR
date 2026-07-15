#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_PLANNING_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_PLANNING_HPP

// Per-function suffix planning and module-level aggregation for the projected
// AIV modules produced by the post-CVPipeline pipeline.  Each isolated module
// (one projected AIV function) is run through the suffix pipeline + PlanMemory
// independently; plans are never merged or concatenated.  Module-level figures
// are the maxima of per-function effective requirements, never sums, because the
// functions execute independently.

#include "types.hpp"
#include "../suffix/suffix_pipeline.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cvub {

// The suffix + PlanMemory result for a single projected AIV function.  The
// selected retry seed is preserved per function rather than collapsed into a
// single module-level seed.
struct FunctionPlanResult {
  std::string sourceFunction;
  std::string function;
  PlanMemoryModelResult plan;
};

// Aggregated UB planning result across every projected AIV module in a
// PostCVPipelineResult.  peakBits/requiredBits are the maxima of the per-function
// effective requirements; success requires every function to succeed and
// overflow is true when any function overflows.
struct ModulePlanResult {
  Precision precision = Precision::Exact;
  bool success = true;
  bool overflow = false;
  uint64_t peakBits = 0;
  uint64_t requiredBits = 0;
  uint64_t capacityBits = kUBCapacityBits;
  std::vector<FunctionPlanResult> functions;
  std::vector<PostCVPipelineDiagnostic> diagnostics;
};

// Counts the AIV func.func operations in a module.  Used to enforce the
// per-module invariant (exactly one AIV function) that replaces the suffix
// bridge's historical multiple-AIV blocker.
inline size_t CountAIVFunctions(const GenericModule &module) {
  size_t count = 0;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func" && IsAIVFunction(operation))
      ++count;
  return count;
}

// Plans UB for each projected AIV module independently and aggregates a
// module-level result.  `seed`, when present, is propagated to every function's
// PlanLocalMemoryForSeed call; when absent each function runs the full retry
// search and keeps its own selected seed.
inline ModulePlanResult PlanProjectedAIVFunctions(
    const PostCVPipelineResult &projected, const SuffixPipelineOptions &options,
    std::optional<uint32_t> seed, bool restrictInplaceAsISA) {
  ModulePlanResult result;
  for (const ProjectedAIVModule &module : projected.functions) {
    const size_t aivFunctionCount = CountAIVFunctions(module.module);
    if (aivFunctionCount != 1U) {
      result.precision = Precision::Incomplete;
      result.success = false;
      PostCVPipelineDiagnostic diagnostic;
      diagnostic.pipelineStage = "PlanProjectedAIVFunctions";
      diagnostic.function = module.projectedFunction;
      diagnostic.reason =
          "expected exactly one AIV function in projected module, found " +
          std::to_string(aivFunctionCount);
      result.diagnostics.push_back(std::move(diagnostic));
      continue;
    }

    const PlanMemoryInput input =
        BuildSuffixPlanMemoryInput(module.module, options);
    FunctionPlanResult functionResult;
    functionResult.sourceFunction = module.sourceFunction;
    functionResult.function = module.projectedFunction;
    functionResult.plan =
        seed ? PlanLocalMemoryForSeed(input, *seed, restrictInplaceAsISA)
             : PlanLocalMemory(input, restrictInplaceAsISA);
    result.functions.push_back(std::move(functionResult));
  }

  // Aggregate.  A successful function's effective requirement is
  // max(peakBits, requiredBits); an overflowing function's effective requirement
  // is requiredBits.  Module peak/required are the maxima of these per-function
  // effective values (never sums).  Module overflow is true when any function
  // overflows; module success requires every function to succeed.
  for (const FunctionPlanResult &function : result.functions) {
    if (!function.plan.success)
      result.success = false;
    if (function.plan.overflow)
      result.overflow = true;
    const uint64_t effective =
        function.plan.overflow
            ? function.plan.requiredBits
            : std::max(function.plan.peakBits, function.plan.requiredBits);
    result.peakBits = std::max(result.peakBits, effective);
    result.requiredBits = std::max(result.requiredBits, effective);
  }
  if (!result.success)
    result.precision = Precision::Incomplete;
  return result;
}

} // namespace cvub

#endif
