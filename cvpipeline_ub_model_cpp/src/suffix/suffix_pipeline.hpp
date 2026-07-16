#ifndef CVPIPELINE_UB_MODEL_CPP_SUFFIX_PIPELINE_HPP
#define CVPIPELINE_UB_MODEL_CPP_SUFFIX_PIPELINE_HPP

#include "planmemory_bridge.hpp"

namespace cvub {

struct SuffixPipelineOptions {
  bool enableAutoMultiBuffer = false;
  MultiBufferStrategy limitAutoMultiBufferOfLocalBuffer =
      MultiBufferStrategy::CubeNoL0C;
  MultiBufferStrategy limitMixAutoMultiBufferBuffer =
      MultiBufferStrategy::OnlyCube;
};

inline PlanMemoryInput BuildSuffixPlanMemoryInput(
    GenericModule module,
    const SuffixPipelineOptions &options = {}) {
  for (GenericOperation &operation : module.operations)
    if (HasModeledOperationSemantics(operation.name))
      ApplyOperationSemantics(operation);

  const OneShotBufferizationResult bufferization =
      RunOneShotBufferize(module);
  MarkMultiBufferOptions multiBufferOptions;
  multiBufferOptions.enableAuto = options.enableAutoMultiBuffer;
  multiBufferOptions.limitAutoMultiBufferOfLocalBuffer =
      options.limitAutoMultiBufferOfLocalBuffer;
  multiBufferOptions.limitMixAutoMultiBufferBuffer =
      options.limitMixAutoMultiBufferBuffer;
  PlanMemoryInputSemanticIR semantic = BuildPlanMemoryInputSemanticIR(
      BuildAfterMarkMultiBufferState(
          BuildAfterInlineLoadCopyState(BuildAfterAllocExtraBufferState(BuildPostBufferizationRewriteState(
              BuildBufferizedSemanticIR(module, bufferization)))),
          multiBufferOptions));

  for (const LocalBufferRecord &buffer : semantic.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.buffers) {
    const std::optional<MemRefTypeModel> type = ParseMemRefType(buffer.type);
    const bool dynamic = type && std::any_of(
        type->shape.begin(), type->shape.end(),
        [](const std::optional<int64_t> &extent) { return !extent; });
    const auto alignment =
        semantic.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.find(
            buffer.sourceIdentity);
    if (dynamic &&
        alignment != semantic.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.end() &&
        !alignment->second.empty())
      throw std::runtime_error(
          "C suffix: dynamic stride-aligned allocation is not supported by "
          "the initial delivery");
  }
  return BuildPlanMemoryInput(semantic);
}

inline PlanMemoryInput BuildSuffixPlanMemoryInput(
    const fs::path &c1GenericIR,
    const SuffixPipelineOptions &options = {}) {
  return BuildSuffixPlanMemoryInput(ParseGenericIR(c1GenericIR, false),
                                    options);
}

} // namespace cvub

#endif
