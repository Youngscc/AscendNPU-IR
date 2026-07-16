// Lightweight SemanticIR and MLIR text parsing shared by UB-affecting passes and PlanMemory.
#ifndef CVPIPELINE_UB_MODEL_CPP_SEMANTIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_SEMANTIC_IR_HPP

#include "../support/checked_math.hpp"
#include "hivm_op_semantics.hpp"
#include "memref_type_model.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cvub {
namespace fs = std::filesystem;

constexpr uint64_t kBitsPerByte = 8;
constexpr uint64_t kUBCapacityBits = 192 * 1024 * kBitsPerByte;

struct IRAllocRecord {
  std::string name;
  std::string memrefType;
  uint64_t constBits = 0;
  uint64_t extentBits = 0;
  bool isTmpBufHint = false;
  int line = 0;
};

struct BranchDestination {
  std::string label;
  std::vector<std::string> operands;
};

struct OperationRecord {
  int index = 0;
  int line = 0;
  int indent = 0;
  int operationId = -1;
  std::string text;
  // Optional full semantic identity used by normalization/CSE. This is kept
  // separate from text because canonical PlanMemory output omits most attrs.
  std::string normalizationKey;
  std::string opName;
  std::vector<int> regionPath;
  int blockId = 0;
  std::string blockLabel;
  std::vector<std::string> blockArguments;
  std::vector<std::string> successorLabels;
  std::vector<BranchDestination> branchDestinations;
};

struct LifetimeRecord {
  std::string name;
  int allocTime = -1;
  int freeTime = -1;
  int directAllocTime = -1;
  int directFreeTime = -1;
  std::vector<std::string> group;
};

struct GenKillRecord {
  int operationIndex = -1;
  std::vector<std::string> gen;
  std::vector<std::string> kill;
};

struct LiveShuffleRecord {
  int operationIndex = -1;
  std::vector<std::string> before;
  std::vector<std::string> after;
};

struct LifetimeAnalysis {
  std::vector<LifetimeRecord> records;
  std::vector<OperationRecord> operations;
  std::vector<std::string> bufferGenOrder;
  std::vector<std::pair<std::string, std::string>> initialInplacePairList;
  std::vector<std::pair<std::string, std::string>> inplacePairList;
  std::vector<std::pair<std::string, std::string>> inplacableBufferPairs;
  std::map<std::string, std::string> canonicalAllocByValue;
  std::map<std::string, uint32_t> buffer2MultiNum;
  std::map<std::string, bool> bufferIgnoreInplace;
  std::vector<GenKillRecord> genKillMap;
  std::vector<LiveShuffleRecord> liveShuffleTrace;
};

struct BufferInfoRecord {
  std::string name;
  uint64_t constBits = 0;
  uint64_t extentBits = 0;
  bool ignoreInplace = false;
};

struct PlannedBufferRecord {
  std::string name;
  uint64_t constBits = 0;
  uint64_t extentBits = 0;
  std::vector<uint64_t> offsetsBytes;
  int allocTime = -1;
  int freeTime = -1;
};

struct PlanMemoryModelResult {
  bool success = false;
  bool overflow = false;
  uint32_t selectedSeed = 0;
  uint64_t peakBits = 0;
  uint64_t requiredBits = 0;
  uint64_t capacityBits = kUBCapacityBits;
  std::vector<PlannedBufferRecord> buffers;
  std::vector<std::pair<std::string, std::string>> inplacePairs;
  std::map<std::string, uint32_t> multiBufferNums;
};

inline std::string readFile(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

inline std::string trim(std::string value) {
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(),
                           [](char c) {
                             return !std::isspace(
                                 static_cast<unsigned char>(c));
                           }));
  value.erase(std::find_if(value.rbegin(), value.rend(),
                           [](char c) {
                             return !std::isspace(
                                 static_cast<unsigned char>(c));
                           })
                  .base(),
              value.end());
  return value;
}

inline bool startsWith(const std::string &value, const std::string &prefix) {
  return value.rfind(prefix, 0) == 0;
}

inline bool endsWith(const std::string &value, const std::string &suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
             0;
}

inline std::vector<std::string> split(const std::string &value,
                                      char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  std::istringstream ss(value);
  while (std::getline(ss, current, delimiter))
    parts.push_back(current);
  if (!value.empty() && value.back() == delimiter)
    parts.emplace_back();
  return parts;
}

inline bool ssaLess(const std::string &lhs, const std::string &rhs) {
  if (lhs.size() != rhs.size())
    return lhs.size() < rhs.size();
  return lhs < rhs;
}

inline int braceDelta(const std::string &line) {
  int delta = 0;
  for (char c : line) {
    if (c == '{')
      ++delta;
    else if (c == '}')
      --delta;
  }
  return delta;
}

inline std::vector<std::string>
GetTargetFunctionNames(const fs::path &path, const std::string &coreType) {
  std::ifstream in(path);
  std::vector<std::string> names;
  std::string currentName;
  bool inFunction = false;
  bool coreMatches = false;
  std::string line;
  static const std::regex funcRegex(R"(func\.func\s+@([\w$.#-]+))");
  const std::string coreMarker =
      "hivm.func_core_type = #hivm.func_core_type<" + coreType + ">";

  while (std::getline(in, line)) {
    std::smatch match;
    if (std::regex_search(line, match, funcRegex)) {
      if (inFunction && coreMatches)
        names.push_back(currentName);
      inFunction = true;
      currentName = match[1].str();
      coreMatches = line.find(coreMarker) != std::string::npos;
      continue;
    }
    if (!inFunction)
      continue;
    coreMatches = coreMatches || line.find(coreMarker) != std::string::npos;
  }
  if (inFunction && coreMatches)
    names.push_back(currentName);
  return names;
}

