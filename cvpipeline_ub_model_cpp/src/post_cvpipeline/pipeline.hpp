#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_PIPELINE_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_PIPELINE_HPP

#include "types.hpp"
#include "buffer_size.hpp"
#include "canonicalization.hpp"
#include "inline_scope.hpp"
#include "split_mix_aiv.hpp"
#include "tensor_empty.hpp"
#include "tile_cube_vector_loop.hpp"

#include <array>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cvub {

inline constexpr std::array<const char *, 14> kPostCVPipelineStageNames = {
    "TileCubeVectorLoop",
    "InferAndSetBufferSize",
    "WorkspaceSemanticProjection",
    "CanonicalizationBeforeSplit",
    "CrossCoreSyncInvariant",
    "SplitMixKernelAIVProjection",
    "InlineScope",
    "TileAndBindSubBlock",
    "FoldTensorEmpty",
    "CanonicalizationAfterSplit",
    "LoopInvariantCodeMotion",
    "LoopInvariantSubsetHoisting",
    "CloneTensorEmpty",
    "InlineOTFLoadStore"};

inline CoverageDisposition ParseCoverageDisposition(const std::string &text) {
  if (text == "modeled")
    return CoverageDisposition::Modeled;
  if (text == "ub-invariant")
    return CoverageDisposition::UBInvariant;
  if (text == "unsupported")
    return CoverageDisposition::Unsupported;
  throw std::runtime_error("post-CVPipeline manifest: unknown disposition '" +
                           text + "'");
}

inline std::vector<std::string> SplitManifestRow(const std::string &line) {
  std::vector<std::string> columns;
  std::istringstream input(line);
  std::string column;
  while (std::getline(input, column, '\t'))
    columns.push_back(std::move(column));
  if (!line.empty() && line.back() == '\t')
    columns.emplace_back();
  return columns;
}

inline std::vector<StageCoverage>
LoadPostCVPipelineManifest(const fs::path &path) {
  std::ifstream input(path);
  if (!input)
    throw std::runtime_error("cannot open post-CVPipeline manifest: " +
                             path.string());

  std::string line;
  if (!std::getline(input, line) || line != "order\tstage\tdisposition")
    throw std::runtime_error("post-CVPipeline manifest: expected order, stage, "
                             "and disposition columns");

  std::vector<StageCoverage> stages;
  std::set<std::string> seen;
  size_t lineNumber = 1;
  while (std::getline(input, line)) {
    ++lineNumber;
    if (line.empty())
      continue;
    const std::vector<std::string> columns = SplitManifestRow(line);
    if (columns.size() != 3 || columns[0].empty() || columns[1].empty() ||
        columns[2].empty()) {
      throw std::runtime_error(
          "post-CVPipeline manifest: missing columns at line " +
          std::to_string(lineNumber));
    }

    if (!seen.insert(columns[1]).second)
      throw std::runtime_error("post-CVPipeline manifest: duplicate stage '" +
                               columns[1] + "'");
    const size_t expectedIndex = stages.size();
    const std::string expectedOrder = std::to_string(expectedIndex + 1);
    if (expectedIndex >= kPostCVPipelineStageNames.size() ||
        columns[0] != expectedOrder ||
        columns[1] != kPostCVPipelineStageNames[expectedIndex]) {
      throw std::runtime_error(
          "post-CVPipeline manifest: stage order mismatch at line " +
          std::to_string(lineNumber));
    }
    stages.push_back({columns[1], ParseCoverageDisposition(columns[2])});
  }

  if (stages.size() != kPostCVPipelineStageNames.size())
    throw std::runtime_error(
        "post-CVPipeline manifest: stage order mismatch; expected " +
        std::to_string(kPostCVPipelineStageNames.size()) + " stages");
  return stages;
}

inline std::vector<std::string>
StageNames(const std::vector<StageCoverage> &stages) {
  std::vector<std::string> names;
  names.reserve(stages.size());
  for (const StageCoverage &stage : stages)
    names.push_back(stage.stage);
  return names;
}

