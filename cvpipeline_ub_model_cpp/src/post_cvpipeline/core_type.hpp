#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_CORE_TYPE_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_CORE_TYPE_HPP

#include "ir_utils.hpp"

#include <set>
#include <string>

namespace cvub {

enum class CoreKind { Cube, Vector, Neutral, Unknown };

struct CoreClassification {
  CoreKind kind = CoreKind::Unknown;
  std::string reason;
};

inline CoreKind ParseTCoreType(const std::string &value) {
  const std::string spelling = trim(value);
  static const std::string prefix = "#hivm.tcore_type<";
  if (!startsWith(spelling, prefix) || spelling.back() != '>')
    return CoreKind::Unknown;
  const std::string payload =
      spelling.substr(prefix.size(), spelling.size() - prefix.size() - 1);
  if (payload == "VECTOR")
    return CoreKind::Vector;
  if (payload == "CUBE")
    return CoreKind::Cube;
  // CUBE_OR_VECTOR and CUBE_AND_VECTOR are valid enum values, but neither is
  // a safe basis for deleting an operation from one side of a MIX function.
  return CoreKind::Unknown;
}

inline std::string CoreAttribute(const GenericOperation &operation,
                                 const std::string &name) {
  std::string value = IRDictionaryValue(operation.properties, name);
  if (value.empty())
    value = IRDictionaryValue(operation.attributes, name);
  return value;
}

inline std::string HIVMAddressSpace(const std::string &type) {
  for (const std::string &prefix : {"#hivm.address_space<",
                                    "#hivm.mem_scope<"}) {
    const size_t begin = type.find(prefix);
    if (begin == std::string::npos)
      continue;
    const size_t payload = begin + prefix.size();
    const size_t end = type.find('>', payload);
    if (end == std::string::npos)
      return "";
    std::string result = type.substr(payload, end - payload);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character) {
                     return static_cast<char>(std::toupper(character));
                   });
    return result;
  }
  return "";
}

inline int OperandIndex(const GenericOperation &operation, int value) {
  const auto found =
      std::find(operation.operands.begin(), operation.operands.end(), value);
  return found == operation.operands.end()
             ? -1
             : static_cast<int>(std::distance(operation.operands.begin(), found));
}

inline std::string OperandAddressSpace(const GenericOperation &operation,
                                       size_t index) {
  return index < operation.operandTypes.size()
             ? HIVMAddressSpace(operation.operandTypes[index])
             : "";
}

inline CoreClassification
ClassifyCoreDetailed(const GenericModule &module,
                     const GenericOperation &operation);

inline CoreClassification ClassifyLoadFromUsers(
    const GenericModule &module, const GenericOperation &operation) {
  if (operation.results.empty())
    return {CoreKind::Unknown, "load has no result for user-based inference"};
  bool sawUser = false;
  CoreKind common = CoreKind::Unknown;
  for (const GenericOperation &user : module.operations) {
    if (user.id == operation.id ||
        std::find(user.operands.begin(), user.operands.end(),
                  operation.results.front()) == user.operands.end())
      continue;
    const CoreKind kind = ClassifyCoreDetailed(module, user).kind;
    if (kind != CoreKind::Cube && kind != CoreKind::Vector)
      return {CoreKind::Unknown,
              "load users do not provide a unanimous core classification"};
    if (!sawUser) {
      sawUser = true;
      common = kind;
    } else if (common != kind) {
      return {CoreKind::Unknown,
              "load users do not provide a unanimous core classification"};
    }
  }
  return sawUser
             ? CoreClassification{common, "load user core classification"}
             : CoreClassification{CoreKind::Unknown,
                                  "load has no classifiable users"};
}