inline std::optional<std::string> RequireUniqueTargetFunction(
    const fs::path &path, const std::string &coreType = "AIV") {
  std::vector<std::string> names = GetTargetFunctionNames(path, coreType);
  if (names.size() > 1) {
    throw std::runtime_error(
        "PlanMemory exact blocker: expected at most one " + coreType +
        " function, found " + std::to_string(names.size()));
  }
  if (names.empty())
    return std::nullopt;
  return names.front();
}

inline int operandDelimiterDelta(const std::string &line) {
  int delta = 0;
  for (char c : line) {
    if (c == '(' || c == '[')
      ++delta;
    else if (c == ')' || c == ']')
      --delta;
  }
  return delta;
}

inline std::string extractSSANameBeforeEqual(const std::string &line) {
  std::string stripped = trim(line);
  if (stripped.empty() || stripped.front() != '%')
    return "";
  size_t equal = stripped.find('=');
  if (equal == std::string::npos)
    return "";
  std::string resultList = trim(stripped.substr(0, equal));
  static const std::regex resultListRegex(
      R"(^%[\w$.#-]+(?::[0-9]+)?(?:\s*,\s*%[\w$.#-]+(?::[0-9]+)?)*$)");
  return std::regex_match(resultList, resultListRegex) ? resultList : "";
}

inline void collectTempBufferNames(const std::string &line,
                                   std::set<std::string> &out) {
  size_t pos = 0;
  while ((pos = line.find("temp_buffer", pos)) != std::string::npos) {
    size_t open = pos + std::string("temp_buffer").size();
    while (open < line.size() &&
           std::isspace(static_cast<unsigned char>(line[open])))
      ++open;
    if (open == line.size() || line[open] != '(') {
      pos = open;
      continue;
    }
    size_t percent = line.find('%', open + 1);
    if (percent == std::string::npos)
      break;
    size_t end = percent + 1;
    while (end < line.size()) {
      char c = line[end];
      if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' ||
            c == '.' || c == '$' || c == '#' || c == '-'))
        break;
      ++end;
    }
    out.insert(line.substr(percent, end - percent));
    pos = end;
  }
}

inline uint64_t getElementTypeBitWidth(const std::string &elementType) {
  if (elementType == "f16" || elementType == "bf16")
    return 16;
  if (elementType == "f32")
    return 32;
  if (elementType == "f64")
    return 64;
  if (elementType == "index")
    return 64;
  if (!elementType.empty() && elementType.front() == 'i')
    return static_cast<uint64_t>(std::stoull(elementType.substr(1)));
  throw std::runtime_error("unsupported static element type: " + elementType);
}

inline uint64_t GetBufferConstBits(const std::string &memrefType) {
  std::optional<MemRefTypeModel> type = ParseMemRefType(memrefType);
  if (!type)
    throw std::runtime_error("missing memref type: " + memrefType);
  uint64_t elementBits = getElementTypeBitWidth(type->elementType);
  uint64_t elementCount = 1;
  for (const std::optional<int64_t> &dimension : type->shape) {
    if (!dimension)
      throw std::runtime_error(
          "Failed to obtain op buffer shape size which should be static: " +
          memrefType);
    if (*dimension < 0)
      throw std::runtime_error("negative static shape: " + memrefType);
    elementCount = CheckedMul(elementCount,
                              static_cast<uint64_t>(*dimension),
                              "static memref element count");
  }
  return CheckedMul(elementCount, elementBits, "static memref bit size");
}

inline std::vector<IRAllocRecord>
parseLocalUBAllocations(const fs::path &path) {
  std::ifstream in(path);
  std::vector<IRAllocRecord> records;
  std::set<std::string> tempBuffers;
  bool inFunction = false;
  bool inAIV = false;
  bool functionBodyStarted = false;
  int functionDepth = 0;
  int lineNo = 0;
  std::string line;
  while (std::getline(in, line)) {
    ++lineNo;
    if (line.find("func.func @") != std::string::npos) {
      inFunction = true;
      inAIV =
          line.find("hivm.func_core_type = #hivm.func_core_type<AIV>") !=
          std::string::npos;
      functionDepth = braceDelta(line);
      functionBodyStarted = functionDepth > 0;
      continue;
    }
    if (!inFunction)
      continue;
    if (line.find("hivm.func_core_type = #hivm.func_core_type<AIV>") !=
        std::string::npos)
      inAIV = true;
    int delta = braceDelta(line);
    if (delta > 0)
      functionBodyStarted = true;
    if (functionBodyStarted && inAIV)
      collectTempBufferNames(line, tempBuffers);
    if (functionBodyStarted && inAIV &&
        line.find("= memref.alloc()") != std::string::npos &&
        line.find("#hivm.address_space<ub>") != std::string::npos) {
      IRAllocRecord record;
      record.name = extractSSANameBeforeEqual(line);
      record.line = lineNo;
      size_t typePos = line.find(": memref<");
      record.memrefType =
          typePos == std::string::npos ? "" : trim(line.substr(typePos + 2));
      record.constBits = GetBufferConstBits(record.memrefType);
      // MemoryDisplay.cpp reports AlignUp(BufferInfo::constBits, 8).
      record.extentBits = AlignUp(record.constBits, kBitsPerByte);
      records.push_back(std::move(record));
    }
    functionDepth += delta;
    if (functionBodyStarted && functionDepth <= 0) {
      inFunction = false;
      inAIV = false;
      functionBodyStarted = false;
      functionDepth = 0;
    }
  }
  for (IRAllocRecord &record : records)
    record.isTmpBufHint = tempBuffers.count(record.name) != 0;
  return records;
}

