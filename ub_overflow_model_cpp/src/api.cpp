#include "../include/ub_overflow_model/api.hpp"

#include "pipeline/cvpipelining_ub_pipeline.hpp"
#include "pipeline/pre_cv_prefix_pipeline.hpp"

#ifdef CVUB_ENABLE_MLIR_API
#include "ir/mlir_module_view.hpp"
#endif

#include <chrono>
#include <iomanip>
#include <new>
#include <sstream>
#include <stdexcept>

#ifndef CVUB_MODEL_BUILD_ID
#define CVUB_MODEL_BUILD_ID "cvub-api-v5-before-autoblockify"
#endif

namespace cvub {
namespace {

using Clock = std::chrono::steady_clock;

uint64_t Nanoseconds(Clock::duration duration) {
  const auto value =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  return value <= 0 ? 0 : static_cast<uint64_t>(value);
}

bool StartsWith(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() &&
         value.substr(0, prefix.size()) == prefix;
}

std::string StableDigest(std::string_view value) {
  // FNV-1a is deliberately used as a stable equality/cache-key digest, not as
  // a security primitive.  The input schema version is part of the payload.
  uint64_t hash = 14695981039346656037ULL;
  for (const char raw : value) {
    const unsigned char byte = static_cast<unsigned char>(raw);
    hash ^= static_cast<uint64_t>(byte);
    hash *= 1099511628211ULL;
  }
  std::ostringstream output;
  output << std::hex << std::setfill('0') << std::setw(16) << hash;
  return output.str();
}

std::string EffectiveOptionsDigest(const Request &request,
                                   const DebugModelControls &debug) {
  const UBRelevantCompileOptions &options = request.options;
  std::ostringstream value;
  value << "ub-relevant-options-v" << request.optionsVersion << '\n'
        << static_cast<unsigned>(request.compilerProfile) << '\n'
        << request.compilerPipelineFingerprint << '\n' << request.target
        << '\n' << options.enableTritonKernelCompile
        << '\n' << options.enableAutoBlockifyLoop
        << '\n' << options.limitAutoMultiBufferOnlyForLocalBuffer
        << '\n' << options.workspaceMultiBufferNum
        << '\n' << options.disableAutoCVWorkSpaceManage << '\n'
        << options.cvPipelineDepth << '\n'
        << options.enableCVLazyLoading << '\n' << options.enableCodeMotion
        << '\n' << options.enablePreload << '\n'
        << options.enableAutoBindSubBlock << '\n'
        << options.enableUbufSaving << '\n' << options.enableAutoMultiBuffer
        << '\n' << options.enableHIVMAutoStorageAlign << '\n'
        << options.enableHIVMCrossCoreGSS << '\n'
        << options.enableHIVMInjectBlockAllSync << '\n'
        << options.disableAutoInjectBlockSync << '\n'
        << options.tileMixVectorLoop << '\n' << options.tileMixCubeLoop << '\n'
        << static_cast<unsigned>(options.localMultiBufferStrategy) << '\n'
        << static_cast<unsigned>(options.mixMultiBufferStrategy) << '\n'
        << debug.disableCVPipelining << '\n'
        << debug.disableAlignAllocSize << '\n'
        << debug.disableEnableStrideAlign << '\n'
        << debug.disableInferHIVMDataLayout << '\n';
  if (debug.fixedPlanMemorySeed)
    value << *debug.fixedPlanMemorySeed;
  else
    value << "retry";
  value << '\n';
  if (debug.capacityOverrideBits)
    value << *debug.capacityOverrideBits;
  else
    value << "target";
  value << '\n' << debug.restrictInplaceAsISA << '\n';
  return StableDigest(value.str());
}

std::optional<uint64_t> CapacityForTarget(std::string_view target) {
  if (StartsWith(target, "Ascend910B") ||
      StartsWith(target, "Ascend910_93"))
    return kDefaultUBCapacityBits;
  return std::nullopt;
}

std::optional<Diagnostic>
ValidateRequest(const Request &request, const DebugModelControls &debug,
                bool hasInput) {
  if (request.apiVersion != kInProcessAPIVersion)
    return Diagnostic{"unsupported_api_version",
                      "unsupported UB model in-process API version"};
  if (request.optionsVersion != kUBRelevantCompileOptionsVersion)
    return Diagnostic{"unsupported_options_version",
                      "unsupported UB-relevant compile options version"};
  const bool legacyBeforeCV =
      request.inputContractVersion == kBeforeCVPipeliningInputContractVersion;
  const bool beforeAutoBlockify =
      request.inputContractVersion == kBeforeAutoBlockifyInputContractVersion;
  if (!legacyBeforeCV && !beforeAutoBlockify)
    return Diagnostic{"unsupported_input_contract_version",
                      "unsupported UB model input contract"};
  if (request.compilerProfile != CompilerProfile::TritonMembaseA2A3)
    return Diagnostic{"unsupported_compiler_profile",
                      "unsupported compiler profile"};
  const std::string_view expectedFingerprint =
      beforeAutoBlockify ? kA3MembaseBeforeAutoBlockifyFingerprint
                         : kA3MembasePipelineFingerprint;
  if (request.compilerPipelineFingerprint != expectedFingerprint)
    return Diagnostic{"unsupported_pipeline_fingerprint",
                      "compiler pipeline has not been certified"};
  if (!hasInput)
    return Diagnostic{"invalid_request", "before-CVPipelining IR is empty"};
  if (request.target.empty())
    return Diagnostic{"invalid_effective_options", "target is empty"};
  if (!CapacityForTarget(request.target))
    return Diagnostic{"unsupported_target",
                      "target capacity/profile is not modeled exactly"};
  if (request.options.tileMixVectorLoop == 0 ||
      request.options.tileMixCubeLoop == 0)
    return Diagnostic{"invalid_effective_options",
                      "tile loop factors must be positive"};
  if (debug.fixedPlanMemorySeed && *debug.fixedPlanMemorySeed >= 20)
    return Diagnostic{"invalid_effective_options",
                      "PlanMemory seed must be in [0, 19]"};
  if (debug.capacityOverrideBits && *debug.capacityOverrideBits == 0)
    return Diagnostic{"invalid_effective_options",
                      "capacity override must be positive"};
  return std::nullopt;
}

CVPipeliningUBPipelineOptions
PipelineOptions(const Request &request, const DebugModelControls &debug,
                DebugTrace *trace) {
  const UBRelevantCompileOptions &options = request.options;
  CVPipeliningUBPipelineOptions result;
  result.cvPipelining.disabled = debug.disableCVPipelining;
  result.cvPipelining.setDepthInUnrollMode = options.cvPipelineDepth;
  result.cvPipelining.enableSkewMode = options.enablePreload;
  result.cvPipelining.enableLazyLoading = options.enableCVLazyLoading;

  UBAffectingPassOptions &passes = result.ubAffectingPasses;
  passes.disableAutoCVWorkSpaceManage =
      options.disableAutoCVWorkSpaceManage;
  passes.enableCodeMotion = options.enableCodeMotion;
  passes.enableAutoBindSubBlock = options.enableAutoBindSubBlock;
  passes.enableUbufSaving = options.enableUbufSaving;
  passes.enableAutoMultiBuffer = options.enableAutoMultiBuffer;
  passes.enableHIVMAutoStorageAlign = options.enableHIVMAutoStorageAlign;
  passes.enableHIVMCrossCoreGSS = options.enableHIVMCrossCoreGSS;
  passes.enableHIVMInjectBlockAllSync =
      options.enableHIVMInjectBlockAllSync;
  passes.disableAutoInjectBlockSync = options.disableAutoInjectBlockSync;
  passes.tileMixVectorLoop = options.tileMixVectorLoop;
  passes.tileMixCubeLoop = options.tileMixCubeLoop;
  passes.limitAutoMultiBufferOfLocalBuffer =
      options.localMultiBufferStrategy;
  passes.limitMixAutoMultiBufferBuffer = options.mixMultiBufferStrategy;
  passes.enableTritonKernelCompile = options.enableTritonKernelCompile;
  passes.disableAlignAllocSize = debug.disableAlignAllocSize;
  passes.disableEnableStrideAlign = debug.disableEnableStrideAlign;
  passes.disableInferHIVMDataLayout = debug.disableInferHIVMDataLayout;

  result.planMemorySeed = debug.fixedPlanMemorySeed;
  result.restrictInplaceAsISA = debug.restrictInplaceAsISA;
  result.capacityBits = debug.capacityOverrideBits.value_or(
      *CapacityForTarget(request.target));
  result.debugTrace = trace;
  return result;
}

PreCVPrefixPipelineOptions PreCVPrefixOptions(const Request &request) {
  const UBRelevantCompileOptions &options = request.options;
  PreCVPrefixPipelineOptions result;
  result.enableTritonKernelCompile = options.enableTritonKernelCompile;
  result.enableAutoBlockifyLoop = options.enableAutoBlockifyLoop;
  result.disableAutoCVWorkSpaceManage =
      options.disableAutoCVWorkSpaceManage;
  result.enableAutoMultiBuffer = options.enableAutoMultiBuffer;
  result.limitAutoMultiBufferOnlyForLocalBuffer =
      options.limitAutoMultiBufferOnlyForLocalBuffer;
  result.localMultiBufferStrategy = options.localMultiBufferStrategy;
  result.mixMultiBufferStrategy = options.mixMultiBufferStrategy;
  result.workspaceMultiBufferNum = options.workspaceMultiBufferNum;
  return result;
}

FunctionResult ConvertFunction(const cvub::FunctionPlanResult &source) {
  FunctionResult result;
  result.function = source.function;
  result.overflow = source.plan.overflow;
  result.ubPeakBits = source.plan.peakBits;
  result.requiredBits = source.plan.requiredBits;
  result.selectedSeed = source.plan.selectedSeed;
  result.inplacePairs = source.plan.inplacePairs;
  result.buffers.reserve(source.plan.buffers.size());
  for (const PlannedBufferRecord &buffer : source.plan.buffers) {
    BufferResult converted;
    converted.name = buffer.name;
    converted.extentBits = buffer.extentBits;
    const auto multi = source.plan.multiBufferNums.find(buffer.name);
    converted.multiBufferNum =
        multi == source.plan.multiBufferNums.end() ? 1 : multi->second;
    converted.offsetsBytes = buffer.offsetsBytes;
    converted.allocTime = buffer.allocTime;
    converted.freeTime = buffer.freeTime;
    result.buffers.push_back(std::move(converted));
  }
  return result;
}

void CopyTimings(const DebugTrace &trace, Result &result) {
  result.stageTimings.reserve(trace.RuntimeTimings().size());
  for (const DebugTrace::RuntimeTimingRecord &record :
       trace.RuntimeTimings()) {
    result.stageTimings.push_back(
        {record.name, record.nanoseconds,
         static_cast<uint32_t>(record.occurrence)});
  }
}

void SetFailure(Result &result, Status status, std::string category,
                std::string message) {
  result.precision = Precision::Incomplete;
  result.status = status;
  result.overflow.reset();
  result.ubPeakBits.reset();
  result.requiredBits.reset();
  result.selectedSeed.reset();
  result.diagnostics.push_back(
      {std::move(category), std::move(message)});
}

} // namespace

const char *toString(Precision precision) noexcept {
  return precision == Precision::Exact ? "exact" : "incomplete";
}

const char *toString(Status status) noexcept {
  switch (status) {
  case Status::Success:
    return "success";
  case Status::Overflow:
    return "overflow";
  case Status::Blocker:
    return "blocker";
  case Status::InternalError:
    return "internal_error";
  }
  return "internal_error";
}

const char *toString(MultiBufferStrategy strategy) noexcept {
  switch (strategy) {
  case MultiBufferStrategy::NoLimit:
    return "no-limit";
  case MultiBufferStrategy::OnlyCube:
    return "only-cube";
  case MultiBufferStrategy::OnlyVector:
    return "only-vector";
  case MultiBufferStrategy::CubeNoL0C:
    return "no-l0c";
  }
  return "no-limit";
}

const char *toString(CompilerProfile profile) noexcept {
  switch (profile) {
  case CompilerProfile::TritonMembaseA2A3:
    return "triton-membase-a2-a3";
  }
  return "unknown";
}

struct PreparedInput {
  GenericModule module;
  std::string digest;
};

template <typename PrepareInput>
Result EvaluateImpl(const Request &request, const DebugModelControls &debug,
                    bool enableDecisionOnlyNonOverflow,
                    bool observeConservativeNonOverflow, bool hasInput,
                    PrepareInput &&prepareInput) noexcept {
  const Clock::time_point started = Clock::now();
  Result result;
  try {
    result.requestId = std::string(request.requestId);
    result.modelBuildId = CVUB_MODEL_BUILD_ID;
    result.compilerPipelineFingerprint =
        std::string(request.compilerPipelineFingerprint);
    result.effectiveOptionsDigest = EffectiveOptionsDigest(request, debug);
    if (const std::optional<uint64_t> capacity =
            debug.capacityOverrideBits
                ? debug.capacityOverrideBits
                : CapacityForTarget(request.target))
      result.capacityBits = *capacity;
    if (const std::optional<Diagnostic> invalid =
            ValidateRequest(request, debug, hasInput)) {
      SetFailure(result, Status::Blocker, invalid->category, invalid->message);
      result.totalTimeNs = Nanoseconds(Clock::now() - started);
      return result;
    }

    std::ostringstream timingOutput;
    std::optional<DebugTrace> trace;
    if (debug.collectStageTimings)
      trace.emplace(timingOutput, std::filesystem::path{}, false, true, false);
    DebugTrace *tracePointer = trace ? &*trace : nullptr;
    PreparedInput prepared = prepareInput(tracePointer);
    result.inputDigest = std::move(prepared.digest);
    if (request.inputContractVersion ==
        kBeforeAutoBlockifyInputContractVersion) {
      prepared.module = RunPreCVPrefixPipeline(
          std::move(prepared.module), PreCVPrefixOptions(request),
          tracePointer);
    }
    CVPipeliningUBPipelineOptions pipelineOptions =
        PipelineOptions(request, debug, tracePointer);
    pipelineOptions.enableDecisionOnlyNonOverflow =
        enableDecisionOnlyNonOverflow;
    pipelineOptions.observeConservativeNonOverflow =
        observeConservativeNonOverflow;
    const ModulePlanResult plan = RunCVPipeliningUBModulePipeline(
        std::move(prepared.module), pipelineOptions);
    if (plan.precision != ModulePlanPrecision::Exact) {
      SetFailure(result, Status::Blocker, "model_blocker",
                 "the UB model could not produce an exact result");
      for (const std::string &diagnostic : plan.diagnostics)
        result.diagnostics.push_back({"model_blocker", diagnostic});
    } else {
      result.precision = Precision::Exact;
      result.status = plan.overflow ? Status::Overflow : Status::Success;
      result.overflow = plan.overflow;
      result.capacityBits = plan.capacityBits;
      result.decisionOnlyNonOverflow = plan.decisionOnlyNonOverflow;
      result.conservativeUpperBoundBits =
          plan.conservativeUpperBoundBits;
      if (!plan.decisionOnlyNonOverflow) {
        result.ubPeakBits = plan.peakBits;
        result.requiredBits = plan.requiredBits;
        result.functions.reserve(plan.functions.size());
        for (const cvub::FunctionPlanResult &function : plan.functions)
          result.functions.push_back(ConvertFunction(function));
      }
      if (!plan.decisionOnlyNonOverflow && !result.functions.empty()) {
        const uint32_t selected = result.functions.front().selectedSeed;
        bool sameSeed = true;
        for (const FunctionResult &function : result.functions)
          sameSeed = sameSeed && function.selectedSeed == selected;
        if (sameSeed)
          result.selectedSeed = selected;
      }
      if (plan.overflow) {
        result.diagnostics.push_back(
            {"predicted_ub_overflow",
             "exact UB requirement exceeds target capacity"});
      }
    }
    if (trace)
      CopyTimings(*trace, result);
  } catch (const std::runtime_error &error) {
    try {
      SetFailure(result, Status::Blocker, "model_blocker", error.what());
    } catch (...) {
      result = Result{};
      result.status = Status::InternalError;
    }
  } catch (const std::bad_alloc &) {
    result = Result{};
    result.status = Status::InternalError;
  } catch (const std::exception &error) {
    try {
      SetFailure(result, Status::InternalError, "model_internal_error",
                 error.what());
    } catch (...) {
      result = Result{};
      result.status = Status::InternalError;
    }
  } catch (...) {
    result = Result{};
    result.status = Status::InternalError;
  }
  result.totalTimeNs = Nanoseconds(Clock::now() - started);
  return result;
}

Result evaluate(const Request &request) noexcept {
  const std::string_view input =
      request.inputContractVersion == kBeforeAutoBlockifyInputContractVersion
          ? request.beforeAutoBlockifyGenericMLIR
          : request.beforeCVPipeliningGenericMLIR;
  const std::string_view boundary =
      request.inputContractVersion == kBeforeAutoBlockifyInputContractVersion
          ? "before-autoblockify-v"
          : "before-cvpipelining-v";
  return EvaluateImpl(
      request, DebugModelControls{}, true, false,
      !input.empty(),
      [&](DebugTrace *trace) {
        PreparedInput input;
        input.digest = StableDigest(
            std::string(boundary) +
            std::to_string(request.inputContractVersion) + "\n" +
            std::string(request.inputContractVersion ==
                                kBeforeAutoBlockifyInputContractVersion
                            ? request.beforeAutoBlockifyGenericMLIR
                            : request.beforeCVPipeliningGenericMLIR));
        input.module = MeasureStage(trace, "ParseGenericIR", [&] {
          return ParseGenericIRText(
              request.inputContractVersion ==
                      kBeforeAutoBlockifyInputContractVersion
                  ? request.beforeAutoBlockifyGenericMLIR
                  : request.beforeCVPipeliningGenericMLIR,
              false);
        });
        return input;
      });
}

Result evaluateForDebug(const Request &request,
                        const DebugModelControls &controls) noexcept {
  const std::string_view input =
      request.inputContractVersion == kBeforeAutoBlockifyInputContractVersion
          ? request.beforeAutoBlockifyGenericMLIR
          : request.beforeCVPipeliningGenericMLIR;
  const std::string_view boundary =
      request.inputContractVersion == kBeforeAutoBlockifyInputContractVersion
          ? "before-autoblockify-v"
          : "before-cvpipelining-v";
  return EvaluateImpl(
      request, controls, false, true,
      !input.empty(),
      [&](DebugTrace *trace) {
        PreparedInput input;
        input.digest = StableDigest(
            std::string(boundary) +
            std::to_string(request.inputContractVersion) + "\n" +
            std::string(request.inputContractVersion ==
                                kBeforeAutoBlockifyInputContractVersion
                            ? request.beforeAutoBlockifyGenericMLIR
                            : request.beforeCVPipeliningGenericMLIR));
        input.module = MeasureStage(trace, "ParseGenericIR", [&] {
          return ParseGenericIRText(
              request.inputContractVersion ==
                      kBeforeAutoBlockifyInputContractVersion
                  ? request.beforeAutoBlockifyGenericMLIR
                  : request.beforeCVPipeliningGenericMLIR,
              false);
        });
        return input;
      });
}

#ifdef CVUB_ENABLE_MLIR_API
Result evaluateModule(mlir::ModuleOp module,
                      const Request &request) noexcept {
  return EvaluateImpl(
      request, DebugModelControls{}, true, false, static_cast<bool>(module),
      [&](DebugTrace *trace) {
        return MeasureStage(trace, "ImportMLIRModule", [&] {
          MLIRModuleView view(module);
          MLIRModuleView::MaterializedModule imported =
              view.materializeLegacyGenericModule();
          return PreparedInput{std::move(imported.module),
                               std::move(imported.structuralDigest)};
        });
      });
}

Result evaluateModuleForDebug(
    mlir::ModuleOp module, const Request &request,
    const DebugModelControls &controls) noexcept {
  return EvaluateImpl(
      request, controls, false, true, static_cast<bool>(module),
      [&](DebugTrace *trace) {
        return MeasureStage(trace, "ImportMLIRModule", [&] {
          MLIRModuleView view(module);
          MLIRModuleView::MaterializedModule imported =
              view.materializeLegacyGenericModule();
          return PreparedInput{std::move(imported.module),
                               std::move(imported.structuralDigest)};
        });
      });
}
#endif

} // namespace cvub
