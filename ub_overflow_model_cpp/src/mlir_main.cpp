// Standalone MLIR boundary for the UB overflow model.  The model core receives
// the same borrowed ModuleOp as the embedded BiSheng pass; this file owns only
// command-line parsing, MLIR parsing and result presentation.

#include "ub_overflow_model/api.hpp"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

struct Options {
  std::string input;
  std::string inputStage = "before-cvpipelining";
  std::string format = "text";
  std::string target = "Ascend910_9382";
  cvub::Request request;
  cvub::DebugModelControls debug;
  bool useDebugEntry = false;
};

bool parseBool(llvm::StringRef value) {
  if (value == "1" || value == "true")
    return true;
  if (value == "0" || value == "false")
    return false;
  throw std::runtime_error("boolean value must be true, false, 1, or 0");
}

int parseInteger(llvm::StringRef value, llvm::StringRef option) {
  int result = 0;
  if (value.getAsInteger(10, result))
    throw std::runtime_error((option + " requires an integer").str());
  return result;
}

cvub::MultiBufferStrategy parseStrategy(llvm::StringRef value) {
  if (value == "no-limit")
    return cvub::MultiBufferStrategy::NoLimit;
  if (value == "only-cube")
    return cvub::MultiBufferStrategy::OnlyCube;
  if (value == "only-vector")
    return cvub::MultiBufferStrategy::OnlyVector;
  if (value == "no-l0c")
    return cvub::MultiBufferStrategy::CubeNoL0C;
  throw std::runtime_error("unknown multi-buffer strategy: " + value.str());
}

void printHelp() {
  llvm::outs()
      << "Usage: bishengir-ub-overflow-model <input.mlir> "
         "[options]\n"
      << "The file is parsed by MLIR and evaluated through the same ModuleOp "
         "API used by bishengir-compile. Use '-' for stdin.\n\n"
      << "  --format=<text|json>\n"
      << "  --input-stage=<before-autoblockify|before-cvpipelining>\n"
      << "  --plan-memory-seed=<-1|0..19>  (-1 uses production retry)\n"
      << "  --enable-triton-kernel-compile=<bool>\n"
      << "  --enable-auto-blockify-loop=<bool>\n"
      << "  --limit-auto-multi-buffer-only-for-local-buffer=<bool>\n"
      << "  --set-workspace-multibuffer=<positive integer>\n"
      << "  --cv-pipeline-depth=<integer>\n"
      << "  --enable-cv-lazy-loading=<bool>\n"
      << "  --enable-preload=<bool>\n"
      << "  --enable-code-motion=<bool>\n"
      << "  --enable-auto-bind-sub-block=<bool>\n"
      << "  --enable-ubuf-saving=<bool>\n"
      << "  --enable-auto-multi-buffer=<bool>\n"
      << "  --enable-hivm-auto-storage-align=<bool>\n"
      << "  --tile-mix-vector-loop=<positive integer>\n"
      << "  --tile-mix-cube-loop=<positive integer>\n"
      << "  --limit-auto-multi-buffer-of-local-buffer=<strategy>\n"
      << "  --limit-auto-multi-buffer-buffer=<strategy>\n"
      << "  --disable-auto-cv-work-space-manage=<bool>\n"
      << "  --enable-hivm-cross-core-gss=<bool>\n"
      << "  --enable-hivm-inject-block-all-sync=<bool>\n"
      << "  --disable-auto-inject-block-sync=<bool>\n"
      << "  --restrict-inplace-as-isa\n";
}

