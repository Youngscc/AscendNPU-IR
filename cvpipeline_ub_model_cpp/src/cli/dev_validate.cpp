// Development-only oracle validation entry.
//
// This binary is for implementation workflow checks. It is not the final model
// interface.

#include "../semantic_ir/c1_semantic_ir.hpp"
#include "../cvpipeline/cvpipelining.hpp"
#include "../cvpipeline/cvpipelining_analysis.hpp"
#include "../cvpipeline/cvpipelining_pass.hpp"
#include "../cvpipeline/cvpipelining_preload_rewrite.hpp"
#include "../cvpipeline/cvpipelining_rewrite.hpp"
#include "../suffix/buffer_topology.hpp"
#include "../suffix/bufferized_semantic_ir.hpp"
#include "../suffix/one_shot_bufferize.hpp"
#include "../suffix/one_shot_analysis.hpp"
#include "../suffix/alloc_extra_buffer.hpp"
#include "../suffix/hivm_decompose_op.hpp"
#include "../suffix/c3_semantic_ir.hpp"
#include "../suffix/c4_semantic_ir.hpp"
#include "../suffix/c5_semantic_ir.hpp"
#include "../suffix/c6_semantic_ir.hpp"
#include "../suffix/planmemory_input_semantic_ir.hpp"
#include "../suffix/planmemory_bridge.hpp"
#include "../validation/memory_info_replay.hpp"
#include "../validation/bufferized_semantic_ir_oracle.hpp"
#include "../validation/hivm_decompose_op_oracle.hpp"
#include "../validation/c3_semantic_ir_oracle.hpp"
#include "../validation/c4_semantic_ir_oracle.hpp"
#include "../validation/c5_semantic_ir_oracle.hpp"
#include "../validation/c6_semantic_ir_oracle.hpp"
#include "../validation/planmemory_input_semantic_ir_oracle.hpp"

#include <cctype>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {
using TsvRow = std::map<std::string, std::string>;

struct Options {
  std::string action;
  std::string root;
  std::string expectedSummary;
  std::string attemptRoot;
  std::string oracleSummary;
  std::string output;
  std::string snapshotRoot;
  std::string afterIr;
  std::string function;
  bool quiet = false;
  bool tryAllSeeds = false;
  bool enableAutoMultiBuffer = false;
  bool disableCVPipelining = false;
  bool enablePreload = false;
  bool enableCVLazyLoading = false;
  int cvPipelineDepth = -1;
  std::string localMultiBufferStrategy = "no-l0c";
  std::string mixMultiBufferStrategy = "only-cube";
  int randomSeed = -1;
  int expectedRuns = 3320;
};

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
    else if (auto rootValue = readValue("--root"))
      opts.root = *rootValue;
    else if (auto summaryValue = readValue("--expected-summary"))
      opts.expectedSummary = *summaryValue;
    else if (auto attemptRootValue = readValue("--attempt-root"))
      opts.attemptRoot = *attemptRootValue;
    else if (auto oracleSummaryValue = readValue("--oracle-summary"))
      opts.oracleSummary = *oracleSummaryValue;
    else if (auto outputValue = readValue("--output"))
      opts.output = *outputValue;
    else if (auto snapshotRootValue = readValue("--snapshot-root"))
      opts.snapshotRoot = *snapshotRootValue;
    else if (auto afterIrValue = readValue("--after-ir"))
      opts.afterIr = *afterIrValue;
    else if (auto functionValue = readValue("--function"))
      opts.function = *functionValue;
    else if (arg == "--quiet")
      opts.quiet = true;
    else if (arg == "--try-all-seeds")
      opts.tryAllSeeds = true;
    else if (arg == "--enable-auto-multi-buffer")
      opts.enableAutoMultiBuffer = true;
    else if (arg == "--disable-cv-pipelining")
      opts.disableCVPipelining = true;
    else if (auto disabled = readValue("--disable-cv-pipelining"))
      opts.disableCVPipelining = *disabled == "1" || *disabled == "true";
    else if (arg == "--enable-preload")
      opts.enablePreload = true;
    else if (auto preload = readValue("--enable-preload"))
      opts.enablePreload = *preload == "1" || *preload == "true";
    else if (arg == "--enable-cv-lazy-loading")
      opts.enableCVLazyLoading = true;
    else if (auto lazy = readValue("--enable-cv-lazy-loading"))
      opts.enableCVLazyLoading = *lazy == "1" || *lazy == "true";
    else if (auto depth = readValue("--cv-pipeline-depth"))
      opts.cvPipelineDepth = std::stoi(*depth);
    else if (auto localStrategy =
                 readValue("--limit-auto-multi-buffer-of-local-buffer"))
      opts.localMultiBufferStrategy = *localStrategy;
    else if (auto mixStrategy = readValue("--limit-auto-multi-buffer-buffer"))
      opts.mixMultiBufferStrategy = *mixStrategy;
    else if (auto seedValue = readValue("--random-seed"))
      opts.randomSeed = std::stoi(*seedValue);
    else if (auto expectedRunsValue = readValue("--expected-runs"))
      opts.expectedRuns = std::stoi(*expectedRunsValue);
    else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: cvpipeline_ub_model_dev_validate "
                   "--action=<validate-memory-info-replay|"
                   "validate-local-ub-allocations|validate-lifetimes|"
                   "validate-planmemory-attempts|validate-memory-plan|"
                   "validate-fixed-seed-lifetimes|"
                   "validate-fixed-seed-plans|dump-c1-semantic-ir|"
                   "dump-generic-semantic-ir|"
                   "dump-suffix-buffer-topology|dump-c2-allocation-oracle|"
                   "model-one-shot-bufferize-allocations|"
                   "model-bufferized-semantic-ir|"
                   "validate-bufferized-semantic-ir|"
                   "dump-one-shot-analysis-oracle|model-one-shot-analysis|"
                   "dump-alloc-extra-buffer-oracle|"
                   "model-alloc-extra-buffer|dump-hivm-decompose-op-oracle|"
                   "model-hivm-decompose-op|"
                   "dump-hivm-decompose-operation-delta-oracle|"
                   "model-hivm-decompose-operation-delta|model-c3-semantic-ir> "
                   "[--snapshot-root=DIR] [options]\n";
      std::exit(0);
    } else {
      std::cerr << "[ERROR] unknown option: " << arg << "\n";
      std::exit(1);
    }
  }
  return opts;
}

std::map<std::string, TsvRow> readExpectedSummary(const cvub::fs::path &path) {
  std::ifstream in(path);
  std::map<std::string, TsvRow> rows;
  std::string headerLine;
  if (!std::getline(in, headerLine))
    return rows;
  std::vector<std::string> header = cvub::split(headerLine, '\t');
  std::string line;
  while (std::getline(in, line)) {
    std::vector<std::string> fields = cvub::split(line, '\t');
    TsvRow row;
    for (size_t i = 0; i < header.size() && i < fields.size(); ++i)
      row[header[i]] = fields[i];
    if (row.count("case"))
      rows[row["case"]] = std::move(row);
  }
  return rows;
}

std::vector<cvub::fs::path> listCaseDirs(const cvub::fs::path &root) {
  std::vector<cvub::fs::path> dirs;
  for (const auto &entry : cvub::fs::directory_iterator(root))
    if (entry.is_directory())
      dirs.push_back(entry.path());
  std::sort(dirs.begin(), dirs.end());
  return dirs;
}

std::optional<cvub::fs::path>
findFileWithSuffix(const cvub::fs::path &directory, const std::string &suffix) {
  for (const auto &entry : cvub::fs::directory_iterator(directory)) {
    const std::string filename = entry.path().filename().string();
    if (entry.is_regular_file() &&
        (cvub::endsWith(filename, suffix) ||
         (suffix == ".before_local_plan_memory.mlir" &&
          filename == "before_local_plan_memory.mlir")))
      return entry.path();
  }
  return std::nullopt;
}

std::ostream &openOutput(const std::string &path, std::ofstream &file) {
  if (path.empty())
    return std::cout;
  file.open(path);
  if (!file) {
    std::cerr << "[ERROR] failed to open output: " << path << "\n";
    std::exit(1);
  }
  return file;
}

std::string join(const std::vector<std::string> &values) {
  std::string out;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i)
      out += ",";
    out += values[i];
  }
  return out;
}

std::string pathComponent(std::string value) {
  for (char &ch : value)
    if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '-' &&
        ch != '_' && ch != '.')
      ch = '_';
  return value;
}

void writeTextFile(const cvub::fs::path &path, const std::string &text) {
  cvub::fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out)
    throw std::runtime_error("failed to open snapshot: " + path.string());
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
  if (!out)
    throw std::runtime_error("failed to write snapshot: " + path.string());
}

size_t firstDifferentLine(const std::string &lhs, const std::string &rhs) {
  size_t line = 1;
  const size_t shared = std::min(lhs.size(), rhs.size());
  for (size_t i = 0; i < shared; ++i) {
    if (lhs[i] != rhs[i])
      return line;
    if (lhs[i] == '\n')
      ++line;
  }
  return lhs.size() == rhs.size() ? 0 : line;
}

std::string rawBufferName(const std::string &name) {
  size_t scope = name.rfind("$scope");
  if (scope == std::string::npos || scope + 6 == name.size())
    return name;
  for (size_t i = scope + 6; i < name.size(); ++i)
    if (!std::isdigit(static_cast<unsigned char>(name[i])))
      return name;
  return name.substr(0, scope);
}

std::map<std::string, std::string>
planMemoryDebugBufferNames(
    const std::vector<cvub::OperationRecord> &operations) {
  using RegionPath = std::vector<int>;
  struct NameState {
    std::set<std::string> usedNames;
    unsigned nextConflictId = 0;
  };
  std::map<RegionPath, std::vector<const cvub::OperationRecord *>>
      operationsByRegion;
  std::map<RegionPath, std::vector<RegionPath>> childrenByRegion;
  std::set<int> seenOperations;
  for (const cvub::OperationRecord &operation : operations) {
    if (!seenOperations.insert(operation.operationId).second)
      continue;
    operationsByRegion[operation.regionPath].push_back(&operation);
    if (!operation.regionPath.empty()) {
      RegionPath parent = operation.regionPath;
      parent.pop_back();
      std::vector<RegionPath> &children = childrenByRegion[parent];
      if (std::find(children.begin(), children.end(), operation.regionPath) ==
          children.end())
        children.push_back(operation.regionPath);
    }
  }

  std::map<std::string, std::string> names;
  std::function<void(const RegionPath &, NameState)> numberRegion =
      [&](const RegionPath &path, NameState state) {
        for (const cvub::OperationRecord *operation :
             operationsByRegion[path]) {
          std::vector<std::string> resultGroups =
              cvub::operationResultBaseNames(*operation);
          std::vector<std::string> results =
              cvub::operationResultNames(*operation);
          for (size_t groupIndex = 0; groupIndex < resultGroups.size();
               ++groupIndex) {
            std::string raw = resultGroups[groupIndex];
            size_t scope = raw.rfind("$scope");
            if (scope != std::string::npos)
              raw.resize(scope);
            if (!raw.empty() && raw.front() == '%')
              raw.erase(raw.begin());
            if (raw.empty() ||
                (operation->opName != "memref.alloc" &&
                 std::all_of(raw.begin(), raw.end(), [](char ch) {
                   return std::isdigit(static_cast<unsigned char>(ch));
                 })))
              continue;

            std::string suggested =
                operation->opName == "memref.alloc" ? "alloc" : raw;
            std::smatch conflictSuffix;
            if (operation->opName != "memref.alloc" &&
                std::regex_match(
                    raw, conflictSuffix,
                    std::regex(R"((.+)_([0-9]+))")) &&
                state.usedNames.count(conflictSuffix[1].str()))
              suggested = conflictSuffix[1].str();

            std::string assigned = suggested;
            if (state.usedNames.count(assigned)) {
              do {
                assigned = suggested + "_" +
                           std::to_string(state.nextConflictId++);
              } while (state.usedNames.count(assigned));
            }
            state.usedNames.insert(assigned);
            if (operation->opName == "memref.alloc" && !results.empty())
              names[results.front()] = "%" + assigned;
          }
        }
        for (const RegionPath &child : childrenByRegion[path])
          numberRegion(child, state);
      };
  numberRegion({}, {});
  return names;
}

