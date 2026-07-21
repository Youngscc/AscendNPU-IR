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
#include "../passes/inline_scope_strict.hpp"
#include "../passes/loop_invariant_code_motion.hpp"
#include "../passes/loop_invariant_subset_hoisting.hpp"
#include "../passes/mark_real_core_type.hpp"
#include "../passes/optimize_dps_op_with_yielded_insert_slice.hpp"
#include "../passes/split_mix_kernel.hpp"
#include "../passes/tile_and_bind_sub_block.hpp"
#include "../passes/tile_cube_vector_loop.hpp"
#include "../passes/tightly_coupled_buffer_guard.hpp"
#include "../passes/sink_op_to_consumer_in_loop.hpp"
#include "../support/debug_trace.hpp"
#include "plan_memory_input_builder.hpp"

#include <algorithm>
#include <optional>
#include <sstream>

namespace cvub {

struct UBAffectingPassOptions {
  unsigned tileMixVectorLoop = 2;
  unsigned tileMixCubeLoop = 2;
  bool enableCodeMotion = true;
  bool enableUbufSaving = false;
  bool enableTritonKernelCompile = false;
  bool disableAlignAllocSize = false;
  bool disableEnableStrideAlign = false;
  bool disableInferHIVMDataLayout = false;
  bool enableAutoMultiBuffer = false;
  MultiBufferStrategy limitAutoMultiBufferOfLocalBuffer =
      MultiBufferStrategy::CubeNoL0C;
  MultiBufferStrategy limitMixAutoMultiBufferBuffer =
      MultiBufferStrategy::OnlyCube;
};

inline GenericModule RequireExactStage(StageResult stage) {
  if (stage.precision == Precision::Exact) {
    ValidateGenericModule(stage.module);
    return std::move(stage.module);
  }
  if (stage.diagnostics.empty())
    throw std::runtime_error("UB-affecting pass is not modeled exactly");
  const PostCVPipelineDiagnostic &diagnostic = stage.diagnostics.front();
  std::string message = diagnostic.pipelineStage;
  if (!diagnostic.function.empty())
    message += "[" + diagnostic.function + "]";
  if (!diagnostic.operation.empty())
    message += ": " + diagnostic.operation;
  if (!diagnostic.reason.empty())
    message += ": " + diagnostic.reason;
  throw std::runtime_error(message);
}

inline void TraceGenericPass(DebugTrace *trace, const std::string &passName,
                             const GenericModule &module) {
  try {
    ValidateGenericModule(module);
  } catch (const std::runtime_error &error) {
    throw std::runtime_error(passName + ": " + error.what());
  }
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
    GenericModule module, const UBAffectingPassOptions &options = {},
    DebugTrace *trace = nullptr) {
  MeasureStage(trace, "ApplyOperationSemantics",
               [&] { ApplyOperationSemanticsToAll(module.operations); });
  module = MeasureStage(trace, "TileCubeVectorLoop", [&] {
    return RequireExactStage(RunTileCubeVectorLoop(
        std::move(module), options.tileMixVectorLoop,
        options.tileMixCubeLoop));
  });
  TraceGenericPass(trace, "TileCubeVectorLoop", module);
  module = MeasureStage(trace, "InferAndSetBufferSize", [&] {
    return RunInferAndSetBufferSizePipeline(std::move(module));
  });
  TraceGenericPass(trace, "InferAndSetBufferSize", module);
  module = MeasureStage(trace, "GlobalWorkspacePlan", [&] {
    return RunGlobalWorkspacePlan(std::move(module));
  });
  TraceGenericPass(trace, "GlobalWorkspacePlan", module);
  module = MeasureStage(trace, "FoldTensorEmpty", [&] {
    return RunFoldTensorEmpty(std::move(module));
  });
  TraceGenericPass(trace, "FoldTensorEmpty", module);
  module = MeasureStage(trace, "CanonicalizationHIVMPipeline", [&] {
    return RunCanonicalizationHIVMPipeline(std::move(module));
  });
  TraceGenericPass(trace, "CanonicalizationHIVMPipeline", module);
  module = MeasureStage(trace, "MarkRealCoreType", [&] {
    return RunMarkRealCoreType(std::move(module));
  });
  TraceGenericPass(trace, "MarkRealCoreType", module);
  module = MeasureStage(trace, "CrossCoreGSS", [&] {
    return RunCrossCoreGSS(std::move(module));
  });
  TraceGenericPass(trace, "CrossCoreGSS", module);
  module = MeasureStage(trace, "MarkRealCoreType", [&] {
    return RunMarkRealCoreType(std::move(module), true);
  });
  TraceGenericPass(trace, "MarkRealCoreType", module);
  // Temporarily disabled together with the corresponding suffix passes.
  // module = RequireExactStage(
  //     GuardTightlyCoupledBufferPasses(std::move(module)));
  // TraceGenericPass(trace,
  //                  "MarkTightlyCoupledBuffer;HoistTightlyCoupledAlloc",
  //                  module);
  module = MeasureStage(trace, "SplitMixKernel", [&] {
    return RunSplitMixKernel(std::move(module));
  });
  TraceGenericPass(trace, "SplitMixKernel", module);
  module = MeasureStage(trace, "InlineScope", [&] {
    return RequireExactStage(RunStrictInlineScope(std::move(module)));
  });
  TraceGenericPass(trace, "InlineScope", module);
  module = MeasureStage(trace, "TileAndBindSubBlock", [&] {
    return RunTileAndBindSubBlock(std::move(module));
  });
  TraceGenericPass(trace, "TileAndBindSubBlock", module);
  module = MeasureStage(trace, "FoldTensorEmpty", [&] {
    return RunFoldTensorEmpty(std::move(module));
  });
  TraceGenericPass(trace, "FoldTensorEmpty", module);
  module = MeasureStage(trace, "CanonicalizationHIVMPipelineSourceAligned",
                        [&] {
                          GenericModule result =
                              RunCanonicalizationHIVMPipelineSourceAligned(
                                  std::move(module));
                          ApplyOperationSemanticsToAll(result.operations);
                          return result;
                        });
  TraceGenericPass(trace, "CanonicalizationHIVMPipelineSourceAligned",
                   module);
  ValidateGenericModule(module);
  return module;
}

inline GenericModule RunPassesBeforeOneShotBufferize(
    GenericModule module, const UBAffectingPassOptions &options = {},
    DebugTrace *trace = nullptr) {
  module =
      RunPassesBeforeLoopInvariantCodeMotion(std::move(module), options, trace);
  if (options.enableCodeMotion) {
    MeasureStage(trace, "LoopInvariantCodeMotion",
                 [&] { RunLoopInvariantCodeMotion(module); });
    TraceGenericPass(trace, "LoopInvariantCodeMotion", module);
    module = MeasureStage(trace, "LoopInvariantSubsetHoisting", [&] {
      return RequireExactStage(
          RunLoopInvariantSubsetHoisting(std::move(module), true));
    });
    TraceGenericPass(trace, "LoopInvariantSubsetHoisting", module);
  } else if (trace) {
    trace->Pass("LoopInvariantCodeMotion", {{"executed", 0}});
    trace->Pass("LoopInvariantSubsetHoisting", {{"executed", 0}});
  }
  module = MeasureStage(trace, "CloneTensorEmpty", [&] {
    return RunCloneTensorEmpty(std::move(module));
  });
  TraceGenericPass(trace, "CloneTensorEmpty", module);
  module = MeasureStage(trace, "HIVMInlineOTFLoadStore", [&] {
    return RunHIVMInlineOTFLoadStore(std::move(module));
  });
  TraceGenericPass(trace, "HIVMInlineOTFLoadStore", module);
  if (options.enableTritonKernelCompile) {
    module = MeasureStage(trace, "OptimizeDpsOpWithYieldedInsertSlice", [&] {
      return RunOptimizeDpsOpWithYieldedInsertSlice(std::move(module));
    });
    TraceGenericPass(trace, "OptimizeDpsOpWithYieldedInsertSlice", module);
    module = MeasureStage(trace, "CloneTensorEmptyBeforeBufferize", [&] {
      return RunCloneTensorEmpty(std::move(module));
    });
    TraceGenericPass(trace, "CloneTensorEmptyBeforeBufferize", module);
  } else if (trace) {
    trace->Pass("OptimizeDpsOpWithYieldedInsertSlice", {{"executed", 0}});
    trace->Pass("CloneTensorEmptyBeforeBufferize", {{"executed", 0}});
  }
  if (options.enableUbufSaving) {
    module = MeasureStage(trace, "SinkOpToConsumerInLoop", [&] {
      return RunSinkOpToConsumerInLoop(std::move(module));
    });
    TraceGenericPass(trace, "SinkOpToConsumerInLoop", module);
  } else if (trace) {
    trace->Pass("SinkOpToConsumerInLoop", {{"executed", 0}});
  }
  ValidateGenericModule(module);
  return module;
}

inline PlanMemoryInput BuildPlanMemoryInputFromBeforeOneShotBufferize(
    const GenericModule &module,
    const UBAffectingPassOptions &options = {}, DebugTrace *trace = nullptr,
    const std::string &targetFunction = {}) {
  const BufferizedSemanticIR oneShotBufferizeOutput =
      MeasureStage(trace, "OneShotBufferize", [&] {
        const OneShotBufferizationResult bufferization =
            RunOneShotBufferize(module);
        return BuildBufferizedSemanticIR(module, bufferization);
      });
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
      MeasureStage(trace, "PostBufferizationRewrites", [&] {
        return BuildPostBufferizationRewriteState(oneShotBufferizeOutput);
      });
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
      MeasureStage(trace, "AlignStorageAndAllocExtraBuffer", [&] {
        return BuildAfterAllocExtraBufferState(
            hivmDecomposeOpOutput, !options.disableAlignAllocSize,
            !options.disableEnableStrideAlign);
      });
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
      MeasureStage(trace, "InlineLoadCopy", [&] {
        return BuildAfterInlineLoadCopyState(
            allocExtraBufferOutput, options.enableTritonKernelCompile);
      });
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
  multiBufferOptions.inferHIVMDataLayout =
      !options.disableInferHIVMDataLayout;
  const AfterMarkMultiBufferState markMultiBufferOutput =
      MeasureStage(trace, "MarkMultiBuffer", [&] {
        return BuildAfterMarkMultiBufferState(inlineLoadCopyOutput,
                                              multiBufferOptions);
      });
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
      MeasureStage(trace, "PlanMemoryInputSemanticIR", [&] {
        return BuildPlanMemoryInputSemanticIR(markMultiBufferOutput);
      });
  if (trace) {
    trace->Pass("PlanMemoryInput",
                {{"buffers", semantic.buffers.size()},
                 {"accesses", semantic.accesses.size()},
                 {"blockers", semantic.accessBlockers.size()}});
    trace->Artifact("PlanMemoryInputSemanticIR", [&] {
      return SerializePlanMemoryInputSemanticIR(semantic);
    });
  }

  PlanMemoryInput input = MeasureStage(trace, "BuildPlanMemoryInput", [&] {
    return targetFunction.empty() ? BuildPlanMemoryInput(semantic)
                                  : BuildPlanMemoryInput(semantic,
                                                         targetFunction);
  });
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
    GenericModule module,
    const UBAffectingPassOptions &options = {}, DebugTrace *trace = nullptr,
    const std::string &targetFunction = {}) {
  module =
      RunPassesBeforeOneShotBufferize(std::move(module), options, trace);
  return BuildPlanMemoryInputFromBeforeOneShotBufferize(
      module, options, trace, targetFunction);
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
    const PlanMemoryInput input = BuildPlanMemoryInputFromBeforeOneShotBufferize(
        projected, options, trace, function);
    PlanMemoryModelResult plan = MeasureStage(trace, "PlanMemory", [&] {
      return planMemorySeed
                 ? PlanLocalMemoryForSeed(input, *planMemorySeed,
                                          restrictInplaceAsISA)
                 : PlanLocalMemory(input, restrictInplaceAsISA);
    });
    TracePlanMemoryResult(trace, plan);
    result.success = result.success && plan.success;
    result.overflow = result.overflow || plan.overflow;
    result.peakBits = std::max(result.peakBits, plan.peakBits);
    result.requiredBits = std::max(result.requiredBits, plan.requiredBits);
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
  MeasureStage(options.debugTrace, "ApplyOperationSemantics",
               [&] { ApplyOperationSemanticsToAll(module.operations); });
  module = MeasureStage(options.debugTrace, "CVPipelining", [&] {
    return RunCVPipeliningPass(std::move(module), options.cvPipelining);
  });
  TraceCVPipelining(options.debugTrace, module);
  return RunUBModuleFromAfterCVPipelining(
      module, options.ubAffectingPasses, options.planMemorySeed,
      options.restrictInplaceAsISA, options.debugTrace);
}

inline ModulePlanResult RunCVPipeliningUBModulePipeline(
    const fs::path &beforeCVPipeliningIR,
    const CVPipeliningUBPipelineOptions &options = {}) {
  GenericModule module = MeasureStage(options.debugTrace, "ParseGenericIR", [&] {
    return ParseGenericIR(beforeCVPipeliningIR, false);
  });
  return RunCVPipeliningUBModulePipeline(std::move(module), options);
}

inline PlanMemoryModelResult RunCVPipeliningUBPipeline(
    GenericModule module, const CVPipeliningUBPipelineOptions &options = {}) {
  MeasureStage(options.debugTrace, "ApplyOperationSemantics",
               [&] { ApplyOperationSemanticsToAll(module.operations); });
  module = MeasureStage(options.debugTrace, "CVPipelining", [&] {
    return RunCVPipeliningPass(std::move(module), options.cvPipelining);
  });
  TraceCVPipelining(options.debugTrace, module);
  const PlanMemoryInput input = BuildPlanMemoryInputFromAfterCVPipelining(
      std::move(module), options.ubAffectingPasses, options.debugTrace);
  PlanMemoryModelResult result =
      MeasureStage(options.debugTrace, "PlanMemory", [&] {
        return options.planMemorySeed
                   ? PlanLocalMemoryForSeed(input, *options.planMemorySeed,
                                            options.restrictInplaceAsISA)
                   : PlanLocalMemory(input, options.restrictInplaceAsISA);
      });
  TracePlanMemoryResult(options.debugTrace, result);
  return result;
}

inline PlanMemoryModelResult RunCVPipeliningUBPipeline(
    const fs::path &beforeCVPipeliningIR,
    const CVPipeliningUBPipelineOptions &options = {}) {
  GenericModule module = MeasureStage(options.debugTrace, "ParseGenericIR", [&] {
    return ParseGenericIR(beforeCVPipeliningIR, false);
  });
  return RunCVPipeliningUBPipeline(std::move(module), options);
}

} // namespace cvub

#endif