Options parseOptions(int argc, char **argv) {
  Options result;
  for (int index = 1; index < argc; ++index) {
    llvm::StringRef argument(argv[index]);
    if (argument == "--help" || argument == "-h") {
      printHelp();
      std::exit(0);
    }
    if (!argument.starts_with("-")) {
      if (!result.input.empty())
        throw std::runtime_error("multiple input files were provided");
      result.input = argument.str();
      continue;
    }
    if (argument == "-") {
      if (!result.input.empty())
        throw std::runtime_error("multiple input files were provided");
      result.input = "-";
      continue;
    }

    auto value = [&](llvm::StringRef name) -> std::optional<llvm::StringRef> {
      if (argument.consume_front(name) && argument.consume_front("="))
        return argument;
      return std::nullopt;
    };
    auto boolean = [&](llvm::StringRef name,
                       bool &destination) -> bool {
      llvm::StringRef copy = argument;
      if (!copy.consume_front(name))
        return false;
      if (copy.empty()) {
        destination = true;
        return true;
      }
      if (!copy.consume_front("="))
        return false;
      destination = parseBool(copy);
      return true;
    };

    if (auto parsed = value("--format"))
      result.format = parsed->str();
    else if (auto parsed = value("--input-stage"))
      result.inputStage = parsed->str();
    else if (auto parsed = value("--target"))
      result.target = parsed->str();
    else if (auto parsed = value("--plan-memory-seed")) {
      const int seed = parseInteger(*parsed, "--plan-memory-seed");
      if (seed < -1 || seed >= 20)
        throw std::runtime_error("--plan-memory-seed must be -1 or 0..19");
      if (seed >= 0) {
        result.debug.fixedPlanMemorySeed = static_cast<uint32_t>(seed);
        result.useDebugEntry = true;
      }
    } else if (auto parsed = value("--cv-pipeline-depth"))
      result.request.options.cvPipelineDepth =
          parseInteger(*parsed, "--cv-pipeline-depth");
    else if (auto parsed = value("--tile-mix-vector-loop"))
      result.request.options.tileMixVectorLoop =
          parseInteger(*parsed, "--tile-mix-vector-loop");
    else if (auto parsed = value("--tile-mix-cube-loop"))
      result.request.options.tileMixCubeLoop =
          parseInteger(*parsed, "--tile-mix-cube-loop");
    else if (auto parsed = value("--set-workspace-multibuffer")) {
      const int count = parseInteger(*parsed, "--set-workspace-multibuffer");
      if (count <= 0)
        throw std::runtime_error(
            "--set-workspace-multibuffer must be positive");
      result.request.options.workspaceMultiBufferNum =
          static_cast<unsigned>(count);
    } else if (auto parsed =
                 value("--limit-auto-multi-buffer-of-local-buffer"))
      result.request.options.localMultiBufferStrategy =
          parseStrategy(*parsed);
    else if (auto parsed = value("--limit-auto-multi-buffer-buffer"))
      result.request.options.mixMultiBufferStrategy = parseStrategy(*parsed);
    else if (boolean("--enable-cv-lazy-loading",
                     result.request.options.enableCVLazyLoading))
      continue;
    else if (boolean("--enable-triton-kernel-compile",
                     result.request.options.enableTritonKernelCompile))
      continue;
    else if (boolean("--enable-auto-blockify-loop",
                     result.request.options.enableAutoBlockifyLoop))
      continue;
    else if (boolean(
                 "--limit-auto-multi-buffer-only-for-local-buffer",
                 result.request.options
                     .limitAutoMultiBufferOnlyForLocalBuffer))
      continue;
    else if (boolean("--enable-lazy-loading",
                     result.request.options.enableCVLazyLoading))
      continue;
    else if (boolean("--enable-preload",
                     result.request.options.enablePreload))
      continue;
    else if (boolean("--enable-code-motion",
                     result.request.options.enableCodeMotion))
      continue;
    else if (boolean("--enable-auto-bind-sub-block",
                     result.request.options.enableAutoBindSubBlock))
      continue;
    else if (boolean("--enable-ubuf-saving",
                     result.request.options.enableUbufSaving))
      continue;
    else if (boolean("--enable-auto-multi-buffer",
                     result.request.options.enableAutoMultiBuffer))
      continue;
    else if (boolean("--enable-hivm-auto-storage-align",
                     result.request.options.enableHIVMAutoStorageAlign))
      continue;
    else if (boolean("--disable-auto-cv-work-space-manage",
                     result.request.options.disableAutoCVWorkSpaceManage))
      continue;
    else if (boolean("--enable-hivm-cross-core-gss",
                     result.request.options.enableHIVMCrossCoreGSS))
      continue;
    else if (boolean("--enable-hivm-inject-block-all-sync",
                     result.request.options.enableHIVMInjectBlockAllSync))
      continue;
    else if (boolean("--disable-auto-inject-block-sync",
                     result.request.options.disableAutoInjectBlockSync))
      continue;
    else if (argument == "--restrict-inplace-as-isa") {
      result.debug.restrictInplaceAsISA = true;
      result.useDebugEntry = true;
    } else {
      throw std::runtime_error("unknown option: " + argument.str());
    }
  }

  if (result.input.empty())
    throw std::runtime_error("an MLIR input is required");
  if (result.format != "text" && result.format != "json")
    throw std::runtime_error("--format must be text or json");
  if (result.inputStage == "before-autoblockify") {
    result.request.inputContractVersion =
        cvub::kBeforeAutoBlockifyInputContractVersion;
    result.request.compilerPipelineFingerprint =
        cvub::kA3MembaseBeforeAutoBlockifyFingerprint;
  } else if (result.inputStage != "before-cvpipelining") {
    throw std::runtime_error(
        "--input-stage must be before-autoblockify or before-cvpipelining");
  }
  if (result.request.options.tileMixVectorLoop == 0 ||
      result.request.options.tileMixCubeLoop == 0)
    throw std::runtime_error("tile loop factors must be positive");
  return result;
}

