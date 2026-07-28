#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_UB_PIPELINE_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_UB_PIPELINE_HPP

#include "../passes/canonicalization_hivm_pipeline.hpp"
#include "../passes/clone_tensor_empty.hpp"
#include "../passes/cross_core_gss.hpp"
#include "../passes/cvpipelining/cvpipelining_pass.hpp"
#include "../passes/fold_tensor_empty.hpp"
#include "../passes/global_workspace_plan.hpp"
#include "../passes/hivm_inline_otf_load_store.hpp"
#include "../passes/infer_hivm_data_layout.hpp"
#include "../passes/infer_and_set_buffer_size.hpp"
#include "../passes/inject_block_sync.hpp"
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
  bool enableAutoBindSubBlock = true;
  bool enableUbufSaving = false;
  bool enableTritonKernelCompile = false;
  bool enableHIVMAutoStorageAlign = true;
  bool enableHIVMCrossCoreGSS = true;
  bool enableHIVMInjectBlockAllSync = false;
  bool disableAutoInjectBlockSync = false;
  bool disableAutoCVWorkSpaceManage = false;
  bool disableAlignAllocSize = false;
  bool disableEnableStrideAlign = false;
  bool disableInferHIVMDataLayout = false;
  bool enableAutoMultiBuffer = false;
  MultiBufferStrategy limitAutoMultiBufferOfLocalBuffer =
      MultiBufferStrategy::CubeNoL0C;
  MultiBufferStrategy limitMixAutoMultiBufferBuffer =
      MultiBufferStrategy::OnlyCube;
};

inline void ValidateDiscardedAICBufferizedCopies(
    const GenericModule &module, const UBAffectingPassOptions &options);

inline void ValidateDiscardedAICBufferizedCopiesOnProjection(
    GenericModule aicProjection, const UBAffectingPassOptions &options);

