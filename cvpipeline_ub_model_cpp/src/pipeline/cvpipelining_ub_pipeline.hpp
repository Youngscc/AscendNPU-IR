#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_UB_PIPELINE_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_UB_PIPELINE_HPP

#include "../passes/canonicalization_hivm_pipeline.hpp"
#include "../passes/clone_tensor_empty.hpp"
#include "../passes/cross_core_gss.hpp"
#include "../passes/cvpipelining/cvpipelining_pass.hpp"
#include "../passes/fold_tensor_empty.hpp"
#include "../passes/global_workspace_plan.hpp"
#include "../passes/hivm_inline_otf_load_store.hpp"
#include "../passes/infer_and_set_buffer_size.hpp"
#include "../passes/inline_scope.hpp"
#include "../passes/loop_invariant_code_motion.hpp"
#include "../passes/mark_real_core_type.hpp"
#include "../passes/optimize_dps_op_with_yielded_insert_slice.hpp"
#include "../passes/split_mix_kernel.hpp"
#include "../passes/tile_and_bind_sub_block.hpp"
#include "../support/debug_trace.hpp"
#include "plan_memory_input_builder.hpp"

#include <algorithm>
#include <optional>
#include <sstream>

namespace cvub {

struct UBAffectingPassOptions {
  bool enableCodeMotion = true;
  bool enableAutoMultiBuffer = false;
  MultiBufferStrategy limitAutoMultiBufferOfLocalBuffer =
      MultiBufferStrategy::NoLimit;
  MultiBufferStrategy limitMixAutoMultiBufferBuffer =
      MultiBufferStrategy::NoLimit;
};

inline void TraceGenericPass(DebugTrace *trace, const std::string &passName,
                             const GenericModule &module) {
  if (!trace)
    return;
  trace->Pass(passName,
              {{"operations", module.operations.size()},
               {"regions", module.regions.size()},
               {"blocks", module.blocks.size()}});
  trace->Artifact(passName, [&] { return SerializeGenericModule(module); });
}

inline void TraceCVPipelining(DebugTrace *trace, const GenericModule &module) {
  if (!trace)
    return;
  trace->Pass("CVPipelining",
              {{"operations", module.operations.size()},
               {"regions", module.regions.size()},
               {"blocks", module.blocks.size()}});
  trace->Artifact("CVPipelining", [&] {
    return SerializeAfterCVPipeliningSemanticIR(module);
  });
}

inline std::string
SerializeDebugPlanMemoryResult(const PlanMemoryModelResult &result) {
  std::ostringstream output;
  output << "PLAN_MEMORY_RESULT\t1\n"
         << "STATUS\t" << (result.success ? "success" : "overflow") << '\n'
         << "SELECTED_SEED\t" << result.selectedSeed << '\n'
         << "PEAK_BITS\t" << result.peakBits << '\n'
         << "REQUIRED_BITS\t" << result.requiredBits << '\n'
         << "CAPACITY_BITS\t" << result.capacityBits << '\n';
  for (const PlannedBufferRecord &buffer : result.buffers) {
    output << "BUFFER\t" << buffer.name << '\t' << buffer.constBits << '\t'
           << buffer.extentBits << '\t' << buffer.allocTime << '\t'
           << buffer.freeTime;
    for (uint64_t offset : buffer.offsetsBytes)
      output << '\t' << offset;
    output << '\n';
  }
  return output.str();
}

inline void TracePlanMemoryResult(DebugTrace *trace,
                                  const PlanMemoryModelResult &result) {
  if (!trace)
    return;
  trace->Pass(
      "PlanMemoryResult",
      {{"success", result.success ? 1U : 0U},
       {"overflow", result.overflow ? 1U : 0U},
       {"selected_seed", result.selectedSeed},
       {"peak_bits", result.peakBits},
       {"required_bits", result.requiredBits},
       {"capacity_bits", result.capacityBits},
       {"buffers", result.buffers.size()}});
  trace->Artifact("PlanMemoryResult",
                  [&] { return SerializeDebugPlanMemoryResult(result); });
}

inline GenericModule RunPassesBeforeLoopInvariantCodeMotion(
    GenericModule module, DebugTrace *trace = nullptr) {
  ApplyOperationSemanticsToAll(module.operations);
  module = RunInferAndSetBufferSizePipeline(std::move(module));
  TraceGenericPass(trace, "InferAndSetBufferSize", module);
  module = RunGlobalWorkspacePlan(std::move(module));
  TraceGenericPass(trace, "GlobalWorkspacePlan", module);
  module = RunFoldTensorEmpty(std::move(module));
  TraceGenericPass(trace, "FoldTensorEmpty", module);
  module = RunCanonicalizationHIVMPipeline(std::move(module));
  TraceGenericPass(trace, "CanonicalizationHIVMPipeline", module);
  module = RunMarkRealCoreType(std::move(module));
  TraceGenericPass(trace, "MarkRealCoreType", module);
  module = RunCrossCoreGSS(std::move(module));
  TraceGenericPass(trace, "CrossCoreGSS", module);
  module = RunMarkRealCoreType(std::move(module), true);
  TraceGenericPass(trace, "MarkRealCoreType", module);
  module = RunSplitMixKernel(std::move(module));
  TraceGenericPass(trace, "SplitMixKernel", module);
  module = RunInlineScope(std::move(module));
  TraceGenericPass(trace, "InlineScope", module);
  module = RunTileAndBindSubBlock(std::move(module));
  TraceGenericPass(trace, "TileAndBindSubBlock", module);
  module = RunFoldTensorEmpty(std::move(module));
  TraceGenericPass(trace, "FoldTensorEmpty", module);
  module =
      RunCanonicalizationHIVMPipelineSourceAligned(std::move(module));
  ApplyOperationSemanticsToAll(module.operations);
  TraceGenericPass(trace, "CanonicalizationHIVMPipelineSourceAligned",
                   module);
  return module;
}

inline GenericModule RunPassesBeforeOneShotBufferize(
    GenericModule module, const UBAffectingPassOptions &options = {},
    DebugTrace *trace = nullptr) {
  module =
      RunPassesBeforeLoopInvariantCodeMotion(std::move(module), trace);
  if (options.enableCodeMotion) {
    RunLoopInvariantCodeMotion(module);
    TraceGenericPass(trace, "LoopInvariantCodeMotion", module);
  } else if (trace) {
    trace->Pass("LoopInvariantCodeMotion", {{"executed", 0}});
  }
  module = RunCloneTensorEmpty(std::move(module));
  TraceGenericPass(trace, "CloneTensorEmpty", module);
  module = RunHIVMInlineOTFLoadStore(std::move(module));
  TraceGenericPass(trace, "HIVMInlineOTFLoadStore", module);
  module = RunOptimizeDpsOpWithYieldedInsertSlice(std::move(module));
  TraceGenericPass(trace, "OptimizeDpsOpWithYieldedInsertSlice", module);
  module = RunCloneTensorEmpty(std::move(module));
  TraceGenericPass(trace, "CloneTensorEmpty", module);
  return module;
}

inline PlanMemoryInput BuildPlanMemoryInputFromAfterCVPipelining(
    GenericModule module,
    const UBAffectingPassOptions &options = {}, DebugTrace *trace = nullptr,
    const std::string &targetFunction = {}) {
  module =
      RunPassesBeforeOneShotBufferize(std::move(module), options, trace);

  const OneShotBufferizationResult bufferization = RunOneShotBufferize(module);
  const BufferizedSemanticIR oneShotBufferizeOutput =
      BuildBufferizedSemanticIR(module, bufferization);
  if (trace) {
    trace->Pass("OneShotBufferize",
                {{"allocations", oneShotBufferizeOutput.allocations.size()},
                 {"values", oneShotBufferizeOutput.values.size()},
                 {"accesses", oneShotBufferizeOutput.accesses.size()}});
    trace->Artifact("OneShotBufferize", [&] {
      return SerializeBufferizedSemanticIR(oneShotBufferizeOutput);
    });
  }
  const PostBufferizationRewriteState hivmDecomposeOpOutput =
      BuildPostBufferizationRewriteState(oneShotBufferizeOutput);
  if (trace) {
    trace->Pass(
        "HIVMOptSinglePoint",
        {{"allocations", hivmDecomposeOpOutput.singlePoint.allocations.size()},
         {"scalarized_operations",
          hivmDecomposeOpOutput.singlePoint.scalarizedOperations.size()}});
    trace->Pass(
        "HIVMDecomposeOp",
        {{"allocations", hivmDecomposeOpOutput.decomposeAllocations.size()},
         {"operation_rewrites",
          hivmDecomposeOpOutput.operationRewrites.size()}});
    trace->Pass("ConvertNonContiguousReshapeToCopy",
                {{"copies", hivmDecomposeOpOutput
                                .nonContiguousReshapeCopies.size()}});
    trace->Artifact("HIVMDecomposeOp", [&] {
      return SerializePostBufferizationRewriteState(hivmDecomposeOpOutput);
    });
  }
  const AfterAllocExtraBufferState allocExtraBufferOutput =
      BuildAfterAllocExtraBufferState(hivmDecomposeOpOutput);
  if (trace) {
    const uint64_t extraBuffers = static_cast<uint64_t>(std::count_if(
        allocExtraBufferOutput.buffers.begin(),
        allocExtraBufferOutput.buffers.end(),
        [](const LocalBufferRecord &buffer) { return buffer.extraBuffer; }));
    trace->Pass("InferHIVMMemScope",
                {{"buffers", allocExtraBufferOutput.buffers.size()}});
    trace->Pass(
        "AlignStorage",
        {{"alloc_alignments",
          allocExtraBufferOutput.alignStorage.allocAlignments.size()},
         {"stride_alignments",
          allocExtraBufferOutput.alignStorage.strideAlignments.size()}});
    trace->Pass("AllocExtraBuffer",
                {{"buffers", allocExtraBufferOutput.buffers.size()},
                 {"extra_buffers", extraBuffers}});
    trace->Artifact("AllocExtraBuffer", [&] {
      return SerializeAfterAllocExtraBufferState(allocExtraBufferOutput);
    });
  }
  const AfterInlineLoadCopyState inlineLoadCopyOutput =
      BuildAfterInlineLoadCopyState(allocExtraBufferOutput);
  if (trace) {
    trace->Pass(
        "InlineLoadCopy",
        {{"rewrites", inlineLoadCopyOutput.inlineLoadCopy.rewrites.size()},
         {"erased_buffers",
          inlineLoadCopyOutput.inlineLoadCopy.erasedBuffers.size()},
         {"buffers", inlineLoadCopyOutput.buffers.size()}});
    trace->Artifact("InlineLoadCopy", [&] {
      return SerializeAfterInlineLoadCopyState(inlineLoadCopyOutput);
    });
  }

  MarkMultiBufferOptions multiBufferOptions;
  multiBufferOptions.enableAuto = options.enableAutoMultiBuffer;
  multiBufferOptions.limitAutoMultiBufferOfLocalBuffer =
      options.limitAutoMultiBufferOfLocalBuffer;
  multiBufferOptions.limitMixAutoMultiBufferBuffer =
      options.limitMixAutoMultiBufferBuffer;
  const AfterMarkMultiBufferState markMultiBufferOutput =
      BuildAfterMarkMultiBufferState(inlineLoadCopyOutput, multiBufferOptions);
  if (trace) {
    trace->Pass(
        "MarkMultiBuffer",
        {{"marks", markMultiBufferOutput.markMultiBuffer.marks.size()},
         {"multi_buffers",
          markMultiBufferOutput.markMultiBuffer.buffer2MultiNum.size()},
         {"preload_buffers", markMultiBufferOutput.markMultiBuffer
                                 .preloadLocalBuffers.size()}});
    trace->Artifact("MarkMultiBuffer", [&] {
      return SerializeAfterMarkMultiBufferState(markMultiBufferOutput);
    });
  }
  const PlanMemoryInputSemanticIR semantic =
      BuildPlanMemoryInputSemanticIR(markMultiBufferOutput);
  if (trace) {
    trace->Pass("PlanMemoryInput",
                {{"buffers", semantic.buffers.size()},
                 {"accesses", semantic.accesses.size()},
                 {"blockers", semantic.accessBlockers.size()}});
    trace->Artifact("PlanMemoryInputSemanticIR", [&] {
      return SerializePlanMemoryInputSemanticIR(semantic);
    });
  }

  const AfterAllocExtraBufferState &afterAllocExtraBuffer =
      semantic.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer;
  for (const LocalBufferRecord &buffer : afterAllocExtraBuffer.buffers) {
    const std::optional<MemRefTypeModel> type = ParseMemRefType(buffer.type);
    const bool dynamic = type && std::any_of(
        type->shape.begin(), type->shape.end(),
        [](const std::optional<int64_t> &extent) { return !extent; });
    const auto alignment =
        afterAllocExtraBuffer.alignStorage.strideAlignments.find(
            buffer.sourceIdentity);
    if (dynamic &&
        alignment != afterAllocExtraBuffer.alignStorage.strideAlignments.end() &&
        !alignment->second.empty())
      throw std::runtime_error(
          "AlignStorage: dynamic stride-aligned allocation is unsupported");
  }
  PlanMemoryInput input = targetFunction.empty()
                              ? BuildPlanMemoryInput(semantic)
                              : BuildPlanMemoryInput(semantic, targetFunction);
  if (trace) {
    trace->Pass("PlanMemory",
                {{"allocations", input.allocations.size()},
                 {"operations", input.operations.size()},
                 {"function_arguments", input.functionArguments.size()}});
    trace->Artifact("PlanMemoryInput",
                    [&] { return SerializeCanonicalPlanMemoryInput(input); });
  }
  return input;
}

inline PlanMemoryInput BuildPlanMemoryInputFromAfterCVPipelining(
    const fs::path &afterCVPipeliningGenericIR,
    const UBAffectingPassOptions &options = {}, DebugTrace *trace = nullptr,
    const std::string &targetFunction = {}) {
  return BuildPlanMemoryInputFromAfterCVPipelining(
      ParseGenericIR(afterCVPipeliningGenericIR, false), options, trace,
      targetFunction);
}

enum class ModulePlanPrecision { Exact, Incomplete };

struct FunctionPlanResult {
  std::string function;
  PlanMemoryModelResult plan;
};

struct ModulePlanResult {
  ModulePlanPrecision precision = ModulePlanPrecision::Exact;
  bool success = true;
  bool overflow = false;
  uint64_t peakBits = 0;
  uint64_t requiredBits = 0;
  uint64_t capacityBits = kUBCapacityBits;
  std::vector<FunctionPlanResult> functions;
  std::vector<std::string> diagnostics;
};

inline std::vector<std::string>
AIVFunctionNames(const GenericModule &module) {
  std::vector<std::string> result;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "func.func" || !IsAIVFunction(operation))
      continue;
    const std::string name = GenericFunctionName(operation);
    if (name.empty())
      throw std::runtime_error("module planning: AIV function has no symbol");
    result.push_back(name);
  }
  return result;
}