inline std::string extractOpName(const std::string &line) {
  std::string stripped = trim(line);
  size_t equal = stripped.find('=');
  if (!stripped.empty() && stripped[0] == '%' && equal != std::string::npos)
    stripped = trim(stripped.substr(equal + 1));
  if (!stripped.empty() && stripped.front() == '"') {
    const size_t closingQuote = stripped.find('"', 1);
    if (closingQuote != std::string::npos)
      return stripped.substr(1, closingQuote - 1);
  }
  std::smatch match;
  if (std::regex_search(stripped, match, std::regex(R"(^([\w.]+))")))
    return match[1].str();
  return "";
}

inline bool isOperationLine(const std::string &line) {
  std::string stripped = trim(line);
  if (stripped.empty() || stripped[0] == '}')
    return false;
  // An MLIR continuation may begin with an SSA value ("%arg : type") or an
  // attribute ("broadcast = ...").  Neither is a new Operation.  Require an
  // optional SSA result list followed by a dialect operation name, or one of
  // the builtin operation spellings used by the input IR.
  static const std::regex multiResultPrefix(
      R"(^%[\w$.#:-]+(?:\s*,\s*%[\w$.#:-]+)*\s*,\s*$)");
  if (std::regex_match(stripped, multiResultPrefix))
    return true;
  const std::string opName = extractOpName(stripped);
  return opName == "return" || opName.find('.') != std::string::npos ||
         (!stripped.empty() && stripped.front() == '"');
}

inline std::vector<std::string> extractSSAs(const std::string &fragment) {
  static const std::regex ssaRegex(R"(%[\w$.#-]+)");
  std::vector<std::string> values;
  for (std::sregex_iterator it(fragment.begin(), fragment.end(), ssaRegex), end;
       it != end; ++it)
    values.push_back(it->str());
  return values;
}

inline std::string extractGroupBody(const std::string &line,
                                    const std::string &prefix) {
  size_t prefixPosition = 0;
  size_t start = std::string::npos;
  while ((prefixPosition = line.find(prefix, prefixPosition)) !=
         std::string::npos) {
    size_t open = prefixPosition + prefix.size();
    while (open < line.size() &&
           std::isspace(static_cast<unsigned char>(line[open])))
      ++open;
    if (open < line.size() && line[open] == '(') {
      start = open + 1;
      break;
    }
    prefixPosition = open;
  }
  if (start == std::string::npos)
    return "";
  int depth = 1;
  for (size_t i = start; i < line.size(); ++i) {
    if (line[i] == '(')
      ++depth;
    else if (line[i] == ')') {
      --depth;
      if (depth == 0)
        return line.substr(start, i - start);
    }
  }
  return "";
}

inline std::vector<std::string> extractGroupSSAs(const std::string &line,
                                                 const std::string &prefix) {
  std::string body = extractGroupBody(line, prefix);
  if (body.empty())
    return {};
  return extractSSAs(body);
}

inline std::vector<BranchDestination>
extractBranchDestinations(const std::string &line) {
  std::vector<BranchDestination> destinations;
  size_t position = 0;
  auto isLabelChar = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' ||
           c == '.' || c == '$' || c == '#' || c == '-';
  };
  while ((position = line.find('^', position)) != std::string::npos) {
    size_t end = position + 1;
    while (end < line.size() && isLabelChar(line[end]))
      ++end;
    BranchDestination destination;
    destination.label = line.substr(position, end - position);
    size_t open = line.find_first_not_of(" \t", end);
    if (open != std::string::npos && line[open] == '(') {
      int depth = 1;
      size_t close = open + 1;
      for (; close < line.size() && depth != 0; ++close) {
        if (line[close] == '(')
          ++depth;
        else if (line[close] == ')')
          --depth;
      }
      if (depth == 0)
        destination.operands =
            extractSSAs(line.substr(open + 1, close - open - 2));
    }
    destinations.push_back(std::move(destination));
    position = end;
  }
  return destinations;
}

inline bool parseBlockHeader(const std::string &line, std::string &label,
                             std::vector<std::string> &arguments) {
  std::string stripped = trim(line);
  if (stripped.empty() || stripped.front() != '^')
    return false;
  size_t end = 1;
  while (end < stripped.size()) {
    char c = stripped[end];
    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' ||
          c == '.' || c == '$' || c == '#' || c == '-'))
      break;
    ++end;
  }
  label = stripped.substr(0, end);
  size_t open = stripped.find('(', end);
  if (open != std::string::npos) {
    int depth = 1;
    size_t close = open + 1;
    for (; close < stripped.size() && depth != 0; ++close) {
      if (stripped[close] == '(')
        ++depth;
      else if (stripped[close] == ')')
        --depth;
    }
    if (depth == 0)
      arguments = extractSSAs(stripped.substr(open + 1, close - open - 2));
  }
  return true;
}

inline std::string canonical(const std::map<std::string, std::string> &alias,
                             const std::string &value) {
  std::set<std::string> seen;
  std::string current = value;
  while (alias.count(current) && !seen.count(current)) {
    seen.insert(current);
    std::string next = alias.at(current);
    if (next == current)
      break;
    current = next;
  }
  return current;
}

inline std::optional<std::string>
canonicalAlloc(const std::map<std::string, std::string> &alias,
               const std::set<std::string> &allocNames,
               const std::string &value) {
  std::string base = canonical(alias, value);
  if (allocNames.count(base))
    return base;
  return std::nullopt;
}

class UnionFind {
public:
  explicit UnionFind(const std::set<std::string> &values) {
    for (const std::string &value : values)
      parent[value] = value;
  }

  std::string find(const std::string &value) {
    std::string p = parent[value];
    if (p != value)
      parent[value] = find(p);
    return parent[value];
  }

