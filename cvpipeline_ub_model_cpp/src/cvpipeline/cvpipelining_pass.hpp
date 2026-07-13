#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_PASS_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_PASS_HPP

#include "cvpipelining_preload_rewrite.hpp"

namespace cvub {

inline bool CVPipelineIsInsideAny(const C1SemanticModule &module, int operation,
                                  const std::set<int> &ancestors) {
  int current = operation;
  while (current >= 0) {
    if (ancestors.count(current) != 0)
      return true;
    current = module.operations.at(static_cast<size_t>(current)).parentId;
  }
  return false;
}

inline bool CVPipelineHasGeneratedPipelineAttr(
    const C1OperationRecord &operation) {
  const std::string text = operation.properties + operation.attributes;
  return text.find("multibuffer_unroll_factor") != std::string::npos ||
         text.find("hivm.preload_num") != std::string::npos;
}

inline C1SemanticModule RunCVPipeliningPass(
    C1SemanticModule module, const CVPipeliningOptions &options) {
  if (options.disabled || options.setDepthInUnrollMode == 0 ||
      options.setDepthInUnrollMode == 1)
    return module;

  for (size_t index = 0; index < module.operations.size(); ++index) {
    C1OperationRecord &operation = module.operations[index];
    if (operation.name == "scf.for")
      CVPipelineMarkRegionCoreTypes(module, operation);
  }

  bool changed = true;
  while (changed) {
    changed = false;
    std::vector<int> loops;
    for (const C1OperationRecord &operation : module.operations)
      if (operation.name == "scf.for")
        loops.push_back(operation.id);

    for (int loop : loops) {
      if (loop < 0 || static_cast<size_t>(loop) >= module.operations.size())
        continue;
      const C1OperationRecord &operation =
          module.operations.at(static_cast<size_t>(loop));
      if (operation.name != "scf.for" ||
          CVPipelineHasGeneratedPipelineAttr(operation))
        continue;

      CVPipelineImplAnalysis analysis(module, loop,
                                      options.setDepthInUnrollMode,
                                      options.enableLazyLoading);
      CVPipelineAnalysisResult result = analysis.run();
      if (!result.success)
        continue;

      bool rewritten = false;
      if (options.enableSkewMode) {
        CVPipelinePreloadRewriter rewriter(module, std::move(result));
        rewritten = rewriter.rewrite();
      } else {
        CVPipelineLoopRewriter rewriter(module, std::move(result));
        rewritten = rewriter.rewrite();
      }
      if (rewritten) {
        changed = true;
        break;
      }
    }
  }

  CVPipelineRemoveWorkspaceMultiBufferMarks(module);
  return module;
}

} // namespace cvub

#endif