inline CoreClassification ClassifyContextualOperation(
    const GenericModule &module, const GenericOperation &operation) {
  if (operation.name == "hivm.hir.load" ||
      operation.name == "hivm.hir.copy") {
    if (operation.operands.size() < 2)
      return {CoreKind::Unknown, "DMA operation has fewer than two operands"};
    const std::string source = OperandAddressSpace(operation, 0);
    size_t destinationIndex = 1;
    if (!operation.dpsInits.empty()) {
      const int index = OperandIndex(operation, operation.dpsInits.front());
      if (index >= 0)
        destinationIndex = static_cast<size_t>(index);
    }
    const std::string destination =
        OperandAddressSpace(operation, destinationIndex);
    if (!source.empty() && !destination.empty()) {
      if (operation.name == "hivm.hir.load")
        return {source == "GM" && destination == "UB" ? CoreKind::Vector
                                                        : CoreKind::Cube,
                "load address spaces"};
      static const std::set<std::pair<std::string, std::string>> vectorCopies = {
          {"GM", "UB"}, {"UB", "GM"}, {"UB", "UB"}, {"UB", "L1"}};
      return {vectorCopies.count({source, destination}) != 0
                  ? CoreKind::Vector
                  : CoreKind::Cube,
              "copy address spaces"};
    }
    if (operation.name == "hivm.hir.load")
      return ClassifyLoadFromUsers(module, operation);
    return {CoreKind::Unknown,
            "copy lacks concrete source and destination address spaces"};
  }

  if (operation.name == "hivm.hir.vbrc") {
    size_t destinationIndex = operation.operands.empty()
                                  ? 0
                                  : operation.operands.size() - 1;
    if (!operation.dpsInits.empty()) {
      const int index = OperandIndex(operation, operation.dpsInits.front());
      if (index >= 0)
        destinationIndex = static_cast<size_t>(index);
    }
    const std::string destination =
        OperandAddressSpace(operation, destinationIndex);
    if (destination.empty() || destination == "UB")
      return {CoreKind::Vector, "vbrc destination address space"};
    if (destination == "L1")
      return {CoreKind::Cube, "vbrc destination address space"};
    return {CoreKind::Unknown, "unsupported vbrc destination address space"};
  }

  if (operation.name == "hivm.hir.mix_matmul" ||
      operation.name == "hivm.hir.mix_group_matmul") {
    int parent = operation.parentId;
    while (parent >= 0) {
      const GenericOperation &ancestor =
          module.operations.at(static_cast<size_t>(parent));
      if (ancestor.name == "func.func") {
        const std::string core =
            trim(IRDictionaryValue(ancestor.attributes, "hivm.func_core_type"));
        if (core == "#hivm.func_core_type<AIV>")
          return {CoreKind::Vector, "enclosing AIV function"};
        if (core == "#hivm.func_core_type<AIC>")
          return {CoreKind::Cube, "enclosing AIC function"};
        break;
      }
      parent = ancestor.parentId;
    }
    return {CoreKind::Unknown,
            "mix matmul has no single-core enclosing function"};
  }

  if (operation.name == "hivm.hir.custom")
    return {CoreKind::Unknown, "operation has no compiler core classification"};

  return {CoreKind::Unknown,
          "operation requires unavailable contextual core classification"};
}

