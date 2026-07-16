#ifndef CVPIPELINE_UB_MODEL_CPP_HIVM_DECOMPOSE_OP_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_HIVM_DECOMPOSE_OP_ORACLE_HPP

#include "../passes/hivm_decompose_op.hpp"

namespace cvub {

inline std::string FunctionSymName(const GenericOperation &operation) {
  std::string name = FindDictionaryValue(operation.properties, "sym_name");
  if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  return name;
}

inline std::vector<std::string> FunctionAllocationTypes(
    const GenericModule &module, const std::string &function) {
  std::vector<std::string> result;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "memref.alloc")
      continue;
    const GenericOperation *owner = EnclosingFunction(module, operation);
    if (!owner || FunctionSymName(*owner) != function)
      continue;
    for (const std::string &type : operation.resultTypes)
      if (IsMemRefType(type))
        result.push_back(type);
  }
  return result;
}

inline std::vector<DecomposeBufferAllocation>
CollectHIVMDecomposeOpOracle(const GenericModule &before,
                             const GenericModule &after,
                             const std::string &function) {
  const std::vector<std::string> lhs =
      FunctionAllocationTypes(before, function);
  const std::vector<std::string> rhs = FunctionAllocationTypes(after, function);
  std::vector<std::vector<size_t>> lcs(
      lhs.size() + 1, std::vector<size_t>(rhs.size() + 1));
  for (size_t left = lhs.size(); left > 0; --left)
    for (size_t right = rhs.size(); right > 0; --right)
      lcs[left - 1][right - 1] =
          lhs[left - 1] == rhs[right - 1]
              ? lcs[left][right] + 1
              : std::max(lcs[left][right - 1], lcs[left - 1][right]);

  std::vector<DecomposeBufferAllocation> result;
  size_t left = 0;
  size_t right = 0;
  while (right < rhs.size()) {
    if (left < lhs.size() && lhs[left] == rhs[right]) {
      ++left;
      ++right;
    } else if (left < lhs.size() &&
               lcs[left + 1][right] >= lcs[left][right + 1]) {
      ++left;
    } else {
      result.push_back({-1, "oracle", rhs[right++]});
    }
  }
  return result;
}

inline bool IsUBRelevantDecomposeOperation(
    const GenericOperation &operation) {
  if (operation.name == "memref.alloc")
    return false;
  if (!startsWith(operation.name, "hivm.hir."))
    return false;
  static const std::set<std::string> ignored = {
      "hivm.hir.create_sync_block_lock", "hivm.hir.pipe_barrier",
      "hivm.hir.sync_block", "hivm.hir.sync_block_lock",
      "hivm.hir.sync_block_set", "hivm.hir.sync_block_unlock",
      "hivm.hir.sync_block_wait"};
  if (ignored.count(operation.name) != 0)
    return false;
  return std::any_of(operation.operandTypes.begin(),
                     operation.operandTypes.end(), [](const std::string &type) {
                       return IsMemRefType(type) || IsTensorType(type);
                     }) ||
         std::any_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                     [](const std::string &type) {
                       return IsMemRefType(type) || IsTensorType(type);
                     });
}

inline DecomposeOperationDelta FunctionUBOperationCounts(
    const GenericModule &module, const std::string &function) {
  DecomposeOperationDelta result;
  for (const GenericOperation &operation : module.operations) {
    const GenericOperation *owner = EnclosingFunction(module, operation);
    if (owner && FunctionSymName(*owner) == function &&
        IsUBRelevantDecomposeOperation(operation))
      ++result[operation.name];
  }
  return result;
}

inline DecomposeOperationDelta CollectHIVMDecomposeOperationDeltaOracle(
    const GenericModule &before, const GenericModule &after,
    const std::string &function) {
  const DecomposeOperationDelta lhs =
      FunctionUBOperationCounts(before, function);
  DecomposeOperationDelta result = FunctionUBOperationCounts(after, function);
  for (const auto &[name, count] : lhs) {
    result[name] -= count;
    if (result[name] == 0)
      result.erase(name);
  }
  return result;
}

} // namespace cvub

#endif
