// Final-facing lightweight CVPipeline UB model entry.
//
// Development validation is intentionally kept out of this binary; see
// dev_validate.cpp for oracle comparison checks used during implementation.

#include "../cvpipeline/cvpipelining_pass.hpp"
#include "../model_core.hpp"
#include "../post_cvpipeline/pipeline.hpp"
#include "../post_cvpipeline/planning.hpp"
#include "../suffix/suffix_pipeline.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
struct Options {
  std::string action;
  std::string beforePlanMemoryIR;
  std::string beforeOneShotBufferizeIR;
  std::string beforeCVPipelineIR;
  std::optional<uint32_t> randomSeed;
  bool restrictInplaceAsISA = false;
  bool disableCVPipelining = false;
  int cvPipelineDepth = -1;
  bool enablePreload = false;
  bool enableCVLazyLoading = false;
  bool enableAutoMultiBuffer = false;
  cvub::MultiBufferStrategy localMultiBufferStrategy =
      cvub::MultiBufferStrategy::CubeNoL0C;
  cvub::MultiBufferStrategy mixMultiBufferStrategy =
      cvub::MultiBufferStrategy::OnlyCube;
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
  if (action == "plan-before-one-shot-bufferize")
    return "suffix_pipeline_plus_planmemory";
  if (action == "plan-before-cvpipeline")
    return "cvpipelining_plus_suffix_pipeline_plus_planmemory";
  return "minimal_suffix";
}

