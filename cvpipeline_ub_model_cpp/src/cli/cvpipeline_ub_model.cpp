// Final-facing lightweight CVPipeline UB model entry.
//
// Development validation is intentionally kept out of this binary; see
// dev_validate.cpp for oracle comparison checks used during implementation.

#include "../cvpipeline/cvpipelining_pass.hpp"
#include "../model_core.hpp"
#include "../suffix/suffix_pipeline.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace {
struct Options {
  std::string action;
  std::string beforePlanMemoryIR;
  std::string c1GenericIR;
  std::string beforeCVPipelineIR;
  std::optional<uint32_t> randomSeed;
  bool restrictInplaceAsISA = false;
  bool disableCVPipelining = false;
  int cvPipelineDepth = -1;
  bool enablePreload = false;
  bool enableCVLazyLoading = false;
  bool enableAutoMultiBuffer = false;
  cvub::MultiBufferStrategy localMultiBufferStrategy =
      cvub::MultiBufferStrategy::NoLimit;
  cvub::MultiBufferStrategy mixMultiBufferStrategy =
      cvub::MultiBufferStrategy::NoLimit;
  std::string format = "text";
};

uint32_t parseRandomSeed(const std::string &text) {
  size_t parsed = 0;
  const unsigned long long value = std::stoull(text, &parsed);
  if (parsed != text.size() || value >= 20)
    throw std::runtime_error("--random-seed must be in [0, 19]");
  return static_cast<uint32_t>(value);
}

bool parseBool(const std::string &text) {
  if (text == "1" || text == "true")
    return true;
  if (text == "0" || text == "false")
    return false;
  throw std::runtime_error("boolean option must be one of 0, 1, true, false");
}

cvub::CVPipeliningOptions cvPipeliningOptions(const Options &opts) {
  cvub::CVPipeliningOptions options;
  options.disabled = opts.disableCVPipelining;
  options.setDepthInUnrollMode = opts.cvPipelineDepth;
  options.enableSkewMode = opts.enablePreload;
  options.enableLazyLoading = opts.enableCVLazyLoading;
  return options;
}

cvub::SuffixPipelineOptions suffixPipelineOptions(const Options &opts) {
  cvub::SuffixPipelineOptions suffixOptions;
  suffixOptions.enableAutoMultiBuffer = opts.enableAutoMultiBuffer;
  suffixOptions.limitAutoMultiBufferOfLocalBuffer =
      opts.localMultiBufferStrategy;
  suffixOptions.limitMixAutoMultiBufferBuffer =
      opts.mixMultiBufferStrategy;
  return suffixOptions;
}

std::string actionOracle(const std::string &action) {
  if (action == "plan-c1-suffix")
    return "c8_bridge_plus_planmemory";
  if (action == "plan-before-cvpipeline")
    return "d1_cvpipelining_plus_c_suffix_plus_planmemory";
  return "minimal_suffix";
}

std::string actionPrecision(const std::string &action) {
  if (action == "plan-c1-suffix")
    return "initial_c_model";
  if (action == "plan-before-cvpipeline")
    return "initial_d_model";
  return "exact_plan";
}

