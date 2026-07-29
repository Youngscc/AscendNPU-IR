#include "UBOverflowPrediction.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>

namespace mlir {
namespace hivm {
namespace {

bool isEmbeddedValidationEnabled() {
  const char *value = std::getenv("BISHENGIR_UB_MODEL_VALIDATION");
  return value != nullptr && value[0] != '\0' && StringRef(value) != "0";
}

bool isUBFlowTraceEnabled() {
  const char *value = std::getenv("BISHENGIR_UB_FLOW_TRACE");
  return value != nullptr && value[0] != '\0' && StringRef(value) != "0";
}

bool isTextEntryComparisonEnabled() {
  const char *value = std::getenv("BISHENGIR_UB_MODEL_COMPARE_TEXT_ENTRY");
  return value != nullptr && value[0] != '\0' && StringRef(value) != "0";
}

bool shouldEmitMachineResult(bool validationEnabled) {
  if (validationEnabled)
    return true;
  const char *value = std::getenv("BISHENGIR_UB_MODEL_EMIT_RESULT");
  return value != nullptr && value[0] != '\0' && StringRef(value) != "0";
}

std::string sourceFileForTrace(ModuleOp module) {
  std::string source = "unknown";
  module.walk([&](Operation *operation) {
    FileLineColLoc fileLoc =
        operation->getLoc()->findInstanceOf<FileLineColLoc>();
    if (!fileLoc)
      return WalkResult::advance();
    source = fileLoc.getFilename().str();
    return WalkResult::interrupt();
  });
  return source;
}

void printBoolean(llvm::raw_ostream &output, bool value) {
  output << (value ? "true" : "false");
}

bool sameBuffer(const cvub::BufferResult &lhs,
                const cvub::BufferResult &rhs) {
  return lhs.name == rhs.name && lhs.extentBits == rhs.extentBits &&
         lhs.multiBufferNum == rhs.multiBufferNum &&
         lhs.offsetsBytes == rhs.offsetsBytes &&
         lhs.allocTime == rhs.allocTime && lhs.freeTime == rhs.freeTime;
}

bool sameFunction(const cvub::FunctionResult &lhs,
                  const cvub::FunctionResult &rhs) {
  if (lhs.function != rhs.function || lhs.overflow != rhs.overflow ||
      lhs.ubPeakBits != rhs.ubPeakBits ||
      lhs.requiredBits != rhs.requiredBits ||
      lhs.selectedSeed != rhs.selectedSeed ||
      lhs.inplacePairs != rhs.inplacePairs ||
      lhs.buffers.size() != rhs.buffers.size())
    return false;
  for (size_t index = 0; index < lhs.buffers.size(); ++index)
    if (!sameBuffer(lhs.buffers[index], rhs.buffers[index]))
      return false;
  return true;
}

bool samePlanContract(const cvub::Result &lhs, const cvub::Result &rhs) {
  if (lhs.precision != rhs.precision || lhs.status != rhs.status ||
      lhs.overflow != rhs.overflow || lhs.ubPeakBits != rhs.ubPeakBits ||
      lhs.requiredBits != rhs.requiredBits ||
      lhs.capacityBits != rhs.capacityBits ||
      lhs.selectedSeed != rhs.selectedSeed ||
      lhs.decisionOnlyNonOverflow != rhs.decisionOnlyNonOverflow ||
      lhs.conservativeUpperBoundBits != rhs.conservativeUpperBoundBits ||
      lhs.functions.size() != rhs.functions.size())
    return false;
  for (size_t index = 0; index < lhs.functions.size(); ++index)
    if (!sameFunction(lhs.functions[index], rhs.functions[index]))
      return false;
  return true;
}

std::optional<uint32_t> validationSeed() {
  const char *value = std::getenv("BISHENGIR_PLAN_MEMORY_FORCE_SEED");
  if (value == nullptr || value[0] == '\0')
    return std::nullopt;
  char *end = nullptr;
  unsigned long seed = std::strtoul(value, &end, 10);
  if (end == value || *end != '\0' || seed >= 20)
    return std::nullopt;
  return static_cast<uint32_t>(seed);
}

uint64_t nextValidationId() {
  static std::atomic<uint64_t> next{0};
  return next.fetch_add(1, std::memory_order_relaxed);
}

void dumpValidationResult(uint64_t validationId, const cvub::Result &result) {
  llvm::errs() << "BISHENGIR_UB_MODEL_VALIDATION_BEGIN\t" << validationId
               << '\n';
  for (const cvub::Diagnostic &diagnostic : result.diagnostics) {
    std::string message = diagnostic.message;
    for (char &character : message) {
      if (character == '\t' || character == '\n' || character == '\r')
        character = ' ';
    }
    llvm::errs() << "BISHENGIR_UB_MODEL_DIAGNOSTIC\t" << validationId
                 << '\t' << diagnostic.category << '\t' << message << '\n';
  }
  for (const cvub::FunctionResult &function : result.functions) {
    llvm::errs() << "BISHENGIR_UB_MODEL_FUNCTION\t" << validationId << '\t'
                 << function.function << '\t'
                 << (function.overflow ? "overflow" : "success") << '\t'
                 << function.ubPeakBits << '\t' << function.requiredBits
                 << '\t' << function.selectedSeed << '\n';
    for (const cvub::BufferResult &buffer : function.buffers) {
      llvm::errs() << "BISHENGIR_UB_MODEL_BUFFER\t" << validationId << '\t'
                   << function.function << '\t' << buffer.name << '\t'
                   << buffer.extentBits << '\t' << buffer.multiBufferNum
                   << '\t' << buffer.allocTime << '\t' << buffer.freeTime;
      for (uint64_t offset : buffer.offsetsBytes)
        llvm::errs() << '\t' << offset;
      llvm::errs() << '\n';
    }
    for (const auto &pair : function.inplacePairs)
      llvm::errs() << "BISHENGIR_UB_MODEL_INPLACE\t" << validationId << '\t'
                   << function.function << '\t' << pair.first << '\t'
                   << pair.second << '\n';
  }
  llvm::errs() << "BISHENGIR_UB_MODEL_VALIDATION_END\t" << validationId
               << '\t' << result.functions.size() << '\n';
}

void emitMachineResult(const cvub::Result &result, uint64_t serializeNs,
                       std::optional<uint64_t> validationId) {
  llvm::errs()
      << "BISHENGIR_UB_MODEL_RESULT contract_version="
      << cvub::kSubprocessResultContractVersion
      << " status=" << cvub::toString(result.status)
      << " precision=" << cvub::toString(result.precision)
      << " overflow="
      << (result.overflow ? (*result.overflow ? "true" : "false")
                          : "unknown")
      << " ub_peak_bits="
      << (result.ubPeakBits ? std::to_string(*result.ubPeakBits) : "unknown")
      << " required_bits="
      << (result.requiredBits ? std::to_string(*result.requiredBits)
                              : "unknown")
      << " capacity_bits=" << result.capacityBits
      << " selected_seed="
      << (result.selectedSeed ? std::to_string(*result.selectedSeed)
                              : "unknown")
      << " decision_path="
      << (result.decisionOnlyNonOverflow
              ? "non_overflow_upper_bound"
              : (result.conservativeUpperBoundBits
                     ? "full_plan_after_non_overflow_upper_bound"
                     : "full_plan"))
      << " non_overflow_upper_bound_proven="
      << (result.conservativeUpperBoundBits ? "true" : "false")
      << " conservative_upper_bound_bits="
      << (result.conservativeUpperBoundBits
              ? std::to_string(*result.conservativeUpperBoundBits)
              : "unknown")
      << " serialize_ns=" << serializeNs
      << " model_ns=" << result.totalTimeNs
      << " input_digest=" << result.inputDigest
      << " options_digest=" << result.effectiveOptionsDigest
      << " pipeline_fingerprint=" << result.compilerPipelineFingerprint
      << " diagnostic_category="
      << (result.diagnostics.empty() ? "none"
                                     : result.diagnostics.front().category);
  if (validationId)
    llvm::errs() << " validation_id=" << *validationId;
  llvm::errs() << '\n';
}

void emitFlowTrace(ModuleOp module, const cvub::Request &request,
                   const cvub::Result &result, uint64_t traceAttempt) {
  size_t operationCount = 0;
  module.walk([&](Operation *) { ++operationCount; });
  llvm::errs() << "[UB-FLOW][ATTEMPT " << traceAttempt
               << "][LIGHTWEIGHT_MODEL][RESULT]"
               << " input=" << sourceFileForTrace(module)
      << " input_operations=" << operationCount
      << " status=" << cvub::toString(result.status)
      << " precision=" << cvub::toString(result.precision)
      << " overflow="
      << (result.overflow ? (*result.overflow ? "true" : "false")
                          : "unknown")
      << '\n';
  llvm::errs() << "[UB-FLOW][ATTEMPT " << traceAttempt
               << "][LIGHTWEIGHT_MODEL][OPTIONS]"
               << " target=" << request.target
               << " cv_pipeline_depth=" << request.options.cvPipelineDepth;
  llvm::errs() << " enable_cv_lazy_loading=";
  printBoolean(llvm::errs(), request.options.enableCVLazyLoading);
  llvm::errs() << " enable_preload=";
  printBoolean(llvm::errs(), request.options.enablePreload);
  llvm::errs() << " enable_code_motion=";
  printBoolean(llvm::errs(), request.options.enableCodeMotion);
  llvm::errs() << " enable_auto_bind_sub_block=";
  printBoolean(llvm::errs(), request.options.enableAutoBindSubBlock);
  llvm::errs() << " enable_ubuf_saving=";
  printBoolean(llvm::errs(), request.options.enableUbufSaving);
  llvm::errs() << " enable_auto_multi_buffer=";
  printBoolean(llvm::errs(), request.options.enableAutoMultiBuffer);
  llvm::errs() << " enable_triton_kernel_compile=";
  printBoolean(llvm::errs(), request.options.enableTritonKernelCompile);
  llvm::errs() << " enable_auto_blockify_loop=";
  printBoolean(llvm::errs(), request.options.enableAutoBlockifyLoop);
  llvm::errs() << " limit_auto_multi_buffer_only_for_local_buffer=";
  printBoolean(llvm::errs(),
               request.options.limitAutoMultiBufferOnlyForLocalBuffer);
  llvm::errs() << " workspace_multi_buffer_num="
               << request.options.workspaceMultiBufferNum;
  llvm::errs() << " enable_hivm_auto_storage_align=";
  printBoolean(llvm::errs(), request.options.enableHIVMAutoStorageAlign);
  llvm::errs() << " tile_mix_vector_loop="
               << request.options.tileMixVectorLoop
               << " tile_mix_cube_loop=" << request.options.tileMixCubeLoop
               << " local_multi_buffer_strategy="
               << cvub::toString(request.options.localMultiBufferStrategy)
               << " mix_multi_buffer_strategy="
               << cvub::toString(request.options.mixMultiBufferStrategy);
  llvm::errs() << " disable_auto_cv_workspace_manage=";
  printBoolean(llvm::errs(), request.options.disableAutoCVWorkSpaceManage);
  llvm::errs() << " enable_hivm_cross_core_gss=";
  printBoolean(llvm::errs(), request.options.enableHIVMCrossCoreGSS);
  llvm::errs() << " enable_hivm_inject_block_all_sync=";
  printBoolean(llvm::errs(), request.options.enableHIVMInjectBlockAllSync);
  llvm::errs() << " disable_auto_inject_block_sync=";
  printBoolean(llvm::errs(), request.options.disableAutoInjectBlockSync);
  llvm::errs() << '\n';
}

class UBOverflowPredictionPass
    : public PassWrapper<UBOverflowPredictionPass,
                         OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(UBOverflowPredictionPass)