  void unite(const std::string &lhs, const std::string &rhs) {
    if (!parent.count(lhs) || !parent.count(rhs))
      return;
    std::string leftRoot = find(lhs);
    std::string rightRoot = find(rhs);
    if (leftRoot == rightRoot)
      return;
    if (rightRoot < leftRoot)
      std::swap(leftRoot, rightRoot);
    parent[rightRoot] = leftRoot;
  }

  std::map<std::string, std::vector<std::string>> groups() {
    std::map<std::string, std::vector<std::string>> result;
    for (const auto &it : parent)
      result[find(it.first)].push_back(it.first);
    for (auto &it : result)
      std::sort(it.second.begin(), it.second.end(), ssaLess);
    return result;
  }

private:
  std::map<std::string, std::string> parent;
};

inline std::vector<OperationRecord> parseFunctionOperations(const fs::path &path,
                                                            const std::string &coreType) {
  std::ifstream in(path);
  std::vector<OperationRecord> operations;
  std::string currentFunction;
  std::string currentCore;
  int functionDepth = 0;
  bool functionBodyStarted = false;
  int opIndex = 0;
  struct RegionFrame {
    int closeDepth = 0;
    int line = 0;
    int indent = 0;
    int id = 0;
    int operationId = -1;
    std::string kind;
    bool hasExplicitYield = false;
    int blockId = 0;
    std::string blockLabel;
    std::vector<std::string> blockArguments;
  };
  std::vector<RegionFrame> regionStack;
  int nextRegionId = 0;
  int nextBlockId = 1;
  int functionBlockId = 0;
  std::string functionBlockLabel = "^bb0";
  std::vector<std::string> functionBlockArguments;
  int nextOperationId = 0;
  int nextLineNo = 0;
  std::optional<std::pair<int, std::string>> pendingLine;
  std::string line;
  static const std::regex funcRegex(R"(func\.func\s+@([\w$.#-]+))");
  static const std::regex coreRegex(
      R"(hivm\.func_core_type\s*=\s*#hivm\.func_core_type<(\w+)>)");

  while (true) {
    int lineNo = 0;
    if (pendingLine) {
      lineNo = pendingLine->first;
      line = std::move(pendingLine->second);
      pendingLine.reset();
    } else {
      if (!std::getline(in, line))
        break;
      lineNo = ++nextLineNo;
    }
    std::smatch match;
    if (std::regex_search(line, match, funcRegex)) {
      currentFunction = match[1].str();
      std::smatch coreMatch;
      currentCore =
          std::regex_search(line, coreMatch, coreRegex) ? coreMatch[1].str() : "";
      const std::string strippedFunctionLine = trim(line);
      functionBodyStarted = !strippedFunctionLine.empty() &&
                            strippedFunctionLine.back() == '{';
      functionDepth = functionBodyStarted ? 1 : 0;
      opIndex = 0;
      regionStack.clear();
      nextRegionId = 0;
      nextBlockId = 1;
      functionBlockId = 0;
      functionBlockLabel = "^bb0";
      functionBlockArguments.clear();
      nextOperationId = 0;
      continue;
    }

    if (currentFunction.empty())
      continue;

    if (currentCore.empty()) {
      std::smatch coreMatch;
      if (std::regex_search(line, coreMatch, coreRegex))
        currentCore = coreMatch[1].str();
    }
    if (!functionBodyStarted) {
      // A func.func signature and its attribute dictionary can span many
      // lines.  Braces in `attributes { ... }` are not regions and therefore
      // are not entries in MemLivenessAnalysis::linearOperation.
      const std::string strippedHeaderLine = trim(line);
      if (!strippedHeaderLine.empty() && strippedHeaderLine.back() == '{' &&
          strippedHeaderLine.find('}') != std::string::npos) {
        functionBodyStarted = true;
        functionDepth = 1;
      }
      continue;
    }
    int lineBraceDelta = braceDelta(line);

    // MLIR printers may split one operation across an op-name line followed by
    // indented ins/outs/type/attribute lines. MemLivenessAnalysis sees one
    // Operation, so join those physical lines before assigning an op index.
    if (functionBodyStarted && currentCore == coreType &&
        isOperationLine(line)) {
      std::string logicalLine = trim(line);
      const bool startsWithResultList =
          !logicalLine.empty() && logicalLine.front() == '%' &&
          logicalLine.find('=') == std::string::npos &&
          logicalLine.back() == ',';
      while (lineBraceDelta <= 0) {
        std::string continuation;
        if (!std::getline(in, continuation))
          break;
        int continuationLineNo = ++nextLineNo;
        std::string strippedContinuation = trim(continuation);
        const bool resultListNeedsDefinition =
            startsWithResultList && logicalLine.find('=') == std::string::npos;
        const bool operandListNeedsContinuation =
            operandDelimiterDelta(logicalLine) > 0 ||
            (!logicalLine.empty() && logicalLine.back() == ',');
        const bool startsBlock = !strippedContinuation.empty() &&
                                 strippedContinuation.front() == '^';
        if (startsBlock ||
            (!strippedContinuation.empty() &&
             isOperationLine(continuation) && !resultListNeedsDefinition &&
             !operandListNeedsContinuation) ||
            (!strippedContinuation.empty() &&
             strippedContinuation.front() == '}')) {
          pendingLine = {continuationLineNo, std::move(continuation)};
          break;
        }
        if (!strippedContinuation.empty()) {
          logicalLine += " ";
          logicalLine += strippedContinuation;
          lineBraceDelta += braceDelta(continuation);
        }
      }
      line = std::move(logicalLine);
    }

    std::string strippedLine = trim(line);
    if (currentCore == coreType && !regionStack.empty() &&
        regionStack.back().kind == "while.before" &&
        startsWith(strippedLine, "} do")) {
      RegionFrame &frame = regionStack.back();
      frame.kind = "while.after";
      frame.id = nextRegionId++;
      frame.blockId = nextBlockId++;
      frame.blockLabel = "^bb0";
      frame.blockArguments.clear();
    }
    if (currentCore == coreType && !regionStack.empty() &&
        regionStack.back().kind == "if" &&
        strippedLine.find("else") != std::string::npos &&
        strippedLine[0] == '}') {
      RegionFrame &frame = regionStack.back();
      if (!frame.hasExplicitYield) {
        OperationRecord yieldOp;
        yieldOp.index = opIndex++;
        yieldOp.line = lineNo;
        yieldOp.indent = frame.indent + 2;
        yieldOp.operationId = nextOperationId++;
        for (const RegionFrame &active : regionStack)
          yieldOp.regionPath.push_back(active.id);
        yieldOp.blockId = frame.blockId;
        yieldOp.blockLabel = frame.blockLabel;
        yieldOp.blockArguments = frame.blockArguments;
        yieldOp.text = "<scf.if.implicit_yield from line " +
                       std::to_string(frame.line) + ">";
        yieldOp.opName = "scf.if.implicit_yield";
        operations.push_back(std::move(yieldOp));
      }
      OperationRecord elsePoint;
      elsePoint.index = opIndex++;
      elsePoint.line = lineNo;
      elsePoint.indent = frame.indent;
      elsePoint.operationId = frame.operationId;
      for (size_t i = 0; i + 1 < regionStack.size(); ++i)
        elsePoint.regionPath.push_back(regionStack[i].id);
      if (regionStack.size() > 1) {
        elsePoint.blockId = regionStack[regionStack.size() - 2].blockId;
        elsePoint.blockLabel =
            regionStack[regionStack.size() - 2].blockLabel;
      } else {
        elsePoint.blockId = functionBlockId;
        elsePoint.blockLabel = functionBlockLabel;
      }
      elsePoint.text = "<scf.if.else from line " +
                       std::to_string(frame.line) + ">";
      elsePoint.opName = "scf.if.else";
      operations.push_back(std::move(elsePoint));
      frame.id = nextRegionId++;
      frame.hasExplicitYield = false;
      frame.blockId = nextBlockId++;
      frame.blockLabel = "^bb0";
      frame.blockArguments.clear();
    }

    std::string parsedBlockLabel;
    std::vector<std::string> parsedBlockArguments;
    if (currentCore == coreType &&
        parseBlockHeader(line, parsedBlockLabel, parsedBlockArguments)) {
      if (regionStack.empty()) {
        functionBlockId = nextBlockId++;
        functionBlockLabel = parsedBlockLabel;
        functionBlockArguments = std::move(parsedBlockArguments);
      } else {
        RegionFrame &frame = regionStack.back();
        frame.blockId = nextBlockId++;
        frame.blockLabel = parsedBlockLabel;
        frame.blockArguments = std::move(parsedBlockArguments);
      }
    }

    if (functionBodyStarted && currentCore == coreType &&
        isOperationLine(line)) {
      OperationRecord op;
      op.index = opIndex++;
      op.line = lineNo;
      op.operationId = nextOperationId++;
      size_t firstNonSpace = line.find_first_not_of(" \t");
      op.indent = firstNonSpace == std::string::npos
                      ? 0
                      : static_cast<int>(firstNonSpace);
      op.text = trim(line);
      op.opName = extractOpName(line);
      for (const RegionFrame &frame : regionStack)
        op.regionPath.push_back(frame.id);
      if (regionStack.empty()) {
        op.blockId = functionBlockId;
        op.blockLabel = functionBlockLabel;
        op.blockArguments = functionBlockArguments;
      } else {
        op.blockId = regionStack.back().blockId;
        op.blockLabel = regionStack.back().blockLabel;
        op.blockArguments = regionStack.back().blockArguments;
      }
      op.branchDestinations = extractBranchDestinations(op.text);
      for (const BranchDestination &destination : op.branchDestinations)
        op.successorLabels.push_back(destination.label);
      operations.push_back(op);
      std::string stripped = trim(line);
      if (startsWith(stripped, "scf.for ") ||
          stripped.find(" = scf.for ") != std::string::npos) {
        regionStack.push_back({functionDepth + braceDelta(line), lineNo,
                               op.indent, nextRegionId++, op.operationId,
                               "for", false, nextBlockId++, "^bb0", {}});
      } else if (startsWith(stripped, "scf.if ") ||
                 stripped.find(" = scf.if ") != std::string::npos) {
        regionStack.push_back({functionDepth + braceDelta(line), lineNo,
                               op.indent, nextRegionId++, op.operationId,
                               "if", false, nextBlockId++, "^bb0", {}});
      } else if (startsWith(stripped, "scf.while ") ||
                 stripped.find(" = scf.while ") != std::string::npos) {
        regionStack.push_back({functionDepth + braceDelta(line), lineNo,
                               op.indent, nextRegionId++, op.operationId,
                               "while.before", false, nextBlockId++, "^bb0",
                               {}});
      } else if (startsWith(stripped, "scope.scope ") ||
                 stripped.find(" = scope.scope ") != std::string::npos) {
        regionStack.push_back({functionDepth + braceDelta(line), lineNo,
                               op.indent, nextRegionId++, op.operationId,
                               "scope", false, nextBlockId++, "^bb0", {}});
      } else if ((stripped == "scf.yield" ||
                  startsWith(stripped, "scf.yield ")) &&
                 !regionStack.empty()) {
        regionStack.back().hasExplicitYield = true;
      }
    }

    functionDepth += lineBraceDelta;
    if (currentCore == coreType) {
      while (!regionStack.empty() &&
             functionDepth < regionStack.back().closeDepth) {
        RegionFrame frame = regionStack.back();
        if ((frame.kind == "if" || frame.kind == "for") &&
            !frame.hasExplicitYield) {
          OperationRecord yieldOp;
          yieldOp.index = opIndex++;
          yieldOp.line = lineNo;
          yieldOp.operationId = nextOperationId++;
          yieldOp.indent = frame.indent + 2;
          for (const RegionFrame &active : regionStack)
            yieldOp.regionPath.push_back(active.id);
          yieldOp.blockId = frame.blockId;
          yieldOp.blockLabel = frame.blockLabel;
          yieldOp.blockArguments = frame.blockArguments;
          yieldOp.text = "<scf." + frame.kind +
                         ".implicit_yield from line " +
                         std::to_string(frame.line) + ">";
          yieldOp.opName = "scf." + frame.kind + ".implicit_yield";
          operations.push_back(yieldOp);
        }
        regionStack.pop_back();
        OperationRecord op;
        op.index = opIndex++;
        op.line = lineNo;
        op.operationId = frame.operationId;
        op.indent = frame.indent;
        for (const RegionFrame &active : regionStack)
          op.regionPath.push_back(active.id);
        if (regionStack.empty()) {
          op.blockId = functionBlockId;
          op.blockLabel = functionBlockLabel;
        } else {
          op.blockId = regionStack.back().blockId;
          op.blockLabel = regionStack.back().blockLabel;
          op.blockArguments = regionStack.back().blockArguments;
        }
        const std::string endKind =
            startsWith(frame.kind, "while.") ? "while" : frame.kind;
        op.text = "<" +
                  std::string(endKind == "scope" ? "scope." : "scf.") +
                  endKind + ".end from line " +
                  std::to_string(frame.line) + ">";
        op.opName =
            std::string(endKind == "scope" ? "scope." : "scf.") + endKind +
            ".end";
        operations.push_back(op);
      }
    }

    if (functionBodyStarted && functionDepth <= 0) {
      currentFunction.clear();
      currentCore.clear();
      functionBodyStarted = false;
      functionDepth = 0;
      regionStack.clear();
    }
  }
  return operations;
}