inline CoreClassification
ClassifyCoreDetailed(const GenericModule &module,
                     const GenericOperation &operation) {
  if (operation.name == "scf.for" || operation.name == "scope.scope") {
    const std::string loop = CoreAttribute(operation, "hivm.loop_core_type");
    if (!loop.empty()) {
      const CoreKind kind = ParseTCoreType(loop);
      return {kind, kind == CoreKind::Unknown
                        ? "unrecognized or ambiguous hivm.loop_core_type: " + loop
                        : "hivm.loop_core_type"};
    }
  }

  static const std::vector<std::string> explicitAttributes = {
      "tcoretype", "hivm.tcoretype", "hivm.tcore_type", "core_type",
      "hivm.core_type"};
  for (const std::string &attribute : explicitAttributes) {
    const std::string value = CoreAttribute(operation, attribute);
    if (value.empty())
      continue;
    const CoreKind kind = ParseTCoreType(value);
    return {kind, kind == CoreKind::Unknown
                      ? "unrecognized or ambiguous " + attribute + ": " + value
                      : "explicit " + attribute};
  }

  static const std::set<std::string> cubeOperations = {
      "hivm.hir.mmadL1", "hivm.hir.batchMmadL1", "hivm.hir.Conv1dL1",
      "hivm.hir.Conv2dL1", "hivm.hir.Conv3dL1", "hivm.hir.fixpipe",
      "hivm.hir.matmul", "hivm.hir.nd2nz", "hivm.hir.nz2nd"};
  if (cubeOperations.count(operation.name) != 0)
    return {CoreKind::Cube, "fixed Cube operation registry"};

  static const std::set<std::string> contextualOperations = {
      "hivm.hir.load", "hivm.hir.copy", "hivm.hir.vbrc",
      "hivm.hir.mix_matmul", "hivm.hir.mix_group_matmul",
      "hivm.hir.custom"};
  if (contextualOperations.count(operation.name) != 0)
    return ClassifyContextualOperation(module, operation);

  static const std::set<std::string> vectorOperations = {
      "hivm.hir.atomic_rmw", "hivm.hir.store", "hivm.hir.indirect_store",
      "hivm.hir.sync_block_lock", "hivm.hir.sync_block_unlock",
      "hivm.hir.free_lock_var", "hivm.hir.vabs", "hivm.hir.vadd",
      "hivm.hir.vand", "hivm.hir.varange", "hivm.hir.vcast",
      "hivm.hir.vcmp", "hivm.hir.vconcat", "hivm.hir.vcos",
      "hivm.hir.vcumprod", "hivm.hir.vcumsum", "hivm.hir.vdeinterleave",
      "hivm.hir.vdiv", "hivm.hir.verf", "hivm.hir.vexp",
      "hivm.hir.vflip", "hivm.hir.vgather", "hivm.hir.vgathermask",
      "hivm.hir.vinterleave", "hivm.hir.vln", "hivm.hir.vmax",
      "hivm.hir.vmin", "hivm.hir.vmod", "hivm.hir.vmul",
      "hivm.hir.vmulext", "hivm.hir.vmulextended", "hivm.hir.vmulextui",
      "hivm.hir.vnot", "hivm.hir.vor", "hivm.hir.vpad",
      "hivm.hir.vpow", "hivm.hir.vrec", "hivm.hir.vreduce",
      "hivm.hir.vrelu", "hivm.hir.vrsqrt", "hivm.hir.vsel",
      "hivm.hir.vshl", "hivm.hir.vshr", "hivm.hir.vsin",
      "hivm.hir.vsort", "hivm.hir.vsqrt", "hivm.hir.vsub",
      "hivm.hir.vtanh", "hivm.hir.vtranspose", "hivm.hir.vxor"};
  if (vectorOperations.count(operation.name) != 0)
    return {CoreKind::Vector, "fixed Vector operation registry"};

  static const std::set<std::string> neutralOperations = {
      "builtin.module", "func.func", "func.return", "scope.scope",
      "cf.br", "cf.cond_br", "scf.condition", "scf.for", "scf.if",
      "scf.while", "scf.yield", "scope.yield"};
  static const std::vector<std::string> neutralPrefixes = {
      "affine.", "annotation.", "arith.", "bufferization.", "cf.",
      "func.", "llvm.", "memref.", "memref_ext.", "scf.", "scope.",
      "tensor."};
  if (neutralOperations.count(operation.name) != 0 ||
      std::any_of(neutralPrefixes.begin(), neutralPrefixes.end(),
                  [&](const std::string &prefix) {
                    return startsWith(operation.name, prefix);
                  }))
    return {CoreKind::Neutral, "structural or annotation operation"};

  return {CoreKind::Unknown, "operation has no compiler core classification"};
}

inline CoreKind ClassifyCore(const GenericModule &module,
                             const GenericOperation &operation) {
  return ClassifyCoreDetailed(module, operation).kind;
}

} // namespace cvub

#endif