void printJsonString(llvm::raw_ostream &output, llvm::StringRef value) {
  output << '"';
  for (char character : value) {
    if (character == '"' || character == '\\')
      output << '\\';
    output << character;
  }
  output << '"';
}

int printResult(const cvub::Result &result, llvm::StringRef format) {
  if (format == "json") {
    llvm::outs() << "{\n  \"precision\": \""
                 << cvub::toString(result.precision) << "\",\n  \"status\": \""
                 << cvub::toString(result.status) << "\",\n  \"overflow\": ";
    result.overflow ? llvm::outs() << (*result.overflow ? "true" : "false")
                    : llvm::outs() << "null";
    llvm::outs() << ",\n  \"ub_peak_bits\": ";
    result.ubPeakBits ? llvm::outs() << *result.ubPeakBits
                      : llvm::outs() << "null";
    llvm::outs() << ",\n  \"required_bits\": ";
    result.requiredBits ? llvm::outs() << *result.requiredBits
                        : llvm::outs() << "null";
    llvm::outs() << ",\n  \"capacity_bits\": " << result.capacityBits
                 << ",\n  \"selected_seed\": ";
    result.selectedSeed ? llvm::outs() << *result.selectedSeed
                        : llvm::outs() << "null";
    llvm::outs() << ",\n  \"decision_only_non_overflow\": "
                 << (result.decisionOnlyNonOverflow ? "true" : "false")
                 << ",\n  \"model_build_id\": ";
    printJsonString(llvm::outs(), result.modelBuildId);
    llvm::outs() << ",\n  \"diagnostics\": [";
    for (size_t index = 0; index < result.diagnostics.size(); ++index) {
      if (index)
        llvm::outs() << ", ";
      printJsonString(llvm::outs(), result.diagnostics[index].message);
    }
    llvm::outs() << "]\n}\n";
  } else {
    llvm::outs() << "precision\t" << cvub::toString(result.precision)
                 << "\nstatus\t" << cvub::toString(result.status)
                 << "\noverflow\t";
    result.overflow ? llvm::outs() << (*result.overflow ? "true" : "false")
                    : llvm::outs() << "null";
    llvm::outs() << "\npeak_bits\t";
    result.ubPeakBits ? llvm::outs() << *result.ubPeakBits
                      : llvm::outs() << "null";
    llvm::outs() << "\nrequired_bits\t";
    result.requiredBits ? llvm::outs() << *result.requiredBits
                        : llvm::outs() << "null";
    llvm::outs() << "\ncapacity_bits\t" << result.capacityBits << '\n';
  }
  if (result.status == cvub::Status::Success)
    return 0;
  if (result.status == cvub::Status::Overflow)
    return 2;
  return 1;
}

} // namespace

int main(int argc, char **argv) {
  try {
    Options options = parseOptions(argc, argv);
    options.request.target = options.target;
    auto input = llvm::MemoryBuffer::getFileOrSTDIN(options.input);
    if (!input)
      throw std::runtime_error("failed to open input: " + options.input);

    mlir::MLIRContext context;
    context.allowUnregisteredDialects(true);
    llvm::SourceMgr sourceManager;
    sourceManager.AddNewSourceBuffer(std::move(*input), llvm::SMLoc());
    mlir::OwningOpRef<mlir::ModuleOp> module =
        mlir::parseSourceFile<mlir::ModuleOp>(sourceManager, &context);
    if (!module)
      throw std::runtime_error("failed to parse input: " + options.input);

    cvub::Result result = options.useDebugEntry
                              ? cvub::evaluateModuleForDebug(
                                    *module, options.request, options.debug)
                              : cvub::evaluateModule(*module, options.request);
    // Result owns every externally visible string/record. Destroying the
    // source module before presentation is a lifetime regression check for the
    // borrowed synchronous API contract.
    module = mlir::OwningOpRef<mlir::ModuleOp>();
    return printResult(result, options.format);
  } catch (const std::exception &error) {
    llvm::errs() << "[ERROR] " << error.what() << '\n';
    return 1;
  }
}