inline ModulePlanResult RunUBModuleFromAfterCVPipelining(
    const GenericModule &module,
    const UBAffectingPassOptions &options = {},
    std::optional<uint32_t> planMemorySeed = std::nullopt,
    bool restrictInplaceAsISA = false, DebugTrace *trace = nullptr) {
  const GenericModule projected =
      RunPassesBeforeOneShotBufferize(module, options, trace);
  const std::vector<std::string> functions = AIVFunctionNames(projected);
  ModulePlanResult result;
  for (const std::string &function : functions) {
    const PlanMemoryInput input = BuildPlanMemoryInputFromAfterCVPipelining(
        module, options, nullptr, function);
    PlanMemoryModelResult plan =
        planMemorySeed
            ? PlanLocalMemoryForSeed(input, *planMemorySeed,
                                     restrictInplaceAsISA)
            : PlanLocalMemory(input, restrictInplaceAsISA);
    result.success = result.success && plan.success;
    result.overflow = result.overflow || plan.overflow;
    result.peakBits = std::max(result.peakBits, plan.peakBits);
    result.requiredBits = std::max(result.requiredBits, plan.requiredBits);
    if (!restrictInplaceAsISA && !plan.inplacePairs.empty()) {
      result.precision = ModulePlanPrecision::Incomplete;
      result.diagnostics.push_back(
          "PlanMemory: default inplace inference is not oracle-proven for " +
          function);
    }
    result.functions.push_back({function, std::move(plan)});
  }
  return result;
}

