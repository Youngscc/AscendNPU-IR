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

inline CoreKind ParseCoreKind(const std::string &value) {
  const std::string payload = [&] {
    std::string result = UnquoteIRString(value);
    const size_t open = result.rfind('<');
    const size_t close = result.rfind('>');
    if (open != std::string::npos && close == result.size() - 1 && open < close)
      result = result.substr(open + 1, close - open - 1);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character) {
                     return static_cast<char>(std::toupper(character));
                   });
    return result;
  }();
  if (payload == "VECTOR" || payload == "AIV")
    return CoreKind::Vector;
  if (payload == "CUBE" || payload == "AIC")
    return CoreKind::Cube;
  return CoreKind::Unknown;
}

inline std::string CoreAttribute(const GenericOperation &operation,
                                 const std::string &name) {
  std::string value = IRDictionaryValue(operation.properties, name);
  if (value.empty())
    value = IRDictionaryValue(operation.attributes, name);
  return value;
}

inline CoreClassification
ClassifyCoreDetailed(const GenericModule &, const GenericOperation &operation) {
  if (operation.name == "scf.for" || operation.name == "scope.scope") {
    const std::string loop = CoreAttribute(operation, "hivm.loop_core_type");
    if (!loop.empty()) {
      const CoreKind kind = ParseCoreKind(loop);
      return {kind, kind == CoreKind::Unknown
                        ? "unrecognized hivm.loop_core_type: " + loop
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
    const CoreKind kind = ParseCoreKind(value);
    return {kind, kind == CoreKind::Unknown
                      ? "unrecognized " + attribute + ": " + value
                      : "explicit " + attribute};
  }

  static const std::set<std::string> cubeOperations = {
      "hivm.hir.mmadL1", "hivm.hir.batchMmadL1", "hivm.hir.Conv1dL1",
      "hivm.hir.Conv2dL1", "hivm.hir.Conv3dL1", "hivm.hir.fixpipe"};
  if (cubeOperations.count(operation.name) != 0)
    return {CoreKind::Cube, "known Cube operation"};

  if (IsDestinationStyleOp(operation.name) &&
      operation.name != "hivm.hir.custom")
    return {CoreKind::Vector, "HIVM destination-style registry"};

  static const std::set<std::string> neutralOperations = {
      "builtin.module", "func.func", "func.return", "scope.scope",
      "cf.br", "cf.cond_br", "scf.condition", "scf.for", "scf.if",
      "scf.while", "scf.yield"};
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
