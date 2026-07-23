#ifndef CVPIPELINE_UB_MODEL_CPP_GENERIC_PIPELINE_CONTEXT_HPP
#define CVPIPELINE_UB_MODEL_CPP_GENERIC_PIPELINE_CONTEXT_HPP

#include "pipeline_metadata_cache.hpp"
#include "../ir/generic_analysis.hpp"

namespace cvub {

// Unified pass-local owner for mutation-aware Generic IR analysis and
// content-keyed metadata.  GenericRewriter notifications are routed through
// listener(), while parsed metadata remains valid because it is keyed by the
// complete source string rather than an operation address.
class GenericPipelineContext {
public:
  explicit GenericPipelineContext(
      GenericModule &module,
      unsigned requestedAnalysisIndexes = kGenericAnalysisAll)
      : analysisContext(module, requestedAnalysisIndexes) {}

  PipelineAnalysisContext &analysis() { return analysisContext; }
  const PipelineAnalysisContext &analysis() const { return analysisContext; }
  PipelineMetadataCache &metadata() { return metadataCache; }
  const PipelineMetadataCache &metadata() const { return metadataCache; }
  GenericMutationListener *listener() { return &analysisContext; }

private:
  PipelineAnalysisContext analysisContext;
  PipelineMetadataCache metadataCache;
};

// Immutable pipeline stages can move this sidecar together with their module.
// Analysis indexes and parsed metadata are then shared by every downstream
// projection instead of being reconstructed at each stage boundary.
struct GenericPipelineSnapshotContext {
  GenericModuleAnalysisIndexes analysis;
  mutable PipelineMetadataCache metadata;
};

} // namespace cvub

#endif
