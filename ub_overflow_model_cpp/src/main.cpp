// Production entry for the exact UB Overflow Model.
// Development-only oracle checks remain in tools/dev_validate.cpp.

#include "pipeline/cvpipelining_ub_pipeline.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace {

enum class DebugEntry {
  BeforeCVPipelining,
  AfterCVPipelining,
  BeforePlanMemory,
};

struct Options {
  std::filesystem::path beforeCVPipeliningIR;
  std::filesystem::path afterCVPipeliningIR;
  std::filesystem::path beforePlanMemoryIR;
  std::optional<uint32_t> randomSeed;
  bool restrictInplaceAsISA = false;
  bool disableCVPipelining = false;
  int cvPipelineDepth = -1;
  bool enablePreload = false;
  bool enableCVLazyLoading = false;
  bool enableCodeMotion = true;
  int tileMixCubeLoop = 2;
  int tileMixVectorLoop = 2;
  bool enableUbufSaving = false;
  bool enableTritonKernelCompile = false;
  bool disableAlignAllocSize = false;
  bool disableEnableStrideAlign = false;
  bool disableInferHIVMDataLayout = false;
  bool enableAutoMultiBuffer = false;
  cvub::MultiBufferStrategy localMultiBufferStrategy =
      cvub::MultiBufferStrategy::CubeNoL0C;
  cvub::MultiBufferStrategy mixMultiBufferStrategy =
      cvub::MultiBufferStrategy::OnlyCube;
  std::string format = "text";
  bool showRuntimeTiming = false;
  bool verifyEachPass = false;
  bool debug = false;
  DebugEntry debugEntry = DebugEntry::BeforeCVPipelining;
  std::filesystem::path debugDirectory;
};

uint32_t ParseRandomSeed(const std::string &text) {
  size_t parsed = 0;
  const unsigned long long value = std::stoull(text, &parsed);
  if (parsed != text.size() || value >= 20)
    throw std::runtime_error("--random-seed must be in [0, 19]");
  return static_cast<uint32_t>(value);
}

bool ParseBool(const std::string &text) {
  if (text == "1" || text == "true")
    return true;
  if (text == "0" || text == "false")
    return false;
  throw std::runtime_error("boolean option must be one of 0, 1, true, false");
}

DebugEntry ParseDebugEntry(const std::string &text) {
  if (text == "before-cvpipelining")
    return DebugEntry::BeforeCVPipelining;
  if (text == "after-cvpipelining")
    return DebugEntry::AfterCVPipelining;
  if (text == "before-plan-memory")
    return DebugEntry::BeforePlanMemory;
  throw std::runtime_error(
      "--debug-entry must be before-cvpipelining, after-cvpipelining, or "
      "before-plan-memory");
}

void PrintHelp() {
  std::cout
      << "Usage: bishengir-ub-overflow-model "
         "--before-cvpipelining-ir=<path> [options]\n"
      << "\nCVPipelining options:\n"
      << "  --disable-cv-pipelining=<bool>\n"
      << "  --cv-pipeline-depth=<n>\n"
      << "  --enable-preload=<bool>\n"
      << "  --enable-cv-lazy-loading=<bool>\n"
      << "\nUB-affecting pass and PlanMemory options:\n"
      << "  --enable-code-motion=<bool>\n"
      << "  --tile-mix-cube-loop=<positive integer>\n"
      << "  --tile-mix-vector-loop=<positive integer>\n"
      << "  --enable-ubuf-saving=<bool>\n"
      << "  --enable-triton-kernel-compile=<bool>\n"
      << "  --disable-align-alloc-size=<bool>\n"
      << "  --disable-enable-stride-align=<bool>\n"
      << "  --disable-infer-hivm-data-layout=<bool>\n"
      << "  --enable-auto-multi-buffer=<bool>\n"
      << "  --limit-auto-multi-buffer-of-local-buffer=<strategy>\n"
      << "  --limit-auto-multi-buffer-buffer=<strategy>\n"
      << "  --restrict-inplace-as-isa\n"
      << "  --random-seed=<0..19>\n"
      << "  --format=<text|json>\n"
      << "  --show-runtime-timing[=<bool>]\n"
      << "      Emit total and per-stage runtime records to stderr.\n"
      << "  --verify-each[=<bool>]\n"
      << "      Validate Generic IR after every modeled pass.\n"
      << "\nDebug options:\n"
      << "  --debug\n"
      << "  --debug-dir=<path>\n"
      << "  --debug-entry=<before-cvpipelining|after-cvpipelining|"
         "before-plan-memory>\n"
      << "  --after-cvpipelining-ir=<path>\n"
      << "  --before-plan-memory-ir=<path>\n";
}

