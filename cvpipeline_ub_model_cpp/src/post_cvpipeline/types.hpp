#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TYPES_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TYPES_HPP

#include "../semantic_ir/generic_ir.hpp"

#include <string>
#include <vector>

namespace cvub {

enum class Precision { Exact, Incomplete };
enum class CoverageDisposition { Modeled, UBInvariant, Unsupported };

struct PostCVPipelineOptions {
  unsigned tileMixVectorLoop = 2;
  unsigned tileMixCubeLoop = 2;
  bool enableAutoBindSubBlock = true;
  bool enableCodeMotion = true;
  bool enableUbufSaving = false;
};

struct PostCVPipelineDiagnostic {
  std::string pipelineStage;
  std::string function;
  int operationId = -1;
  std::string operation;
  std::string reason;
};

struct StageCoverage {
  std::string stage;
  CoverageDisposition disposition = CoverageDisposition::Unsupported;
};

struct ProjectedAIVModule {
  std::string sourceFunction;
  std::string projectedFunction;
  GenericModule module;
};

struct StageResult {
  GenericModule module;
  Precision precision = Precision::Exact;
  std::vector<PostCVPipelineDiagnostic> diagnostics;
};

struct PostCVPipelineResult {
  Precision precision = Precision::Exact;
  std::vector<ProjectedAIVModule> functions;
  std::vector<StageCoverage> coverage;
  std::vector<PostCVPipelineDiagnostic> diagnostics;
};

} // namespace cvub

#endif