inline std::vector<std::string>
parseFunctionArgumentNames(const fs::path &path, const std::string &coreType) {
  std::ifstream in(path);
  std::string line;
  std::string coreMarker =
      "hivm.func_core_type = #hivm.func_core_type<" + coreType + ">";
  std::string header;
  bool inHeader = false;
  while (std::getline(in, line)) {
    if (line.find("func.func @") != std::string::npos) {
      header = line;
      inHeader = true;
    } else if (inHeader) {
      header += " ";
      header += trim(line);
    } else {
      continue;
    }
    if (header.find(coreMarker) == std::string::npos)
      continue;
    inHeader = false;
    size_t begin = header.find('(');
    size_t end = header.find(") attributes", begin);
    if (begin == std::string::npos || end == std::string::npos)
      return {};
    return extractSSAs(header.substr(begin + 1, end - begin - 1));
  }
  return {};
}

inline std::string resultNameBeforeEqual(const std::string &line) {
  std::string result = extractSSANameBeforeEqual(line);
  size_t colon = result.find(':');
  if (colon != std::string::npos)
    result = result.substr(0, colon);
  return result;
}

inline std::vector<std::string> resultNamesBeforeEqual(const std::string &line) {
  std::string result = extractSSANameBeforeEqual(line);
  if (result.empty())
    return {};
  if (result.find(',') != std::string::npos) {
    std::vector<std::string> names;
    std::istringstream groups(result);
    std::string group;
    while (std::getline(groups, group, ',')) {
      group = trim(group);
      size_t colon = group.find(':');
      if (colon == std::string::npos) {
        names.push_back(group);
        continue;
      }
      std::string base = trim(group.substr(0, colon));
      std::string arityText = trim(group.substr(colon + 1));
      if (base.empty() || arityText.empty() ||
          !std::all_of(arityText.begin(), arityText.end(), [](char c) {
            return std::isdigit(static_cast<unsigned char>(c));
          }))
        return {};
      size_t arity = static_cast<size_t>(std::stoull(arityText));
      for (size_t i = 0; i < arity; ++i)
        names.push_back(base + "#" + std::to_string(i));
    }
    return names;
  }
  size_t colon = result.find(':');
  if (colon == std::string::npos)
    return {result};
  std::string base = trim(result.substr(0, colon));
  std::string arityText = trim(result.substr(colon + 1));
  if (base.empty() || arityText.empty() ||
      !std::all_of(arityText.begin(), arityText.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
      }))
    return {};
  size_t arity = static_cast<size_t>(std::stoull(arityText));
  std::vector<std::string> names;
  for (size_t i = 0; i < arity; ++i)
    names.push_back(base + "#" + std::to_string(i));
  return names;
}

