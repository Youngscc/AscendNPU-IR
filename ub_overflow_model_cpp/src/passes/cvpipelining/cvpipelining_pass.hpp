#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_PASS_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_PASS_HPP

#include "cvpipelining_preload_rewrite.hpp"

namespace cvub {

inline bool CVPipelineIsInsideAny(const GenericModule &module, int operation,
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
    const GenericOperation &operation) {
  const std::string text = operation.properties + operation.attributes;
  return text.find("multibuffer_unroll_factor") != std::string::npos ||
         text.find("hivm.preload_num") != std::string::npos;
}

// The real CVPipelining pass rewrites workspace Store/Fixpipe destinations to
// memref views before it materializes tensor slices for actual result users.
// The lightweight rewriter has to build a value map eagerly, so it can leave a
// dead extract_slice behind for a destination that is subsequently replaced.
// Match the real pass boundary by removing only those dead extracts whose
// source is a workspace allocation.  Delay this until all generated loops are
// complete because a later rewrite item may still consume an eager mapping.
inline GenericModule CVPipelineRemoveUnusedGeneratedWorkspaceSlices(
    GenericModule module) {
  std::map<int, int> definitions;
  std::map<int, size_t> uses;
  for (const GenericOperation &operation : module.operations) {
    for (int value : operation.results)
      definitions[value] = operation.id;
    for (int value : operation.operands)
      ++uses[value];
  }

  std::vector<std::pair<int, int>> removals;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "tensor.extract_slice" ||
        operation.operands.empty() || operation.results.empty() ||
        std::any_of(operation.results.begin(), operation.results.end(),
                    [&](int value) { return uses[value] != 0; }))
      continue;

    const auto tensorDefinition = definitions.find(operation.operands.front());
    if (tensorDefinition == definitions.end())
      continue;
    const GenericOperation &toTensor = module.operations.at(
        static_cast<size_t>(tensorDefinition->second));
    if (toTensor.name != "bufferization.to_tensor" ||
        toTensor.operands.empty())
      continue;

    const auto allocationDefinition = definitions.find(toTensor.operands.front());
    if (allocationDefinition == definitions.end() ||
        module.operations.at(static_cast<size_t>(allocationDefinition->second))
                .name != "memref_ext.alloc_workspace")
      continue;
    removals.emplace_back(operation.blockId, operation.id);
  }

  if (removals.empty())
    return module;
  GenericRewriter rewriter(module);
  for (const auto &[blockId, operationId] : removals)
    rewriter.removeFromBlock(blockId, operationId);
  return CompactGenericModule(std::move(module));
}

inline GenericModule RunCVPipeliningPass(
    GenericModule module, const CVPipeliningOptions &options) {
  if (options.disabled || options.setDepthInUnrollMode == 0 ||
      options.setDepthInUnrollMode == 1)
    return module;

  for (size_t index = 0; index < module.operations.size(); ++index) {
    GenericOperation &operation = module.operations[index];
    if (operation.name == "scf.for")
      CVPipelineMarkRegionCoreTypes(module, operation);
  }

  bool changed = true;
  while (changed) {
    changed = false;
    std::vector<int> loops;
    for (const GenericOperation &operation : module.operations)
      if (operation.name == "scf.for")
        loops.push_back(operation.id);

    for (int loop : loops) {
      if (loop < 0 || static_cast<size_t>(loop) >= module.operations.size())
        continue;
      const GenericOperation &operation =
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

  module = CVPipelineRemoveUnusedGeneratedWorkspaceSlices(std::move(module));
  CVPipelineRemoveWorkspaceMultiBufferMarks(module);
  return module;
}

} // namespace cvub

#endif