Options parseOptions(int argc, char **argv) {
  Options opts;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto readValue = [&](const std::string &name) -> std::optional<std::string> {
      if (cvub::startsWith(arg, name + "="))
        return arg.substr(name.size() + 1);
      if (arg == name && i + 1 < argc)
        return std::string(argv[++i]);
      return std::nullopt;
    };
    if (auto actionValue = readValue("--action"))
      opts.action = *actionValue;
    else if (auto beforeIRValue = readValue("--before-planmemory-ir"))
      opts.beforePlanMemoryIR = *beforeIRValue;
    else if (auto c1IRValue = readValue("--c1-generic-ir"))
      opts.c1GenericIR = *c1IRValue;
    else if (auto beforeCVValue = readValue("--before-cvpipeline-ir"))
      opts.beforeCVPipelineIR = *beforeCVValue;
    else if (auto seedValue = readValue("--random-seed"))
      opts.randomSeed = parseRandomSeed(*seedValue);
    else if (auto formatValue = readValue("--format"))
      opts.format = *formatValue;
    else if (arg == "--restrict-inplace-as-isa")
      opts.restrictInplaceAsISA = true;
    else if (arg == "--disable-cv-pipelining")
      opts.disableCVPipelining = true;
    else if (auto disabled = readValue("--disable-cv-pipelining"))
      opts.disableCVPipelining = parseBool(*disabled);
    else if (auto depth = readValue("--cv-pipeline-depth"))
      opts.cvPipelineDepth = std::stoi(*depth);
    else if (arg == "--enable-preload")
      opts.enablePreload = true;
    else if (auto preload = readValue("--enable-preload"))
      opts.enablePreload = parseBool(*preload);
    else if (arg == "--enable-cv-lazy-loading")
      opts.enableCVLazyLoading = true;
    else if (auto lazy = readValue("--enable-cv-lazy-loading"))
      opts.enableCVLazyLoading = parseBool(*lazy);
    else if (arg == "--enable-auto-multi-buffer")
      opts.enableAutoMultiBuffer = true;
    else if (auto autoMultiBuffer = readValue("--enable-auto-multi-buffer"))
      opts.enableAutoMultiBuffer = parseBool(*autoMultiBuffer);
    else if (auto localStrategy = readValue(
                 "--limit-auto-multi-buffer-of-local-buffer"))
      opts.localMultiBufferStrategy =
          cvub::ParseMultiBufferStrategy(*localStrategy);
    else if (auto mixStrategy =
                 readValue("--limit-auto-multi-buffer-buffer"))
      opts.mixMultiBufferStrategy =
          cvub::ParseMultiBufferStrategy(*mixStrategy);
    else if (arg == "--help" || arg == "-h") {
      std::cout
          << "Usage: cvpipeline_ub_model "
             "--action=<analyze-lifetimes|dump-liveness-state|"
             "plan-local-memory|plan-c1-suffix|plan-before-cvpipeline> "
             "[options]\n";
      std::exit(0);
    } else {
      std::cerr << "[ERROR] unknown option: " << arg << "\n";
      std::exit(1);
    }
  }
  return opts;
}

std::string joinGroup(const std::vector<std::string> &values) {
  std::ostringstream out;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i != 0)
      out << ",";
    out << values[i];
  }
  return out.str();
}

std::string jsonEscape(const std::string &value) {
  std::string result;
  for (char c : value) {
    switch (c) {
    case '\\':
      result += "\\\\";
      break;
    case '"':
      result += "\\\"";
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
    default: {
      const unsigned char byte = static_cast<unsigned char>(c);
      if (byte < 0x20) {
        static constexpr char hex[] = "0123456789abcdef";
        result += "\\u00";
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 0x0f]);
      } else {
        result.push_back(c);
      }
      break;
    }
    }
  }
  return result;
}

int analyzeLifetimes(const Options &opts) {
  if (opts.beforePlanMemoryIR.empty()) {
    std::cerr << "[ERROR] --before-planmemory-ir is required\n";
    return 1;
  }
  cvub::LifetimeAnalysis analysis =
      cvub::analyzeLifetimes(opts.beforePlanMemoryIR, "AIV",
                             opts.randomSeed.value_or(0),
                             opts.restrictInplaceAsISA);
  std::cout << "before_ir\t" << opts.beforePlanMemoryIR << "\n";
  std::cout << "operation_count\t" << analysis.operations.size() << "\n";
  std::cout << "lifetime_count\t" << analysis.records.size() << "\n";
  std::cout << "random_seed\t" << opts.randomSeed.value_or(0) << "\n";
  std::cout << "restrict_inplace_as_isa\t"
            << (opts.restrictInplaceAsISA ? "true" : "false") << "\n";
  std::cout
      << "name\talloc_time\tfree_time\tdirect_alloc_time\tdirect_free_time\t"
         "group\n";
  for (const cvub::LifetimeRecord &record : analysis.records) {
    std::cout << record.name << "\t" << record.allocTime << "\t"
              << record.freeTime << "\t" << record.directAllocTime << "\t"
              << record.directFreeTime << "\t" << joinGroup(record.group)
              << "\n";
  }
  return 0;
}

