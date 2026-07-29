#ifndef UB_OVERFLOW_MODEL_CPP_MODULE_EXTENDED_CANONICALIZER_HPP
#define UB_OVERFLOW_MODEL_CPP_MODULE_EXTENDED_CANONICALIZER_HPP

#include "canonicalization_hivm_pipeline.hpp"
#include "outer_extended_canonicalizer.hpp"

namespace cvub {

// The module-scoped ExtendedCanonicalizer inside
// canonicalizationHIVMPipeline.  This is a distinct pipeline boundary from the
// outer canonicalizer even though both register the same dialect patterns:
// ArithToAffine and CanonicalizeIterArg have exposed affine composition,
// constant-folding and slice-folding opportunities at this point.
inline GenericModule RunModuleExtendedCanonicalizer(GenericModule module) {
  module = RunOuterExtendedCanonicalizer(std::move(module));
  module = RunExistingAffineCanonicalization(std::move(module));

  // Native applyPatternsAndFoldGreedily reaches one fixed point across all
  // registered dialect patterns. Affine folding can expose constant slice
  // sizes. Run those newly-enabled patterns before the next iteration's
  // OperationFolder constant CSE: an old constant may become dead in this
  // iteration while a newly materialized constant of the same value stays
  // live, and native only hoists the survivor on the following iteration.
  RunExistingAffineExposedArithFolds(module);
  const GenericModuleAnalysisIndexes foldIndexes(
      module, kGenericAnalysisDefinitions | kGenericAnalysisUsers);
  while (FoldConstantOffsetSizeAndStrideOperands(module, foldIndexes)) {
  }
  PipelineAnalysisContext useLists(
      module, kGenericAnalysisDefinitions | kGenericAnalysisUsers);
  while (FoldCanonicalizationBooleanOps(module, useLists)) {
  }
  while (EliminateCanonicalizationDeadCode(module, useLists)) {
  }
  RunExistingAffineDeadCodeElimination(module);

  // This is the next greedy iteration's OperationFolder pass. It CSEs a
  // freshly materialized constant with an older live constant, or hoists the
  // fresh constant when the older one became dead above.
  std::vector<int> functions;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      functions.push_back(operation.id);
  for (int functionId : functions)
    RunGreedyOperationFolder(module, functionId);

  module = RunOuterExtendedCanonicalizer(std::move(module));
  RunExistingAffineDeadCodeElimination(module);
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