Options ParseOptions(int argc, char **argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    auto readValue = [&](const std::string &name) -> std::optional<std::string> {
      if (cvub::startsWith(argument, name + "="))
        return argument.substr(name.size() + 1);
      if (argument == name && i + 1 < argc)
        return std::string(argv[++i]);
      return std::nullopt;
    };

    if (auto beforeCV = readValue("--before-cvpipelining-ir"))
      options.beforeCVPipeliningIR = *beforeCV;
    else if (auto afterCV = readValue("--after-cvpipelining-ir"))
      options.afterCVPipeliningIR = *afterCV;
    else if (auto beforePlan = readValue("--before-plan-memory-ir"))
      options.beforePlanMemoryIR = *beforePlan;
    else if (auto seed = readValue("--random-seed"))
      options.randomSeed = ParseRandomSeed(*seed);
    else if (auto format = readValue("--format"))
      options.format = *format;
    else if (argument == "--restrict-inplace-as-isa")
      options.restrictInplaceAsISA = true;
    else if (argument == "--disable-cv-pipelining")
      options.disableCVPipelining = true;
    else if (auto disableCV = readValue("--disable-cv-pipelining"))
      options.disableCVPipelining = ParseBool(*disableCV);
    else if (auto depth = readValue("--cv-pipeline-depth"))
      options.cvPipelineDepth = std::stoi(*depth);
    else if (argument == "--enable-preload")
      options.enablePreload = true;
    else if (auto preload = readValue("--enable-preload"))
      options.enablePreload = ParseBool(*preload);
    else if (argument == "--enable-cv-lazy-loading")
      options.enableCVLazyLoading = true;
    else if (auto lazyLoading = readValue("--enable-cv-lazy-loading"))
      options.enableCVLazyLoading = ParseBool(*lazyLoading);
    else if (auto codeMotion = readValue("--enable-code-motion"))
      options.enableCodeMotion = ParseBool(*codeMotion);
    else if (auto cubeLoop = readValue("--tile-mix-cube-loop"))
      options.tileMixCubeLoop = std::stoi(*cubeLoop);
    else if (auto vectorLoop = readValue("--tile-mix-vector-loop"))
      options.tileMixVectorLoop = std::stoi(*vectorLoop);
    else if (argument == "--enable-ubuf-saving")
      options.enableUbufSaving = true;
    else if (auto ubufSaving = readValue("--enable-ubuf-saving"))
      options.enableUbufSaving = ParseBool(*ubufSaving);
    else if (argument == "--enable-triton-kernel-compile")
      options.enableTritonKernelCompile = true;
    else if (auto enableTriton =
                 readValue("--enable-triton-kernel-compile"))
      options.enableTritonKernelCompile = ParseBool(*enableTriton);
    else if (argument == "--disable-align-alloc-size")
      options.disableAlignAllocSize = true;
    else if (auto disableAlign = readValue("--disable-align-alloc-size"))
      options.disableAlignAllocSize = ParseBool(*disableAlign);
    else if (argument == "--disable-enable-stride-align")
      options.disableEnableStrideAlign = true;
    else if (auto disableStride =
                 readValue("--disable-enable-stride-align"))
      options.disableEnableStrideAlign = ParseBool(*disableStride);
    else if (argument == "--disable-infer-hivm-data-layout")
      options.disableInferHIVMDataLayout = true;
    else if (auto disableLayout =
                 readValue("--disable-infer-hivm-data-layout"))
      options.disableInferHIVMDataLayout = ParseBool(*disableLayout);
    else if (argument == "--enable-auto-multi-buffer")
      options.enableAutoMultiBuffer = true;
    else if (auto autoMultiBuffer = readValue("--enable-auto-multi-buffer"))
      options.enableAutoMultiBuffer = ParseBool(*autoMultiBuffer);
    else if (auto localStrategy =
                 readValue("--limit-auto-multi-buffer-of-local-buffer"))
      options.localMultiBufferStrategy =
          cvub::ParseMultiBufferStrategy(*localStrategy);
    else if (auto mixStrategy =
                 readValue("--limit-auto-multi-buffer-buffer"))
      options.mixMultiBufferStrategy =
          cvub::ParseMultiBufferStrategy(*mixStrategy);
    else if (argument == "--show-runtime-timing")
      options.showRuntimeTiming = true;
    else if (auto showTiming = readValue("--show-runtime-timing"))
      options.showRuntimeTiming = ParseBool(*showTiming);
    else if (argument == "--verify-each")
      options.verifyEachPass = true;
    else if (auto verifyEach = readValue("--verify-each"))
      options.verifyEachPass = ParseBool(*verifyEach);
    else if (argument == "--debug")
      options.debug = true;
    else if (auto debugDirectory = readValue("--debug-dir"))
      options.debugDirectory = *debugDirectory;
    else if (auto debugEntry = readValue("--debug-entry"))
      options.debugEntry = ParseDebugEntry(*debugEntry);
    else if (argument == "--help" || argument == "-h") {
      PrintHelp();
      std::exit(0);
    } else {
      throw std::runtime_error("unknown option: " + argument);
    }
  }
  return options;
}