inline ModulePlanResult
ModulePlanFromSingle(std::string function, PlanMemoryModelResult plan) {
  ModulePlanResult result;
  result.success = plan.success;
  result.overflow = plan.overflow;
  result.peakBits = plan.peakBits;
  result.requiredBits = plan.requiredBits;
  result.capacityBits = plan.capacityBits;
  result.functions.push_back({std::move(function), std::move(plan)});
  return result;
}

struct CVPipeliningUBPipelineOptions {
  CVPipeliningOptions cvPipelining;
  UBAffectingPassOptions ubAffectingPasses;
  std::optional<uint32_t> planMemorySeed;
  bool restrictInplaceAsISA = false;
  DebugTrace *debugTrace = nullptr;
};

inline ModulePlanResult RunCVPipeliningUBModulePipeline(
    GenericModule module, const CVPipeliningUBPipelineOptions &options = {}) {
  ApplyOperationSemanticsToAll(module.operations);
  module = RunCVPipeliningPass(std::move(module), options.cvPipelining);
  TraceCVPipelining(options.debugTrace, module);
  return RunUBModuleFromAfterCVPipelining(
      module, options.ubAffectingPasses, options.planMemorySeed,
      options.restrictInplaceAsISA, options.debugTrace);
}

inline ModulePlanResult RunCVPipeliningUBModulePipeline(
    const fs::path &beforeCVPipeliningIR,
    const CVPipeliningUBPipelineOptions &options = {}) {
  return RunCVPipeliningUBModulePipeline(
      ParseGenericIR(beforeCVPipeliningIR, false), options);
}

inline PlanMemoryModelResult RunCVPipeliningUBPipeline(
    GenericModule module, const CVPipeliningUBPipelineOptions &options = {}) {
  ApplyOperationSemanticsToAll(module.operations);
  module = RunCVPipeliningPass(std::move(module), options.cvPipelining);
  TraceCVPipelining(options.debugTrace, module);
  const PlanMemoryInput input = BuildPlanMemoryInputFromAfterCVPipelining(
      std::move(module), options.ubAffectingPasses, options.debugTrace);
  PlanMemoryModelResult result =
      options.planMemorySeed
          ? PlanLocalMemoryForSeed(input, *options.planMemorySeed,
                                   options.restrictInplaceAsISA)
          : PlanLocalMemory(input, options.restrictInplaceAsISA);
  TracePlanMemoryResult(options.debugTrace, result);
  return result;
}

inline PlanMemoryModelResult RunCVPipeliningUBPipeline(
    const fs::path &beforeCVPipeliningIR,
    const CVPipeliningUBPipelineOptions &options = {}) {
  return RunCVPipeliningUBPipeline(
      ParseGenericIR(beforeCVPipeliningIR, false), options);
}

} // namespace cvub

#endif
