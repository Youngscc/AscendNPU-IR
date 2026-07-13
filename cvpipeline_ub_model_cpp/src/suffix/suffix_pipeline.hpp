#ifndef CVPIPELINE_UB_MODEL_CPP_SUFFIX_PIPELINE_HPP
#define CVPIPELINE_UB_MODEL_CPP_SUFFIX_PIPELINE_HPP

#include "planmemory_bridge.hpp"

namespace cvub {

struct SuffixPipelineOptions {
  bool enableAutoMultiBuffer = false;
  MultiBufferStrategy limitAutoMultiBufferOfLocalBuffer =
      MultiBufferStrategy::NoLimit;
  MultiBufferStrategy limitMixAutoMultiBufferBuffer =
      MultiBufferStrategy::NoLimit;
};

inline PlanMemoryInput BuildSuffixPlanMemoryInput(
    C1SemanticModule module,
    const SuffixPipelineOptions &options = {}) {
  for (C1OperationRecord &operation : module.operations)
    if (C1IsReviewedOperation(operation.name))
      ApplyC1OpSemantics(operation);

  const OneShotBufferizationResult bufferization =
      RunOneShotBufferize(module);
  MarkMultiBufferOptions multiBufferOptions;
  multiBufferOptions.enableAuto = options.enableAutoMultiBuffer;
  multiBufferOptions.limitAutoMultiBufferOfLocalBuffer =
      options.limitAutoMultiBufferOfLocalBuffer;
  multiBufferOptions.limitMixAutoMultiBufferBuffer =
      options.limitMixAutoMultiBufferBuffer;
  PlanMemoryInputSemanticIR semantic = BuildPlanMemoryInputSemanticIR(
      BuildC6SemanticIR(
          BuildC5SemanticIR(BuildC4SemanticIR(BuildC3SemanticIR(
              BuildBufferizedSemanticIR(module, bufferization)))),
          multiBufferOptions));

  for (const C4BufferRecord &buffer : semantic.c6.c5.c4.buffers) {
    const std::optional<MemRefTypeModel> type = ParseMemRefType(buffer.type);
    const bool dynamic = type && std::any_of(
        type->shape.begin(), type->shape.end(),
        [](const std::optional<int64_t> &extent) { return !extent; });
    const auto alignment =
        semantic.c6.c5.c4.alignStorage.strideAlignments.find(
            buffer.sourceIdentity);
    if (dynamic &&
        alignment != semantic.c6.c5.c4.alignStorage.strideAlignments.end() &&
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
  return BuildSuffixPlanMemoryInput(ParseC1GenericIR(c1GenericIR, false),
                                    options);
}

} // namespace cvub

#endif
