#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_BUFFER_SIZE_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_BUFFER_SIZE_HPP

#include "core_type.hpp"
#include "types.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cvub {

inline bool HasDictionaryUnit(const std::string &dictionary,
                              const std::string &name) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return false;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2)))
    if (trim(entry) == name)
      return true;
  return false;
}

inline std::optional<int64_t> ParseIntegerAttribute(std::string value) {
  value = trim(std::move(value));
  static const std::regex integer(R"(^(-?[0-9]+)(?:\s*:\s*[^ ]+)?$)");
  std::smatch match;
  if (!std::regex_match(value, match, integer))
    return std::nullopt;
  try {
    return std::stoll(match[1].str());
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline std::optional<unsigned> ShapedElementBits(const std::string &type) {
  static const std::regex element(R"(x(bf16|[fiu][0-9]+)(?:,|>))");
  std::smatch match;
  if (!std::regex_search(type, match, element))
    return std::nullopt;
  if (match[1].str() == "bf16")
    return 16U;
  try {
    const unsigned long parsed = std::stoul(match[1].str().substr(1));
    if (parsed > std::numeric_limits<unsigned>::max())
      return std::nullopt;
    return static_cast<unsigned>(parsed);
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline bool IsDynamicMemRef(const std::string &type) {
  return startsWith(trim(type), "memref<") && type.find('?') != std::string::npos;
}

inline std::string PhysicalByteMemRef(const std::string &logical,
                                      int64_t bytes) {
  const size_t open = logical.find('<');
  const size_t close = logical.rfind('>');
  if (open == std::string::npos || close == std::string::npos || close <= open)
    return "";
  const std::vector<std::string> fields =
      splitTopLevel(logical.substr(open + 1, close - open - 1));
  std::string result = "memref<" + std::to_string(bytes) + "xi8";
  // createAllocWithSettingBufferSize and ConstantizeBufferSize both create an
  // identity-layout byte allocation.  The original layout belongs only to the
  // logical memref.view result; copying it to the i8 backing changes its byte
  // capacity and is not compiler-equivalent.
  if (fields.size() > 1) {
    const std::string &memorySpace = fields.back();
    if (memorySpace.find("address_space") != std::string::npos)
      result += ", " + memorySpace;
  }
  return result + ">";
}

inline std::string RemoveDictionaryAttribute(const std::string &dictionary,
                                             const std::string &name) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return dictionary;
  std::vector<std::string> kept;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    const std::string key =
        trim(entry.substr(0, equal == std::string::npos ? entry.size() : equal));
    if (key != name && !trim(entry).empty())
      kept.push_back(trim(entry));
  }
  std::string result = "{";
  for (size_t index = 0; index < kept.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += kept[index];
  }
  return result + "}";
}

inline std::string KeepOnlyDictionaryAttribute(const std::string &dictionary,
                                               const std::string &name) {
  const std::string value = IRDictionaryValue(dictionary, name);
  return value.empty() ? "{}" : "{" + name + " = " + value + "}";
}

struct BufferSizeBound {
  int64_t value = 0;
  bool exact = false;
};

inline const GenericOperation *DefinitionForBufferSize(
    const GenericModule &module, int value);

inline std::optional<BufferSizeBound>
ResolveBufferSizeBound(const GenericModule &module, int value,
                       std::set<int> &visiting) {
  if (!visiting.insert(value).second)
    return std::nullopt;
  const GenericOperation *definition = DefinitionForBufferSize(module, value);
  if (definition == nullptr) {
    visiting.erase(value);
    return std::nullopt;
  }
  const auto done = [&](std::optional<BufferSizeBound> result) {
    visiting.erase(value);
    return result;
  };
  if (definition->name == "arith.constant") {
    const auto parsed = ParseIntegerAttribute(
        IRDictionaryValue(definition->attributes, "value"));
    return done(parsed ? std::optional<BufferSizeBound>{{*parsed, true}}
                       : std::nullopt);
  }
  if (definition->name == "arith.index_cast" &&
      definition->operands.size() == 1)
    return done(ResolveBufferSizeBound(module, definition->operands[0],
                                       visiting));

  std::vector<std::optional<BufferSizeBound>> operands;
  for (int operand : definition->operands)
    operands.push_back(ResolveBufferSizeBound(module, operand, visiting));
  auto both = [&]() {
    return operands.size() == 2 && operands[0].has_value() &&
           operands[1].has_value();
  };
  int64_t result = 0;
  if (both()) {
    const int64_t lhs = operands[0]->value;
    const int64_t rhs = operands[1]->value;
    bool valid = true;
    if (definition->name == "arith.addi")
      valid = !__builtin_add_overflow(lhs, rhs, &result);
    else if (definition->name == "arith.subi")
      valid = operands[1]->exact && rhs >= 0 &&
              !__builtin_sub_overflow(lhs, rhs, &result);
    else if (definition->name == "arith.muli")
      valid = lhs >= 0 && rhs >= 0 &&
              !__builtin_mul_overflow(lhs, rhs, &result);
    else if (definition->name == "arith.ceildivui" ||
             definition->name == "arith.ceildivsi") {
      valid = lhs >= 0 && rhs > 0;
      if (valid)
        result = lhs / rhs + (lhs % rhs != 0);
    } else if (definition->name == "arith.divui" ||
               definition->name == "arith.divsi") {
      valid = lhs >= 0 && rhs > 0;
      if (valid)
        result = lhs / rhs;
    } else if (definition->name == "arith.remsi") {
      valid = lhs >= 0 && rhs > 0;
      if (valid)
        // ArithToAffine rewrites index remsi to `(d0 mod d1)`.  The real
        // Constantize pass then substitutes each operand's resolved upper
        // bound and constant-folds that affine map, even when d0 is only a
        // bound.  Mirror that observable behavior instead of inventing the
        // mathematically safer but compiler-incompatible rhs-1 bound.
        result = lhs % rhs;
    } else if (definition->name == "arith.minui" ||
               definition->name == "arith.minsi")
      result = std::min(lhs, rhs);
    else if (definition->name == "arith.maxui" ||
             definition->name == "arith.maxsi")
      result = std::max(lhs, rhs);
    else
      valid = false;
    if (valid)
      return done(BufferSizeBound{
          result, operands[0]->exact && operands[1]->exact});
  }
  if ((definition->name == "arith.minui" ||
       definition->name == "arith.minsi") &&
      operands.size() == 2 && (operands[0] || operands[1])) {
    const BufferSizeBound bound = operands[0] ? *operands[0] : *operands[1];
    return done(BufferSizeBound{bound.value, false});
  }

  if (definition->name == "affine.apply" && definition->results.size() == 1) {
    // Generic IR keeps the affine map spelling.  Constant operands are the
    // exact and common ArithToAffine case.  Support the affine operators used
    // by size expressions without claiming bounds for non-monotone maps.
    bool allExact = operands.size() == definition->operands.size();
    for (const auto &operand : operands)
      allExact = allExact && operand && operand->exact;
    std::string expression = IRDictionaryValue(definition->properties, "map");
    if (expression.empty())
      expression = IRDictionaryValue(definition->attributes, "map");
    static const std::regex constantMap(
        R"(^affine_map<\(\) -> \((-?[0-9]+)\)>$)");
    std::smatch match;
    const std::string spelling = trim(expression);
    if (std::regex_match(spelling, match, constantMap)) {
      const auto parsed = ParseIntegerAttribute(match[1].str());
      return done(parsed ? std::optional<BufferSizeBound>{{*parsed, true}}
                         : std::nullopt);
    }
    static const std::regex binaryMap(
        R"(^affine_map<\(d0, d1\) -> \(d0\s*(\+|-|\*|ceildiv|floordiv|mod)\s*d1\)>$)");
    if (operands.size() == 2 && operands[0] && operands[1] &&
        std::regex_match(spelling, match, binaryMap)) {
      const int64_t lhs = operands[0]->value;
      const int64_t rhs = operands[1]->value;
      const std::string op = match[1].str();
      bool valid = true;
      if (op == "+")
        valid = !__builtin_add_overflow(lhs, rhs, &result);
      else if (op == "-")
        valid = !__builtin_sub_overflow(lhs, rhs, &result);
      else if (op == "*")
        valid = !__builtin_mul_overflow(lhs, rhs, &result);
      else if (rhs <= 0)
        valid = false;
      else if (op == "ceildiv") {
        const int64_t quotient = lhs / rhs;
        const int64_t remainder = lhs % rhs;
        result = quotient + (remainder > 0 ? 1 : 0);
      } else if (op == "floordiv") {
        const int64_t quotient = lhs / rhs;
        const int64_t remainder = lhs % rhs;
        result = quotient - (remainder < 0 ? 1 : 0);
      } else {
        const int64_t remainder = lhs % rhs;
        result = remainder < 0 ? remainder + rhs : remainder;
      }
      if (valid)
        return done(BufferSizeBound{result, allExact});
    }
  }
  return done(std::nullopt);
}

inline std::optional<BufferSizeBound>
ResolveBufferSizeBound(const GenericModule &module, int value) {
  std::set<int> visiting;
  return ResolveBufferSizeBound(module, value, visiting);
}

inline std::optional<int64_t>
LogicalMemRefBytes(const std::string &type,
                   const std::vector<BufferSizeBound> &dynamicBounds) {
  if (!startsWith(trim(type), "memref<"))
    return std::nullopt;
  const size_t open = type.find('<');
  const size_t close = type.rfind('>');
  const auto fields = splitTopLevel(type.substr(open + 1, close - open - 1));
  if (fields.empty())
    return std::nullopt;
  const auto bits = ShapedElementBits(type);
  if (!bits || *bits == 0)
    return std::nullopt;
  std::string shaped = fields.front();
  const size_t element = shaped.rfind('x');
  if (element == std::string::npos)
    return std::nullopt;
  const std::vector<std::string> dimensions = split(shaped.substr(0, element), 'x');
  int64_t elements = 1;
  size_t dynamicIndex = 0;
  for (const std::string &dimension : dimensions) {
    int64_t extent = 0;
    if (trim(dimension) == "?") {
      if (dynamicIndex >= dynamicBounds.size())
        return std::nullopt;
      extent = dynamicBounds[dynamicIndex++].value;
    } else {
      try {
        extent = std::stoll(trim(dimension));
      } catch (const std::exception &) {
        return std::nullopt;
      }
    }
    if (extent < 0 || __builtin_mul_overflow(elements, extent, &elements))
      return std::nullopt;
  }
  int64_t totalBits = 0;
  if (__builtin_mul_overflow(elements, static_cast<int64_t>(*bits), &totalBits) ||
      totalBits > std::numeric_limits<int64_t>::max() - 7)
    return std::nullopt;
  return (totalBits + 7) / 8;
}

inline int EnclosingFunctionId(const GenericModule &module,
                               const GenericOperation &operation) {
  int parent = operation.parentId;
  while (parent >= 0) {
    const GenericOperation &candidate =
        module.operations.at(static_cast<size_t>(parent));
    if (candidate.name == "func.func")
      return parent;
    parent = candidate.parentId;
  }
  return -1;
}

inline const GenericOperation *DefinitionForBufferSize(
    const GenericModule &module, int value) {
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.results.begin(), operation.results.end(), value) !=
        operation.results.end())
      return &operation;
  return nullptr;
}

