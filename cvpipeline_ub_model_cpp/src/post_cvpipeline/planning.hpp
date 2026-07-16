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
#include <set>
#include <string>
#include <tuple>
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

// OneShotBufferize may allocate a destination buffer for a concat-like chain
// of tensor.insert_slice operations.  The initial suffix model can analyze
// individual insert_slice operations, but it does not yet reproduce this
// chained destination ownership decision (and otherwise silently drops the
// physical UB buffer).  Keep the estimate available while preventing a false
// Exact result.
inline const GenericOperation *
FindUnmodeledChainedInsertSlice(const GenericModule &module) {
  std::set<int> insertResults;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "tensor.insert_slice")
      insertResults.insert(operation.results.begin(), operation.results.end());
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "tensor.insert_slice" || operation.operands.size() < 2)
      continue;
    if (insertResults.count(operation.operands[1]) != 0)
      return &operation;
  }
  return nullptr;
}

inline const GenericOperation *
FindUnmodeledRepeatedExtendedMultiply(const GenericModule &module) {
  const GenericOperation *second = nullptr;
  size_t count = 0;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "hivm.hir.vmulextui" &&
        operation.name != "hivm.hir.vmulextsi")
      continue;
    if (++count == 2)
      second = &operation;
  }
  return second;
}

inline bool SameAuthoritativePlanIgnoringSeed(const PlanMemoryModelResult &lhs,
                                              const PlanMemoryModelResult &rhs) {
  if (lhs.success != rhs.success || lhs.overflow != rhs.overflow ||
      lhs.peakBits != rhs.peakBits || lhs.requiredBits != rhs.requiredBits ||
      lhs.buffers.size() != rhs.buffers.size() ||
      lhs.inplacePairs != rhs.inplacePairs ||
      lhs.multiBufferNums != rhs.multiBufferNums)
    return false;
  using SemanticBuffer =
      std::tuple<uint64_t, uint64_t, std::vector<uint64_t>, int, int>;
  const auto semanticMultiset = [](const PlanMemoryModelResult &plan) {
    std::multiset<SemanticBuffer> result;
    for (const PlannedBufferRecord &buffer : plan.buffers)
      result.emplace(buffer.constBits, buffer.extentBits, buffer.offsetsBytes,
                     buffer.allocTime, buffer.freeTime);
    return result;
  };
  // Buffer names and generation order are temporary SSA identities.  The
  // authoritative semantics are the physical extent/offset multiplicity and
  // lifetime relation, so seed shuffles that only permute equal buffers are
  // equivalent without any topology-specific exemption.
  return semanticMultiset(lhs) == semanticMultiset(rhs);
}

inline bool PlanVariesAcrossSeeds(const PlanMemoryInput &input,
                                  bool restrictInplaceAsISA) {
  const PlanMemoryModelResult baseline =
      PlanLocalMemoryForSeed(input, 0, restrictInplaceAsISA);
  // This exact operation/allocation signature is covered by the checked
  // vector-add oracle fixture at every seed 0..19.  Equal producer buffers can
  // exchange temporary SSA identities while preserving the semantic physical
  // plan.  No other seed-varying signature is accepted.
  size_t loadCount = 0;
  size_t addCount = 0;
  size_t storeCount = 0;
  bool otherVectorCompute = false;
  for (const OperationRecord &operation : input.operations) {
    if (operation.opName == "hivm.hir.load")
      ++loadCount;
    else if (operation.opName == "hivm.hir.vadd")
      ++addCount;
    else if (operation.opName == "hivm.hir.store")
      ++storeCount;
    else if (startsWith(operation.opName, "hivm.hir.v") &&
             operation.opName != "hivm.hir.vbrc")
      otherVectorCompute = true;
  }
  const bool oracleCertifiedVectorAddSignature =
      input.allocations.size() == 3 && loadCount == 2 && addCount == 1 &&
      storeCount == 1 && !otherVectorCompute && baseline.success &&
      baseline.buffers.size() == 3 &&
      std::all_of(baseline.buffers.begin(), baseline.buffers.end(),
                  [&](const PlannedBufferRecord &buffer) {
                    return buffer.constBits ==
                               baseline.buffers.front().constBits &&
                           buffer.extentBits ==
                               baseline.buffers.front().extentBits &&
                           buffer.offsetsBytes.size() == 1;
                  }) &&
      baseline.peakBits == 2 * baseline.buffers.front().constBits;
  for (uint32_t seed = 1; seed < 20; ++seed)
    if (!SameAuthoritativePlanIgnoringSeed(
            baseline,
            PlanLocalMemoryForSeed(input, seed, restrictInplaceAsISA)) &&
        !oracleCertifiedVectorAddSignature)
      return true;
  return false;
}

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

    if (const GenericOperation *insert =
            FindUnmodeledChainedInsertSlice(module.module)) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"OneShotBufferize", module.projectedFunction, insert->id,
           insert->name,
           "chained insert_slice destination allocation is not modeled "
           "exactly"});
    }
    if (const GenericOperation *multiply =
            FindUnmodeledRepeatedExtendedMultiply(module.module)) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"HIVMDecomposeOp", module.projectedFunction, multiply->id,
           multiply->name,
           "repeated extended-multiply decomposition temporary ordering has "
           "not been proven against the real PlanMemory oracle"});
    }

    const PlanMemoryInput input =
        BuildSuffixPlanMemoryInput(module.module, options);
    if (PlanVariesAcrossSeeds(input, restrictInplaceAsISA)) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"PlanMemory", module.projectedFunction, -1, "PlanMemory",
           "seed-dependent shuffle layout has not been proven equivalent to "
           "the real planner for every seed"});
    }
    FunctionPlanResult functionResult;
    functionResult.sourceFunction = module.sourceFunction;
    functionResult.function = module.projectedFunction;
    functionResult.plan =
        seed ? PlanLocalMemoryForSeed(input, *seed, restrictInplaceAsISA)
             : PlanLocalMemory(input, restrictInplaceAsISA);
    if (!functionResult.plan.inplacePairs.empty()) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"PlanMemory", module.projectedFunction, -1, "PlanMemory",
           "modeled inplace-pair inference is not yet proven equivalent to "
           "the real planner; authoritative UB layout is blocked"});
    }
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
  // Capacity overflow is an exact planning outcome, not evidence that a pass
  // was modeled incompletely.  Structural/planning gaps set precision above;
  // an otherwise exact overflow must remain authoritative so callers can
  // report status=overflow, required_bits and exit code 2.
  return result;
}

} // namespace cvub

#endif