inline std::vector<std::pair<std::string, std::string>>
extractIterArgPairs(const std::string &line) {
  std::string body = extractGroupBody(line, "iter_args");
  std::vector<std::pair<std::string, std::string>> pairs;
  if (body.empty())
    return pairs;
  static const std::regex pairRegex(R"((%[\w$.#-]+)\s*=\s*(%[\w$.#-]+))");
  for (std::sregex_iterator it(body.begin(), body.end(), pairRegex), end;
       it != end; ++it)
    pairs.push_back({(*it)[1].str(), (*it)[2].str()});
  return pairs;
}

inline std::vector<std::pair<std::string, std::string>>
extractWhileInitArgPairs(const std::string &line) {
  size_t whilePosition = line.find("scf.while");
  if (whilePosition == std::string::npos)
    return {};
  size_t open = line.find('(', whilePosition);
  if (open == std::string::npos)
    return {};
  int depth = 1;
  size_t close = open + 1;
  for (; close < line.size() && depth != 0; ++close) {
    if (line[close] == '(')
      ++depth;
    else if (line[close] == ')')
      --depth;
  }
  if (depth != 0)
    return {};
  std::string body = line.substr(open + 1, close - open - 2);
  std::vector<std::pair<std::string, std::string>> pairs;
  static const std::regex pairRegex(R"((%[\w$.#-]+)\s*=\s*(%[\w$.#-]+))");
  for (std::sregex_iterator it(body.begin(), body.end(), pairRegex), end;
       it != end; ++it)
    pairs.push_back({(*it)[1].str(), (*it)[2].str()});
  return pairs;
}

