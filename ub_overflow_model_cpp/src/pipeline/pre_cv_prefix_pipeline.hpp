#ifndef UB_OVERFLOW_MODEL_CPP_PRE_CV_PREFIX_PIPELINE_HPP
#define UB_OVERFLOW_MODEL_CPP_PRE_CV_PREFIX_PIPELINE_HPP

#include "../passes/auto_blockify_parallel_loop.hpp"
#include "../passes/func_extended_canonicalizer.hpp"
#include "../passes/outer_extended_canonicalizer.hpp"
#include "../passes/pre_cv_cse.hpp"
#include "../passes/pre_cv_hivm_opt_single_point.hpp"
#include "../passes/pre_cv_inline_otf_broadcast.hpp"
#include "../passes/pre_cv_mark_multi_buffer.hpp"
#include "../passes/pre_cv_memref_dead_store_elimination.hpp"
#include "../passes/scf_for_loop_canonicalization.hpp"
#include "../support/debug_trace.hpp"

namespace cvub {

// Resolved options for the exact native prefix between the checkpoint before
// AutoBlockify and the checkpoint before CVPipelining.  Callers must copy
// these values from the same HIVMPipelineOptions instance used to construct
// the native pipeline; this layer does not reconstruct compiler defaults.
struct PreCVPrefixPipelineOptions {
  bool enableTritonKernelCompile = true;
  bool enableAutoBlockifyLoop = true;
  bool disableAutoCVWorkSpaceManage = false;
  bool enableAutoMultiBuffer = false;
  bool limitAutoMultiBufferOnlyForLocalBuffer = false;
  MultiBufferStrategy localMultiBufferStrategy =
      MultiBufferStrategy::CubeNoL0C;
  MultiBufferStrategy mixMultiBufferStrategy = MultiBufferStrategy::OnlyCube;
  unsigned workspaceMultiBufferNum = 4;
};

template <typename Transform>
inline GenericModule RunPreCVPrefixStage(DebugTrace *trace,
                                         const char *stage,
                                         GenericModule module,
                                         Transform &&transform) {
  return MeasureStage(trace, stage, [&] {
    return transform(std::move(module));
  });
}

inline GenericModule RunPreCVPrefixPipeline(
    GenericModule module, const PreCVPrefixPipelineOptions &options,
    DebugTrace *trace = nullptr) {
  AutoBlockifyPrefixOptions autoBlockify;
  autoBlockify.enableTritonKernelCompile = options.enableTritonKernelCompile;
  autoBlockify.enableAutoBlockifyLoop = options.enableAutoBlockifyLoop;
  module = RunPreCVPrefixStage(
      trace, "PreCV.AutoBlockify", std::move(module),
      [&](GenericModule current) {
        return RunAutoBlockifyPrefixStage(std::move(current), autoBlockify);
      });

  PreCVMarkMultiBufferOptions markMultiBuffer;
  markMultiBuffer.disableAutoCVWorkSpaceManage =
      options.disableAutoCVWorkSpaceManage;
  markMultiBuffer.enableAuto = options.enableAutoMultiBuffer;
  markMultiBuffer.limitAutoMultiBufferOnlyForLocalBuffer =
      options.limitAutoMultiBufferOnlyForLocalBuffer;
  markMultiBuffer.limitAutoMultiBufferOfLocalBuffer =
      options.localMultiBufferStrategy;
  markMultiBuffer.limitMixAutoMultiBufferBuffer =
      options.mixMultiBufferStrategy;
  markMultiBuffer.workspaceMultiBufferNum = options.workspaceMultiBufferNum;
  module = RunPreCVPrefixStage(
      trace, "PreCV.MarkMultiBuffer", std::move(module),
      [&](GenericModule current) {
        return RunPreCVMarkMultiBuffer(std::move(current), markMultiBuffer);
      });

  module = RunPreCVPrefixStage(
      trace, "PreCV.OuterExtendedCanonicalizer", std::move(module),
      [](GenericModule current) {
        return RunOuterExtendedCanonicalizer(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.ArithToAffine", std::move(module),
      [](GenericModule current) {
        return RunArithToAffineConversionPass(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.CanonicalizeIterArg", std::move(module),
      [](GenericModule current) {
        return RunCanonicalizationHIVMAfterArithToAffine(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.ModuleExtendedCanonicalizer", std::move(module),
      [](GenericModule current) {
        return RunModuleExtendedCanonicalizer(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.SCFForLoopCanonicalization", std::move(module),
      [](GenericModule current) {
        return RunSCFForLoopCanonicalization(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.CSE", std::move(module), [](GenericModule current) {
        return RunPreCVCSE(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.FirstFuncExtendedCanonicalizer", std::move(module),
      [](GenericModule current) {
        return RunFirstFuncExtendedCanonicalizer(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.HIVMOptSinglePoint", std::move(module),
      [](GenericModule current) {
        return RunPreCVHIVMOptSinglePoint(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.SecondFuncExtendedCanonicalizer", std::move(module),
      [](GenericModule current) {
        return RunSecondFuncExtendedCanonicalizer(std::move(current));
      });
  module = RunPreCVPrefixStage(
      trace, "PreCV.MemrefDeadStoreElimination", std::move(module),
      [](GenericModule current) {
        return RunPreCVMemrefDeadStoreElimination(std::move(current));
      });
  return RunPreCVPrefixStage(
      trace, "PreCV.InlineOTFBroadcast", std::move(module),
      [](GenericModule current) {
        return RunPreCVInlineOTFBroadcast(std::move(current));
      });
}

} // namespace cvub

#endif