inline int TraceBufferSizeAlloc(const GenericModule &module, int value) {
  std::set<int> visited;
  while (visited.insert(value).second) {
    const GenericOperation *definition = DefinitionForBufferSize(module, value);
    if (definition == nullptr)
      return -1;
    if ((definition->name == "memref.alloc" ||
         definition->name == "memref.alloca") &&
        definition->results.size() == 1)
      return definition->id;
    static const std::set<std::string> aliases = {
        "memref.cast", "memref.reinterpret_cast", "memref.subview",
        "memref.view", "memref.collapse_shape", "memref.expand_shape"};
    if (aliases.count(definition->name) == 0 || definition->operands.empty())
      return -1;
    value = definition->operands.front();
  }
  return -1;
}

inline PostCVPipelineDiagnostic BufferSizeDiagnostic(
    const GenericModule &module, const GenericOperation &operation,
    const std::string &reason) {
  const int functionId = EnclosingFunctionId(module, operation);
  return {"InferAndSetBufferSize",
          functionId >= 0 ? FunctionSymbolName(module.operations.at(
                                 static_cast<size_t>(functionId)))
                          : "",
          operation.id, operation.name, reason};
}

inline bool IsAIVReachableBufferOperation(const GenericModule &module,
                                          const GenericOperation &operation) {
  const int functionId = EnclosingFunctionId(module, operation);
  if (functionId < 0)
    return false;
  const std::string core = trim(IRDictionaryValue(
      module.operations.at(static_cast<size_t>(functionId)).attributes,
      "hivm.func_core_type"));
  return core == "#hivm.func_core_type<AIV>" ||
         core == "#hivm.func_core_type<MIX>";
}