std::string findCoreFunctionName(const cvub::fs::path &path,
                                 const std::string &coreType) {
  std::ifstream in(path);
  std::string function;
  std::string line;
  const std::regex functionRegex(R"(func\.func\s+@([\w$.#-]+))");
  const std::regex coreRegex(
      R"(hivm\.func_core_type\s*=\s*#hivm\.func_core_type<(\w+)>)");
  while (std::getline(in, line)) {
    std::smatch match;
    if (std::regex_search(line, match, functionRegex))
      function = match[1].str();
    if (!function.empty() && std::regex_search(line, match, coreRegex)) {
      if (match[1].str() == coreType)
        return function;
      function.clear();
    }
  }
  return {};
}

int validateMemoryInfoReplay(const Options &opts) {
  if (opts.root.empty() || opts.expectedSummary.empty()) {
    std::cerr << "[ERROR] --root and --expected-summary are required\n";
    return 1;
  }
  auto expected = readExpectedSummary(opts.expectedSummary);
  std::ofstream file;
  std::ostream &os = openOutput(opts.output, file);
  os << "case\tmatch\texpected_peak_bits\tactual_peak_bits\tdelta_bits\t"
        "expected_peak_bytes\tactual_peak_bytes\texpected_alloc_count\t"
        "actual_alloc_count\tmemory_info\n";
  int failures = 0;
  int cases = 0;
  for (const cvub::fs::path &caseDir : listCaseDirs(opts.root)) {
    std::string caseName = caseDir.filename().string();
    auto expectedIt = expected.find(caseName);
    cvub::fs::path memoryPath = caseDir / "memory_info_aiv.json";
    if (expectedIt == expected.end() || !cvub::fs::exists(memoryPath))
      continue;
    std::vector<cvub::MemoryRecord> records = cvub::loadMemoryInfo(memoryPath);
    cvub::ReplayResult result = cvub::replayMemoryInfoPeak(records);
    uint64_t expectedBits = std::stoull(expectedIt->second["aiv_ub_peak_bits"]);
    uint64_t expectedBytes =
        std::stoull(expectedIt->second["aiv_ub_peak_bytes"]);
    uint64_t expectedCount =
        std::stoull(expectedIt->second["aiv_ub_alloc_count"]);
    bool match = result.peakBits == expectedBits &&
                 result.peakBytes == expectedBytes &&
                 result.allocCount == expectedCount;
    failures += match ? 0 : 1;
    ++cases;
    os << caseName << "\t" << (match ? "true" : "false") << "\t"
       << expectedBits << "\t" << result.peakBits << "\t"
       << static_cast<int64_t>(result.peakBits) -
              static_cast<int64_t>(expectedBits)
       << "\t" << expectedBytes << "\t" << result.peakBytes << "\t"
       << expectedCount << "\t" << result.allocCount << "\t"
       << memoryPath.string() << "\n";
  }
  if (!opts.quiet) {
    std::cout << "validated_cases=" << cases << "\n";
    std::cout << "failures=" << failures << "\n";
  }
  return cases > 0 && failures == 0 ? 0 : 1;
}

int validateLocalUBAllocations(const Options &opts) {
  if (opts.root.empty() || opts.expectedSummary.empty()) {
    std::cerr << "[ERROR] --root and --expected-summary are required\n";
    return 1;
  }
  auto expected = readExpectedSummary(opts.expectedSummary);
  std::ofstream file;
  std::ostream &os = openOutput(opts.output, file);
  os << "case\tmatch\tir_alloc_count\texpected_alloc_count\t"
        "memory_info_alloc_count\talloc_count_match\tname_set_match\t"
        "tmpbuf_set_match\tir_tmpbuf_count\tmemory_info_tmpbuf_count\t"
        "missing_in_ir\textra_in_ir\tbefore_ir\tmemory_info\n";
  int failures = 0;
  int cases = 0;
  for (const cvub::fs::path &caseDir : listCaseDirs(opts.root)) {
    std::string caseName = caseDir.filename().string();
    auto expectedIt = expected.find(caseName);
    cvub::fs::path memoryPath = caseDir / "memory_info_aiv.json";
    auto beforeIR = findFileWithSuffix(caseDir, ".before_local_plan_memory.mlir");
    if (expectedIt == expected.end() || !cvub::fs::exists(memoryPath) ||
        !beforeIR)
      continue;
    std::vector<cvub::MemoryRecord> memoryRecords =
        cvub::loadMemoryInfo(memoryPath);
    std::vector<cvub::IRAllocRecord> irRecords =
        cvub::parseLocalUBAllocations(*beforeIR);
    uint64_t expectedCount =
        std::stoull(expectedIt->second["aiv_ub_alloc_count"]);
    std::set<std::string> irNames = cvub::irNameSet(irRecords);
    std::set<std::string> memNames = cvub::memoryNameSet(memoryRecords);
    std::set<std::string> irTmp = cvub::irTmpbufSet(irRecords);
    std::set<std::string> memTmp = cvub::memoryTmpbufSet(memoryRecords);
    std::vector<std::string> missing;
    std::vector<std::string> extra;
    std::set_difference(memNames.begin(), memNames.end(), irNames.begin(),
                        irNames.end(), std::back_inserter(missing));
    std::set_difference(irNames.begin(), irNames.end(), memNames.begin(),
                        memNames.end(), std::back_inserter(extra));
    bool allocCountMatch = irRecords.size() == expectedCount;
    bool nameSetMatch = irNames == memNames;
    bool tmpbufSetMatch = irTmp == memTmp;
    bool match = allocCountMatch && nameSetMatch && tmpbufSetMatch &&
                 memoryRecords.size() == expectedCount;
    failures += match ? 0 : 1;
    ++cases;
    os << caseName << "\t" << (match ? "true" : "false") << "\t"
       << irRecords.size() << "\t" << expectedCount << "\t"
       << memoryRecords.size() << "\t"
       << (allocCountMatch ? "true" : "false") << "\t"
       << (nameSetMatch ? "true" : "false") << "\t"
       << (tmpbufSetMatch ? "true" : "false") << "\t" << irTmp.size()
       << "\t" << memTmp.size() << "\t" << join(missing) << "\t"
       << join(extra) << "\t" << beforeIR->string() << "\t"
       << memoryPath.string() << "\n";
  }
  if (!opts.quiet) {
    std::cout << "validated_ir_cases=" << cases << "\n";
    std::cout << "failures=" << failures << "\n";
  }
  return cases > 0 && failures == 0 ? 0 : 1;
}

bool overlaps(std::pair<int, int> lhs, std::pair<int, int> rhs) {
  return lhs.first <= rhs.second && rhs.first <= lhs.second;
}

int validateLifetimes(const Options &opts) {
  if (opts.root.empty()) {
    std::cerr << "[ERROR] --root is required\n";
    return 1;
  }
  std::ofstream file;
  std::ostream &os = openOutput(opts.output, file);
  os << "case\tseed\tname\tmatch\tpred_alloc_time\tpred_free_time\t"
        "memory_alloc_time\tmemory_free_time\tdelta_start\tdelta_end\t"
        "direct_alloc_time\tdirect_free_time\tgroup\tbefore_ir\tmemory_info\n";

  int cases = 0;
  int records = 0;
  int strictFailures = 0;
  int pairwiseFailures = 0;
  int pairwiseChecks = 0;

  for (const cvub::fs::path &caseDir : listCaseDirs(opts.root)) {
    cvub::fs::path memoryPath = caseDir / "memory_info_aiv.json";
    auto beforeIR = findFileWithSuffix(caseDir, ".before_local_plan_memory.mlir");
    if (!cvub::fs::exists(memoryPath) || !beforeIR)
      continue;

    std::vector<cvub::MemoryRecord> memoryRecords =
        cvub::loadMemoryInfo(memoryPath);
    uint32_t selectedSeed =
        opts.randomSeed >= 0 ? static_cast<uint32_t>(opts.randomSeed) : 0;
    cvub::LifetimeAnalysis analysis =
        cvub::analyzeLifetimes(*beforeIR, "AIV", selectedSeed);
    if (opts.tryAllSeeds) {
      int bestFailures = std::numeric_limits<int>::max();
      for (uint32_t seed = 0; seed < 20; ++seed) {
        cvub::LifetimeAnalysis candidate =
            cvub::analyzeLifetimes(*beforeIR, "AIV", seed);
        std::map<std::string, cvub::LifetimeRecord> candidateByName;
        for (const cvub::LifetimeRecord &record : candidate.records)
          candidateByName[record.name] = record;
        int failures = 0;
        for (const cvub::MemoryRecord &record : memoryRecords) {
          auto it = candidateByName.find(record.name);
          if (it == candidateByName.end() ||
              it->second.allocTime != record.lifeTimeInIR.first ||
              it->second.freeTime != record.lifeTimeInIR.second)
            ++failures;
        }
        if (failures < bestFailures) {
          bestFailures = failures;
          selectedSeed = seed;
          analysis = std::move(candidate);
        }
      }
    }
    std::map<std::string, cvub::LifetimeRecord> predicted;
    for (const cvub::LifetimeRecord &record : analysis.records)
      predicted[record.name] = record;
    std::map<std::string, cvub::MemoryRecord> memoryByName;
    for (const cvub::MemoryRecord &record : memoryRecords)
      memoryByName[record.name] = record;

    std::string caseName = caseDir.filename().string();
    ++cases;
    for (const auto &it : memoryByName) {
      const std::string &name = it.first;
      const cvub::MemoryRecord &memory = it.second;
      cvub::LifetimeRecord pred;
      bool hasPred = predicted.count(name) != 0;
      if (hasPred)
        pred = predicted[name];
      bool match = hasPred && pred.allocTime == memory.lifeTimeInIR.first &&
                   pred.freeTime == memory.lifeTimeInIR.second;
      strictFailures += match ? 0 : 1;
      ++records;
      os << caseName << "\t" << selectedSeed << "\t" << name << "\t"
         << (match ? "true" : "false")
         << "\t" << (hasPred ? pred.allocTime : -1) << "\t"
         << (hasPred ? pred.freeTime : -1) << "\t" << memory.lifeTimeInIR.first
         << "\t" << memory.lifeTimeInIR.second << "\t"
         << (hasPred ? pred.allocTime - memory.lifeTimeInIR.first : -999999)
         << "\t"
         << (hasPred ? pred.freeTime - memory.lifeTimeInIR.second : -999999)
         << "\t" << (hasPred ? pred.directAllocTime : -1) << "\t"
         << (hasPred ? pred.directFreeTime : -1) << "\t"
         << (hasPred ? join(pred.group) : "") << "\t" << beforeIR->string()
         << "\t" << memoryPath.string() << "\n";
    }

    std::vector<std::string> names;
    for (const auto &it : memoryByName)
      if (predicted.count(it.first))
        names.push_back(it.first);
    for (size_t i = 0; i < names.size(); ++i) {
      for (size_t j = i + 1; j < names.size(); ++j) {
        const std::string &lhs = names[i];
        const std::string &rhs = names[j];
        bool predOverlap =
            overlaps({predicted[lhs].allocTime, predicted[lhs].freeTime},
                     {predicted[rhs].allocTime, predicted[rhs].freeTime});
        bool memoryOverlap =
            overlaps(memoryByName[lhs].lifeTimeInIR, memoryByName[rhs].lifeTimeInIR);
        pairwiseFailures += predOverlap == memoryOverlap ? 0 : 1;
        ++pairwiseChecks;
      }
    }
  }

  if (!opts.quiet) {
    std::cout << "validated_lifetime_cases=" << cases << "\n";
    std::cout << "validated_lifetime_records=" << records << "\n";
    std::cout << "strict_failures=" << strictFailures << "\n";
    std::cout << "pairwise_overlap_failures=" << pairwiseFailures << " / "
              << pairwiseChecks << "\n";
  }
  return cases > 0 && strictFailures == 0 && pairwiseFailures == 0 ? 0 : 1;
}