std::string actionPrecision(const std::string &action) {
  if (action == "plan-before-one-shot-bufferize")
    return "before_one_shot_bufferize_model";
  if (action == "plan-before-cvpipeline")
    return "before_cvpipeline_model";
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
    else if (auto beforeOneShotValue =
                 readValue("--before-one-shot-bufferize-ir"))
      opts.beforeOneShotBufferizeIR = *beforeOneShotValue;
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
             "plan-local-memory|plan-before-one-shot-bufferize|"
             "plan-before-cvpipeline> "
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

const char *boolStr(bool value) { return value ? "true" : "false"; }

std::string precisionString(cvub::Precision precision) {
  return precision == cvub::Precision::Exact ? "exact" : "incomplete";
}

std::string coverageDispositionName(cvub::CoverageDisposition disposition) {
  switch (disposition) {
  case cvub::CoverageDisposition::OracleExact:
    return "oracle-exact";
  case cvub::CoverageDisposition::Partial:
    return "partial";
  case cvub::CoverageDisposition::UBInvariant:
    return "ub-invariant";
  case cvub::CoverageDisposition::Unsupported:
    return "unsupported";
  }
  return "unsupported";
}

// Planning status before the exactness gate: a successful module is "success";
// a module with any overflowing function is "overflow"; anything else is a
// "blocker". plan-before-cvpipeline applies the exactness gate separately and
// turns every Incomplete result into a non-authoritative blocker.
std::string moduleStatus(const cvub::ModulePlanResult &result) {
  if (result.success)
    return "success";
  if (result.overflow)
    return "overflow";
  return "blocker";
}

std::string functionStatus(const cvub::FunctionPlanResult &function) {
  if (function.plan.success)
    return "success";
  if (function.plan.overflow)
    return "overflow";
  return "blocker";
}

int exitCodeForStatus(const std::string &status) {
  if (status == "success")
    return 0;
  if (status == "overflow")
    return 2;
  return 1;
}

cvub::Precision combinedPrecision(const cvub::PostCVPipelineResult &projected,
                                  const cvub::ModulePlanResult &planned) {
  if (projected.precision == cvub::Precision::Incomplete ||
      planned.precision == cvub::Precision::Incomplete)
    return cvub::Precision::Incomplete;
  return cvub::Precision::Exact;
}

// The five real PostCVPipelineOptions defaults the model assumes (the CLI does
// not expose post-CVPipeline knobs, so every run uses these defaults).
void emitAssumedPostCVPipelineOptionsJson(std::ostream &out) {
  out << "    \"tile_mix_vector_loop\": 2,\n"
      << "    \"tile_mix_cube_loop\": 2,\n"
      << "    \"enable_auto_bind_sub_block\": true,\n"
      << "    \"enable_code_motion\": true,\n"
      << "    \"enable_ubuf_saving\": false\n";
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
  const bool fromSuffix = opts.action == "plan-before-one-shot-bufferize";
  if ((!fromSuffix && opts.beforePlanMemoryIR.empty()) ||
      (fromSuffix && opts.beforeOneShotBufferizeIR.empty())) {
    std::cerr << "[ERROR] "
              << (fromSuffix ? "--before-one-shot-bufferize-ir"
                             : "--before-planmemory-ir")
              << " is required\n";
    return 1;
  }
  if (opts.format != "text" && opts.format != "json")
    throw std::runtime_error("--format must be text or json");
  cvub::PlanMemoryModelResult result;
  if (fromSuffix) {
    const cvub::PlanMemoryInput input =
        cvub::BuildSuffixPlanMemoryInput(opts.beforeOneShotBufferizeIR,
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
      std::cout << record.name << "\t" << record.constBits << "\t"
                << offset << "\t" << record.allocTime << "\t"
                << record.freeTime << "\n";
  }
  return result.success ? 0 : 2;
}

// Emits the per-function CVPipeline UB plan for the `plan-before-cvpipeline`
// action.  Replaces the historical direct CVPipeline -> suffix bridge call with
// the post-CVPipeline AIV projection + per-function planning chain, then reports
// module and per-function results as stable JSON or text.
int planBeforeCVPipeline(const Options &opts) {
  if (opts.beforeCVPipelineIR.empty()) {
    std::cerr << "[ERROR] --before-cvpipeline-ir is required\n";
    return 1;
  }
  if (opts.format != "text" && opts.format != "json")
    throw std::runtime_error("--format must be text or json");

  cvub::GenericModule module =
      cvub::ParseGenericIR(opts.beforeCVPipelineIR, false);
  for (cvub::GenericOperation &operation : module.operations)
    if (cvub::HasModeledOperationSemantics(operation.name))
      cvub::ApplyOperationSemantics(operation);
  module = cvub::RunCVPipeliningPass(std::move(module),
                                     cvPipeliningOptions(opts));
  const cvub::PostCVPipelineResult projected =
      cvub::RunPostCVPipelineAIVProjection(std::move(module),
                                           cvub::PostCVPipelineOptions{});
  const cvub::ModulePlanResult result = cvub::PlanProjectedAIVFunctions(
      projected, suffixPipelineOptions(opts), opts.randomSeed,
      opts.restrictInplaceAsISA);

  const cvub::Precision precision = combinedPrecision(projected, result);
  const bool authoritative = precision == cvub::Precision::Exact;
  const std::string estimateStatus = moduleStatus(result);
  const std::string status = authoritative ? estimateStatus : "blocker";
  // Module UB peak is the max of per-function raw simultaneous peaks: projected
  // functions execute independently, so the module never sees their peaks sum.
  // required_bits is the module effective requirement (max of per-function
  // effective requirements) computed by PlanProjectedAIVFunctions.
  uint64_t modulePeakBits = 0;
  for (const cvub::FunctionPlanResult &function : result.functions)
    modulePeakBits = std::max(modulePeakBits, function.plan.peakBits);
  // Top-level seed is reported only when every planned function settled on the
  // same retry seed; otherwise null, so the report never invents a single
  // global seed for independently-planned functions.
  std::optional<uint32_t> moduleSeed;
  if (!result.functions.empty()) {
    const uint32_t first = result.functions.front().plan.selectedSeed;
    bool agrees = true;
    for (const cvub::FunctionPlanResult &function : result.functions)
      if (function.plan.selectedSeed != first) {
        agrees = false;
        break;
      }
    if (agrees)
      moduleSeed = first;
  }
  // Diagnostics combine post-CVPipeline stage diagnostics (why precision is
  // incomplete) with per-function planning diagnostics (e.g. one-AIV invariant).
  std::vector<cvub::PostCVPipelineDiagnostic> diagnostics = projected.diagnostics;
  diagnostics.insert(diagnostics.end(), result.diagnostics.begin(),
                     result.diagnostics.end());

  if (opts.format == "json") {
    std::cout << "{\n"
              << "  \"precision\": \"" << precisionString(precision) << "\",\n"
              << "  \"status\": \"" << status << "\",\n"
              << "  \"success\": "
              << boolStr(authoritative && result.success) << ",\n"
              << "  \"overflow\": "
              << boolStr(authoritative && result.overflow) << ",\n"
              << "  \"ub_peak_bits\": ";
    if (status == "blocker")
      std::cout << "null";
    else
      std::cout << modulePeakBits;
    std::cout << ",\n  \"required_bits\": ";
    if (status == "blocker")
      std::cout << "null";
    else
      std::cout << result.requiredBits;
    std::cout << ",\n"
              << "  \"capacity_bits\": " << result.capacityBits << ",\n"
              << "  \"restrict_inplace_as_isa\": "
              << boolStr(opts.restrictInplaceAsISA) << ",\n"
              << "  \"selected_seed\": ";
    if (authoritative && moduleSeed)
      std::cout << *moduleSeed;
    else
      std::cout << "null";
    std::cout << ",\n"
              << "  \"assumed_post_cvpipeline_options\": {\n";
    emitAssumedPostCVPipelineOptionsJson(std::cout);
    std::cout << "  },\n";

    std::cout << "  \"stage_coverage\": [\n";
    for (size_t i = 0; i < projected.coverage.size(); ++i) {
      const cvub::StageCoverage &stage = projected.coverage[i];
      std::cout << "    {\"stage\": \"" << jsonEscape(stage.stage)
                << "\", \"disposition\": \""
                << coverageDispositionName(stage.disposition) << "\"}";
      std::cout << (i + 1 == projected.coverage.size() ? "\n" : ",\n");
    }
    std::cout << "  ],\n";

    std::cout << "  \"functions\": [\n";
    const size_t authoritativeFunctionCount =
        authoritative ? result.functions.size() : 0;
    for (size_t i = 0; i < authoritativeFunctionCount; ++i) {
      const cvub::FunctionPlanResult &function = result.functions[i];
      std::cout << "    {\n"
                << "      \"source_function\": \""
                << jsonEscape(function.sourceFunction) << "\",\n"
                << "      \"function\": \"" << jsonEscape(function.function)
                << "\",\n"
                << "      \"status\": \"" << functionStatus(function)
                << "\",\n"
                << "      \"success\": " << boolStr(function.plan.success)
                << ",\n"
                << "      \"overflow\": " << boolStr(function.plan.overflow)
                << ",\n"
                << "      \"ub_peak_bits\": " << function.plan.peakBits
                << ",\n"
                << "      \"required_bits\": " << function.plan.requiredBits
                << ",\n"
                << "      \"selected_seed\": " << function.plan.selectedSeed
                << ",\n"
                << "      \"buffers\": [\n";
      bool firstBuffer = true;
      for (const cvub::PlannedBufferRecord &record : function.plan.buffers) {
        for (uint64_t offset : record.offsetsBytes) {
          if (!firstBuffer)
            std::cout << ",\n";
          firstBuffer = false;
          std::cout << "        {\"name\": \"" << jsonEscape(record.name)
                    << "\", \"extent_bits\": " << record.constBits
                    << ", \"offset_bytes\": " << offset
                    << ", \"multi_buffer_num\": "
                    << (function.plan.multiBufferNums.count(record.name)
                            ? function.plan.multiBufferNums.at(record.name)
                            : 1U)
                    << ", \"alloc_time\": " << record.allocTime
                    << ", \"free_time\": " << record.freeTime << "}";
        }
      }
      if (!firstBuffer)
        std::cout << "\n      ";
      std::cout << "],\n      \"inplace_pairs\": [";
      for (size_t pairIndex = 0;
           pairIndex < function.plan.inplacePairs.size(); ++pairIndex) {
        if (pairIndex != 0)
          std::cout << ", ";
        const auto &pair = function.plan.inplacePairs[pairIndex];
        std::cout << "[\"" << jsonEscape(pair.first) << "\", \""
                  << jsonEscape(pair.second) << "\"]";
      }
      std::cout << "]\n"
                << "    }";
      std::cout << (i + 1 == authoritativeFunctionCount ? "\n" : ",\n");
    }
    std::cout << "  ],\n";

    std::cout << "  \"diagnostics\": [\n";
    for (size_t i = 0; i < diagnostics.size(); ++i) {
      const cvub::PostCVPipelineDiagnostic &diagnostic = diagnostics[i];
      std::cout << "    {\"pipeline_stage\": \""
                << jsonEscape(diagnostic.pipelineStage) << "\", \"function\": \""
                << jsonEscape(diagnostic.function) << "\", \"operation_id\": "
                << diagnostic.operationId << ", \"operation\": \""
                << jsonEscape(diagnostic.operation) << "\", \"reason\": \""
                << jsonEscape(diagnostic.reason) << "\"}";
      std::cout << (i + 1 == diagnostics.size() ? "\n" : ",\n");
    }
    std::cout << "  ],\n";
    std::cout << "  \"debug_estimate\": ";
    if (authoritative) {
      std::cout << "null\n";
    } else {
      std::cout << "{\n"
                << "    \"status\": \"" << estimateStatus << "\",\n"
                << "    \"ub_peak_bits\": " << modulePeakBits << ",\n"
                << "    \"required_bits\": " << result.requiredBits << ",\n"
                << "    \"selected_seed\": ";
      if (moduleSeed)
        std::cout << *moduleSeed;
      else
        std::cout << "null";
      std::cout << ",\n    \"functions\": [\n";
      for (size_t i = 0; i < result.functions.size(); ++i) {
        const cvub::FunctionPlanResult &function = result.functions[i];
        std::cout << "      {\"source_function\": \""
                  << jsonEscape(function.sourceFunction)
                  << "\", \"function\": \"" << jsonEscape(function.function)
                  << "\", \"status\": \"" << functionStatus(function)
                  << "\", \"ub_peak_bits\": " << function.plan.peakBits
                  << ", \"required_bits\": " << function.plan.requiredBits
                  << ", \"selected_seed\": " << function.plan.selectedSeed
                  << ", \"buffers\": [";
        bool first = true;
        for (const cvub::PlannedBufferRecord &record : function.plan.buffers)
          for (uint64_t offset : record.offsetsBytes) {
            if (!first)
              std::cout << ", ";
            first = false;
            std::cout << "{\"name\": \"" << jsonEscape(record.name)
                      << "\", \"extent_bits\": " << record.constBits
                      << ", \"offset_bytes\": " << offset
                      << ", \"multi_buffer_num\": "
                      << (function.plan.multiBufferNums.count(record.name)
                              ? function.plan.multiBufferNums.at(record.name)
                              : 1U)
                      << ", \"alloc_time\": " << record.allocTime
                      << ", \"free_time\": " << record.freeTime << "}";
          }
        std::cout << "]}" << (i + 1 == result.functions.size() ? "\n" : ",\n");
      }
      std::cout << "    ]\n  }\n";
    }
    std::cout << "}\n";
    return exitCodeForStatus(status);
  }

  std::cout << "precision\t" << precisionString(precision) << "\n";
  std::cout << "status\t" << status << "\n";
  std::cout << "success\t" << boolStr(authoritative && result.success) << "\n";
  std::cout << "overflow\t" << boolStr(authoritative && result.overflow) << "\n";
  std::cout << "selected_seed\t"
            << (authoritative && moduleSeed ? std::to_string(*moduleSeed)
                                            : std::string())
            << "\n";
  std::cout << "restrict_inplace_as_isa\t" << boolStr(opts.restrictInplaceAsISA)
            << "\n";
  std::cout << "peak_bits\t"
            << (authoritative ? std::to_string(modulePeakBits) : std::string())
            << "\n";
  std::cout << "required_bits\t"
            << (authoritative ? std::to_string(result.requiredBits)
                              : std::string())
            << "\n";
  std::cout << "capacity_bits\t" << result.capacityBits << "\n";
  std::cout << "function_count\t"
            << (authoritative ? result.functions.size() : 0) << "\n";
  std::cout << "diagnostic_count\t" << diagnostics.size() << "\n";
  std::cout << "assumed_post_cvpipeline_options.tile_mix_vector_loop\t2\n";
  std::cout << "assumed_post_cvpipeline_options.tile_mix_cube_loop\t2\n";
  std::cout << "assumed_post_cvpipeline_options.enable_auto_bind_sub_block\ttrue\n";
  std::cout << "assumed_post_cvpipeline_options.enable_code_motion\ttrue\n";
  std::cout << "assumed_post_cvpipeline_options.enable_ubuf_saving\tfalse\n";
  std::cout << "function\tname\textent_bits\toffset_bytes\talloc_time\tfree_time\n";
  if (!authoritative) {
    std::cout << "debug_estimate.status\t" << estimateStatus << "\n";
    std::cout << "debug_estimate.peak_bits\t" << modulePeakBits << "\n";
    std::cout << "debug_estimate.required_bits\t" << result.requiredBits << "\n";
  }
  if (authoritative)
    for (const cvub::FunctionPlanResult &function : result.functions) {
    for (const cvub::PlannedBufferRecord &record : function.plan.buffers) {
      for (uint64_t offset : record.offsetsBytes)
        std::cout << function.function << "\t" << record.name << "\t"
                  << record.constBits << "\t" << offset << "\t"
                  << record.allocTime << "\t" << record.freeTime << "\n";
    }
    }
  return exitCodeForStatus(status);
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
    if (opts.action == "plan-before-one-shot-bufferize")
      return planLocalMemory(opts);
    if (opts.action == "plan-before-cvpipeline")
      return planBeforeCVPipeline(opts);
    std::cerr << "[ERROR] unknown or missing --action: " << opts.action << "\n";
    return 1;
  } catch (const std::exception &ex) {
    if (opts.format == "json") {
      if (opts.action == "plan-before-cvpipeline") {
        std::cout << "{\n"
                  << "  \"precision\": \"incomplete\",\n"
                  << "  \"status\": \"blocker\",\n"
                  << "  \"success\": false,\n"
                  << "  \"overflow\": false,\n"
                  << "  \"ub_peak_bits\": null,\n"
                  << "  \"required_bits\": null,\n"
                  << "  \"capacity_bits\": " << cvub::kUBCapacityBits
                  << ",\n"
                  << "  \"restrict_inplace_as_isa\": "
                  << boolStr(opts.restrictInplaceAsISA) << ",\n"
                  << "  \"selected_seed\": null,\n"
                  << "  \"assumed_post_cvpipeline_options\": {\n";
        emitAssumedPostCVPipelineOptionsJson(std::cout);
        std::cout << "  },\n"
                  << "  \"stage_coverage\": [],\n"
                  << "  \"functions\": [],\n"
                  << "  \"diagnostics\": [\n"
                  << "    {\"pipeline_stage\": \"cli\", \"function\": \"\", "
                     "\"operation_id\": -1, \"operation\": \"\", \"reason\": \""
                  << jsonEscape(ex.what()) << "\"}\n"
                  << "  ]\n"
                  << "}\n";
      } else {
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
      }
      std::cerr << "[ERROR] " << ex.what() << "\n";
      return 1;
    }
    std::cerr << "[ERROR] " << ex.what() << "\n";
    return 1;
  }
}