inline std::vector<std::string>
extractConditionForwardedValues(const std::string &line) {
  size_t close = line.find(')');
  if (close == std::string::npos)
    return {};
  size_t type = line.find(':', close + 1);
  return extractSSAs(line.substr(close + 1, type - close - 1));
}

inline std::string replaceSSAUse(std::string text, const std::string &from,
                                 const std::string &to) {
  size_t pos = 0;
  auto isSSAChar = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' ||
           c == '.' || c == '$' || c == '#' || c == '-';
  };
  while ((pos = text.find(from, pos)) != std::string::npos) {
    bool leftBoundary = pos == 0 || !isSSAChar(text[pos - 1]);
    size_t end = pos + from.size();
    bool rightBoundary = end == text.size() || !isSSAChar(text[end]);
    if (leftBoundary && rightBoundary) {
      text.replace(pos, from.size(), to);
      pos += to.size();
    } else {
      pos = end;
    }
  }
  return text;
}

inline std::optional<std::string>
extractForInductionVariable(const std::string &line) {
  static const std::regex regex(R"(scf\.for\s+(%[\w$.#-]+)\s*=)");
  std::smatch match;
  if (std::regex_search(line, match, regex))
    return match[1].str();
  return std::nullopt;
}

inline std::vector<std::string>
operationResultNames(const OperationRecord &op) {
  return resultNamesBeforeEqual(op.text);
}

inline std::vector<std::string>
operationResultBaseNames(const OperationRecord &op) {
  std::string resultList = extractSSANameBeforeEqual(op.text);
  if (resultList.empty())
    return {};
  std::vector<std::string> bases;
  std::istringstream groups(resultList);
  std::string group;
  while (std::getline(groups, group, ',')) {
    group = trim(group);
    size_t colon = group.find(':');
    std::string base = trim(group.substr(0, colon));
    if (!base.empty())
      bases.push_back(std::move(base));
  }
  return bases;
}

inline std::vector<std::string>
operationOperandNames(const OperationRecord &op) {
  if ((op.opName == "cf.br" || op.opName == "cf.cond_br") &&
      !op.branchDestinations.empty()) {
    std::vector<std::string> operands;
    if (op.opName == "cf.cond_br") {
      std::vector<std::string> textualOperands = extractSSAs(op.text);
      if (!textualOperands.empty())
        operands.push_back(textualOperands.front());
    }
    for (const BranchDestination &destination : op.branchDestinations)
      operands.insert(operands.end(), destination.operands.begin(),
                      destination.operands.end());
    return operands;
  }
  std::vector<std::string> operands = extractSSAs(op.text);
  std::set<std::string> definitions;
  for (const std::string &result : operationResultNames(op))
    definitions.insert(result);
  for (const std::string &base : operationResultBaseNames(op))
    definitions.insert(base);
  if (op.opName == "scf.for") {
    if (auto induction = extractForInductionVariable(op.text))
      definitions.insert(*induction);
    for (const auto &pair : extractIterArgPairs(op.text))
      definitions.insert(pair.first);
  } else if (op.opName == "scf.while") {
    for (const auto &pair : extractWhileInitArgPairs(op.text))
      definitions.insert(pair.first);
  }
  std::vector<std::string> result;
  for (const std::string &value : operands)
    if (!definitions.count(value))
      result.push_back(value);
  return result;
}

inline bool pathPrefix(const std::vector<int> &prefix,
                       const std::vector<int> &path) {
  return prefix.size() <= path.size() &&
         std::equal(prefix.begin(), prefix.end(), path.begin());
}

inline bool isViewLikeMemrefOp(const std::string &opName) {
  static const std::set<std::string> aliasOps = {
      "bufferization.to_memref", "bufferization.to_buffer",
      "bufferization.to_tensor", "hivm.hir.bitcast",
      "memref.cast",             "memref.collapse_shape",
      "memref.expand_shape",     "memref.extract_strided_metadata",
      "memref.memory_space_cast", "memref.reinterpret_cast",
      "memref.reshape",          "memref.subview",
      "memref.transpose",        "memref.view",
      "tensor.collapse_shape",   "tensor.expand_shape",
      "tensor.extract_slice"};
  return aliasOps.count(opName) != 0;
}