struct OracleLivenessBuffer {
  uint64_t constBits = 0;
  int bufferScope = -1;
  bool ignoreInplace = false;
  int allocTime = -1;
  int freeTime = -1;

  bool operator==(const OracleLivenessBuffer &other) const {
    return constBits == other.constBits && bufferScope == other.bufferScope &&
           ignoreInplace == other.ignoreInplace &&
           allocTime == other.allocTime && freeTime == other.freeTime;
  }
};

struct OracleAttempt {
  std::string function;
  uint32_t seed = 0;
  std::map<std::string, OracleLivenessBuffer> buffers;
  std::map<int, std::pair<std::vector<std::string>, std::vector<std::string>>>
      genKillMap;
  std::vector<std::pair<std::string, std::string>> inplacePairs;
  std::map<std::string, uint32_t> buffer2MultiNum;
  std::map<int, std::vector<std::string>> liveBefore;
  std::map<int, std::vector<std::string>> liveAfter;
  std::optional<bool> planSuccess;
  std::map<std::string, std::vector<uint64_t>> plannedOffsets;
  std::map<std::string, uint64_t> plannedConstBits;
  std::map<int, uint64_t> peakBitsByScope;
  std::map<int, uint64_t> requiredBitsByScope;
  std::map<size_t, OracleLivenessBuffer> exactBuffers;
  std::map<size_t, std::string> exactBufferNames;
  std::map<int, std::pair<std::vector<size_t>, std::vector<size_t>>>
      exactGenKillMap;
  std::vector<std::pair<size_t, size_t>> exactInplacePairs;
  std::map<size_t, uint32_t> exactBuffer2MultiNum;
  std::map<size_t, std::vector<uint64_t>> exactPlannedOffsets;
  std::map<size_t, uint64_t> exactPlannedConstBits;
};

std::vector<OracleAttempt> readOracleAttempts(const cvub::fs::path &path) {
  std::ifstream in(path);
  std::vector<OracleAttempt> attempts;
  OracleAttempt *current = nullptr;
  OracleAttempt *currentPlanAttempt = nullptr;
  std::string pendingExactBufferName;
  std::string line;
  while (std::getline(in, line)) {
    std::vector<std::string> fields = cvub::split(line, '\t');
    if (fields.empty())
      continue;
    if (fields[0] == "PLANMEM_LIVENESS_ATTEMPT" && fields.size() >= 4) {
      attempts.push_back({});
      current = &attempts.back();
      current->function = fields[1];
      current->seed = static_cast<uint32_t>(std::stoul(fields[3]));
      pendingExactBufferName.clear();
    } else if (fields[0] == "PLANMEM_GEN" && fields.size() >= 4 && current) {
      current->genKillMap[std::stoi(fields[2])].first.push_back(fields[3]);
    } else if (fields[0] == "PLANMEM_KILL" && fields.size() >= 4 && current) {
      current->genKillMap[std::stoi(fields[2])].second.push_back(fields[3]);
    } else if (fields[0] == "PLANMEM_BUFFER" && fields.size() >= 8 && current) {
      current->buffers[fields[2]] =
          {std::stoull(fields[3]), std::stoi(fields[4]),
           fields[5] == "1", std::stoi(fields[6]), std::stoi(fields[7])};
      pendingExactBufferName = fields[2];
    } else if (fields[0] == "PLANMEM_INPLACE" && fields.size() >= 4 && current) {
      current->inplacePairs.push_back({fields[2], fields[3]});
    } else if (fields[0] == "PLANMEM_MULTI" && fields.size() >= 4 && current) {
      current->buffer2MultiNum[fields[2]] =
          static_cast<uint32_t>(std::stoul(fields[3]));
    } else if (fields[0] == "PLANMEM_LIVE_BEFORE" && fields.size() >= 3 &&
               current) {
      current->liveBefore[std::stoi(fields[2])] =
          std::vector<std::string>(fields.begin() + 3, fields.end());
    } else if (fields[0] == "PLANMEM_LIVE_AFTER" && fields.size() >= 3 &&
               current) {
      current->liveAfter[std::stoi(fields[2])] =
          std::vector<std::string>(fields.begin() + 3, fields.end());
    } else if (fields[0] == "PLANMEM_EXACT_GEN" && fields.size() >= 4 &&
               current) {
      current->exactGenKillMap[std::stoi(fields[2])].first.push_back(
          static_cast<size_t>(std::stoull(fields[3])));
    } else if (fields[0] == "PLANMEM_EXACT_KILL" && fields.size() >= 4 &&
               current) {
      current->exactGenKillMap[std::stoi(fields[2])].second.push_back(
          static_cast<size_t>(std::stoull(fields[3])));
    } else if (fields[0] == "PLANMEM_EXACT_BUFFER" && fields.size() >= 8 &&
               current) {
      size_t bufferId = static_cast<size_t>(std::stoull(fields[2]));
      current->exactBuffers[bufferId] =
          {std::stoull(fields[3]), std::stoi(fields[4]), fields[5] == "1",
           std::stoi(fields[6]), std::stoi(fields[7])};
      if (pendingExactBufferName.empty())
        throw std::runtime_error("exact buffer record has no readable name");
      current->exactBufferNames[bufferId] = pendingExactBufferName;
      pendingExactBufferName.clear();
    } else if (fields[0] == "PLANMEM_EXACT_INPLACE" && fields.size() >= 4 &&
               current) {
      current->exactInplacePairs.push_back(
          {static_cast<size_t>(std::stoull(fields[2])),
           static_cast<size_t>(std::stoull(fields[3]))});
    } else if (fields[0] == "PLANMEM_EXACT_MULTI" && fields.size() >= 4 &&
               current) {
      current->exactBuffer2MultiNum[static_cast<size_t>(
          std::stoull(fields[2]))] = static_cast<uint32_t>(
          std::stoul(fields[3]));
    } else if (fields[0] == "PLANMEM_PLAN_ATTEMPT" && fields.size() >= 4) {
      const std::string &function = fields[1];
      uint32_t seed = static_cast<uint32_t>(std::stoul(fields[2]));
      auto it = std::find_if(attempts.rbegin(), attempts.rend(),
                             [&](const OracleAttempt &attempt) {
                               return attempt.function == function &&
                                      attempt.seed == seed;
                             });
      if (it != attempts.rend())
        it->planSuccess = fields[3] == "success";
      currentPlanAttempt = it != attempts.rend() ? &*it : nullptr;
    } else if (fields[0] == "PLANMEM_PLANNED_BUFFER" && fields.size() >= 6 &&
               currentPlanAttempt) {
      std::vector<uint64_t> offsets;
      for (size_t i = 5; i < fields.size(); ++i)
        offsets.push_back(std::stoull(fields[i]));
      currentPlanAttempt->plannedOffsets[fields[3]] = std::move(offsets);
      currentPlanAttempt->plannedConstBits[fields[3]] =
          std::stoull(fields[4]);
    } else if (fields[0] == "PLANMEM_EXACT_PLANNED_BUFFER" &&
               fields.size() >= 6 && currentPlanAttempt) {
      size_t bufferId = static_cast<size_t>(std::stoull(fields[3]));
      std::vector<uint64_t> offsets;
      for (size_t i = 5; i < fields.size(); ++i)
        offsets.push_back(std::stoull(fields[i]));
      currentPlanAttempt->exactPlannedOffsets[bufferId] = std::move(offsets);
      currentPlanAttempt->exactPlannedConstBits[bufferId] =
          std::stoull(fields[4]);
    } else if (fields[0] == "PLANMEM_PEAK" && fields.size() >= 4 &&
               currentPlanAttempt) {
      currentPlanAttempt->peakBitsByScope[std::stoi(fields[2])] =
          std::stoull(fields[3]);
    } else if (fields[0] == "PLANMEM_REQUIRED" && fields.size() >= 4 &&
               currentPlanAttempt) {
      currentPlanAttempt->requiredBitsByScope[std::stoi(fields[2])] =
          std::stoull(fields[3]);
    }
  }
  return attempts;
}

struct FixedSeedRun {
  std::string caseName;
  uint32_t seed = 0;
  std::string configId;
  bool restrictInplaceAsISA = false;
  bool complete = false;
  int compilerStatus = -1;
  cvub::fs::path input;
  cvub::fs::path dump;
};

cvub::fs::path snapshotDirectory(const Options &opts,
                                 const FixedSeedRun &run) {
  if (opts.snapshotRoot.empty())
    return {};
  std::ostringstream seed;
  seed << "seed_" << std::setw(2) << std::setfill('0') << run.seed;
  return cvub::fs::path(opts.snapshotRoot) / pathComponent(run.caseName) /
         pathComponent(run.configId) / seed.str();
}

struct OracleRunConfig {
  bool found = false;
  bool resultFound = false;
  bool resultSuccess = false;
  int seed = -1;
  bool restrictInplaceAsISA = false;
  bool localPlanMemoryOnly = false;
};

OracleRunConfig readOracleRunConfig(const cvub::fs::path &dump) {
  std::ifstream in(dump);
  OracleRunConfig config;
  std::string line;
  while (std::getline(in, line)) {
    std::vector<std::string> fields = cvub::split(line, '\t');
    if (fields.empty())
      continue;
    if (fields[0] == "PLANMEM_RUN_RESULT") {
      if (config.resultFound)
        throw std::runtime_error("duplicate PLANMEM_RUN_RESULT record");
      config.resultFound = fields.size() >= 2;
      config.resultSuccess = config.resultFound && fields[1] == "success";
      continue;
    }
    if (fields[0] != "PLANMEM_RUN_CONFIG")
      continue;
    if (config.found)
      throw std::runtime_error("duplicate PLANMEM_RUN_CONFIG record");
    config.found = true;
    for (size_t i = 1; i + 1 < fields.size(); i += 2) {
      if (fields[i] == "seed")
        config.seed = std::stoi(fields[i + 1]);
      else if (fields[i] == "restrict_inplace_as_isa")
        config.restrictInplaceAsISA = fields[i + 1] == "1";
      else if (fields[i] == "local_plan_memory_only")
        config.localPlanMemoryOnly = fields[i + 1] == "1";
    }
  }
  return config;
}