void ValidateOptions(const Options &options) {
  if (options.format != "text" && options.format != "json")
    throw std::runtime_error("--format must be text or json");
  if (options.tileMixCubeLoop <= 0 || options.tileMixVectorLoop <= 0)
    throw std::runtime_error(
        "--tile-mix-cube-loop and --tile-mix-vector-loop must be positive");
  if ((!options.debugDirectory.empty() ||
       options.debugEntry != DebugEntry::BeforeCVPipelining) &&
      !options.debug)
    throw std::runtime_error("--debug-dir and --debug-entry require --debug");
  if (!options.debug && (!options.afterCVPipeliningIR.empty() ||
                         !options.beforePlanMemoryIR.empty()))
    throw std::runtime_error(
        "alternative input boundaries require --debug and --debug-entry");

  if (options.debugEntry == DebugEntry::BeforeCVPipelining &&
      options.beforeCVPipeliningIR.empty())
    throw std::runtime_error("--before-cvpipelining-ir is required");
  if (options.debugEntry == DebugEntry::AfterCVPipelining &&
      options.afterCVPipeliningIR.empty())
    throw std::runtime_error("--after-cvpipelining-ir is required");
  if (options.debugEntry == DebugEntry::BeforePlanMemory &&
      options.beforePlanMemoryIR.empty())
    throw std::runtime_error("--before-plan-memory-ir is required");

  const std::filesystem::path *input = &options.beforeCVPipeliningIR;
  if (options.debugEntry == DebugEntry::AfterCVPipelining)
    input = &options.afterCVPipeliningIR;
  else if (options.debugEntry == DebugEntry::BeforePlanMemory)
    input = &options.beforePlanMemoryIR;
  if (!std::filesystem::is_regular_file(*input))
    throw std::runtime_error("input is not a regular file: " +
                             input->string());
}

cvub::CVPipeliningOptions CVPipeliningOptions(const Options &options) {
  cvub::CVPipeliningOptions result;
  result.disabled = options.disableCVPipelining;
  result.setDepthInUnrollMode = options.cvPipelineDepth;
  result.enableSkewMode = options.enablePreload;
  result.enableLazyLoading = options.enableCVLazyLoading;
  return result;
}