inline void ReplaceExistingUsesWithView(GenericModule &module, int oldValue,
                                        int newValue) {
  const std::map<int, int> replacement = {{oldValue, newValue}};
  for (GenericOperation &operation : module.operations) {
    RemapValues(operation.operands, replacement);
    RemapValues(operation.dpsInputs, replacement);
    RemapValues(operation.dpsInits, replacement);
  }
}

inline StageResult RunInferAndSetBufferSize(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  const GenericModule original = result.module;
  // Direct annotations participate in AutoInfer/Constantize. SetBufferSize
  // traces both direct and aliased annotations to the root allocation.
  std::map<int, int64_t> annotatedSizes;
  std::map<int, int64_t> setAnnotatedSizes;
  std::map<int, int64_t> functionElements;
  std::map<int, std::vector<int>> allocationMarks;
  std::set<int> directAllocationMarks;

  // AutoInferBufferSize: derive the shared element count from the first mark.
  for (const GenericOperation &mark : result.module.operations) {
    if (mark.name != "annotation.mark")
      continue;
    if (!IsAIVReachableBufferOperation(result.module, mark))
      continue;
    const std::string raw =
        IRDictionaryValue(mark.attributes, "buffer_size_in_byte");
    if (raw.empty())
      continue;
    const std::optional<int64_t> bytes = ParseIntegerAttribute(raw);
    if (!bytes || *bytes <= 0 || mark.operands.size() != 1 ||
        mark.operandTypes.size() != 1) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(BufferSizeDiagnostic(
          result.module, mark, "unsupported buffer_size_in_byte annotation"));
      continue;
    }
    const int allocation = TraceBufferSizeAlloc(result.module, mark.operands[0]);
    if (allocation < 0) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(BufferSizeDiagnostic(
          result.module, mark, "cannot trace annotation to a local alloc"));
      continue;
    }
    allocationMarks[allocation].push_back(mark.id);
    const GenericOperation &root = result.module.operations.at(
        static_cast<size_t>(allocation));
    // AutoInfer's isAnnotatedWithSize and Constantize's annotation lookup only
    // inspect direct users of the alloc result.  SetBufferSize alone traces
    // aliases such as subview/view back to their root allocation.
    const bool direct = !root.results.empty() &&
                        mark.operands.front() == root.results.front();
    if (direct)
      directAllocationMarks.insert(mark.id);
    if (direct && root.resultTypes.size() == 1 &&
        IsDynamicMemRef(root.resultTypes[0])) {
      const auto inserted = annotatedSizes.emplace(allocation, *bytes);
      if (!inserted.second && inserted.first->second != *bytes) {
        result.precision = Precision::Incomplete;
        result.diagnostics.push_back(BufferSizeDiagnostic(
            result.module, mark,
            "conflicting buffer size annotation on the same alloc"));
      }
    }
    if (root.resultTypes.size() == 1 && IsDynamicMemRef(root.resultTypes[0])) {
      const auto inserted = setAnnotatedSizes.emplace(allocation, *bytes);
      if (!inserted.second && inserted.first->second != *bytes) {
        result.precision = Precision::Incomplete;
        result.diagnostics.push_back(BufferSizeDiagnostic(
            result.module, mark,
            "conflicting buffer size annotation on the same alloc"));
      }
    }
    const std::optional<unsigned> bits = ShapedElementBits(mark.operandTypes[0]);
    const int functionId = EnclosingFunctionId(result.module, mark);
    if (bits && *bits != 0U && functionId >= 0) {
      int64_t bufferBits = 0;
      if (__builtin_mul_overflow(*bytes, int64_t{8}, &bufferBits)) {
        result.precision = Precision::Incomplete;
        result.diagnostics.push_back(BufferSizeDiagnostic(
            result.module, mark, "auto-inferred buffer size overflows"));
        continue;
      }
      const int64_t elements = bufferBits / static_cast<int64_t>(*bits);
      (void)functionElements.emplace(functionId, elements);
    }
  }
  if (result.precision == Precision::Incomplete) {
    result.module = original;
    return result;
  }

  // The real AutoInferBufferSize pass walks memref.alloc only.  Alloca is
  // handled by Constantize/Set, but never receives a synthesized mark.
  for (const GenericOperation &allocation : result.module.operations) {
    if (allocation.name != "memref.alloc" ||
        allocation.results.size() != 1 || allocation.resultTypes.size() != 1 ||
        !IsDynamicMemRef(allocation.resultTypes[0]) ||
        !IsAIVReachableBufferOperation(result.module, allocation) ||
        annotatedSizes.count(allocation.id) != 0)
      continue;
    const int functionId = EnclosingFunctionId(result.module, allocation);
    if (functionId < 0 || functionElements.count(functionId) == 0)
      continue;
    const GenericOperation &function =
        result.module.operations.at(static_cast<size_t>(functionId));
    if (!HasDictionaryUnit(function.attributes, "enable_auto_mark_buffer_size"))
      continue;
    const std::optional<unsigned> bits = ShapedElementBits(allocation.resultTypes[0]);
    if (!bits || *bits == 0U || (*bits % 8U) != 0U) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(BufferSizeDiagnostic(
          result.module, allocation,
          "dynamic alloc element width cannot be converted to bytes"));
      continue;
    }
    const int64_t elements = functionElements.at(functionId);
    if (elements > INT64_MAX / static_cast<int64_t>(*bits / 8U)) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(BufferSizeDiagnostic(
          result.module, allocation, "auto-inferred buffer size overflows"));
      continue;
    }
    annotatedSizes[allocation.id] =
        elements * static_cast<int64_t>(*bits / 8U);
    setAnnotatedSizes[allocation.id] = annotatedSizes[allocation.id];
  }
  if (result.precision == Precision::Incomplete) {
    result.module = original;
    return result;
  }

  // ConstantizeBufferSize: resolve every dimension to a constant or closed
  // upper bound before SetBufferSize consumes any remaining annotation.
  std::map<int, int64_t> physicalSizes;
  std::set<int> constantized;
  std::set<int> unsupportedAffineBounds;
  for (const GenericOperation &allocation : result.module.operations) {
    if ((allocation.name != "memref.alloc" && allocation.name != "memref.alloca") ||
        allocation.resultTypes.size() != 1 || !IsDynamicMemRef(allocation.resultTypes[0]) ||
        !IsAIVReachableBufferOperation(result.module, allocation))
      continue;
    std::vector<BufferSizeBound> bounds;
    bool resolved = allocation.operands.size() == allocation.operandTypes.size();
    for (int dimension : allocation.operands) {
      const auto bound = ResolveBufferSizeBound(result.module, dimension);
      if (!bound || bound->value < 0) {
        const GenericOperation *producer =
            DefinitionForBufferSize(result.module, dimension);
        if (producer != nullptr && producer->name == "affine.apply")
          unsupportedAffineBounds.insert(allocation.id);
        resolved = false;
        break;
      }
      bounds.push_back(*bound);
    }
    if (!resolved)
      continue;
    const auto logicalBytes = LogicalMemRefBytes(allocation.resultTypes[0], bounds);
    if (!logicalBytes) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(BufferSizeDiagnostic(
          result.module, allocation,
          "constantized logical buffer size overflows or is unsupported"));
      continue;
    }
    // ConstantizeBufferSize compares getStaticTotalSizeInBits with the raw
    // annotation integer.  Preserve that observable implementation behavior,
    // including the unit mismatch, so the model follows the compiler exactly.
    const auto annotated = annotatedSizes.find(allocation.id);
    if (annotated != annotatedSizes.end() &&
        (*logicalBytes > std::numeric_limits<int64_t>::max() / 8 ||
         *logicalBytes * 8 > annotated->second))
      continue;
    physicalSizes[allocation.id] = *logicalBytes;
    constantized.insert(allocation.id);
  }
  if (result.precision == Precision::Incomplete) {
    result.module = original;
    return result;
  }

  for (int allocationId : unsupportedAffineBounds) {
    const GenericOperation &allocation = result.module.operations.at(
        static_cast<size_t>(allocationId));
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(BufferSizeDiagnostic(
        result.module, allocation,
        "unsupported ValueBounds expression for dynamic local alloc"));
  }
  if (result.precision == Precision::Incomplete) {
    result.module = original;
    return result;
  }

  // SetBufferSize applies only to allocations that Constantize did not
  // rewrite.  Remaining annotations determine their physical byte capacity.
  for (const auto &[allocationId, bytes] : setAnnotatedSizes)
    if (constantized.count(allocationId) == 0)
      physicalSizes[allocationId] = bytes;

  for (const GenericOperation &allocation : result.module.operations) {
    if ((allocation.name != "memref.alloc" && allocation.name != "memref.alloca") ||
        allocation.resultTypes.size() != 1 || !IsDynamicMemRef(allocation.resultTypes[0]) ||
        !IsAIVReachableBufferOperation(result.module, allocation) ||
        HIVMAddressSpace(allocation.resultTypes[0]) != "UB" ||
        physicalSizes.count(allocation.id) != 0)
      continue;
    result.precision = Precision::Incomplete;
    const GenericOperation *producer = allocation.operands.empty()
                                           ? nullptr
                                           : DefinitionForBufferSize(
                                                 result.module,
                                                 allocation.operands.front());
    result.diagnostics.push_back(BufferSizeDiagnostic(
        result.module, allocation,
        producer && producer->name != "func.func"
            ? "unsupported ValueBounds expression for dynamic local alloc"
            : "unresolved physical size for dynamic local alloc"));
  }
  if (result.precision == Precision::Incomplete) {
    result.module = original;
    return result;
  }

  for (const auto &[allocationId, bytes] : physicalSizes) {
    GenericOperation &allocation =
        result.module.operations.at(static_cast<size_t>(allocationId));
    if (!IsDynamicMemRef(allocation.resultTypes.front()))
      continue;
    const std::string logicalType = allocation.resultTypes.front();
    const std::string physicalType = PhysicalByteMemRef(logicalType, bytes);
    if (physicalType.empty()) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(BufferSizeDiagnostic(
          result.module, allocation, "cannot construct physical byte view"));
      return result;
    }
    const int oldValue = allocation.results.front();
    const int parentId = allocation.parentId;
    const int regionId = allocation.regionId;
    const int blockId = allocation.blockId;
    const int ordinal = allocation.ordinal;
    const std::vector<int> dynamicSizes = allocation.operands;
    const std::vector<std::string> dynamicTypes = allocation.operandTypes;
    GenericRewriter rewriter(result.module);
    const int zero = rewriter.createOperation(
        parentId, regionId, blockId,
        "arith.constant", {"index"}, {}, {}, "", "{value = 0 : index}");
    const int zeroValue =
        result.module.operations.at(static_cast<size_t>(zero)).results.front();
    const int view = rewriter.createOperation(
        parentId, regionId, blockId,
        "memref.view", {logicalType},
        [&] {
          std::vector<int> operands = {oldValue, zeroValue};
          operands.insert(operands.end(), dynamicSizes.begin(), dynamicSizes.end());
          return operands;
        }(),
        [&] {
          std::vector<std::string> types = {physicalType, "index"};
          types.insert(types.end(), dynamicTypes.begin(), dynamicTypes.end());
          return types;
        }());
    const int viewValue =
        result.module.operations.at(static_cast<size_t>(view)).results.front();
    ReplaceExistingUsesWithView(result.module, oldValue, viewValue);
    result.module.operations.at(static_cast<size_t>(view)).operands.front() =
        oldValue;
    GenericOperation &updatedAllocation =
        result.module.operations.at(static_cast<size_t>(allocationId));
    updatedAllocation.resultTypes.front() = physicalType;
    updatedAllocation.operands.clear();
    updatedAllocation.operandTypes.clear();
    // Constantize creates a fresh attr-less alloc. Set uses the helper that
    // explicitly forwards alignment and no other allocation attributes.
    updatedAllocation.attributes =
        constantized.count(allocationId) != 0
            ? "{}"
            : KeepOnlyDictionaryAttribute(updatedAllocation.attributes,
                                          "alignment");
    ApplyOperationSemantics(updatedAllocation);
    rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), zero);
    rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 2), view);
  }

  // Constantize erases every size-mark user after a successful rewrite,
  // while SetBufferSize removes only the size attribute.  Keeping the two
  // paths distinct matters for hivm.multi_buffer and therefore UB peak.
  for (const auto &[allocationId, markIds] : allocationMarks) {
    for (int markId : markIds) {
      GenericOperation &mark = result.module.operations.at(
          static_cast<size_t>(markId));
      if (mark.blockId < 0)
        continue;
      if (constantized.count(allocationId) != 0 &&
          directAllocationMarks.count(markId) != 0) {
        GenericRewriter(result.module).removeFromBlock(mark.blockId, mark.id);
        continue;
      }
      mark.attributes = RemoveDictionaryAttribute(mark.attributes,
                                                  "buffer_size_in_byte");
      if (mark.attributes == "{}" && mark.blockId >= 0)
        GenericRewriter(result.module).removeFromBlock(mark.blockId, mark.id);
    }
  }
  result.module = CompactGenericModule(std::move(result.module));
  ValidateGenericModule(result.module);
  return result;
}