inline GenericModule RequireExactStage(StageResult stage) {
  if (stage.precision == Precision::Exact)
    return std::move(stage.module);
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
  if (trace && trace->VerifyEachPass()) {
    try {
      ValidateGenericModule(module);
    } catch (const std::runtime_error &error) {
      throw std::runtime_error(passName + ": " + error.what());
    }
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

inline GenericModule RunPassesAfterSplitMixKernel(
    GenericModule module, const UBAffectingPassOptions &options,
    DebugTrace *trace) {
  module = MeasureStage(trace, "InlineScope", [&] {
    return RequireExactStage(RunStrictInlineScope(std::move(module)));
  });
  TraceGenericPass(trace, "InlineScope", module);
  module = MeasureStage(trace, "TileAndBindSubBlock", [&] {
    return RunTileAndBindSubBlock(std::move(module), trace,
                                  options.enableAutoBindSubBlock);
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

inline GenericModule RunPassesAfterPostSplitCanonicalization(
    GenericModule module, const UBAffectingPassOptions &options,
    DebugTrace *trace) {
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
    return RunCloneTensorEmpty(std::move(module), trace);
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
      return RunCloneTensorEmpty(std::move(module), trace);
    });
    TraceGenericPass(trace, "CloneTensorEmptyBeforeBufferize", module);
  } else if (trace) {
    trace->Pass("OptimizeDpsOpWithYieldedInsertSlice", {{"executed", 0}});
    trace->Pass("CloneTensorEmptyBeforeBufferize", {{"executed", 0}});
  }
  if (options.enableUbufSaving) {
    // cv2pm's production bufferizationPipeline clones again immediately
    // before sinking.  This is distinct from the unconditional post-split
    // clone and the Triton DPS clone above.
    module = MeasureStage(trace,
                          "CloneTensorEmptyBeforeUbufSavingSink", [&] {
      return RunCloneTensorEmpty(std::move(module), trace);
    });
    TraceGenericPass(trace, "CloneTensorEmptyBeforeUbufSavingSink", module);
    module = MeasureStage(trace, "SinkOpToConsumerInLoop", [&] {
      return RunSinkOpToConsumerInLoop(std::move(module));
    });
    TraceGenericPass(trace, "SinkOpToConsumerInLoop", module);
  } else if (trace) {
    trace->Pass("CloneTensorEmptyBeforeUbufSavingSink", {{"executed", 0}});
    trace->Pass("SinkOpToConsumerInLoop", {{"executed", 0}});
  }
  ValidateGenericModule(module);
  return module;
}

inline GenericModule RunPassesBeforeLoopInvariantCodeMotion(
    GenericModule module, const UBAffectingPassOptions &options = {},
    DebugTrace *trace = nullptr) {
  MeasureStage(trace, "ApplyOperationSemantics",
               [&] { ApplyOperationSemanticsToAll(module.operations); });
  // The production HIVM pipeline runs an additional CloneTensorEmpty/Sink
  // pair immediately after CVPipelining when UB-saving is enabled.  This is
  // distinct from the unconditional clone and UB-saving sink immediately
  // before OneShotBufferize below: the early pair changes the tensor SSA seen
  // by tiling, MIX projection, and the later bufferization passes.
  if (options.enableUbufSaving) {
    module = MeasureStage(trace, "CloneTensorEmptyAfterCVPipelining", [&] {
      return RunCloneTensorEmpty(std::move(module), trace);
    });
    TraceGenericPass(trace, "CloneTensorEmptyAfterCVPipelining", module);
    module = MeasureStage(trace, "SinkOpToConsumerInLoopAfterCVPipelining", [&] {
      return RunSinkOpToConsumerInLoop(std::move(module));
    });
    TraceGenericPass(trace, "SinkOpToConsumerInLoopAfterCVPipelining", module);
  } else if (trace) {
    trace->Pass("CloneTensorEmptyAfterCVPipelining", {{"executed", 0}});
    trace->Pass("SinkOpToConsumerInLoopAfterCVPipelining",
                {{"executed", 0}});
  }
  module = MeasureStage(trace, "TileCubeVectorLoop", [&] {
    return RequireExactStage(RunTileCubeVectorLoop(
        std::move(module), options.tileMixVectorLoop,
        options.tileMixCubeLoop));
  });
  TraceGenericPass(trace, "TileCubeVectorLoop", module);
  if (!options.disableAutoCVWorkSpaceManage) {
    module = MeasureStage(trace, "InferAndSetBufferSize", [&] {
      return RunInferAndSetBufferSizePipeline(std::move(module));
    });
    TraceGenericPass(trace, "InferAndSetBufferSize", module);
    module = MeasureStage(trace, "GlobalWorkspacePlan", [&] {
      return RunGlobalWorkspacePlan(std::move(module));
    });
    TraceGenericPass(trace, "GlobalWorkspacePlan", module);
  } else if (trace) {
    trace->Pass("InferAndSetBufferSize", {{"executed", 0}});
    trace->Pass("GlobalWorkspacePlan", {{"executed", 0}});
  }
  module = MeasureStage(trace, "CanonicalizationHIVMPipeline", [&] {
    return RunCanonicalizationHIVMPipeline(std::move(module));
  });
  TraceGenericPass(trace, "CanonicalizationHIVMPipeline", module);
  module = MeasureStage(trace, "MarkRealCoreType", [&] {
    return RunMarkRealCoreType(std::move(module), false,
                               /*inputCanonicalized=*/true);
  });
  TraceGenericPass(trace, "MarkRealCoreType", module);
  if (options.enableHIVMCrossCoreGSS &&
      !options.enableHIVMInjectBlockAllSync &&
      !options.disableAutoInjectBlockSync) {
    module = MeasureStage(trace, "CrossCoreGSS", [&] {
      return RunCrossCoreGSS(std::move(module));
    });
    TraceGenericPass(trace, "CrossCoreGSS", module);
  } else {
    module = MeasureStage(trace, "InjectBlockSync", [&] {
      return RunInjectBlockSync(std::move(module),
                                options.enableHIVMInjectBlockAllSync,
                                options.disableAutoInjectBlockSync);
    });
    TraceGenericPass(trace, "InjectBlockSync", module);
  }
  module = MeasureStage(trace, "MarkRealCoreType", [&] {
    return RunMarkRealCoreType(std::move(module), true);
  });
  TraceGenericPass(trace, "MarkRealCoreType", module);
  // cv2pm runs these two passes before SplitMixKernel.  They are proven
  // no-ops for the supported A2/A3 profile; the guard keeps that path exact
  // while failing closed if an Ascend950 UB/L1 allocation reaches the model.
  module = RequireExactStage(
      GuardTightlyCoupledBufferPasses(std::move(module)));
  TraceGenericPass(trace,
                   "MarkTightlyCoupledBuffer;HoistTightlyCoupledAlloc",
                   module);
  const bool mayContainAICProjection =
      MayContainAICProjection(module);
  std::optional<GenericModule> aicProjection;
  if (!options.disableInferHIVMDataLayout)
    MeasureStage(trace, "InferHIVMDataLayout.AICProjection", [&] {
      if (!mayContainAICProjection)
        return;
      aicProjection =
          RunSplitMixKernelProjection(module, SplitMixCoreType::Cube);
      ValidateInferHIVMDataLayoutOnAICProjection(*aicProjection);
    });
  MeasureStage(trace, "CopyOpVerifier.AICProjection", [&] {
    if (!mayContainAICProjection)
      return;
    if (!aicProjection)
      aicProjection =
          RunSplitMixKernelProjection(module, SplitMixCoreType::Cube);
    ValidateDiscardedAICBufferizedCopiesOnProjection(
        std::move(*aicProjection), options);
  });
  module = MeasureStage(trace, "SplitMixKernel", [&] {
    return RunSplitMixKernel(std::move(module));
  });
  TraceGenericPass(trace, "SplitMixKernel", module);
  return RunPassesAfterSplitMixKernel(std::move(module), options, trace);
}

inline GenericModule RunPassesBeforeOneShotBufferize(
    GenericModule module, const UBAffectingPassOptions &options = {},
    DebugTrace *trace = nullptr) {
  module =
      RunPassesBeforeLoopInvariantCodeMotion(std::move(module), options, trace);
  return RunPassesAfterPostSplitCanonicalization(std::move(module), options,
                                                  trace);
}

// SplitMixKernel keeps only the AIV projection in the lightweight pipeline,
// but cv2pm later bufferizes both projections.  Reproduce the verifier-visible
// CopyOp failures of the discarded AIC projection before dropping it.  This
// follows the same projection preflight already used for
// InferHIVMDataLayout's AIC-only failures.
inline void ValidateDiscardedAICBufferizedCopies(
    const GenericModule &module, const UBAffectingPassOptions &options) {
  const bool mayContainCubeProjection = std::any_of(
      module.operations.begin(), module.operations.end(),
      [](const GenericOperation &operation) {
        return IsSplitMixFunction(operation) ||
               (operation.name == "func.func" &&
                SplitMixEnumValue(FindDictionaryValue(
                    operation.attributes, "hivm.func_core_type")) == "AIC");
      });
  if (!mayContainCubeProjection)
    return;

  GenericModule aicProjection =
      RunSplitMixKernelProjection(module, SplitMixCoreType::Cube);
  ValidateDiscardedAICBufferizedCopiesOnProjection(
      std::move(aicProjection), options);
}

inline void ValidateDiscardedAICBufferizedCopiesOnProjection(
    GenericModule aicProjection, const UBAffectingPassOptions &options) {
  aicProjection = RunPassesAfterSplitMixKernel(
      std::move(aicProjection), options, nullptr);
  aicProjection = RunPassesAfterPostSplitCanonicalization(
      std::move(aicProjection), options, nullptr);
  OneShotBufferizationResult oneShot = RunOneShotBufferize(aicProjection);
  BufferizedSemanticIR bufferized =
      BuildBufferizedSemanticIR(std::move(aicProjection), std::move(oneShot));
  PostBufferizationRewriteState postBufferization =
      BuildPostBufferizationRewriteState(std::move(bufferized));
  const std::map<std::string, AddressSpace> scopes =
      InferHIVMMemScope(postBufferization);
  ValidateBufferizedCopyAddressSpaces(postBufferization, scopes);
}

inline AfterMarkMultiBufferState
BuildAfterMarkMultiBufferFromBeforeOneShotBufferize(
    GenericModule module,
    const UBAffectingPassOptions &options = {}, DebugTrace *trace = nullptr) {
  BufferizedSemanticIR oneShotBufferizeOutput =
      MeasureStage(trace, "OneShotBufferize", [&] {
        module = RunPostOneShotScalarCSEProjection(std::move(module));
        OneShotBufferizationResult bufferization =
            MeasureStage(trace, "OneShotBufferize.Analysis", [&] {
              return RunOneShotBufferize(module);
            });
        if (trace)
          trace->Artifact("OneShotBufferize.Analysis", [&] {
            return SerializeOneShotAnalysis(bufferization.decisions);
          });
        return MeasureStage(trace, "OneShotBufferize.BuildSemanticIR", [&] {
          return BuildBufferizedSemanticIR(std::move(module),
                                           std::move(bufferization));
        });
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
  PostBufferizationRewriteState hivmDecomposeOpOutput =
      MeasureStage(trace, "PostBufferizationRewrites", [&] {
        return BuildPostBufferizationRewriteState(
            std::move(oneShotBufferizeOutput));
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
  AfterAllocExtraBufferState allocExtraBufferOutput =
      MeasureStage(trace, "AlignStorageAndAllocExtraBuffer", [&] {
        // MarkStrideAlign only creates annotations; EnableStrideAlign is what
        // materializes them into physical layouts consumed by PlanMemory.  At
        // this modeled boundary their UB effect is therefore the conjunction,
        // while the two effective compiler options remain separate in the
        // public request and its digest.
        return BuildAfterAllocExtraBufferState(
            std::move(hivmDecomposeOpOutput),
            !options.disableAlignAllocSize,
            options.enableHIVMAutoStorageAlign &&
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
  AfterInlineLoadCopyState inlineLoadCopyOutput =
      MeasureStage(trace, "InlineLoadCopy", [&] {
        return BuildAfterInlineLoadCopyState(
            std::move(allocExtraBufferOutput),
            options.enableTritonKernelCompile);
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
  AfterMarkMultiBufferState markMultiBufferOutput =
      MeasureStage(trace, "MarkMultiBuffer", [&] {
        return BuildAfterMarkMultiBufferState(std::move(inlineLoadCopyOutput),
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
  return markMultiBufferOutput;
}

inline PlanMemoryInput BuildPlanMemoryInputFromBeforeOneShotBufferize(
    GenericModule module,
    const UBAffectingPassOptions &options = {}, DebugTrace *trace = nullptr,
    const std::string &targetFunction = {}) {
  AfterMarkMultiBufferState markMultiBufferOutput =
      BuildAfterMarkMultiBufferFromBeforeOneShotBufferize(
          std::move(module), options, trace);
  return MeasureStage(trace, "BuildPlanMemoryInput", [&] {
    return BuildPlanMemoryInput(std::move(markMultiBufferOutput),
                                targetFunction, trace);
  });
}

inline PlanMemoryInput BuildPlanMemoryInputFromAfterCVPipelining(
    GenericModule module,
    const UBAffectingPassOptions &options = {}, DebugTrace *trace = nullptr,
    const std::string &targetFunction = {}) {
  module =
      RunPassesBeforeOneShotBufferize(std::move(module), options, trace);
  return BuildPlanMemoryInputFromBeforeOneShotBufferize(
      std::move(module), options, trace, targetFunction);
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
  bool decisionOnlyNonOverflow = false;
  std::optional<uint64_t> conservativeUpperBoundBits;
  std::vector<FunctionPlanResult> functions;
  std::vector<std::string> diagnostics;
};

struct ConservativeNonOverflowProof {
  bool proven = false;
  uint64_t maxFunctionUpperBoundBits = 0;
};

// A successful PlanMemory placement cannot require more space than assigning
// every surviving UB buffer an independent 256-bit-aligned extent.  Applying
// the final multi-buffer multiplicity before any lifetime reuse or inplace
// merge therefore gives a conservative upper bound.  If every AIV function's
// bound fits, non-overflow is exact even though the concrete plan is unknown.
inline ConservativeNonOverflowProof ProveConservativeNonOverflow(
    const AfterMarkMultiBufferState &state, uint64_t capacityBits) {
  ConservativeNonOverflowProof result;
  const AfterInlineLoadCopyState &afterInline = state.afterInlineLoadCopy;
  const AfterAllocExtraBufferState &afterAlloc =
      afterInline.afterAllocExtraBuffer;
  const BufferizedSemanticIR &bufferized =
      afterAlloc.postBufferization.bufferized;
  const GenericModule &module = bufferized.logicalModule;
  const GenericModuleAnalysisIndexes &analysis =
      bufferized.logicalContext.analysis;
  analysis.ensureCompatible(module);

  std::map<int, uint64_t> functionUpperBounds;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name == "func.func" && IsAIVFunction(operation))
      functionUpperBounds.emplace(operation.id, 0);
  }
  if (functionUpperBounds.empty())
    return result;

  for (const LocalBufferRecord &buffer : afterInline.buffers) {
    if (buffer.addressSpace != AddressSpace::UB)
      continue;
    const int owner = BufferOwnerOperation(afterAlloc, buffer);
    if (owner < 0 || static_cast<size_t>(owner) >= module.operations.size())
      return result;
    const int function = analysis.enclosingFunctionId(owner);
    auto functionBound = functionUpperBounds.find(function);
    if (functionBound == functionUpperBounds.end())
      continue;
    uint32_t multiBufferNum = 1;
    const auto multi =
        state.markMultiBuffer.buffer2MultiNum.find(buffer.sourceIdentity);
    if (multi != state.markMultiBuffer.buffer2MultiNum.end()) {
      if (multi->second == 0)
        return result;
      multiBufferNum = multi->second;
    }
    const uint64_t alignedBits = AlignUp(buffer.constBits, 256);
    const uint64_t physicalBits =
        CheckedMul(alignedBits, multiBufferNum,
                   "conservative non-overflow multi-buffer extent");
    functionBound->second =
        CheckedAdd(functionBound->second, physicalBits,
                   "conservative non-overflow function upper bound");
    if (functionBound->second > capacityBits)
      return result;
  }

  for (const auto &function : functionUpperBounds)
    result.maxFunctionUpperBoundBits =
        std::max(result.maxFunctionUpperBoundBits, function.second);
  result.proven = true;
  return result;
}

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
    GenericModule module,
    const UBAffectingPassOptions &options = {},
    std::optional<uint32_t> planMemorySeed = std::nullopt,
    bool restrictInplaceAsISA = false, DebugTrace *trace = nullptr,
    uint64_t capacityBits = kUBCapacityBits,
    bool enableDecisionOnlyNonOverflow = false,
    bool observeConservativeNonOverflow = false) {
  GenericModule projected =
      RunPassesBeforeOneShotBufferize(std::move(module), options, trace);
  const std::vector<std::string> functions = AIVFunctionNames(projected);
  ModulePlanResult result;
  result.capacityBits = capacityBits;
  std::optional<AfterMarkMultiBufferState> decisionState;
  if ((enableDecisionOnlyNonOverflow || observeConservativeNonOverflow) &&
      !functions.empty()) {
    GenericModule decisionModule =
        functions.size() == 1 ? std::move(projected) : projected;
    decisionState = BuildAfterMarkMultiBufferFromBeforeOneShotBufferize(
        std::move(decisionModule), options, trace);
    const ConservativeNonOverflowProof proof =
        ProveConservativeNonOverflow(*decisionState, capacityBits);
    if (proof.proven) {
      result.conservativeUpperBoundBits =
          proof.maxFunctionUpperBoundBits;
      if (enableDecisionOnlyNonOverflow) {
        result.decisionOnlyNonOverflow = true;
        return result;
      }
    }
  }
  for (size_t functionIndex = 0; functionIndex < functions.size();
       ++functionIndex) {
    const std::string &function = functions[functionIndex];
    PlanMemoryInput input;
    if (functions.size() == 1 && decisionState) {
      input = MeasureStage(trace, "BuildPlanMemoryInput", [&] {
        return BuildPlanMemoryInput(std::move(*decisionState), function,
                                    trace);
      });
    } else {
      GenericModule functionModule =
          functions.size() == 1 ? std::move(projected) : projected;
      input = BuildPlanMemoryInputFromBeforeOneShotBufferize(
          std::move(functionModule), options, trace, function);
    }
    PlanMemoryModelResult plan = MeasureStage(trace, "PlanMemory", [&] {
      return planMemorySeed
                 ? PlanLocalMemoryForSeed(input, *planMemorySeed,
                                          restrictInplaceAsISA, trace,
                                          capacityBits)
                 : PlanLocalMemory(input, restrictInplaceAsISA, trace,
                                   capacityBits);
    });
    TracePlanMemoryResult(trace, plan);
    result.success = result.success && plan.success;
    result.overflow = result.overflow || plan.overflow;
    result.peakBits = std::max(result.peakBits, plan.peakBits);
    result.requiredBits = std::max(result.requiredBits, plan.requiredBits);
    result.functions.push_back({function, std::move(plan)});
  }
  if (result.conservativeUpperBoundBits && result.overflow)
    throw std::runtime_error(
        "conservative non-overflow proof contradicted full PlanMemory");
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
  uint64_t capacityBits = kUBCapacityBits;
  DebugTrace *debugTrace = nullptr;
  bool enableDecisionOnlyNonOverflow = false;
  bool observeConservativeNonOverflow = false;
};

inline ModulePlanResult RunCVPipeliningUBModulePipeline(
    GenericModule module, const CVPipeliningUBPipelineOptions &options = {}) {
  MeasureStage(options.debugTrace, "ApplyOperationSemantics",
               [&] { ApplyOperationSemanticsToAll(module.operations); });
  module = MeasureStage(options.debugTrace, "CVPipelining", [&] {
    CVPipeliningOptions cvOptions = options.cvPipelining;
    cvOptions.disabled = cvOptions.disabled ||
                         options.ubAffectingPasses.disableAutoCVWorkSpaceManage;
    return RunCVPipeliningPass(std::move(module), cvOptions);
  });
  TraceCVPipelining(options.debugTrace, module);
  return RunUBModuleFromAfterCVPipelining(
      std::move(module), options.ubAffectingPasses, options.planMemorySeed,
      options.restrictInplaceAsISA, options.debugTrace,
      options.capacityBits, options.enableDecisionOnlyNonOverflow,
      options.observeConservativeNonOverflow);
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
    CVPipeliningOptions cvOptions = options.cvPipelining;
    cvOptions.disabled = cvOptions.disabled ||
                         options.ubAffectingPasses.disableAutoCVWorkSpaceManage;
    return RunCVPipeliningPass(std::move(module), cvOptions);
  });
  TraceCVPipelining(options.debugTrace, module);
  const PlanMemoryInput input = BuildPlanMemoryInputFromAfterCVPipelining(
      std::move(module), options.ubAffectingPasses, options.debugTrace);
  PlanMemoryModelResult result =
      MeasureStage(options.debugTrace, "PlanMemory", [&] {
        return options.planMemorySeed
                   ? PlanLocalMemoryForSeed(input, *options.planMemorySeed,
                                            options.restrictInplaceAsISA,
                                            options.debugTrace,
                                            options.capacityBits)
                   : PlanLocalMemory(input, options.restrictInplaceAsISA,
                                     options.debugTrace,
                                     options.capacityBits);
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