std::vector<FixedSeedRun>
readFixedSeedRuns(const cvub::fs::path &summaryPath) {
  std::ifstream in(summaryPath);
  if (!in)
    throw std::runtime_error("failed to open oracle summary: " +
                             summaryPath.string());
  std::string headerLine;
  if (!std::getline(in, headerLine))
    throw std::runtime_error("empty oracle summary: " + summaryPath.string());
  std::vector<std::string> header = cvub::split(headerLine, '\t');
  std::map<std::string, size_t> column;
  for (size_t i = 0; i < header.size(); ++i)
    column[header[i]] = i;
  for (const std::string &required :
       {"case", "seed", "config_id", "restrict_inplace_as_isa", "status",
        "compiler_status", "input", "dump"})
    if (!column.count(required))
      throw std::runtime_error("oracle summary missing column: " + required);

  std::vector<FixedSeedRun> runs;
  std::set<std::tuple<std::string, std::string, uint32_t>> keys;
  std::string line;
  while (std::getline(in, line)) {
    std::vector<std::string> fields = cvub::split(line, '\t');
    if (fields.size() < header.size())
      throw std::runtime_error("truncated oracle summary row: " + line);
    FixedSeedRun run;
    run.caseName = fields[column.at("case")];
    run.seed = static_cast<uint32_t>(
        std::stoul(fields[column.at("seed")]));
    run.configId = fields[column.at("config_id")];
    run.restrictInplaceAsISA =
        fields[column.at("restrict_inplace_as_isa")] == "1";
    run.complete = fields[column.at("status")] == "complete";
    run.compilerStatus = std::stoi(fields[column.at("compiler_status")]);
    run.input = fields[column.at("input")];
    run.dump = fields[column.at("dump")];
    if (run.seed >= 20)
      throw std::runtime_error("oracle seed outside [0, 19]");
    auto key = std::make_tuple(run.caseName, run.configId, run.seed);
    if (!keys.insert(key).second)
      throw std::runtime_error("duplicate case/config/seed oracle tuple");
    runs.push_back(std::move(run));
  }
  return runs;
}

const OracleAttempt *findUBOracleAttempt(
    const std::vector<OracleAttempt> &attempts,
    const std::string &expectedFunction) {
  const OracleAttempt *result = nullptr;
  for (const OracleAttempt &attempt : attempts) {
    bool functionMatch = !expectedFunction.empty() &&
                         attempt.function == expectedFunction;
    bool nonEmptyAllUB = !attempt.exactBuffers.empty() && std::all_of(
        attempt.exactBuffers.begin(), attempt.exactBuffers.end(),
        [](const auto &item) { return item.second.bufferScope == 6; });
    if (!functionMatch && !nonEmptyAllUB)
      continue;
    if (result)
      throw std::runtime_error("multiple UB PlanMemory attempts in one run");
    result = &attempt;
  }
  return result;
}

struct ModelLivenessState {
  std::string function;
  std::map<size_t, OracleLivenessBuffer> buffers;
  std::map<size_t, std::string> bufferNames;
  std::map<int, std::pair<std::vector<size_t>, std::vector<size_t>>> genKill;
  std::vector<std::pair<size_t, size_t>> inplacePairs;
  std::map<size_t, uint32_t> multiBuffer;
};

ModelLivenessState buildModelLivenessState(
    const cvub::fs::path &input, uint32_t seed, bool restrictInplaceAsISA) {
  std::vector<cvub::BufferInfoRecord> infos = cvub::BuildBufferInfos(input);
  cvub::LifetimeAnalysis analysis = cvub::analyzeLifetimes(
      input, "AIV", seed, restrictInplaceAsISA);
  std::map<std::string, size_t> idByName;
  for (size_t i = 0; i < infos.size(); ++i)
    idByName[infos[i].name] = i;
  std::map<std::string, cvub::LifetimeRecord> lifeByName;
  for (const cvub::LifetimeRecord &life : analysis.records)
    lifeByName[life.name] = life;

  ModelLivenessState state;
  state.function = findCoreFunctionName(input, "AIV");
  std::map<std::string, std::string> debugNames =
      planMemoryDebugBufferNames(analysis.operations);
  for (size_t i = 0; i < infos.size(); ++i) {
    const cvub::BufferInfoRecord &info = infos[i];
    auto life = lifeByName.find(info.name);
    int allocTime = life == lifeByName.end() ? -1 : life->second.directAllocTime;
    int freeTime = life == lifeByName.end() ? -1 : life->second.directFreeTime;
    bool ignoreInplace = analysis.bufferIgnoreInplace.count(info.name)
                             ? analysis.bufferIgnoreInplace.at(info.name)
                             : false;
    state.buffers[i] =
        {info.constBits, 6, ignoreInplace, allocTime, freeTime};
    auto debugName = debugNames.find(info.name);
    state.bufferNames[i] = debugName == debugNames.end()
                               ? rawBufferName(info.name)
                               : debugName->second;
  }
  auto idFor = [&](const std::string &name) -> size_t {
    auto it = idByName.find(name);
    if (it == idByName.end())
      throw std::runtime_error("liveness references unknown buffer: " + name);
    return it->second;
  };
  for (const cvub::GenKillRecord &record : analysis.genKillMap) {
    auto &entry = state.genKill[record.operationIndex];
    for (const std::string &name : record.gen)
      entry.first.push_back(idFor(name));
    for (const std::string &name : record.kill)
      entry.second.push_back(idFor(name));
  }
  for (const auto &pair : analysis.initialInplacePairList)
    state.inplacePairs.push_back({idFor(pair.first), idFor(pair.second)});
  for (const auto &item : analysis.buffer2MultiNum)
    state.multiBuffer[idFor(item.first)] = item.second;
  return state;
}

void appendSnapshotConfig(std::ostringstream &os, int seed,
                          bool restrictInplaceAsISA,
                          bool localPlanMemoryOnly) {
  os << "SEED\t" << seed << '\n';
  os << "RESTRICT_INPLACE_AS_ISA\t"
     << static_cast<int>(restrictInplaceAsISA) << '\n';
  os << "LOCAL_PLAN_MEMORY_ONLY\t"
     << static_cast<int>(localPlanMemoryOnly) << '\n';
}

std::string canonicalLifetimeSnapshot(
    int seed, bool restrictInplaceAsISA, bool localPlanMemoryOnly,
    const ModelLivenessState &state) {
  std::ostringstream os;
  os << "PLANMEM_LIFETIME_SNAPSHOT\t1\n";
  appendSnapshotConfig(os, seed, restrictInplaceAsISA,
                       localPlanMemoryOnly);
  os << "FUNCTION\t" << state.function << '\n';
  os << "BUFFER_COUNT\t" << state.buffers.size() << '\n';
  for (const auto &item : state.buffers) {
    auto name = state.bufferNames.find(item.first);
    if (name == state.bufferNames.end())
      throw std::runtime_error("lifetime buffer is missing its name");
    const OracleLivenessBuffer &buffer = item.second;
    os << "BUFFER\t" << item.first << '\t' << name->second << '\t'
       << buffer.constBits << '\t' << buffer.bufferScope << '\t'
       << static_cast<int>(buffer.ignoreInplace) << '\t' << buffer.allocTime
       << '\t' << buffer.freeTime << '\n';
  }
  os << "GEN_KILL_OP_COUNT\t" << state.genKill.size() << '\n';
  for (const auto &item : state.genKill) {
    os << "GEN\t" << item.first << '\t' << item.second.first.size();
    for (size_t id : item.second.first)
      os << '\t' << id;
    os << '\n';
    os << "KILL\t" << item.first << '\t' << item.second.second.size();
    for (size_t id : item.second.second)
      os << '\t' << id;
    os << '\n';
  }
  os << "INPLACE_COUNT\t" << state.inplacePairs.size() << '\n';
  for (size_t i = 0; i < state.inplacePairs.size(); ++i)
    os << "INPLACE\t" << i << '\t' << state.inplacePairs[i].first << '\t'
       << state.inplacePairs[i].second << '\n';
  os << "MULTI_COUNT\t" << state.multiBuffer.size() << '\n';
  for (const auto &item : state.multiBuffer)
    os << "MULTI\t" << item.first << '\t' << item.second << '\n';
  return os.str();
}

ModelLivenessState oracleLivenessState(const OracleAttempt *oracle) {
  ModelLivenessState state;
  if (!oracle)
    return state;
  state.function = oracle->function;
  state.buffers = oracle->exactBuffers;
  state.bufferNames = oracle->exactBufferNames;
  state.genKill = oracle->exactGenKillMap;
  state.inplacePairs = oracle->exactInplacePairs;
  state.multiBuffer = oracle->exactBuffer2MultiNum;
  return state;
}

struct PlanSnapshotState {
  std::string function;
  bool success = true;
  bool overflow = false;
  uint64_t peakBits = 0;
  uint64_t requiredBits = 0;
  uint64_t capacityBits = cvub::kUBCapacityBits;
  std::map<size_t, std::string> bufferNames;
  std::map<size_t, uint64_t> constBits;
  std::map<size_t, std::vector<uint64_t>> offsets;
};

std::string canonicalPlanSnapshot(int seed, bool restrictInplaceAsISA,
                                  bool localPlanMemoryOnly,
                                  const PlanSnapshotState &state) {
  std::ostringstream os;
  os << "PLANMEM_PLAN_SNAPSHOT\t1\n";
  appendSnapshotConfig(os, seed, restrictInplaceAsISA,
                       localPlanMemoryOnly);
  os << "FUNCTION\t" << state.function << '\n';
  os << "STATUS\t" << (state.success ? "success" : "failure") << '\n';
  os << "OVERFLOW\t" << static_cast<int>(state.overflow) << '\n';
  os << "UB_PEAK_BITS\t" << state.peakBits << '\n';
  os << "UB_REQUIRED_BITS\t" << state.requiredBits << '\n';
  os << "UB_CAPACITY_BITS\t" << state.capacityBits << '\n';
  os << "BUFFER_COUNT\t" << state.offsets.size() << '\n';
  for (const auto &item : state.offsets) {
    auto name = state.bufferNames.find(item.first);
    auto bits = state.constBits.find(item.first);
    if (name == state.bufferNames.end() || bits == state.constBits.end())
      throw std::runtime_error("planned buffer is missing name or constBits");
    os << "BUFFER\t" << item.first << '\t' << name->second << '\t'
       << bits->second << '\t' << item.second.size();
    for (uint64_t offset : item.second)
      os << '\t' << offset;
    os << '\n';
  }
  return os.str();
}

PlanSnapshotState oraclePlanState(const OracleAttempt *oracle) {
  PlanSnapshotState state;
  if (!oracle)
    return state;
  state.function = oracle->function;
  if (!oracle->planSuccess.has_value())
    throw std::runtime_error("oracle is missing plan status");
  state.success = *oracle->planSuccess;
  state.overflow = !state.success;
  state.bufferNames = oracle->exactBufferNames;
  state.constBits = oracle->exactPlannedConstBits;
  state.offsets = oracle->exactPlannedOffsets;
  if (state.success) {
    auto peak = oracle->peakBitsByScope.find(6);
    if (peak == oracle->peakBitsByScope.end() && !state.offsets.empty())
      throw std::runtime_error("successful UB plan is missing peak");
    if (peak != oracle->peakBitsByScope.end())
      state.peakBits = peak->second;
  } else {
    auto required = oracle->requiredBitsByScope.find(6);
    if (required == oracle->requiredBitsByScope.end())
      throw std::runtime_error("failed UB plan is missing required bits");
    state.requiredBits = required->second;
  }
  return state;
}