cvub::UBAffectingPassOptions UBAffectingPassOptions(const Options &options) {
  cvub::UBAffectingPassOptions result;
  result.tileMixCubeLoop = static_cast<unsigned>(options.tileMixCubeLoop);
  result.tileMixVectorLoop = static_cast<unsigned>(options.tileMixVectorLoop);
  result.enableCodeMotion = options.enableCodeMotion;
  result.enableUbufSaving = options.enableUbufSaving;
  result.enableTritonKernelCompile = options.enableTritonKernelCompile;
  result.disableAlignAllocSize = options.disableAlignAllocSize;
  result.disableEnableStrideAlign = options.disableEnableStrideAlign;
  result.disableInferHIVMDataLayout = options.disableInferHIVMDataLayout;
  result.enableAutoMultiBuffer = options.enableAutoMultiBuffer;
  result.limitAutoMultiBufferOfLocalBuffer =
      options.localMultiBufferStrategy;
  result.limitMixAutoMultiBufferBuffer = options.mixMultiBufferStrategy;
  return result;
}

cvub::PlanMemoryModelResult PlanMemory(const cvub::PlanMemoryInput &input,
                                       const Options &options,
                                       cvub::DebugTrace *trace = nullptr) {
  if (options.randomSeed)
    return cvub::PlanLocalMemoryForSeed(
        input, *options.randomSeed, options.restrictInplaceAsISA, trace);
  return cvub::PlanLocalMemory(input, options.restrictInplaceAsISA, trace);
}

cvub::ModulePlanResult RunModel(const Options &options,
                                cvub::DebugTrace *trace) {
  if (options.debugEntry == DebugEntry::BeforeCVPipelining) {
    cvub::CVPipeliningUBPipelineOptions pipelineOptions;
    pipelineOptions.cvPipelining = CVPipeliningOptions(options);
    pipelineOptions.ubAffectingPasses = UBAffectingPassOptions(options);
    pipelineOptions.planMemorySeed = options.randomSeed;
    pipelineOptions.restrictInplaceAsISA = options.restrictInplaceAsISA;
    pipelineOptions.debugTrace = trace;
    return cvub::RunCVPipeliningUBModulePipeline(
        options.beforeCVPipeliningIR, pipelineOptions);
  }

  if (options.debugEntry == DebugEntry::AfterCVPipelining) {
    return cvub::RunUBModuleFromAfterCVPipelining(
        cvub::MeasureStage(trace, "ParseGenericIR", [&] {
          return cvub::ParseGenericIR(options.afterCVPipeliningIR, false);
        }),
        UBAffectingPassOptions(options), options.randomSeed,
        options.restrictInplaceAsISA, trace);
  }
  const cvub::PlanMemoryInput input = cvub::MeasureStage(
      trace, "ParsePlanMemoryInput", [&] {
        return cvub::ParsePlanMemoryInput(options.beforePlanMemoryIR, "AIV");
      });
  if (trace) {
    trace->Pass("PlanMemory",
                {{"allocations", input.allocations.size()},
                 {"operations", input.operations.size()},
                 {"function_arguments", input.functionArguments.size()}});
    trace->Artifact("PlanMemoryInput", [&] {
      return cvub::SerializeCanonicalPlanMemoryInput(input);
    });
  }
  cvub::PlanMemoryModelResult result = cvub::MeasureStage(
      trace, "PlanMemory", [&] { return PlanMemory(input, options, trace); });
  cvub::TracePlanMemoryResult(trace, result);
  return cvub::ModulePlanFromSingle("debug_aiv", std::move(result));
}

std::string Precision(const cvub::ModulePlanResult &result) {
  return result.precision == cvub::ModulePlanPrecision::Exact ? "exact"
                                                              : "incomplete";
}

std::string Oracle(const Options &options) {
  if (options.debugEntry == DebugEntry::AfterCVPipelining)
    return "ub_affecting_passes_plus_plan_memory";
  if (options.debugEntry == DebugEntry::BeforePlanMemory)
    return "plan_memory";
  return "cvpipelining_to_plan_memory";
}

std::string JsonEscape(const std::string &value) {
  std::string result;
  static constexpr char hex[] = "0123456789abcdef";
  for (char raw : value) {
    const unsigned char c = static_cast<unsigned char>(raw);
    switch (c) {
    case '"':
      result += "\\\"";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '\b':
      result += "\\b";
      break;
    case '\f':
      result += "\\f";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\t':
      result += "\\t";
      break;
    default:
      if (c < 0x20) {
        result += "\\u00";
        result.push_back(hex[c >> 4]);
        result.push_back(hex[c & 0x0f]);
      } else {
        result.push_back(static_cast<char>(c));
      }
    }
  }
  return result;
}