int dumpLivenessState(const Options &opts) {
  if (opts.beforePlanMemoryIR.empty()) {
    std::cerr << "[ERROR] --before-planmemory-ir is required\n";
    return 1;
  }
  cvub::LifetimeAnalysis analysis =
      cvub::analyzeLifetimes(opts.beforePlanMemoryIR, "AIV",
                             opts.randomSeed.value_or(0),
                             opts.restrictInplaceAsISA);
  const uint32_t randomSeed = opts.randomSeed.value_or(0);
  std::cout << "PLANMEM_LIVENESS_ATTEMPT\t" << randomSeed << "\n";
  for (const cvub::GenKillRecord &entry : analysis.genKillMap) {
    for (const std::string &value : entry.gen)
      std::cout << "PLANMEM_GEN\t" << entry.operationIndex << "\t" << value
                << "\n";
    for (const std::string &value : entry.kill)
      std::cout << "PLANMEM_KILL\t" << entry.operationIndex << "\t" << value
                << "\n";
  }
  for (const cvub::LiveShuffleRecord &trace : analysis.liveShuffleTrace) {
    std::cout << "PLANMEM_LIVE_BEFORE\t" << trace.operationIndex;
    for (const std::string &value : trace.before)
      std::cout << "\t" << value;
    std::cout << "\nPLANMEM_LIVE_AFTER\t" << trace.operationIndex;
    for (const std::string &value : trace.after)
      std::cout << "\t" << value;
    std::cout << "\n";
  }
  for (const cvub::LifetimeRecord &record : analysis.records)
    std::cout << "PLANMEM_LIFETIME\t" << record.name << "\t"
              << record.directAllocTime << "\t" << record.directFreeTime
              << "\n";
  for (const auto &alias : analysis.canonicalAllocByValue)
    std::cout << "PLANMEM_ALIAS\t" << alias.first << "\t" << alias.second
              << "\n";
  for (const auto &pair : analysis.inplacePairList)
    std::cout << "PLANMEM_INPLACE\t" << pair.first << "\t" << pair.second
              << "\n";
  std::cout << "PLANMEM_LIVENESS_ATTEMPT_END\t" << randomSeed << "\n";
  return 0;
}