inline bool IsWorkspaceOperation(const GenericOperation &operation) {
  return operation.name == "memref_ext.alloc_workspace" ||
         operation.name.find("workspace") != std::string::npos;
}

inline bool IsAIVLoad(const GenericModule &module,
                      const GenericOperation &operation) {
  return operation.name == "hivm.hir.load" &&
         ClassifyCoreDetailed(module, operation).kind == CoreKind::Vector;
}

inline bool IsLocalBufferType(const std::string &type);

inline std::string InvariantFunctionName(const GenericModule &module,
                                         const GenericOperation &operation) {
  const int function = EnclosingFunctionId(module, operation);
  return function < 0
             ? ""
             : FunctionSymbolName(module.operations.at(static_cast<size_t>(function)));
}

inline std::string InvariantValueOrigin(const GenericModule &module, int value) {
  for (const GenericBlock &block : module.blocks) {
    const auto found = std::find(block.arguments.begin(), block.arguments.end(), value);
    if (found != block.arguments.end())
      return "arg:" + std::to_string(std::distance(block.arguments.begin(), found));
  }
  const GenericOperation *definition = DefinitionForBufferSize(module, value);
  if (definition == nullptr)
    return "external";
  const auto found = std::find(definition->results.begin(), definition->results.end(), value);
  return definition->name + ":" +
         std::to_string(std::distance(definition->results.begin(), found));
}