PlanSnapshotState modelPlanState(const cvub::fs::path &input, uint32_t seed,
                                 bool restrictInplaceAsISA) {
  PlanSnapshotState state;
  state.function = findCoreFunctionName(input, "AIV");
  cvub::PlanMemoryModelResult model = cvub::PlanLocalMemoryForSeed(
      input, seed, restrictInplaceAsISA);
  state.success = model.success;
  state.overflow = model.overflow;
  state.peakBits = model.success ? model.peakBits : 0;
  state.requiredBits = model.overflow ? model.requiredBits : 0;
  state.capacityBits = model.capacityBits;
  std::vector<cvub::BufferInfoRecord> infos = cvub::BuildBufferInfos(input);
  cvub::LifetimeAnalysis liveness = cvub::analyzeLifetimes(
      input, "AIV", seed, restrictInplaceAsISA);
  std::map<std::string, std::string> debugNames =
      planMemoryDebugBufferNames(liveness.operations);
  std::map<std::string, size_t> idByName;
  for (size_t i = 0; i < infos.size(); ++i) {
    idByName[infos[i].name] = i;
    auto debugName = debugNames.find(infos[i].name);
    state.bufferNames[i] = debugName == debugNames.end()
                               ? rawBufferName(infos[i].name)
                               : debugName->second;
  }
  for (const cvub::PlannedBufferRecord &buffer : model.buffers) {
    auto id = idByName.find(buffer.name);
    if (id == idByName.end())
      throw std::runtime_error("plan references unknown buffer: " +
                               buffer.name);
    state.constBits[id->second] = buffer.constBits;
    state.offsets[id->second] = buffer.offsetsBytes;
  }
  return state;
}

int validateFixedSeedLifetimes(const Options &opts) {
  if (opts.oracleSummary.empty()) {
    std::cerr << "[ERROR] --oracle-summary is required\n";
    return 1;
  }
  std::vector<FixedSeedRun> runs = readFixedSeedRuns(opts.oracleSummary);
  std::ofstream file;
  std::ostream &os = openOutput(opts.output, file);
  os << "case\tseed\tconfig_id\tmatch\tbyte_equal\tfirst_diff_line\t"
        "run_complete\toracle_valid\terror\toracle_snapshot\tmodel_snapshot\t"
        "input\tdump\n";
  int failures = 0;
  for (const FixedSeedRun &run : runs) {
    bool byteEqual = false;
    bool oracleValid = false;
    size_t diffLine = 0;
    std::string error;
    std::string oracleSnapshot;
    std::string modelSnapshot;
    cvub::fs::path oraclePath;
    cvub::fs::path modelPath;
    try {
      std::vector<OracleAttempt> attempts = readOracleAttempts(run.dump);
      OracleRunConfig config = readOracleRunConfig(run.dump);
      oracleValid = config.found && config.resultFound &&
                    config.resultSuccess == (run.compilerStatus == 0);
      ModelLivenessState modelState = buildModelLivenessState(
          run.input, run.seed, run.restrictInplaceAsISA);
      const OracleAttempt *oracle =
          findUBOracleAttempt(attempts, modelState.function);
      oracleSnapshot = canonicalLifetimeSnapshot(
          config.seed, config.restrictInplaceAsISA,
          config.localPlanMemoryOnly, oracleLivenessState(oracle));
      modelSnapshot = canonicalLifetimeSnapshot(
          static_cast<int>(run.seed), run.restrictInplaceAsISA, true,
          modelState);
      cvub::fs::path directory = snapshotDirectory(opts, run);
      if (!directory.empty()) {
        oraclePath = directory / "oracle.lifetime.tsv";
        modelPath = directory / "model.lifetime.tsv";
        writeTextFile(oraclePath, oracleSnapshot);
        writeTextFile(modelPath, modelSnapshot);
      }
      byteEqual = oracleSnapshot == modelSnapshot;
      diffLine = firstDifferentLine(oracleSnapshot, modelSnapshot);
    } catch (const std::exception &ex) {
      error = ex.what();
    }
    bool match = run.complete && oracleValid && error.empty() && byteEqual;
    failures += match ? 0 : 1;
    os << run.caseName << '\t' << run.seed << '\t' << run.configId << '\t'
       << (match ? "true" : "false") << '\t'
       << (byteEqual ? "true" : "false") << '\t' << diffLine << '\t'
       << (run.complete ? "true" : "false") << '\t'
       << (oracleValid ? "true" : "false") << '\t' << error << '\t'
       << oraclePath.string() << '\t' << modelPath.string() << '\t'
       << run.input.string() << '\t' << run.dump.string() << '\n';
  }
  bool countMatch = opts.expectedRuns <= 0 ||
                    static_cast<int>(runs.size()) == opts.expectedRuns;
  if (!opts.quiet) {
    std::cout << "validated_lifetime_runs=" << runs.size() << "\n";
    std::cout << "expected_runs=" << opts.expectedRuns << "\n";
    std::cout << "run_count_match=" << (countMatch ? "true" : "false")
              << "\n";
    std::cout << "lifetime_failures=" << failures << "\n";
  }
  return countMatch && failures == 0 ? 0 : 1;
}

int validateFixedSeedPlans(const Options &opts) {
  if (opts.oracleSummary.empty()) {
    std::cerr << "[ERROR] --oracle-summary is required\n";
    return 1;
  }
  std::vector<FixedSeedRun> runs = readFixedSeedRuns(opts.oracleSummary);
  std::ofstream file;
  std::ostream &os = openOutput(opts.output, file);
  os << "case\tseed\tconfig_id\tmatch\tbyte_equal\tfirst_diff_line\t"
        "run_complete\toracle_valid\terror\toracle_snapshot\tmodel_snapshot\t"
        "input\tdump\n";
  int failures = 0;
  for (const FixedSeedRun &run : runs) {
    bool byteEqual = false;
    bool oracleValid = false;
    size_t diffLine = 0;
    std::string error;
    std::string oracleSnapshot;
    std::string modelSnapshot;
    cvub::fs::path oraclePath;
    cvub::fs::path modelPath;
    try {
      std::vector<OracleAttempt> attempts = readOracleAttempts(run.dump);
      OracleRunConfig config = readOracleRunConfig(run.dump);
      oracleValid = config.found && config.resultFound &&
                    config.resultSuccess == (run.compilerStatus == 0);
      PlanSnapshotState modelState =
          modelPlanState(run.input, run.seed, run.restrictInplaceAsISA);
      const OracleAttempt *oracle =
          findUBOracleAttempt(attempts, modelState.function);
      oracleSnapshot = canonicalPlanSnapshot(
          config.seed, config.restrictInplaceAsISA,
          config.localPlanMemoryOnly, oraclePlanState(oracle));
      modelSnapshot = canonicalPlanSnapshot(
          static_cast<int>(run.seed), run.restrictInplaceAsISA, true,
          modelState);
      cvub::fs::path directory = snapshotDirectory(opts, run);
      if (!directory.empty()) {
        oraclePath = directory / "oracle.plan.tsv";
        modelPath = directory / "model.plan.tsv";
        writeTextFile(oraclePath, oracleSnapshot);
        writeTextFile(modelPath, modelSnapshot);
      }
      byteEqual = oracleSnapshot == modelSnapshot;
      diffLine = firstDifferentLine(oracleSnapshot, modelSnapshot);
    } catch (const std::exception &ex) {
      error = ex.what();
    }
    bool match = run.complete && oracleValid && error.empty() && byteEqual;
    failures += match ? 0 : 1;
    os << run.caseName << '\t' << run.seed << '\t' << run.configId << '\t'
       << (match ? "true" : "false") << '\t'
       << (byteEqual ? "true" : "false") << '\t' << diffLine << '\t'
       << (run.complete ? "true" : "false") << '\t'
       << (oracleValid ? "true" : "false") << '\t' << error << '\t'
       << oraclePath.string() << '\t' << modelPath.string() << '\t'
       << run.input.string() << '\t' << run.dump.string() << '\n';
  }
  bool countMatch = opts.expectedRuns <= 0 ||
                    static_cast<int>(runs.size()) == opts.expectedRuns;
  if (!opts.quiet) {
    std::cout << "validated_plan_runs=" << runs.size() << "\n";
    std::cout << "expected_runs=" << opts.expectedRuns << "\n";
    std::cout << "run_count_match=" << (countMatch ? "true" : "false")
              << "\n";
    std::cout << "plan_failures=" << failures << "\n";
  }
  return countMatch && failures == 0 ? 0 : 1;
}

int validatePlanMemoryAttempts(const Options &opts) {
  if (opts.root.empty() || opts.attemptRoot.empty()) {
    std::cerr << "[ERROR] --root and --attempt-root are required\n";
    return 1;
  }
  std::ofstream file;
  std::ostream &os = openOutput(opts.output, file);
  os << "case\tseed\tsemantic_match\tbuffer_failures\tgen_kill_set_match\t"
        "gen_kill_order_match\tinitial_inplace_match\tmulti_match\t"
        "plan_success_match\toffset_match\tpeak_match\trequired_match\n";
  int cases = 0;
  int attemptsChecked = 0;
  int failures = 0;
  int sameSeedLayoutFailures = 0;
  for (const cvub::fs::path &caseDir : listCaseDirs(opts.root)) {
    auto beforeIR = findFileWithSuffix(caseDir, ".before_local_plan_memory.mlir");
    cvub::fs::path dump = cvub::fs::path(opts.attemptRoot) /
                          caseDir.filename() / "planmemory_attempts.tsv";
    if (!beforeIR || !cvub::fs::exists(dump))
      continue;
    std::set<std::string> allocNames =
        cvub::irNameSet(cvub::parseLocalUBAllocations(*beforeIR));
    std::vector<OracleAttempt> oracleAttempts = readOracleAttempts(dump);
    bool foundCase = false;
    for (const OracleAttempt &oracle : oracleAttempts) {
      std::set<std::string> oracleNames;
      for (const auto &item : oracle.buffers)
        oracleNames.insert(item.first);
      if (oracleNames != allocNames)
        continue;
      foundCase = true;
      cvub::LifetimeAnalysis model =
          cvub::analyzeLifetimes(*beforeIR, "AIV", oracle.seed);
      cvub::PlanMemoryModelResult modelAttempt =
          cvub::PlanLocalMemoryForSeed(*beforeIR, oracle.seed);
      std::map<std::string, cvub::LifetimeRecord> modelLife;
      for (const cvub::LifetimeRecord &record : model.records)
        modelLife[record.name] = record;
      int bufferFailures = 0;
      for (const auto &item : oracle.buffers) {
        auto it = modelLife.find(item.first);
        if (it == modelLife.end() ||
            it->second.directAllocTime != item.second.allocTime ||
            it->second.directFreeTime != item.second.freeTime)
          ++bufferFailures;
      }
      std::map<int, std::pair<std::vector<std::string>,
                             std::vector<std::string>>> modelGenKill;
      for (const cvub::GenKillRecord &record : model.genKillMap)
        modelGenKill[record.operationIndex] = {record.gen, record.kill};
      bool genKillOrderMatch = modelGenKill == oracle.genKillMap;
      auto sortGenKill = [](auto genKillMap) {
        for (auto &item : genKillMap) {
          std::sort(item.second.first.begin(), item.second.first.end());
          std::sort(item.second.second.begin(), item.second.second.end());
        }
        return genKillMap;
      };
      bool genKillSetMatch =
          sortGenKill(modelGenKill) == sortGenKill(oracle.genKillMap);
      std::set<std::pair<std::string, std::string>> modelInplace;
      for (auto pair : model.initialInplacePairList) {
        if (pair.first == pair.second)
          continue;
        if (pair.second < pair.first)
          std::swap(pair.first, pair.second);
        modelInplace.insert(std::move(pair));
      }
      std::set<std::pair<std::string, std::string>> oracleInplace;
      for (auto pair : oracle.inplacePairs)
        if (pair.first != pair.second)
          oracleInplace.insert(std::move(pair));
      bool inplaceMatch = modelInplace == oracleInplace;
      bool multiMatch = model.buffer2MultiNum == oracle.buffer2MultiNum;
      bool expectedPlanSuccess = modelAttempt.success;
      bool planSuccessMatch = !oracle.planSuccess.has_value() ||
                              *oracle.planSuccess == expectedPlanSuccess;
      std::map<std::string, std::vector<uint64_t>> modelOffsets;
      for (const cvub::PlannedBufferRecord &record : modelAttempt.buffers)
        modelOffsets[record.name] = record.offsetsBytes;
      bool offsetMatch = modelOffsets == oracle.plannedOffsets;
      int ubScope = oracle.buffers.empty()
                        ? -1
                        : oracle.buffers.begin()->second.bufferScope;
      auto oraclePeak = oracle.peakBitsByScope.find(ubScope);
      bool peakMatch = oraclePeak == oracle.peakBitsByScope.end() ||
                       oraclePeak->second == modelAttempt.peakBits;
      auto oracleRequired = oracle.requiredBitsByScope.find(ubScope);
      bool requiredMatch = oracleRequired == oracle.requiredBitsByScope.end() ||
                           oracleRequired->second == modelAttempt.requiredBits;
      bool match = bufferFailures == 0 && genKillSetMatch &&
                   genKillOrderMatch && inplaceMatch && multiMatch &&
                   planSuccessMatch && offsetMatch && peakMatch &&
                   requiredMatch;
      failures += match ? 0 : 1;
      sameSeedLayoutFailures += genKillOrderMatch && offsetMatch ? 0 : 1;
      ++attemptsChecked;
      os << caseDir.filename().string() << '\t' << oracle.seed << '\t'
         << (match ? "true" : "false") << '\t' << bufferFailures << '\t'
         << (genKillSetMatch ? "true" : "false") << '\t'
         << (genKillOrderMatch ? "true" : "false") << '\t'
         << (inplaceMatch ? "true" : "false") << '\t'
         << (multiMatch ? "true" : "false") << '\t'
         << (planSuccessMatch ? "true" : "false") << '\t'
         << (offsetMatch ? "true" : "false") << '\t'
         << (peakMatch ? "true" : "false") << '\t'
         << (requiredMatch ? "true" : "false") << '\n';
    }
    cases += foundCase ? 1 : 0;
  }
  if (!opts.quiet) {
    std::cout << "validated_attempt_cases=" << cases << "\n";
    std::cout << "validated_attempts=" << attemptsChecked << "\n";
    std::cout << "semantic_failures=" << failures << "\n";
    std::cout << "same_seed_layout_failures=" << sameSeedLayoutFailures
              << "\n";
  }
  return cases > 0 && failures == 0 ? 0 : 1;
}

