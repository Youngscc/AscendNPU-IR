// Production entry for exact CVPipelining UB overflow modeling.
// Development-only oracle checks remain in tools/dev_validate.cpp.

#include "pipeline/cvpipelining_ub_pipeline.hpp"

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
  bool enableAutoMultiBuffer = false;
  cvub::MultiBufferStrategy localMultiBufferStrategy =
      cvub::MultiBufferStrategy::NoLimit;
  cvub::MultiBufferStrategy mixMultiBufferStrategy =
      cvub::MultiBufferStrategy::NoLimit;
  std::string format = "text";
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
      << "Usage: cvpipeline_ub_model --before-cvpipelining-ir=<path> [options]\n"
      << "\nCVPipelining options:\n"
      << "  --disable-cv-pipelining=<bool>\n"
      << "  --cv-pipeline-depth=<n>\n"
      << "  --enable-preload=<bool>\n"
      << "  --enable-cv-lazy-loading=<bool>\n"
      << "\nUB-affecting pass and PlanMemory options:\n"
      << "  --enable-code-motion=<bool>\n"
      << "  --enable-auto-multi-buffer=<bool>\n"
      << "  --limit-auto-multi-buffer-of-local-buffer=<strategy>\n"
      << "  --limit-auto-multi-buffer-buffer=<strategy>\n"
      << "  --restrict-inplace-as-isa\n"
      << "  --random-seed=<0..19>\n"
      << "  --format=<text|json>\n"
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
  result.enableCodeMotion = options.enableCodeMotion;
  result.enableAutoMultiBuffer = options.enableAutoMultiBuffer;
  result.limitAutoMultiBufferOfLocalBuffer =
      options.localMultiBufferStrategy;
  result.limitMixAutoMultiBufferBuffer = options.mixMultiBufferStrategy;
  return result;
}

cvub::PlanMemoryModelResult PlanMemory(const cvub::PlanMemoryInput &input,
                                       const Options &options) {
  if (options.randomSeed)
    return cvub::PlanLocalMemoryForSeed(
        input, *options.randomSeed, options.restrictInplaceAsISA);
  return cvub::PlanLocalMemory(input, options.restrictInplaceAsISA);
}

cvub::PlanMemoryModelResult RunModel(const Options &options,
                                     cvub::DebugTrace *trace) {
  if (options.debugEntry == DebugEntry::BeforeCVPipelining) {
    cvub::CVPipeliningUBPipelineOptions pipelineOptions;
    pipelineOptions.cvPipelining = CVPipeliningOptions(options);
    pipelineOptions.ubAffectingPasses = UBAffectingPassOptions(options);
    pipelineOptions.planMemorySeed = options.randomSeed;
    pipelineOptions.restrictInplaceAsISA = options.restrictInplaceAsISA;
    pipelineOptions.debugTrace = trace;
    return cvub::RunCVPipeliningUBPipeline(options.beforeCVPipeliningIR,
                                           pipelineOptions);
  }

  cvub::PlanMemoryInput input;
  if (options.debugEntry == DebugEntry::AfterCVPipelining) {
    input = cvub::BuildPlanMemoryInputFromAfterCVPipelining(
        options.afterCVPipeliningIR, UBAffectingPassOptions(options), trace);
  } else {
    input = cvub::ParsePlanMemoryInput(options.beforePlanMemoryIR, "AIV");
    if (trace) {
      trace->Pass("PlanMemory",
                  {{"allocations", input.allocations.size()},
                   {"operations", input.operations.size()},
                   {"function_arguments", input.functionArguments.size()}});
      trace->Artifact("PlanMemoryInput", [&] {
        return cvub::SerializeCanonicalPlanMemoryInput(input);
      });
    }
  }
  cvub::PlanMemoryModelResult result = PlanMemory(input, options);
  cvub::TracePlanMemoryResult(trace, result);
  return result;
}

std::string Precision(const Options &options) {
  if (options.debugEntry == DebugEntry::AfterCVPipelining)
    return "after_cvpipelining_model";
  if (options.debugEntry == DebugEntry::BeforePlanMemory)
    return "plan_memory_model";
  return "before_cvpipelining_model";
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

int PrintResult(const Options &options,
                const cvub::PlanMemoryModelResult &result) {
  if (options.format == "json") {
    std::cout << "{\n"
              << "  \"precision\": \"" << Precision(options) << "\",\n"
              << "  \"oracle\": \"" << Oracle(options) << "\",\n"
              << "  \"status\": \""
              << (result.success ? "success" : "overflow") << "\",\n"
              << "  \"ub_peak_bits\": " << result.peakBits
              << ",\n  \"required_bits\": ";
    if (result.overflow)
      std::cout << result.requiredBits;
    else
      std::cout << "null";
    std::cout << ",\n"
              << "  \"capacity_bits\": " << result.capacityBits << ",\n"
              << "  \"restrict_inplace_as_isa\": "
              << (options.restrictInplaceAsISA ? "true" : "false") << ",\n"
              << "  \"overflow\": "
              << (result.overflow ? "true" : "false") << ",\n"
              << "  \"selected_seed\": " << result.selectedSeed << ",\n"
              << "  \"blockers\": []\n"
              << "}\n";
    return result.success ? 0 : 2;
  }

  std::cout << "precision\t" << Precision(options) << '\n'
            << "success\t" << (result.success ? "true" : "false") << '\n'
            << "overflow\t" << (result.overflow ? "true" : "false") << '\n'
            << "selected_seed\t" << result.selectedSeed << '\n'
            << "restrict_inplace_as_isa\t"
            << (options.restrictInplaceAsISA ? "true" : "false") << '\n'
            << "peak_bits\t" << result.peakBits << '\n'
            << "required_bits\t" << result.requiredBits << '\n'
            << "capacity_bits\t" << result.capacityBits << '\n'
            << "name\textent_bits\toffset_bytes\talloc_time\tfree_time\n";
  for (const cvub::PlannedBufferRecord &buffer : result.buffers)
    for (uint64_t offset : buffer.offsetsBytes)
      std::cout << buffer.name << '\t' << buffer.extentBits << '\t' << offset
                << '\t' << buffer.allocTime << '\t' << buffer.freeTime << '\n';
  return result.success ? 0 : 2;
}

void PrintBlocker(const Options &options, const std::string &message) {
  if (options.format != "json")
    return;
  std::cout << "{\n"
            << "  \"precision\": \"blocked\",\n"
            << "  \"oracle\": \"" << Oracle(options) << "\",\n"
            << "  \"status\": \"blocker\",\n"
            << "  \"ub_peak_bits\": null,\n"
            << "  \"required_bits\": null,\n"
            << "  \"capacity_bits\": " << cvub::kUBCapacityBits << ",\n"
            << "  \"overflow\": null,\n"
            << "  \"blockers\": [\"" << JsonEscape(message) << "\"]\n"
            << "}\n";
}

} // namespace

int main(int argc, char **argv) {
  Options options;
  try {
    options = ParseOptions(argc, argv);
    ValidateOptions(options);
    std::optional<cvub::DebugTrace> debugTrace;
    if (options.debug)
      debugTrace.emplace(std::cerr, options.debugDirectory);
    const cvub::PlanMemoryModelResult result =
        RunModel(options, debugTrace ? &*debugTrace : nullptr);
    return PrintResult(options, result);
  } catch (const std::exception &error) {
    PrintBlocker(options, error.what());
    std::cerr << "[ERROR] " << error.what() << '\n';
    return 1;
  }
}