inline std::string StableInvariantKey(const GenericModule &module,
                                      const GenericOperation &operation) {
  // A semantic role key: independent of parser ids, test-only `case` attrs,
  // block order and workspace offsets.  Fields verified below are omitted so
  // a mutation pairs with its original and receives a precise diagnostic.
  std::string attributes = RemoveDictionaryAttribute(operation.attributes, "case");
  attributes = RemoveDictionaryAttribute(attributes, "workspace_offset");
  attributes = RemoveDictionaryAttribute(attributes, "alias_source");
  std::string key = InvariantFunctionName(module, operation) + "|" +
                    operation.name + "|" + operation.properties + "|" +
                    attributes + "|" +
                    std::to_string(operation.operands.size());
  for (int operand : operation.operands)
    key += "|" + InvariantValueOrigin(module, operand);
  return key;
}

inline bool IsWorkspaceUBRelevant(const GenericModule &module,
                                  const GenericOperation &operation) {
  if (IsWorkspaceOperation(operation) || IsAIVLoad(module, operation))
    return true;
  return std::any_of(operation.operandTypes.begin(), operation.operandTypes.end(),
                     IsLocalBufferType) ||
         std::any_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                     IsLocalBufferType);
}

inline StageResult VerifyWorkspaceUBInvariant(const GenericModule &before,
                                              GenericModule after) {
  StageResult result;
  result.module = std::move(after);
  std::map<std::string, std::vector<const GenericOperation *>> oldOps;
  for (const GenericOperation &operation : before.operations)
    if (IsWorkspaceUBRelevant(before, operation))
      oldOps[StableInvariantKey(before, operation)].push_back(&operation);

  for (const GenericOperation &operation : result.module.operations) {
    if (!IsWorkspaceUBRelevant(result.module, operation))
      continue;
    const std::string key = StableInvariantKey(result.module, operation);
    auto found = oldOps.find(key);
    std::string reason;
    const GenericOperation *old =
        found == oldOps.end() || found->second.empty() ? nullptr
                                                       : found->second.back();
    if (old == nullptr)
      reason = "workspace rewrite added a UB-relevant operation";
    else if (operation.resultTypes != old->resultTypes)
      reason = IsAIVLoad(result.module, operation)
                   ? "workspace rewrite changes AIV load result shape"
                   : "workspace rewrite changes local result type";
    else if (operation.operandTypes != old->operandTypes)
      reason = "workspace rewrite changes local operand type";
    else if (IsWorkspaceOperation(operation)) {
      const std::string oldAlias =
          IRDictionaryValue(old->attributes, "alias_source");
      const std::string newAlias =
          IRDictionaryValue(operation.attributes, "alias_source");
      if (oldAlias != newAlias ||
          (oldAlias.empty() &&
           ((operation.operands.empty() != old->operands.empty()) ||
            (!operation.operands.empty() &&
             InvariantValueOrigin(result.module, operation.operands.front()) !=
                 InvariantValueOrigin(before, old->operands.front())))))
        reason = "workspace rewrite changes alias source";
    }
    if (!reason.empty()) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"WorkspaceSemanticProjection", "", operation.id, operation.name,
           reason});
    }
    if (old != nullptr) {
      found->second.pop_back();
      if (found->second.empty())
        oldOps.erase(found);
    }
  }
  for (const auto &[key, operations] : oldOps) {
    (void)key;
    for (const GenericOperation *operation : operations) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"WorkspaceSemanticProjection", "", operation->id, operation->name,
           "workspace rewrite removed a UB-relevant operation"});
    }
  }
  return result;
}