int validateMemoryPlan(const Options &opts) {
  if (opts.root.empty()) {
    std::cerr << "[ERROR] --root is required\n";
    return 1;
  }
  std::ofstream file;
  std::ostream &os = openOutput(opts.output, file);
  os << "case\tpeak_match\tlayout_match\texpected_peak_bits\t"
        "actual_peak_bits\tdelta_bits\tselected_seed\toffset_mismatches\t"
        "extent_mismatches\tlifetime_mismatches\tbefore_ir\tmemory_info\n";
  int cases = 0;
  int peakFailures = 0;
  int layoutFailures = 0;
  for (const cvub::fs::path &caseDir : listCaseDirs(opts.root)) {
    cvub::fs::path memoryPath = caseDir / "memory_info_aiv.json";
    auto beforeIR = findFileWithSuffix(caseDir, ".before_local_plan_memory.mlir");
    if (!cvub::fs::exists(memoryPath) || !beforeIR)
      continue;
    std::vector<cvub::MemoryRecord> oracle = cvub::loadMemoryInfo(memoryPath);
    cvub::ReplayResult replay = cvub::replayMemoryInfoPeak(oracle);
    cvub::PlanMemoryModelResult model = cvub::PlanLocalMemory(*beforeIR);
    std::map<std::string, cvub::MemoryRecord> oracleByName;
    for (const cvub::MemoryRecord &record : oracle)
      oracleByName[record.name] = record;
    int offsetMismatches = 0;
    int extentMismatches = 0;
    int lifetimeMismatches = 0;
    for (const cvub::PlannedBufferRecord &record : model.buffers) {
      auto it = oracleByName.find(record.name);
      if (it == oracleByName.end()) {
        ++offsetMismatches;
        ++extentMismatches;
        ++lifetimeMismatches;
        continue;
      }
      offsetMismatches += record.offsetsBytes == it->second.offsetsBytes ? 0 : 1;
      extentMismatches += record.extentBits == it->second.extentBits ? 0 : 1;
      lifetimeMismatches +=
          std::make_pair(record.allocTime, record.freeTime) ==
                  it->second.lifeTimeInIR
              ? 0
              : 1;
    }
    bool peakMatch = model.peakBits == replay.peakBits;
    bool layoutMatch = offsetMismatches == 0 && extentMismatches == 0 &&
                       lifetimeMismatches == 0 &&
                       model.buffers.size() == oracle.size();
    peakFailures += peakMatch ? 0 : 1;
    layoutFailures += layoutMatch ? 0 : 1;
    ++cases;
    os << caseDir.filename().string() << "\t"
       << (peakMatch ? "true" : "false") << "\t"
       << (layoutMatch ? "true" : "false") << "\t" << replay.peakBits
       << "\t" << model.peakBits << "\t"
       << static_cast<int64_t>(model.peakBits) -
              static_cast<int64_t>(replay.peakBits)
       << "\t" << model.selectedSeed << "\t" << offsetMismatches << "\t"
       << extentMismatches << "\t" << lifetimeMismatches << "\t"
       << beforeIR->string() << "\t" << memoryPath.string() << "\n";
  }
  if (!opts.quiet) {
    std::cout << "validated_plan_cases=" << cases << "\n";
    std::cout << "peak_failures=" << peakFailures << "\n";
    std::cout << "layout_failures=" << layoutFailures << "\n";
  }
  return cases > 0 && peakFailures == 0 && layoutFailures == 0 ? 0 : 1;
}
} // namespace

