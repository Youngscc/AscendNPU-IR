#include "ub_overflow_model/api.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

std::string ReadFile(const char *path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error(std::string("cannot open API fixture: ") + path);
  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

} // namespace

int main() {
  static_assert(noexcept(cvub::evaluate(std::declval<const cvub::Request &>())),
                "the in-process boundary must be noexcept");

  const std::string ir = ReadFile(
      "ub_overflow_model_cpp/data/before_cvpipelining/"
      "ascend_tutorial_01-vector-add.ttadapter/"
      "before_cvpipelining_func_func_add_kernel_32.mlir");
  const std::string original = ir;
  cvub::Request request;
  request.beforeCVPipeliningGenericMLIR = ir;
  request.requestId = "api-test-candidate";
  const cvub::Result productionResult = cvub::evaluate(request);

  Check(productionResult.precision == cvub::Precision::Exact &&
            productionResult.status == cvub::Status::Success &&
            productionResult.overflow && !*productionResult.overflow,
        "the production API must prove vector-add non-overflow exactly");
  Check(productionResult.decisionOnlyNonOverflow &&
            productionResult.conservativeUpperBoundBits &&
            *productionResult.conservativeUpperBoundBits <=
                productionResult.capacityBits,
        "the production API must expose its conservative fast-path proof");
  Check(!productionResult.ubPeakBits && !productionResult.requiredBits &&
            !productionResult.selectedSeed &&
            productionResult.functions.empty(),
        "a decision-only proof must not fabricate a concrete plan");

  const cvub::Result result =
      cvub::evaluateForDebug(request, cvub::DebugModelControls{});

  Check(ir == original, "evaluate must not mutate the caller's IR text");
  Check(result.precision == cvub::Precision::Exact,
        "supported before-CVPipelining IR must be exact");
  Check(result.status == cvub::Status::Success ||
            result.status == cvub::Status::Overflow,
        "an exact API result must have a terminal status");
  Check(result.overflow.has_value() && result.ubPeakBits.has_value() &&
            result.requiredBits.has_value(),
        "an exact result must carry structured UB values");
  Check(*result.ubPeakBits == 65536 && *result.requiredBits == 65536,
        "the API option adapter must preserve the direct model result");
  Check(result.capacityBits == cvub::kDefaultUBCapacityBits,
        "effective target capacity must be returned");
  Check(result.selectedSeed.has_value(),
        "default PlanMemory retry must select a seed");
  Check(!result.functions.empty(),
        "the development result must retain function details");
  Check(!result.modelBuildId.empty() && !result.inputDigest.empty() &&
            !result.effectiveOptionsDigest.empty(),
        "cache identity fields must be present");
  Check(result.compilerPipelineFingerprint ==
            cvub::kA3MembasePipelineFingerprint,
        "the compiler pipeline fingerprint must round-trip");
  Check(result.stageTimings.empty() && result.totalTimeNs > 0,
        "the default debug API must avoid detailed timing overhead");
  Check(result.requestId == "api-test-candidate",
        "request ID must round-trip as owned result data");
  Check(!result.decisionOnlyNonOverflow &&
            result.conservativeUpperBoundBits &&
            *result.conservativeUpperBoundBits <= result.capacityBits,
        "the differential API must observe the proof but materialize the "
        "full plan");

  cvub::Request beforeAuto = request;
  beforeAuto.inputContractVersion =
      cvub::kBeforeAutoBlockifyInputContractVersion;
  beforeAuto.compilerPipelineFingerprint =
      cvub::kA3MembaseBeforeAutoBlockifyFingerprint;
  beforeAuto.beforeCVPipeliningGenericMLIR = {};
  beforeAuto.beforeAutoBlockifyGenericMLIR = ir;
  beforeAuto.options.enableAutoBlockifyLoop = false;
  beforeAuto.options.disableAutoCVWorkSpaceManage = true;
  const cvub::Result beforeAutoResult =
      cvub::evaluateForDebug(beforeAuto, cvub::DebugModelControls{});
  Check(beforeAutoResult.precision == cvub::Precision::Exact &&
            beforeAutoResult.compilerPipelineFingerprint ==
                cvub::kA3MembaseBeforeAutoBlockifyFingerprint,
        "the versioned before-AutoBlockify contract must run the combined prefix");

  const std::string cubeOnlyIR = ReadFile(
      "ub_overflow_model_cpp/data/before_cvpipelining/"
      "ascend_tutorial_08-grouped-gemm.ttadapter/"
      "before_cvpipelining_func_func_grouped_matmul_kernel_32.mlir");
  cvub::Request cubeOnlyRequest;
  cubeOnlyRequest.beforeCVPipeliningGenericMLIR = cubeOnlyIR;
  const cvub::Result cubeOnly = cvub::evaluate(cubeOnlyRequest);
  Check(cubeOnly.precision == cvub::Precision::Exact &&
            cubeOnly.status == cvub::Status::Success && cubeOnly.overflow &&
            !*cubeOnly.overflow && cubeOnly.ubPeakBits &&
            *cubeOnly.ubPeakBits == 0 && cubeOnly.requiredBits &&
            *cubeOnly.requiredBits == 0 && cubeOnly.functions.empty() &&
            !cubeOnly.selectedSeed,
        "a cube-only module must be an exact zero-UB result");

  cvub::Request invalid;
  invalid.requestId = "invalid";
  const cvub::Result empty = cvub::evaluate(invalid);
  Check(empty.precision == cvub::Precision::Incomplete &&
            empty.status == cvub::Status::Blocker &&
            !empty.overflow.has_value(),
        "invalid requests must fail open as a blocker");

  cvub::Request changed = request;
  changed.options.tileMixVectorLoop += 1;
  const cvub::Result changedResult = cvub::evaluate(changed);
  Check(changedResult.effectiveOptionsDigest != result.effectiveOptionsDigest,
        "all effective option changes must alter the digest");

  cvub::Request changedPrefix = request;
  changedPrefix.options.enableAutoBlockifyLoop = false;
  const cvub::Result changedPrefixResult = cvub::evaluate(changedPrefix);
  Check(changedPrefixResult.effectiveOptionsDigest !=
            result.effectiveOptionsDigest,
        "pre-CV prefix option changes must alter the digest");

  cvub::Request inject = request;
  inject.options.enableHIVMCrossCoreGSS = false;
  const cvub::Result injectResult = cvub::evaluate(inject);
  Check(injectResult.effectiveOptionsDigest != result.effectiveOptionsDigest,
        "cross-core branch selection must alter the digest");
  inject.options.enableHIVMInjectBlockAllSync = true;
  const cvub::Result blockAllResult = cvub::evaluate(inject);
  Check(blockAllResult.effectiveOptionsDigest !=
            injectResult.effectiveOptionsDigest,
        "block-all selection must alter the digest");
  inject.options.disableAutoInjectBlockSync = true;
  const cvub::Result disabledInjectResult = cvub::evaluate(inject);
  Check(disabledInjectResult.effectiveOptionsDigest !=
            blockAllResult.effectiveOptionsDigest,
        "disabled auto block-sync selection must alter the digest");

  cvub::Request future = request;
  ++future.optionsVersion;
  const cvub::Result unsupported = cvub::evaluate(future);
  Check(unsupported.status == cvub::Status::Blocker,
        "unknown future option schemas must fail open");

  cvub::Request unknownPipeline = request;
  unknownPipeline.compilerPipelineFingerprint = "unknown-pipeline";
  const cvub::Result unknown = cvub::evaluate(unknownPipeline);
  Check(unknown.status == cvub::Status::Blocker,
        "unknown compiler pipelines must fail open");

  cvub::DebugModelControls debug;
  debug.fixedPlanMemorySeed = 0;
  debug.collectStageTimings = true;
  const cvub::Result fixedSeed = cvub::evaluateForDebug(request, debug);
  Check(fixedSeed.selectedSeed && *fixedSeed.selectedSeed == 0,
        "the oracle API must preserve a fixed PlanMemory seed");
  Check(!fixedSeed.stageTimings.empty(),
        "the debug API must collect stage timings when requested");
  bool foundCVToPlanMemory = false;
  for (const cvub::StageTiming &timing : fixedSeed.stageTimings)
    foundCVToPlanMemory |= timing.stage == "CVToPlanMemoryPipeline";
  Check(foundCVToPlanMemory,
        "stage timing must expose the complete CV-to-PlanMemory boundary");

  cvub::DebugModelControls fullPlanOnly;
  fullPlanOnly.disableConservativeNonOverflowProof = true;
  const cvub::Result withoutProof =
      cvub::evaluateForDebug(request, fullPlanOnly);
  Check(withoutProof.precision == cvub::Precision::Exact &&
            !withoutProof.decisionOnlyNonOverflow &&
            !withoutProof.conservativeUpperBoundBits,
        "structural timing control must run the full plan without the proof");
  Check(withoutProof.effectiveOptionsDigest != result.effectiveOptionsDigest,
        "the full-plan-only debug control must alter the digest");
  const cvub::Result beforeAutoWithoutProof =
      cvub::evaluateForDebug(beforeAuto, fullPlanOnly);
  Check(beforeAutoWithoutProof.precision == cvub::Precision::Exact &&
            !beforeAutoWithoutProof.decisionOnlyNonOverflow &&
            !beforeAutoWithoutProof.conservativeUpperBoundBits,
        "the before-AutoBlockify contract must honor full-plan-only timing");

  debug.capacityOverrideBits = cvub::kDefaultUBCapacityBits + 8;
  const cvub::Result overridden = cvub::evaluateForDebug(request, debug);
  Check(overridden.capacityBits == cvub::kDefaultUBCapacityBits + 8,
        "capacity override must be confined to the oracle API");
  Check(overridden.effectiveOptionsDigest != fixedSeed.effectiveOptionsDigest,
        "debug controls must participate in the effective digest");

  debug.capacityOverrideBits = 1;
  const cvub::Result forcedOverflow = cvub::evaluateForDebug(request, debug);
  Check(forcedOverflow.precision == cvub::Precision::Exact &&
            forcedOverflow.status == cvub::Status::Overflow &&
            forcedOverflow.overflow && *forcedOverflow.overflow,
        "an exact capacity failure must have structured overflow status");
  Check(forcedOverflow.ubPeakBits && forcedOverflow.requiredBits &&
            forcedOverflow.selectedSeed,
        "an exact overflow must retain peak, requirement and selected seed");
  Check(!forcedOverflow.diagnostics.empty() &&
            forcedOverflow.diagnostics.front().category ==
                "predicted_ub_overflow",
        "an exact overflow must expose the stable diagnostic category");

  std::cout << "[PASS] in-process API contract\n";
  return 0;
}