  explicit UBOverflowPredictionPass(UBOverflowPredictionConfig config)
      : config(std::move(config)) {}

  UBOverflowPredictionPass(const UBOverflowPredictionPass &other)
      : PassWrapper(other), config(other.config) {}

  StringRef getArgument() const override {
    return "hivm-ub-overflow-prediction";
  }

  StringRef getDescription() const override {
    return "Predict local UB usage from before AutoBlockify";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    cvub::Request request;
    request.inputContractVersion =
        cvub::kBeforeAutoBlockifyInputContractVersion;
    request.compilerProfile = cvub::CompilerProfile::TritonMembaseA2A3;
    request.compilerPipelineFingerprint =
        cvub::kA3MembaseBeforeAutoBlockifyFingerprint;
    request.target = config.target;
    request.options = config.modelOptions;

    // Production always uses the narrow evaluate() contract.  The detailed
    // fixed-seed path exists only for same-process differential validation
    // against the real local PlanMemory pass.
    const bool validationEnabled = isEmbeddedValidationEnabled();
    cvub::Result result;
    uint64_t validationId = 0;
    if (validationEnabled) {
      cvub::DebugModelControls controls;
      controls.fixedPlanMemorySeed = validationSeed();
      result = cvub::evaluateModuleForDebug(module, request, controls);
      if (isTextEntryComparisonEnabled()) {
        std::string genericMLIR;
        llvm::raw_string_ostream output(genericMLIR);
        OpPrintingFlags flags;
        flags.printGenericOpForm();
        module.print(output, flags);
        output.flush();
        cvub::Request textRequest = request;
        textRequest.beforeAutoBlockifyGenericMLIR = genericMLIR;
        const cvub::Result textResult =
            cvub::evaluateForDebug(textRequest, controls);
        if (!samePlanContract(result, textResult)) {
          module.emitError()
              << "direct MLIR and compatibility text model entries differ";
          signalPassFailure();
          return;
        }
        llvm::errs() << "BISHENGIR_UB_MODEL_INPUT_PARITY\tmatched\n";
      }
      validationId = nextValidationId();
      dumpValidationResult(validationId, result);
    } else {
      result = cvub::evaluateModule(module, request);
    }
    if (shouldEmitMachineResult(validationEnabled))
      emitMachineResult(result, /*serializeNs=*/0,
                        validationEnabled
                            ? std::optional<uint64_t>(validationId)
                            : std::nullopt);

    const bool traceEnabled = isUBFlowTraceEnabled();
    if (traceEnabled)
      emitFlowTrace(module, request, result, config.traceAttempt);

    if (config.pruneOnOverflow &&
        result.precision == cvub::Precision::Exact &&
        result.status == cvub::Status::Overflow) {
      if (traceEnabled)
        llvm::errs() << "[UB-FLOW][ATTEMPT " << config.traceAttempt
                     << "][LIGHTWEIGHT_MODEL][DECISION]"
                        " exact overflow; stop before real CVPipelining\n";
      module.emitError()
          << "predicted_ub_overflow: ub overflow, requires "
          << result.requiredBits.value_or(0) << " bits while "
          << result.capacityBits << " bits available"
          << "; ub_peak_bits=" << result.ubPeakBits.value_or(0)
          << " selected_seed=" << result.selectedSeed.value_or(0);
      signalPassFailure();
      return;
    }

    markAllAnalysesPreserved();
  }

private:
  UBOverflowPredictionConfig config;
};

} // namespace

std::unique_ptr<Pass>
createUBOverflowPredictionPass(UBOverflowPredictionConfig config) {
  return std::make_unique<UBOverflowPredictionPass>(std::move(config));
}

} // namespace hivm
} // namespace mlir