int main(int argc, char **argv) {
  Options opts = parseOptions(argc, argv);
  try {
    if (opts.action == "validate-memory-info-replay")
      return validateMemoryInfoReplay(opts);
    if (opts.action == "validate-local-ub-allocations")
      return validateLocalUBAllocations(opts);
    if (opts.action == "validate-lifetimes")
      return validateLifetimes(opts);
    if (opts.action == "validate-planmemory-attempts")
      return validatePlanMemoryAttempts(opts);
    if (opts.action == "validate-memory-plan")
      return validateMemoryPlan(opts);
    if (opts.action == "validate-fixed-seed-lifetimes")
      return validateFixedSeedLifetimes(opts);
    if (opts.action == "validate-fixed-seed-plans")
      return validateFixedSeedPlans(opts);
    if (opts.action == "dump-c1-semantic-ir") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(opts.output,
                    cvub::SerializeC1SemanticIR(
                        cvub::ParseC1GenericIR(opts.root)));
      return 0;
    }
    if (opts.action == "dump-generic-semantic-ir") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(opts.output, cvub::SerializeC1SemanticIR(
                                     cvub::ParseC1GenericIR(opts.root, false)));
      return 0;
    }
    if (opts.action == "model-cvpipelining-semantic-ir") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::CVPipeliningOptions options;
      options.disabled = opts.disableCVPipelining;
      options.setDepthInUnrollMode = opts.cvPipelineDepth;
      options.enableSkewMode = opts.enablePreload;
      options.enableLazyLoading = opts.enableCVLazyLoading;
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      writeTextFile(
          opts.output,
          cvub::SerializeC1SemanticIR(cvub::RunCVPipeliningPass(
              std::move(module), options)));
      return 0;
    }
    if (opts.action == "model-cvpipelining-analysis") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      std::optional<cvub::CVPipelineAnalysisResult> selected;
      for (const cvub::C1OperationRecord &operation : module.operations) {
        if (operation.name != "scf.for")
          continue;
        cvub::CVPipelineImplAnalysis candidate(
            module, operation.id, opts.cvPipelineDepth,
            opts.enableCVLazyLoading);
        cvub::CVPipelineAnalysisResult result = candidate.run();
        if (!selected)
          selected = result;
        if (result.success) {
          selected = std::move(result);
          break;
        }
      }
      if (!selected)
        throw std::runtime_error("CVPipelining analysis: no scf.for");
      writeTextFile(opts.output, cvub::SerializeCVPipelineAnalysis(*selected));
      return 0;
    }
    if (opts.action == "model-cvpipelining-normal-rewrite") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      std::optional<cvub::CVPipelineAnalysisResult> selected;
      for (const cvub::C1OperationRecord &operation : module.operations) {
        if (operation.name != "scf.for")
          continue;
        cvub::CVPipelineImplAnalysis candidate(
            module, operation.id, opts.cvPipelineDepth,
            opts.enableCVLazyLoading);
        cvub::CVPipelineAnalysisResult result = candidate.run();
        if (result.success) {
          selected = std::move(result);
          break;
        }
      }
      if (!selected)
        throw std::runtime_error("CVPipelining rewrite: no eligible scf.for");
      cvub::CVPipelineLoopRewriter rewriter(module, std::move(*selected));
      if (!rewriter.rewrite())
        throw std::runtime_error("CVPipelining rewrite failed");
      writeTextFile(opts.output, cvub::SerializeC1SemanticIR(module));
      return 0;
    }
    if (opts.action == "model-cvpipelining-preload-rewrite") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      std::optional<cvub::CVPipelineAnalysisResult> selected;
      for (const cvub::C1OperationRecord &operation : module.operations) {
        if (operation.name != "scf.for")
          continue;
        cvub::CVPipelineImplAnalysis candidate(
            module, operation.id, opts.cvPipelineDepth,
            opts.enableCVLazyLoading);
        cvub::CVPipelineAnalysisResult result = candidate.run();
        if (result.success) {
          selected = std::move(result);
          break;
        }
      }
      if (!selected)
        throw std::runtime_error("CVPipelining preload rewrite: no eligible scf.for");
      cvub::CVPipelinePreloadRewriter rewriter(module, std::move(*selected));
      if (!rewriter.rewrite())
        throw std::runtime_error("CVPipelining preload rewrite failed");
      writeTextFile(opts.output, cvub::SerializeC1SemanticIR(module));
      return 0;
    }
    if (opts.action == "dump-inline-load-copy-candidates") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(opts.output, cvub::CollectInlineLoadCopyCandidates(
                                     cvub::ParseC1GenericIR(opts.root, false)));
      return 0;
    }
    if (opts.action == "dump-mark-multi-buffer-oracle") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(opts.output, cvub::CollectMarkMultiBufferOracle(
                                     cvub::ParseC1GenericIR(opts.root, false)));
      return 0;
    }
    if (opts.action == "dump-planmemory-input-buffer-oracle") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(opts.output, cvub::CollectPlanMemoryInputBufferOracle(
                                     cvub::ParseC1GenericIR(opts.root, false)));
      return 0;
    }
    if (opts.action == "dump-planmemory-input-access-oracle") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(opts.output, cvub::CollectPlanMemoryInputAccessOracle(
                                     cvub::ParseC1GenericIR(opts.root, false)));
      return 0;
    }
    if (opts.action == "dump-planmemory-operation-sequence-oracle") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(opts.output, cvub::CollectPlanMemoryOperationSequenceOracle(
                                     cvub::ParseC1GenericIR(opts.root, false)));
      return 0;
    }
    if (opts.action == "dump-planmemory-structured-input-oracle") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(
          opts.output,
          cvub::SerializePlanMemoryInput(
              cvub::ParsePlanMemoryInput(opts.root, "AIV")));
      return 0;
    }
    if (opts.action == "dump-planmemory-canonical-input-oracle") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      writeTextFile(
          opts.output,
          cvub::SerializeCanonicalPlanMemoryInput(
              cvub::ParsePlanMemoryInput(opts.root, "AIV")));
      return 0;
    }
    if (opts.action == "dump-suffix-buffer-topology") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      const cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      writeTextFile(opts.output, cvub::SerializeSuffixBufferTopology(
                                      cvub::BuildSuffixBufferTopology(module)));
      return 0;
    }
    if (opts.action == "dump-c2-allocation-oracle" ||
        opts.action == "model-one-shot-bufferize-allocations") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      const cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      const std::vector<cvub::BufferAllocation> allocations =
          opts.action == "dump-c2-allocation-oracle"
              ? cvub::CollectBufferAllocationOracle(module)
              : cvub::ModelOneShotBufferizeAllocationsExact(module);
      writeTextFile(opts.output,
                    cvub::SerializeBufferAllocations(allocations));
      return 0;
    }
    if (opts.action == "model-bufferized-semantic-ir") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(module);
      writeTextFile(opts.output,
                    cvub::SerializeBufferizedSemanticIR(
                        cvub::BuildBufferizedSemanticIR(module,
                                                        bufferization)));
      return 0;
    }
    if (opts.action == "model-c3-semantic-ir") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(module);
      const cvub::BufferizedSemanticIR bufferized =
          cvub::BuildBufferizedSemanticIR(module, bufferization);
      writeTextFile(opts.output,
                    cvub::SerializeC3SemanticIR(
                        cvub::BuildC3SemanticIR(bufferized)));
      return 0;
    }
    if (opts.action == "model-c4-semantic-ir" ||
        opts.action == "model-c4-buffers") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(module);
      cvub::C4SemanticIR c4 = cvub::BuildC4SemanticIR(
          cvub::BuildC3SemanticIR(
              cvub::BuildBufferizedSemanticIR(module, bufferization)));
      writeTextFile(opts.output,
                    opts.action == "model-c4-buffers"
                        ? cvub::SerializeC4Buffers(c4.buffers)
                        : cvub::SerializeC4SemanticIR(c4));
      return 0;
    }
    if (opts.action == "model-c4-buffer-projection" ||
        opts.action == "dump-c4-buffer-projection-oracle" ||
        opts.action == "dump-c4-buffers-oracle") {
      if (opts.root.empty() || opts.output.empty() ||
          (opts.action != "model-c4-buffer-projection" &&
           (opts.attemptRoot.empty() || opts.afterIr.empty() ||
            opts.expectedSummary.empty()))) {
        std::cerr << "[ERROR] C4 projection requires --root, --output and "
                     "oracle --attempt-root/--after-ir/--expected-summary\n";
        return 1;
      }
      std::vector<cvub::C4BufferRecord> buffers;
      if (opts.action == "model-c4-buffer-projection") {
        cvub::C1SemanticModule module =
            cvub::ParseC1GenericIR(opts.root, false);
        for (cvub::C1OperationRecord &operation : module.operations)
          if (cvub::C1IsReviewedOperation(operation.name))
            cvub::ApplyC1OpSemantics(operation);
        const cvub::OneShotBufferizationResult bufferization =
            cvub::RunOneShotBufferize(module);
        buffers = cvub::BuildC4SemanticIR(cvub::BuildC3SemanticIR(
            cvub::BuildBufferizedSemanticIR(module, bufferization))).buffers;
      } else {
        std::vector<std::pair<cvub::C1SemanticModule,
                              cvub::C1SemanticModule>> extraPairs;
        for (const auto &entry :
             cvub::fs::recursive_directory_iterator(opts.expectedSummary)) {
          if (!entry.is_regular_file() ||
              entry.path().filename() != "7_18_hivm-alloc-extra-buffer.mlir")
            continue;
          const cvub::fs::path after = entry.path().parent_path() /
              "7_19_hivm-alloc-extra-buffer.mlir";
          if (!cvub::fs::is_regular_file(after))
            throw std::runtime_error("C4 oracle: missing AllocExtraBuffer after");
          extraPairs.push_back({
              cvub::ParseC1GenericIR(entry.path(), false),
              cvub::ParseC1GenericIR(after, false)});
        }
        buffers = cvub::CollectC4BufferOracle(
            cvub::ParseC1GenericIR(opts.attemptRoot, false),
            cvub::ParseC1GenericIR(opts.afterIr, false),
            cvub::CollectC4ExtraBufferKeys(extraPairs));
      }
      writeTextFile(opts.output,
                    opts.action == "dump-c4-buffers-oracle"
                        ? cvub::SerializeC4Buffers(buffers)
                        : cvub::SerializeC4BufferProjection(buffers));
      return 0;
    }
    if (opts.action == "model-c5-semantic-ir" ||
        opts.action == "model-c5-buffer-projection") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(module);
      const cvub::C5SemanticIR c5 = cvub::BuildC5SemanticIR(
          cvub::BuildC4SemanticIR(cvub::BuildC3SemanticIR(
              cvub::BuildBufferizedSemanticIR(module, bufferization))));
      writeTextFile(opts.output,
                    opts.action == "model-c5-buffer-projection"
                        ? cvub::SerializeC5BufferProjection(c5.buffers)
                        : cvub::SerializeC5SemanticIR(c5));
      return 0;
    }
    if (opts.action == "model-c6-semantic-ir" ||
        opts.action == "model-c6-multi-buffer-projection") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(module);
      cvub::MarkMultiBufferOptions options;
      options.enableAuto = opts.enableAutoMultiBuffer;
      options.limitAutoMultiBufferOfLocalBuffer =
          cvub::ParseMultiBufferStrategy(opts.localMultiBufferStrategy);
      options.limitMixAutoMultiBufferBuffer =
          cvub::ParseMultiBufferStrategy(opts.mixMultiBufferStrategy);
      const cvub::C6SemanticIR c6 = cvub::BuildC6SemanticIR(
          cvub::BuildC5SemanticIR(cvub::BuildC4SemanticIR(
              cvub::BuildC3SemanticIR(cvub::BuildBufferizedSemanticIR(
                  module, bufferization)))),
          options);
      writeTextFile(opts.output,
                    opts.action == "model-c6-multi-buffer-projection"
                        ? cvub::SerializeC6MultiBufferProjection(c6)
                        : cvub::SerializeC6SemanticIR(c6));
      return 0;
    }
    if (opts.action == "model-planmemory-input-semantic-ir" ||
        opts.action == "model-planmemory-input-buffers" ||
        opts.action == "model-planmemory-input-accesses" ||
        opts.action == "model-planmemory-input-access-debug" ||
        opts.action == "model-planmemory-structured-input" ||
        opts.action == "model-planmemory-canonical-input" ||
        opts.action == "model-planmemory-operation-sequence" ||
        opts.action == "model-planmemory-structured-lifetime" ||
        opts.action == "model-planmemory-live-shuffle" ||
        opts.action == "model-planmemory-structured-plan") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(module);
      cvub::MarkMultiBufferOptions options;
      options.enableAuto = opts.enableAutoMultiBuffer;
      options.limitAutoMultiBufferOfLocalBuffer =
          cvub::ParseMultiBufferStrategy(opts.localMultiBufferStrategy);
      options.limitMixAutoMultiBufferBuffer =
          cvub::ParseMultiBufferStrategy(opts.mixMultiBufferStrategy);
      const cvub::PlanMemoryInputSemanticIR c7 =
          cvub::BuildPlanMemoryInputSemanticIR(cvub::BuildC6SemanticIR(
              cvub::BuildC5SemanticIR(cvub::BuildC4SemanticIR(
                  cvub::BuildC3SemanticIR(cvub::BuildBufferizedSemanticIR(
                      module, bufferization)))),
              options));
      if (opts.action == "model-planmemory-structured-input" ||
          opts.action == "model-planmemory-canonical-input" ||
          opts.action == "model-planmemory-operation-sequence" ||
          opts.action == "model-planmemory-structured-lifetime" ||
          opts.action == "model-planmemory-live-shuffle" ||
          opts.action == "model-planmemory-structured-plan") {
        const cvub::PlanMemoryInput input = cvub::BuildPlanMemoryInput(c7);
        if (opts.action == "model-planmemory-structured-input") {
          writeTextFile(opts.output, cvub::SerializePlanMemoryInput(input));
          return 0;
        }
        if (opts.action == "model-planmemory-canonical-input") {
          writeTextFile(opts.output,
                        cvub::SerializeCanonicalPlanMemoryInput(input));
          return 0;
        }
        if (opts.action == "model-planmemory-operation-sequence") {
          writeTextFile(opts.output,
                        cvub::SerializePlanMemoryOperationSequence(input));
          return 0;
        }
        const uint32_t seed =
            opts.randomSeed < 0 ? 0U : static_cast<uint32_t>(opts.randomSeed);
        std::ostringstream output;
        if (opts.action == "model-planmemory-live-shuffle") {
          const cvub::LifetimeAnalysis analysis =
              cvub::analyzeLifetimes(input, seed, false);
          output << "PLANMEMORY_LIVE_SHUFFLE\t1\n";
          for (const cvub::LiveShuffleRecord &record :
               analysis.liveShuffleTrace) {
            output << "LIVE_BEFORE\t" << record.operationIndex;
            for (const std::string &value : record.before)
              output << '\t' << cvub::HexEncode(value);
            output << '\n' << "LIVE_AFTER\t" << record.operationIndex;
            for (const std::string &value : record.after)
              output << '\t' << cvub::HexEncode(value);
            output << '\n';
          }
          writeTextFile(opts.output, output.str());
          return 0;
        }
        if (opts.action == "model-planmemory-structured-lifetime") {
          const cvub::LifetimeAnalysis analysis =
              cvub::analyzeLifetimes(input, seed, false);
          output << "PLANMEMORY_STRUCTURED_LIFETIME\t1\n";
          std::map<std::string, size_t> ids;
          std::map<std::string, cvub::LifetimeRecord> lives;
          const std::map<std::string, std::string> debugNames =
              planMemoryDebugBufferNames(analysis.operations);
          for (const cvub::LifetimeRecord &record : analysis.records)
            lives[record.name] = record;
          for (size_t id = 0; id < input.allocations.size(); ++id) {
            const cvub::IRAllocRecord &allocation = input.allocations[id];
            ids[allocation.name] = id;
            const cvub::LifetimeRecord &record = lives.at(allocation.name);
            output << "BUFFER\t" << id << '\t'
                   << cvub::HexEncode(debugNames.count(allocation.name)
                                          ? debugNames.at(allocation.name)
                                          : allocation.name)
                   << '\t'
                   << allocation.constBits << '\t'
                   << static_cast<int>(analysis.bufferIgnoreInplace.count(
                                           allocation.name) &&
                                       analysis.bufferIgnoreInplace.at(
                                           allocation.name))
                   << '\t' << record.directAllocTime << '\t'
                   << record.directFreeTime << '\n';
          }
          for (const cvub::GenKillRecord &record : analysis.genKillMap) {
            output << "GEN_KILL\t" << record.operationIndex;
            for (const std::string &name : record.gen)
              output << "\tg:" << ids.at(name);
            for (const std::string &name : record.kill)
              output << "\tk:" << ids.at(name);
            output << '\n';
          }
          for (const auto &[first, second] : analysis.initialInplacePairList)
            output << "INPLACE\t" << ids.at(first) << '\t' << ids.at(second)
                   << '\n';
          for (const auto &[name, count] : analysis.buffer2MultiNum)
            output << "MULTI\t" << ids.at(name) << '\t' << count << '\n';
        } else {
          const cvub::PlanMemoryModelResult plan =
              cvub::PlanLocalMemoryForSeed(input, seed, false);
          output << "PLANMEMORY_STRUCTURED_PLAN\t1\n"
                 << "STATUS\t" << (plan.success ? "success" : "overflow")
                 << '\n' << "PEAK_BITS\t" << plan.peakBits << '\n'
                 << "REQUIRED_BITS\t" << plan.requiredBits << '\n';
          std::map<std::string, size_t> ids;
          const cvub::LifetimeAnalysis analysis =
              cvub::analyzeLifetimes(input, seed, false);
          const std::map<std::string, std::string> debugNames =
              planMemoryDebugBufferNames(analysis.operations);
          for (size_t id = 0; id < input.allocations.size(); ++id)
            ids[input.allocations[id].name] = id;
          for (const cvub::PlannedBufferRecord &buffer : plan.buffers) {
            output << "BUFFER\t" << ids.at(buffer.name) << '\t'
                   << cvub::HexEncode(debugNames.count(buffer.name)
                                          ? debugNames.at(buffer.name)
                                          : buffer.name)
                   << '\t' << buffer.constBits;
            for (uint64_t offset : buffer.offsetsBytes)
              output << '\t' << offset;
            output << '\n';
          }
        }
        writeTextFile(opts.output, output.str());
        return 0;
      }
      writeTextFile(
          opts.output,
          opts.action == "model-planmemory-input-buffers"
              ? cvub::SerializePlanMemoryInputBuffers(c7)
              : opts.action == "model-planmemory-input-accesses"
                    ? cvub::SerializePlanMemoryInputAccesses(c7)
                    : opts.action == "model-planmemory-input-access-debug"
                          ? cvub::SerializePlanMemoryInputAccessDebug(c7)
                    : cvub::SerializePlanMemoryInputSemanticIR(c7));
      return 0;
    }
    if (opts.action == "model-post-bufferization-allocations") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : module.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(module);
      const cvub::HIVMOptSinglePointResult normalized =
          cvub::ModelHIVMOptSinglePoint(
              cvub::BuildBufferizedSemanticIR(module, bufferization));
      writeTextFile(opts.output,
                    cvub::SerializeBufferAllocations(normalized.allocations));
      return 0;
    }
    if (opts.action == "model-non-contiguous-reshape-to-copy" ||
        opts.action == "dump-non-contiguous-reshape-to-copy-oracle") {
      if (opts.root.empty() || opts.output.empty() ||
          (opts.action == "dump-non-contiguous-reshape-to-copy-oracle" &&
           opts.afterIr.empty())) {
        std::cerr << "[ERROR] non-contiguous reshape requires --root, "
                     "--output and oracle --after-ir\n";
        return 1;
      }
      const cvub::C1SemanticModule before =
          cvub::ParseC1GenericIR(opts.root, false);
      const std::vector<cvub::NonContiguousReshapeCopy> copies =
          opts.action == "model-non-contiguous-reshape-to-copy"
              ? cvub::ModelConvertNonContiguousReshapeToCopy(before)
              : cvub::CollectNonContiguousReshapeToCopyOracle(
                    before, cvub::ParseC1GenericIR(opts.afterIr, false));
      writeTextFile(opts.output,
                    cvub::SerializeNonContiguousReshapeCopies(copies));
      return 0;
    }
    if (opts.action == "model-canonicalized-iter-arg-results" ||
        opts.action == "dump-canonicalized-iter-arg-results-oracle") {
      if (opts.root.empty() || opts.output.empty() ||
          (opts.action == "dump-canonicalized-iter-arg-results-oracle" &&
           opts.afterIr.empty())) {
        std::cerr << "[ERROR] canonicalized-iter-arg action requires --root, "
                     "--output and oracle --after-ir\n";
        return 1;
      }
      if (opts.action == "model-canonicalized-iter-arg-results") {
        cvub::C1SemanticModule module =
            cvub::ParseC1GenericIR(opts.root, false);
        for (cvub::C1OperationRecord &operation : module.operations)
          if (cvub::C1IsReviewedOperation(operation.name))
            cvub::ApplyC1OpSemantics(operation);
        const cvub::OneShotBufferizationResult bufferization =
            cvub::RunOneShotBufferize(module);
        const cvub::HIVMOptSinglePointResult normalized =
            cvub::ModelHIVMOptSinglePoint(
                cvub::BuildBufferizedSemanticIR(module, bufferization));
        writeTextFile(
            opts.output, cvub::SerializeCanonicalizedIterArgResults(
                             normalized.canonicalizedIterArgResults));
      } else {
        writeTextFile(
            opts.output,
            cvub::SerializeCanonicalizedIterArgResults(
                cvub::CollectCanonicalizedIterArgResultsOracle(
                    cvub::ParseC1GenericIR(opts.root, false),
                    cvub::ParseC1GenericIR(opts.afterIr, false))));
      }
      return 0;
    }
    if (opts.action == "validate-c3-operation-rewrites") {
      if (opts.root.empty() || opts.attemptRoot.empty() ||
          opts.afterIr.empty() || opts.output.empty() ||
          opts.function.empty()) {
        std::cerr << "[ERROR] C3 rewrite validation requires source --root, "
                     "pass --attempt-root, --after-ir, --function and "
                     "--output\n";
        return 1;
      }
      cvub::C1SemanticModule source =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : source.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(source);
      const cvub::C3SemanticIR model = cvub::BuildC3SemanticIR(
          cvub::BuildBufferizedSemanticIR(source, bufferization));
      const cvub::C1SemanticModule before = cvub::ParseC1GenericIR(
          opts.attemptRoot, false);
      const cvub::C1SemanticModule after = cvub::ParseC1GenericIR(
          opts.afterIr, false);
      const cvub::C3RewriteValidation validation =
          cvub::ValidateC3OperationRewrites(model, before, after,
                                            opts.function);
      std::ostringstream report;
      report << "C3_OPERATION_REWRITE_VALIDATION\t1\nSUMMARY\t"
             << validation.checkedSignatures << '\t'
             << validation.errors.size() << '\n';
      for (const std::string &error : validation.errors)
        report << "ERROR\t" << cvub::HexEncode(error) << '\n';
      writeTextFile(opts.output, report.str());
      return validation.errors.empty() ? 0 : 1;
    }
    if (opts.action == "validate-bufferized-semantic-ir") {
      if (opts.root.empty() || opts.afterIr.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root, --after-ir and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule before =
          cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : before.operations)
        if (cvub::C1IsReviewedOperation(operation.name))
          cvub::ApplyC1OpSemantics(operation);
      const cvub::OneShotBufferizationResult bufferization =
          cvub::RunOneShotBufferize(before);
      const cvub::BufferizedSemanticIR model =
          cvub::BuildBufferizedSemanticIR(before, bufferization);
      const cvub::BufferizedSemanticIRValidation validation =
          cvub::ValidateBufferizedSemanticIR(
              before, model,
              cvub::ParseC1GenericIR(opts.afterIr, false));
      std::ostringstream report;
      report << "BUFFERIZED_SEMANTIC_IR_VALIDATION\t1\n"
             << "SUMMARY\t" << validation.checkedValues << '\t'
             << validation.checkedAccesses << '\t'
             << validation.errors.size() << '\n';
      for (const std::string &error : validation.errors)
        report << "ERROR\t" << cvub::HexEncode(error) << '\n';
      writeTextFile(opts.output, report.str());
      return validation.errors.empty() ? 0 : 1;
    }
    if (opts.action == "model-alloc-extra-buffer" ||
        opts.action == "dump-alloc-extra-buffer-oracle") {
      if (opts.root.empty() || opts.output.empty() ||
          (opts.action == "dump-alloc-extra-buffer-oracle" &&
           opts.afterIr.empty())) {
        std::cerr << "[ERROR] --root, --output and oracle --after-ir are "
                     "required\n";
        return 1;
      }
      cvub::C1SemanticModule before = cvub::ParseC1GenericIR(opts.root, false);
      for (cvub::C1OperationRecord &operation : before.operations)
        if (cvub::HasAllocExtraBufferInterface(operation.name) &&
            operation.name != "hivm.hir.vxor")
          cvub::ApplyC1OpSemantics(operation);
      const std::vector<cvub::ExtraBufferAllocation> allocations =
          opts.action == "model-alloc-extra-buffer"
              ? cvub::ModelAllocExtraBuffer(before, opts.function)
              : cvub::CollectAllocExtraBufferOracle(
                    before, cvub::ParseC1GenericIR(opts.afterIr, false));
      writeTextFile(opts.output,
                    cvub::SerializeExtraBufferAllocations(allocations));
      return 0;
    }
    if (opts.action == "model-hivm-decompose-op" ||
        opts.action == "dump-hivm-decompose-op-oracle") {
      if (opts.root.empty() || opts.output.empty() || opts.function.empty() ||
          (opts.action == "dump-hivm-decompose-op-oracle" &&
           opts.afterIr.empty())) {
        std::cerr << "[ERROR] --root, --function, --output and oracle "
                     "--after-ir are required\n";
        return 1;
      }
      const cvub::C1SemanticModule before =
          cvub::ParseC1GenericIR(opts.root, false);
      const std::vector<cvub::DecomposeBufferAllocation> allocations =
          opts.action == "model-hivm-decompose-op"
              ? cvub::ModelHIVMDecomposeOp(before, opts.function)
              : cvub::CollectHIVMDecomposeOpOracle(
                    before, cvub::ParseC1GenericIR(opts.afterIr, false),
                    opts.function);
      writeTextFile(opts.output,
                    cvub::SerializeDecomposeBufferAllocations(allocations));
      return 0;
    }
    if (opts.action == "model-hivm-decompose-operation-delta" ||
        opts.action == "dump-hivm-decompose-operation-delta-oracle") {
      if (opts.root.empty() || opts.output.empty() || opts.function.empty() ||
          (opts.action == "dump-hivm-decompose-operation-delta-oracle" &&
           opts.afterIr.empty())) {
        std::cerr << "[ERROR] decompose delta requires --root, --function, "
                     "--output and oracle --after-ir\n";
        return 1;
      }
      const cvub::C1SemanticModule before =
          cvub::ParseC1GenericIR(opts.root, false);
      const cvub::DecomposeOperationDelta delta =
          opts.action == "model-hivm-decompose-operation-delta"
              ? cvub::ModelHIVMDecomposeOperationDelta(before, opts.function)
              : cvub::CollectHIVMDecomposeOperationDeltaOracle(
                    before, cvub::ParseC1GenericIR(opts.afterIr, false),
                    opts.function);
      writeTextFile(opts.output,
                    cvub::SerializeDecomposeOperationDelta(delta));
      return 0;
    }
    if (opts.action == "dump-one-shot-analysis-oracle" ||
        opts.action == "model-one-shot-analysis") {
      if (opts.root.empty() || opts.output.empty()) {
        std::cerr << "[ERROR] --root and --output are required\n";
        return 1;
      }
      cvub::C1SemanticModule module = cvub::ParseC1GenericIR(opts.root, false);
      if (opts.action == "model-one-shot-analysis")
        for (cvub::C1OperationRecord &operation : module.operations)
          if (cvub::C1IsReviewedOperation(operation.name))
            cvub::ApplyC1OpSemantics(operation);
      const std::vector<cvub::OneShotOpOperandDecision> decisions =
          opts.action == "dump-one-shot-analysis-oracle"
              ? cvub::CollectOneShotAnalysisOracle(module)
              : cvub::ModelOneShotAnalysis(module);
      writeTextFile(opts.output, cvub::SerializeOneShotAnalysis(decisions));
      return 0;
    }
    std::cerr << "[ERROR] unknown or missing --action: " << opts.action << "\n";
    return 1;
  } catch (const std::exception &ex) {
    std::cerr << "[ERROR] " << ex.what() << "\n";
    return 1;
  }
}
