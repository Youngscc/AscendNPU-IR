#ifndef UB_OVERFLOW_MODEL_CPP_API_HPP
#define UB_OVERFLOW_MODEL_CPP_API_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mlir {
class ModuleOp;
}

namespace cvub {

inline constexpr uint32_t kInProcessAPIVersion = 2;
inline constexpr uint32_t kUBRelevantCompileOptionsVersion = 4;
inline constexpr uint32_t kBeforeCVPipeliningInputContractVersion = 1;
inline constexpr uint32_t kSubprocessResultContractVersion = 1;
inline constexpr uint64_t kDefaultUBCapacityBits = 192ULL * 1024ULL * 8ULL;
inline constexpr std::string_view kA3MembasePipelineFingerprint =
    "bishengir-a3-membase-cvpipelining-v1";

enum class Precision { Exact, Incomplete };
enum class Status { Success, Overflow, Blocker, InternalError };

enum class CompilerProfile {
  TritonMembaseA2A3,
};

enum class MultiBufferStrategy {
  NoLimit,
  OnlyCube,
  OnlyVector,
  CubeNoL0C,
};

// These are the effective values of the UB-affecting compiler branches after
// defaults, aliases and disable_* options have been resolved. Production
// callers must assign every field from the real pipeline options.
struct UBRelevantCompileOptions {
  int cvPipelineDepth = -1;
  bool enableCVLazyLoading = false;
  bool enablePreload = false;
  bool enableCodeMotion = true;
  bool enableAutoBindSubBlock = true;
  bool enableUbufSaving = false;
  bool enableAutoMultiBuffer = false;
  bool enableHIVMAutoStorageAlign = true;
  unsigned tileMixVectorLoop = 2;
  unsigned tileMixCubeLoop = 2;
  MultiBufferStrategy localMultiBufferStrategy =
      MultiBufferStrategy::CubeNoL0C;
  MultiBufferStrategy mixMultiBufferStrategy = MultiBufferStrategy::OnlyCube;
  bool disableAutoCVWorkSpaceManage = false;
  bool enableHIVMCrossCoreGSS = true;
  bool enableHIVMInjectBlockAllSync = false;
  bool disableAutoInjectBlockSync = false;
};

// Development/oracle controls are deliberately separate from the production
// request. The BiShengIR prediction pass must call evaluateModule(), not a
// debug entry.
struct DebugModelControls {
  std::optional<uint32_t> fixedPlanMemorySeed;
  std::optional<uint64_t> capacityOverrideBits;
  // Detailed per-stage timing is an opt-in diagnostic because recording every
  // nested stage is measurable overhead on the production autotune path.
  bool collectStageTimings = false;
  bool restrictInplaceAsISA = false;
  bool disableCVPipelining = false;
  bool disableAlignAllocSize = false;
  bool disableEnableStrideAlign = false;
  bool disableInferHIVMDataLayout = false;
};

struct Request {
  uint32_t apiVersion = kInProcessAPIVersion;
  uint32_t optionsVersion = kUBRelevantCompileOptionsVersion;
  uint32_t inputContractVersion =
      kBeforeCVPipeliningInputContractVersion;
  CompilerProfile compilerProfile = CompilerProfile::TritonMembaseA2A3;
  std::string_view compilerPipelineFingerprint =
      kA3MembasePipelineFingerprint;
  std::string_view target = "Ascend910_9382";
  // Compatibility text input owned by the caller, never a file path. Embedded
  // production callers leave this empty and pass a borrowed ModuleOp to
  // evaluateModule(). The view only needs to remain valid for evaluate().
  std::string_view beforeCVPipeliningGenericMLIR;
  UBRelevantCompileOptions options;
  std::string_view requestId;
};

struct Diagnostic {
  // Stable machine-readable category.  Callers should not parse message.
  std::string category;
  std::string message;
};

struct StageTiming {
  std::string stage;
  uint64_t nanoseconds = 0;
  uint32_t occurrence = 0;
};

// Detailed records preserve the standalone CLI's development/oracle utility;
// production evaluate() leaves stageTimings empty and pruning callers normally
// only inspect the summary fields in Result.
struct BufferResult {
  std::string name;
  uint64_t extentBits = 0;
  uint32_t multiBufferNum = 1;
  std::vector<uint64_t> offsetsBytes;
  int allocTime = -1;
  int freeTime = -1;
};

struct FunctionResult {
  std::string function;
  bool overflow = false;
  uint64_t ubPeakBits = 0;
  uint64_t requiredBits = 0;
  uint32_t selectedSeed = 0;
  std::vector<BufferResult> buffers;
  std::vector<std::pair<std::string, std::string>> inplacePairs;
};

struct Result {
  Precision precision = Precision::Incomplete;
  Status status = Status::InternalError;
  std::optional<bool> overflow;
  std::optional<uint64_t> ubPeakBits;
  std::optional<uint64_t> requiredBits;
  uint64_t capacityBits = 0;
  std::optional<uint32_t> selectedSeed;
  uint64_t totalTimeNs = 0;
  // Production evaluate() may prove non-overflow from the sum of all
  // independently allocated, aligned UB buffers after MarkMultiBuffer.  That
  // proof deliberately does not fabricate an exact peak, plan, lifetime, or
  // selected seed; callers can distinguish the narrow decision result here.
  bool decisionOnlyNonOverflow = false;
  std::optional<uint64_t> conservativeUpperBoundBits;

  std::string requestId;
  std::string modelBuildId;
  std::string compilerPipelineFingerprint;
  std::string inputDigest;
  std::string effectiveOptionsDigest;
  std::vector<StageTiming> stageTimings;
  std::vector<FunctionResult> functions;
  std::vector<Diagnostic> diagnostics;
};

// The API never throws across the library boundary.  Blockers and internal
// failures are fail-open results; only Exact + Overflow may prune a candidate.
Result evaluate(const Request &request) noexcept;

// Standalone tests and differential-oracle tools may override fixed compiler
// facts through this entry point. Production prediction passes must not.
Result evaluateForDebug(const Request &request,
                        const DebugModelControls &controls) noexcept;

// Embedded BiSheng entry. The ModuleOp is borrowed for this synchronous call
// and is never modified or retained. The text field in Request is ignored.
Result evaluateModule(mlir::ModuleOp module,
                      const Request &request) noexcept;
Result evaluateModuleForDebug(
    mlir::ModuleOp module, const Request &request,
    const DebugModelControls &controls) noexcept;

const char *toString(Precision precision) noexcept;
const char *toString(Status status) noexcept;
const char *toString(MultiBufferStrategy strategy) noexcept;
const char *toString(CompilerProfile profile) noexcept;

} // namespace cvub

#endif
