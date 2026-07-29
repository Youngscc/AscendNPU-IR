#ifndef UB_OVERFLOW_MODEL_CPP_FUNC_EXTENDED_CANONICALIZER_HPP
#define UB_OVERFLOW_MODEL_CPP_FUNC_EXTENDED_CANONICALIZER_HPP

#include "module_extended_canonicalizer.hpp"

namespace cvub {

// The first func.func-nested ExtendedCanonicalizer in
// canonicalizationHIVMPipeline.  Every modeled rewrite in the shared
// implementation is already partitioned by enclosing function: operation
// folder state is constructed once per func.func, while affine/Arith/slice
// rewrites never inspect or replace values across an isolated function
// boundary.  Keep a distinct stage entry so its checkpoint and later second
// invocation remain independently verifiable.
inline GenericModule RunFirstFuncExtendedCanonicalizer(GenericModule module) {
  return RunModuleExtendedCanonicalizer(std::move(module));
}

// The second invocation runs immediately after HIVMOptSinglePoint.  Native
// BiSheng registers the identical func.func-nested canonicalize-ext pass a
// second time so newly created scalar arith/math and memref operations reach
// the same greedy fixed point.  Preserve a separate entry and checkpoint even
// though the implementation and function-isolation proof are shared.
inline GenericModule RunSecondFuncExtendedCanonicalizer(GenericModule module) {
  return RunModuleExtendedCanonicalizer(std::move(module));
}

} // namespace cvub

#endif