inline std::vector<OperationRecord>
qualifyScopedSSAValues(std::vector<OperationRecord> operations) {
  struct Definition {
    std::vector<int> path;
    int blockId = -1;
    std::string qualified;
  };
  std::map<std::string, std::vector<Definition>> definitions;
  size_t uniqueId = 0;

  auto resolve = [&](const std::string &raw, const std::vector<int> &path,
                     int blockId) {
    auto it = definitions.find(raw);
    // MLIR accepts `%value#0` as an operand spelling for a single-result
    // operation whose printed result is `%value`. Both spellings identify the
    // same Value; resolve through the visible base definition in that case.
    std::string lookup = raw;
    if (it == definitions.end() && raw.size() > 2 &&
        raw.compare(raw.size() - 2, 2, "#0") == 0) {
      lookup = raw.substr(0, raw.size() - 2);
      it = definitions.find(lookup);
    }
    if (it == definitions.end())
      return raw;
    const Definition *best = nullptr;
    for (const Definition &definition : it->second) {
      if (!pathPrefix(definition.path, path))
        continue;
      bool exactBlock = definition.path == path &&
                        definition.blockId == blockId;
      bool bestExactBlock = best && best->path == path &&
                            best->blockId == blockId;
      if (!best || (exactBlock && !bestExactBlock) ||
          (exactBlock == bestExactBlock &&
           definition.path.size() > best->path.size()))
        best = &definition;
    }
    return best ? best->qualified : raw;
  };

  auto addDefinition = [&](const std::string &raw,
                           const std::vector<int> &path, int blockId) {
    std::string qualified = raw;
    if (definitions.count(raw))
      qualified = raw + "$scope" + std::to_string(uniqueId++);
    definitions[raw].push_back({path, blockId, qualified});
    return qualified;
  };

  std::set<int> seenOperations;
  std::set<int> seenBlocks;
  for (size_t index = 0; index < operations.size(); ++index) {
    OperationRecord &op = operations[index];
    if (seenOperations.count(op.operationId))
      continue;
    seenOperations.insert(op.operationId);

    if (seenBlocks.insert(op.blockId).second) {
      for (std::string &argument : op.blockArguments) {
        std::string qualified =
            addDefinition(argument, op.regionPath, op.blockId);
        if (qualified != argument)
          argument = qualified;
      }
    } else {
      for (std::string &argument : op.blockArguments)
        argument = resolve(argument, op.regionPath, op.blockId);
    }

    std::vector<std::string> rawResults = operationResultNames(op);
    std::vector<std::string> rawOperands = operationOperandNames(op);
    for (const std::string &operand : rawOperands) {
      std::string qualified = resolve(operand, op.regionPath, op.blockId);
      if (qualified != operand)
        op.text = replaceSSAUse(op.text, operand, qualified);
    }
    for (BranchDestination &destination : op.branchDestinations)
      for (std::string &operand : destination.operands)
        operand = resolve(operand, op.regionPath, op.blockId);

    if (rawResults.size() == 1) {
      std::string qualified =
          addDefinition(rawResults.front(), op.regionPath, op.blockId);
      if (qualified != rawResults.front())
        op.text = replaceSSAUse(op.text, rawResults.front(), qualified);
    } else {
      for (const std::string &result : rawResults) {
        std::string qualified =
            addDefinition(result, op.regionPath, op.blockId);
        if (qualified != result)
          op.text = replaceSSAUse(op.text, result, qualified);
      }
    }

    if (op.opName == "scf.for") {
      std::vector<int> childPath = op.regionPath;
      int childBlockId = -1;
      for (size_t next = index + 1; next < operations.size(); ++next) {
        if (operations[next].regionPath.size() == op.regionPath.size() + 1 &&
            pathPrefix(op.regionPath, operations[next].regionPath)) {
          childPath = operations[next].regionPath;
          childBlockId = operations[next].blockId;
          break;
        }
      }
      if (auto induction = extractForInductionVariable(op.text)) {
        std::string qualified =
            addDefinition(*induction, childPath, childBlockId);
        if (qualified != *induction)
          op.text = replaceSSAUse(op.text, *induction, qualified);
      }
      for (const auto &pair : extractIterArgPairs(op.text)) {
        std::string qualified =
            addDefinition(pair.first, childPath, childBlockId);
        if (qualified != pair.first)
          op.text = replaceSSAUse(op.text, pair.first, qualified);
      }
    } else if (op.opName == "scf.while") {
      std::vector<int> childPath = op.regionPath;
      int childBlockId = -1;
      for (size_t next = index + 1; next < operations.size(); ++next) {
        if (operations[next].regionPath.size() == op.regionPath.size() + 1 &&
            pathPrefix(op.regionPath, operations[next].regionPath)) {
          childPath = operations[next].regionPath;
          childBlockId = operations[next].blockId;
          break;
        }
      }
      for (const auto &pair : extractWhileInitArgPairs(op.text)) {
        std::string qualified =
            addDefinition(pair.first, childPath, childBlockId);
        if (qualified != pair.first)
          op.text = replaceSSAUse(op.text, pair.first, qualified);
      }
    }
  }

  // Duplicate linearOperation points share an Operation* and therefore the same
  // rewritten textual identity as the first point.
  std::map<int, std::string> firstText;
  for (OperationRecord &op : operations) {
    if (!firstText.count(op.operationId))
      firstText[op.operationId] = op.text;
    else if (op.opName == "scf.for" || op.opName == "scf.if" ||
             op.opName == "scf.while" || op.opName == "scope.scope")
      op.text = firstText[op.operationId];
  }
  return operations;
}

inline std::vector<IRAllocRecord>
qualifyScopedAllocRecords(const fs::path &beforeIR,
                          const std::string &coreType = "AIV") {
  std::vector<IRAllocRecord> records = parseLocalUBAllocations(beforeIR);
  std::vector<OperationRecord> operations =
      qualifyScopedSSAValues(parseFunctionOperations(beforeIR, coreType));
  std::map<int, std::string> allocByLine;
  for (const OperationRecord &operation : operations) {
    if (operation.opName != "memref.alloc")
      continue;
    std::vector<std::string> results = operationResultNames(operation);
    if (!results.empty())
      allocByLine[operation.line] = results.front();
  }
  for (IRAllocRecord &record : records) {
    auto it = allocByLine.find(record.line);
    if (it != allocByLine.end())
      record.name = it->second;
  }
  return records;
}


} // namespace cvub

#endif