void PrintFunctionsJson(const std::vector<cvub::FunctionPlanResult> &functions,
                        const std::string &indent) {
  std::cout << "[";
  for (size_t functionIndex = 0; functionIndex < functions.size();
       ++functionIndex) {
    const cvub::FunctionPlanResult &function = functions[functionIndex];
    if (functionIndex)
      std::cout << ",";
    std::cout << "\n" << indent << "  {\"function\": \""
              << JsonEscape(function.function) << "\", \"status\": \""
              << (function.plan.success ? "success" : "overflow")
              << "\", \"ub_peak_bits\": " << function.plan.peakBits
              << ", \"required_bits\": " << function.plan.requiredBits
              << ", \"selected_seed\": " << function.plan.selectedSeed
              << ", \"buffers\": [";
    for (size_t bufferIndex = 0; bufferIndex < function.plan.buffers.size();
         ++bufferIndex) {
      const cvub::PlannedBufferRecord &buffer =
          function.plan.buffers[bufferIndex];
      if (bufferIndex)
        std::cout << ", ";
      std::cout << "{\"name\": \"" << JsonEscape(buffer.name)
                << "\", \"extent_bits\": " << buffer.extentBits
                << ", \"multi_buffer_num\": ";
      const auto multi = function.plan.multiBufferNums.find(buffer.name);
      std::cout << (multi == function.plan.multiBufferNums.end()
                        ? static_cast<uint32_t>(1)
                        : multi->second)
                << ", \"offsets_bytes\": [";
      for (size_t offsetIndex = 0; offsetIndex < buffer.offsetsBytes.size();
           ++offsetIndex) {
        if (offsetIndex)
          std::cout << ", ";
        std::cout << buffer.offsetsBytes[offsetIndex];
      }
      std::cout << "], \"alloc_time\": " << buffer.allocTime
                << ", \"free_time\": " << buffer.freeTime << "}";
    }
    std::cout << "], \"inplace_pairs\": [";
    for (size_t pairIndex = 0;
         pairIndex < function.plan.inplacePairs.size(); ++pairIndex) {
      if (pairIndex)
        std::cout << ", ";
      const auto &pair = function.plan.inplacePairs[pairIndex];
      std::cout << "[\"" << JsonEscape(pair.first) << "\", \""
                << JsonEscape(pair.second) << "\"]";
    }
    std::cout << "]}";
  }
  if (!functions.empty())
    std::cout << "\n" << indent;
  std::cout << "]";
}