inline StageResult ProveWorkspacePlanningUBInvariant(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  for (const GenericOperation &operation : result.module.operations) {
    if (!IsWorkspaceOperation(operation))
      continue;
    const bool allResultsGM =
        !operation.resultTypes.empty() &&
        std::all_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                    [](const std::string &type) {
                      return HIVMAddressSpace(type) == "GM";
                    });
    const bool noLocalOperand =
        std::none_of(operation.operandTypes.begin(), operation.operandTypes.end(),
                     IsLocalBufferType);
    if (allResultsGM && noLocalOperand)
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"WorkspaceSemanticProjection", InvariantFunctionName(result.module, operation),
         operation.id, operation.name,
         "cannot prove global workspace planning is UB invariant for a local workspace value"});
  }
  // GLOBAL_WORKSPACE_PLAN only assigns/reuses GM workspace offsets.  Loads
  // remain the same operations with the same tensor result types, and local
  // allocations are not planned in this mode.  The checks above establish the
  // pass precondition instead of comparing an artifact with itself.
  return result;
}

inline bool IsCrossCoreSync(const GenericOperation &operation) {
  return startsWith(operation.name, "hivm.hir.") &&
         (operation.name.find("sync") != std::string::npos ||
          operation.name.find("barrier") != std::string::npos);
}

