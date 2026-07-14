#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_BUFFER_SIZE_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_BUFFER_SIZE_HPP

#include "core_type.hpp"
#include "types.hpp"

#include <cstdint>
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
    return static_cast<unsigned>(std::stoul(match[1].str().substr(1)));
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
  for (size_t index = 1; index < fields.size(); ++index)
    result += ", " + fields[index];
  return result + ">";
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
  std::map<int, int64_t> allocationSizes;
  std::map<int, int64_t> functionElements;
  std::vector<int> sizeMarks;

  for (const GenericOperation &mark : result.module.operations) {
    if (mark.name != "annotation.mark")
      continue;
    if (!IsAIVReachableBufferOperation(result.module, mark))
      continue;
    const std::string raw =
        IRDictionaryValue(mark.attributes, "buffer_size_in_byte");
    if (raw.empty())
      continue;
    sizeMarks.push_back(mark.id);
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
    const GenericOperation &root =
        result.module.operations.at(static_cast<size_t>(allocation));
    if (root.resultTypes.size() == 1 && IsDynamicMemRef(root.resultTypes[0])) {
      const auto inserted = allocationSizes.emplace(allocation, *bytes);
      if (!inserted.second && inserted.first->second != *bytes) {
        result.precision = Precision::Incomplete;
        result.diagnostics.push_back(BufferSizeDiagnostic(
            result.module, mark,
            "conflicting buffer size annotation on the same alloc"));
      }
    }
    const std::optional<unsigned> bits = ShapedElementBits(mark.operandTypes[0]);
    const int functionId = EnclosingFunctionId(result.module, mark);
    if (bits && *bits != 0U && (*bytes * 8) % static_cast<int64_t>(*bits) == 0 &&
        functionId >= 0) {
      const int64_t elements =
          (*bytes * 8) / static_cast<int64_t>(*bits);
      (void)functionElements.emplace(functionId, elements);
    }
  }
  if (result.precision == Precision::Incomplete)
    return result;

  for (const GenericOperation &allocation : result.module.operations) {
    if ((allocation.name != "memref.alloc" &&
         allocation.name != "memref.alloca") ||
        allocation.results.size() != 1 || allocation.resultTypes.size() != 1 ||
        !IsDynamicMemRef(allocation.resultTypes[0]) ||
        !IsAIVReachableBufferOperation(result.module, allocation) ||
        allocationSizes.count(allocation.id) != 0)
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
    allocationSizes[allocation.id] =
        elements * static_cast<int64_t>(*bits / 8U);
  }
  if (result.precision == Precision::Incomplete)
    return result;

  for (const GenericOperation &allocation : result.module.operations) {
    if ((allocation.name != "memref.alloc" &&
         allocation.name != "memref.alloca") ||
        allocation.resultTypes.size() != 1 ||
        !IsDynamicMemRef(allocation.resultTypes[0]) ||
        !IsAIVReachableBufferOperation(result.module, allocation) ||
        HIVMAddressSpace(allocation.resultTypes[0]) != "UB" ||
        allocationSizes.count(allocation.id) != 0)
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(BufferSizeDiagnostic(
        result.module, allocation,
        "unresolved physical size for dynamic local alloc"));
  }
  if (result.precision == Precision::Incomplete)
    return result;

  for (const auto &[allocationId, bytes] : allocationSizes) {
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
    ApplyOperationSemantics(updatedAllocation);
    rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), zero);
    rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 2), view);
  }

  for (int markId : sizeMarks) {
    GenericOperation &mark =
        result.module.operations.at(static_cast<size_t>(markId));
    if (mark.blockId >= 0)
      GenericRewriter(result.module).removeFromBlock(mark.blockId, mark.id);
  }
  result.module = CompactGenericModule(std::move(result.module));
  ValidateGenericModule(result.module);
  return result;
}

inline std::string StableInvariantKey(const GenericOperation &operation) {
  std::string key = UnquoteIRString(IRDictionaryValue(operation.attributes, "case"));
  if (key.empty())
    key = operation.name + "#" + std::to_string(operation.id);
  return key;
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

inline StageResult VerifyWorkspaceUBInvariant(const GenericModule &before,
                                              GenericModule after) {
  StageResult result;
  result.module = std::move(after);
  std::map<std::string, const GenericOperation *> oldOps;
  for (const GenericOperation &operation : before.operations)
    if (IsWorkspaceOperation(operation) || IsAIVLoad(before, operation) ||
        std::any_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                    [](const std::string &type) {
                      return HIVMAddressSpace(type) == "UB";
                    }))
      oldOps[StableInvariantKey(operation)] = &operation;

  for (const GenericOperation &operation : result.module.operations) {
    const bool relevant =
        IsWorkspaceOperation(operation) || IsAIVLoad(result.module, operation) ||
        std::any_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                    [](const std::string &type) {
                      return HIVMAddressSpace(type) == "UB";
                    });
    if (!relevant)
      continue;
    const auto found = oldOps.find(StableInvariantKey(operation));
    std::string reason;
    if (found == oldOps.end())
      reason = "workspace rewrite added a UB-relevant operation";
    else if (operation.resultTypes != found->second->resultTypes)
      reason = IsAIVLoad(result.module, operation)
                   ? "workspace rewrite changes AIV load result shape"
                   : "workspace rewrite changes local result type";
    else if (IsWorkspaceOperation(operation)) {
      const std::string oldAlias =
          IRDictionaryValue(found->second->attributes, "alias_source");
      const std::string newAlias =
          IRDictionaryValue(operation.attributes, "alias_source");
      if (oldAlias != newAlias ||
          (oldAlias.empty() &&
           ((operation.operands.empty() != found->second->operands.empty()) ||
            (!operation.operands.empty() &&
             operation.operands.front() != found->second->operands.front()))))
        reason = "workspace rewrite changes alias source";
    }
    if (!reason.empty()) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"WorkspaceSemanticProjection", "", operation.id, operation.name,
           reason});
    }
    oldOps.erase(StableInvariantKey(operation));
  }
  for (const auto &[key, operation] : oldOps) {
    (void)key;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"WorkspaceSemanticProjection", "", operation->id, operation->name,
         "workspace rewrite removed a UB-relevant operation"});
  }
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

} // namespace cvub

#endif