int PrintResult(const Options &options, const cvub::ModulePlanResult &result) {
  const bool exact = result.precision == cvub::ModulePlanPrecision::Exact;
  if (options.format == "json") {
    std::cout << "{\n"
              << "  \"precision\": \"" << Precision(result) << "\",\n"
              << "  \"oracle\": \"" << Oracle(options) << "\",\n"
              << "  \"status\": \"";
    if (!exact)
      std::cout << "blocker";
    else
      std::cout << (result.success ? "success" : "overflow");
    std::cout << "\",\n  \"ub_peak_bits\": ";
    exact ? std::cout << result.peakBits : std::cout << "null";
    std::cout << ",\n  \"required_bits\": ";
    exact ? std::cout << result.requiredBits : std::cout << "null";
    std::cout << ",\n"
              << "  \"capacity_bits\": " << result.capacityBits << ",\n"
              << "  \"restrict_inplace_as_isa\": "
              << (options.restrictInplaceAsISA ? "true" : "false") << ",\n"
              << "  \"overflow\": "
              << (exact && result.overflow ? "true" : "false") << ",\n"
              << "  \"functions\": ";
    exact ? PrintFunctionsJson(result.functions, "  ")
          : PrintFunctionsJson({}, "  ");
    std::cout << ",\n  \"diagnostics\": [";
    for (size_t index = 0; index < result.diagnostics.size(); ++index) {
      if (index)
        std::cout << ", ";
      std::cout << "\"" << JsonEscape(result.diagnostics[index]) << "\"";
    }
    std::cout << "]";
    if (!exact) {
      std::cout << ",\n  \"debug_estimate\": {\"ub_peak_bits\": "
                << result.peakBits << ", \"required_bits\": "
                << result.requiredBits << ", \"functions\": ";
      PrintFunctionsJson(result.functions, "    ");
      std::cout << "}";
    }
    std::cout << "\n}\n";
    if (!exact)
      return 1;
    return result.success ? 0 : 2;
  }

  std::cout << "precision\t" << Precision(result) << '\n'
            << "status\t"
            << (!exact ? "blocker"
                       : (result.success ? "success" : "overflow"))
            << '\n'
            << "success\t"
            << (exact && result.success ? "true" : "false") << '\n'
            << "overflow\t"
            << (!exact ? "null" : (result.overflow ? "true" : "false"))
            << '\n'
            << "restrict_inplace_as_isa\t"
            << (options.restrictInplaceAsISA ? "true" : "false") << '\n'
            << "peak_bits\t";
  exact ? std::cout << result.peakBits : std::cout << "null";
  std::cout << '\n' << "required_bits\t";
  exact ? std::cout << result.requiredBits : std::cout << "null";
  std::cout << '\n'
            << "capacity_bits\t" << result.capacityBits << '\n';
  if (exact) {
    std::cout << "name\textent_bits\toffset_bytes\talloc_time\tfree_time\n";
    for (const cvub::FunctionPlanResult &function : result.functions)
      for (const cvub::PlannedBufferRecord &buffer : function.plan.buffers)
        for (uint64_t offset : buffer.offsetsBytes)
          std::cout << function.function << '\t' << buffer.name << '\t'
                    << buffer.extentBits << '\t' << offset << '\t'
                    << buffer.allocTime << '\t' << buffer.freeTime << '\n';
  }
  if (!exact) {
    std::cout << "debug_estimate_peak_bits\t" << result.peakBits << '\n'
              << "debug_estimate_required_bits\t" << result.requiredBits
              << '\n';
    for (const std::string &diagnostic : result.diagnostics)
      std::cout << "diagnostic\t" << diagnostic << '\n';
  }
  if (!exact)
    return 1;
  return result.success ? 0 : 2;
}

void PrintBlocker(const Options &options, const std::string &message) {
  if (options.format != "json")
    return;
  std::cout << "{\n"
            << "  \"precision\": \"incomplete\",\n"
            << "  \"oracle\": \"" << Oracle(options) << "\",\n"
            << "  \"status\": \"blocker\",\n"
            << "  \"ub_peak_bits\": null,\n"
            << "  \"required_bits\": null,\n"
            << "  \"capacity_bits\": " << cvub::kUBCapacityBits << ",\n"
            << "  \"overflow\": null,\n"
            << "  \"functions\": [],\n"
            << "  \"diagnostics\": [\"" << JsonEscape(message) << "\"]\n"
            << "}\n";
}

} // namespace

int main(int argc, char **argv) {
  Options options;
  std::optional<cvub::DebugTrace> debugTrace;
  std::optional<std::chrono::steady_clock::time_point> runtimeStarted;
  bool runtimeTimingPrinted = false;
  try {
    options = ParseOptions(argc, argv);
    ValidateOptions(options);
    if (options.debug || options.showRuntimeTiming || options.verifyEachPass)
      debugTrace.emplace(std::cerr, options.debugDirectory, options.debug,
                         options.showRuntimeTiming,
                         options.debug || options.verifyEachPass);
    if (options.showRuntimeTiming)
      runtimeStarted = std::chrono::steady_clock::now();
    const cvub::ModulePlanResult result =
        RunModel(options, debugTrace ? &*debugTrace : nullptr);
    if (debugTrace && runtimeStarted) {
      debugTrace->PrintRuntimeTiming(
          std::chrono::steady_clock::now() - *runtimeStarted);
      runtimeTimingPrinted = true;
    }
    return PrintResult(options, result);
  } catch (const std::exception &error) {
    if (debugTrace && runtimeStarted && !runtimeTimingPrinted)
      debugTrace->PrintRuntimeTiming(
          std::chrono::steady_clock::now() - *runtimeStarted);
    PrintBlocker(options, error.what());
    std::cerr << "[ERROR] " << error.what() << '\n';
    return 1;
  }
}