inline bool IsLocalBufferType(const std::string &type) {
  const std::string space = HIVMAddressSpace(type);
  return space == "UB" || (space.empty() && startsWith(trim(type), "memref<"));
}

inline StageResult VerifyCrossCoreSyncUBInvariant(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  for (const GenericOperation &operation : result.module.operations) {
    if (!IsCrossCoreSync(operation))
      continue;
    const bool localOperand =
        std::any_of(operation.operandTypes.begin(), operation.operandTypes.end(),
                    IsLocalBufferType);
    const bool localResult =
        std::any_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                    IsLocalBufferType);
    if (!localOperand && !localResult)
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"CrossCoreSyncInvariant", "", operation.id, operation.name,
         "sync operation carries a local buffer operand or result"});
  }
  return result;
}

inline StageResult ProveCrossCoreSyncPipelineUBInvariant(GenericModule module) {
  // InjectBlockSync/CrossCoreGSS materialize only sync_block, sync_block_set,
  // sync_block_wait and pipe-barrier operations.  Their optional SSA operands
  // are scalar flag/base-address values and they have no memref results.  A
  // full scan also protects already-present sync operations and fails closed
  // if the artifact violates that real op-schema boundary.
  return VerifyCrossCoreSyncUBInvariant(std::move(module));
}

} // namespace cvub

#endif