int planLocalMemory(const Options &opts) {
  const bool fromSuffix = opts.action == "plan-c1-suffix";
  const bool fromBeforeCVPipeline = opts.action == "plan-before-cvpipeline";
  if ((!fromSuffix && !fromBeforeCVPipeline && opts.beforePlanMemoryIR.empty()) ||
      (fromSuffix && opts.c1GenericIR.empty()) ||
      (fromBeforeCVPipeline && opts.beforeCVPipelineIR.empty())) {
    std::cerr << "[ERROR] "
              << (fromBeforeCVPipeline
                      ? "--before-cvpipeline-ir"
                      : (fromSuffix ? "--c1-generic-ir"
                                    : "--before-planmemory-ir"))
              << " is required\n";
    return 1;
  }
  if (opts.format != "text" && opts.format != "json")
    throw std::runtime_error("--format must be text or json");
  cvub::PlanMemoryModelResult result;
  if (fromBeforeCVPipeline) {
    cvub::C1SemanticModule module =
        cvub::ParseC1GenericIR(opts.beforeCVPipelineIR, false);
    for (cvub::C1OperationRecord &operation : module.operations)
      if (cvub::C1IsReviewedOperation(operation.name))
        cvub::ApplyC1OpSemantics(operation);
    module = cvub::RunCVPipeliningPass(std::move(module),
                                       cvPipeliningOptions(opts));
    const cvub::PlanMemoryInput input =
        cvub::BuildSuffixPlanMemoryInput(std::move(module),
                                         suffixPipelineOptions(opts));
    result = opts.randomSeed
                 ? cvub::PlanLocalMemoryForSeed(
                       input, *opts.randomSeed, opts.restrictInplaceAsISA)
                 : cvub::PlanLocalMemory(input, opts.restrictInplaceAsISA);
  } else if (fromSuffix) {
    const cvub::PlanMemoryInput input =
        cvub::BuildSuffixPlanMemoryInput(opts.c1GenericIR,
                                         suffixPipelineOptions(opts));
    result = opts.randomSeed
                 ? cvub::PlanLocalMemoryForSeed(
                       input, *opts.randomSeed, opts.restrictInplaceAsISA)
                 : cvub::PlanLocalMemory(input, opts.restrictInplaceAsISA);
  } else {
    result = opts.randomSeed
                 ? cvub::PlanLocalMemoryForSeed(
                       opts.beforePlanMemoryIR, *opts.randomSeed,
                       opts.restrictInplaceAsISA)
                 : cvub::PlanLocalMemory(opts.beforePlanMemoryIR,
                                         opts.restrictInplaceAsISA);
  }
  const std::string precision = actionPrecision(opts.action);
  const std::string oracle = actionOracle(opts.action);
  if (opts.format == "json") {
    std::cout << "{\n"
              << "  \"precision\": \"" << precision << "\",\n"
              << "  \"oracle\": \"" << oracle << "\",\n"
              << "  \"status\": \""
              << (result.success ? "success" : "overflow") << "\",\n"
              << "  \"ub_peak_bits\": ";
    if (result.success)
      std::cout << result.peakBits;
    else
      std::cout << "null";
    std::cout << ",\n  \"required_bits\": ";
    if (result.overflow)
      std::cout << result.requiredBits;
    else
      std::cout << "null";
    std::cout << ",\n"
              << "  \"capacity_bits\": " << result.capacityBits << ",\n"
              << "  \"restrict_inplace_as_isa\": "
              << (opts.restrictInplaceAsISA ? "true" : "false") << ",\n"
              << "  \"overflow\": "
              << (result.overflow ? "true" : "false") << ",\n"
              << "  \"selected_seed\": " << result.selectedSeed << ",\n"
              << "  \"blockers\": []\n"
              << "}\n";
    return result.success ? 0 : 2;
  }
  std::cout << "precision\t" << precision << "\n";
  std::cout << "success\t" << (result.success ? "true" : "false") << "\n";
  std::cout << "overflow\t" << (result.overflow ? "true" : "false") << "\n";
  std::cout << "selected_seed\t" << result.selectedSeed << "\n";
  std::cout << "restrict_inplace_as_isa\t"
            << (opts.restrictInplaceAsISA ? "true" : "false") << "\n";
  std::cout << "peak_bits\t" << result.peakBits << "\n";
  std::cout << "required_bits\t" << result.requiredBits << "\n";
  std::cout << "capacity_bits\t" << result.capacityBits << "\n";
  std::cout << "name\textent_bits\toffset_bytes\talloc_time\tfree_time\n";
  for (const cvub::PlannedBufferRecord &record : result.buffers) {
    for (uint64_t offset : record.offsetsBytes)
      std::cout << record.name << "\t" << record.extentBits << "\t"
                << offset << "\t" << record.allocTime << "\t"
                << record.freeTime << "\n";
  }
  return result.success ? 0 : 2;
}

} // namespace

int main(int argc, char **argv) {
  Options opts;
  try {
    opts = parseOptions(argc, argv);
    if (opts.action == "analyze-lifetimes")
      return analyzeLifetimes(opts);
    if (opts.action == "dump-liveness-state")
      return dumpLivenessState(opts);
    if (opts.action == "plan-local-memory")
      return planLocalMemory(opts);
    if (opts.action == "plan-c1-suffix")
      return planLocalMemory(opts);
    if (opts.action == "plan-before-cvpipeline")
      return planLocalMemory(opts);
    std::cerr << "[ERROR] unknown or missing --action: " << opts.action << "\n";
    return 1;
  } catch (const std::exception &ex) {
    if (opts.format == "json") {
      const std::string oracle = actionOracle(opts.action);
      std::cout << "{\n"
                << "  \"precision\": \"blocked\",\n"
                << "  \"oracle\": \"" << oracle << "\",\n"
                << "  \"status\": \"blocker\",\n"
                << "  \"ub_peak_bits\": null,\n"
                << "  \"required_bits\": null,\n"
                << "  \"capacity_bits\": " << cvub::kUBCapacityBits
                << ",\n"
                << "  \"overflow\": null,\n"
                << "  \"blockers\": [\"" << jsonEscape(ex.what())
                << "\"]\n"
                << "}\n";
      std::cerr << "[ERROR] " << ex.what() << "\n";
      return 1;
    }
    std::cerr << "[ERROR] " << ex.what() << "\n";
    return 1;
  }
}