inline std::vector<StageCoverage> CompiledPostCVPipelineCoverage() {
  std::vector<StageCoverage> stages;
  stages.reserve(kPostCVPipelineStageNames.size());
  for (const char *stage : kPostCVPipelineStageNames)
    stages.push_back({stage, CoverageDisposition::Unsupported});
  stages.front().disposition = CoverageDisposition::Modeled;
  stages[1].disposition = CoverageDisposition::Modeled;
  stages[2].disposition = CoverageDisposition::UBInvariant;
  stages[3].disposition = CoverageDisposition::Modeled;
  stages[4].disposition = CoverageDisposition::UBInvariant;
  stages[5].disposition = CoverageDisposition::Modeled;
  stages[6].disposition = CoverageDisposition::Modeled;
  stages[8].disposition = CoverageDisposition::Modeled;
  stages[9].disposition = CoverageDisposition::Modeled;
  stages[12].disposition = CoverageDisposition::Modeled;
  return stages;
}

inline void MergeStageResult(PostCVPipelineResult &result, StageResult stage) {
  if (stage.precision == Precision::Incomplete)
    result.precision = Precision::Incomplete;
  result.diagnostics.insert(result.diagnostics.end(),
                            stage.diagnostics.begin(),
                            stage.diagnostics.end());
  result.module = std::move(stage.module);
}

inline PostCVPipelineResult RunPostCVPipelineAIVProjection(
    GenericModule module, const PostCVPipelineOptions &options) {
  PostCVPipelineResult result;
  result.coverage = CompiledPostCVPipelineCoverage();

  StageResult tiled = RunTileCubeVectorLoop(
      std::move(module), options.tileMixVectorLoop, options.tileMixCubeLoop);
  MergeStageResult(result, std::move(tiled));

  MergeStageResult(result,
                   RunInferAndSetBufferSize(std::move(result.module)));
  MergeStageResult(result,
                   ProveWorkspacePlanningUBInvariant(std::move(result.module)));
  MergeStageResult(result,
                   RunPreSplitCanonicalization(std::move(result.module)));
  MergeStageResult(result,
                   ProveCrossCoreSyncPipelineUBInvariant(
                       std::move(result.module)));

  PostCVPipelineResult split = ProjectMixFunctionsToAIV(result.module);
  if (split.precision == Precision::Incomplete)
    result.precision = Precision::Incomplete;
  result.diagnostics.insert(result.diagnostics.end(), split.diagnostics.begin(),
                            split.diagnostics.end());
  result.functions = std::move(split.functions);

  const auto applyProjectedStage = [&](ProjectedAIVModule &function,
                                       StageResult stage) {
    if (stage.precision == Precision::Incomplete)
      result.precision = Precision::Incomplete;
    for (PostCVPipelineDiagnostic &diagnostic : stage.diagnostics) {
      if (diagnostic.function.empty())
        diagnostic.function = function.projectedFunction;
      result.diagnostics.push_back(std::move(diagnostic));
    }
    function.module = std::move(stage.module);
  };
  for (ProjectedAIVModule &function : result.functions) {
    applyProjectedStage(function, RunInlineScope(std::move(function.module)));
    // TileAndBindSubBlock is inserted here by Task 7.  Keeping the subsequent
    // stages in their real relative order makes unsupported coverage explicit.
    applyProjectedStage(function,
                        RunFoldTensorEmpty(std::move(function.module)));
    applyProjectedStage(
        function, RunPostSplitCanonicalization(std::move(function.module)));
    // Code-motion stages are inserted here by Task 8.
    applyProjectedStage(function,
                        RunCloneTensorEmpty(std::move(function.module)));
  }

  if (options.enableUbufSaving) {
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"configuration", "", -1, "", "enableUbufSaving is unsupported"});
  }

  for (const StageCoverage &stage : result.coverage) {
    if (stage.disposition != CoverageDisposition::Unsupported)
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {stage.stage, "", -1, "", "pipeline stage is unsupported"});
  }
  return result;
}

} // namespace cvub

#endif
