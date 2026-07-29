#include "../src/passes/pre_cv_memref_dead_store_elimination.hpp"
#include "../src/pipeline/buffer_topology.hpp"

#include <iostream>
#include <map>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

std::string FunctionName(const cvub::GenericModule &module,
                         const cvub::GenericModuleAnalysisIndexes &indexes,
                         int operationId) {
  const int functionId = indexes.enclosingFunctionId(operationId);
  if (functionId < 0)
    return "";
  return cvub::FindDictionaryValue(
      module.operations.at(static_cast<size_t>(functionId)).properties,
      "sym_name");
}

} // namespace

int main() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/pre_cv_memref_dse.mlir", false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunPreCVMemrefDeadStoreElimination(std::move(module));

  const cvub::GenericModuleAnalysisIndexes indexes(
      module, cvub::kGenericAnalysisEnclosingFunctions);
  std::map<std::string, std::map<std::string, size_t>> counts;
  for (const cvub::GenericOperation &operation : module.operations)
    ++counts[FunctionName(module, indexes, operation.id)][operation.name];

  Check(counts["\"forward_and_barrier\""]["memref.load"] == 1,
        "exact-index and scalar-like loads must forward; write barrier must remain");
  Check(counts["\"nested_level\""]["memref.load"] == 1,
        "same-level nested loads must forward and nested writes must clear parent cache");
  Check(counts["\"view_alias\""]["memref.load"] == 0,
        "view-like values must resolve to the same allocation root");
  Check(counts["\"dead_alloc\""]["memref.alloc"] == 0 &&
            counts["\"dead_alloc\""]["memref.store"] == 0,
        "unread alloc and store must be erased");
  Check(counts["\"dead_alloc_subview\""]["memref.alloc"] == 0 &&
            counts["\"dead_alloc_subview\""]["memref.subview"] == 0 &&
            counts["\"dead_alloc_subview\""]["memref.store"] == 0,
        "unread alloc, subview, and store chain must be erased");
  Check(counts["\"call_barrier\""]["memref.load"] == 1,
        "func.call must invalidate forwarding for every argument root");
  cvub::GenericModule regBased = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/pre_cv_memref_dse_reg_based.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(regBased.operations);
  regBased = cvub::RunPreCVMemrefDeadStoreElimination(std::move(regBased));
  const cvub::GenericModuleAnalysisIndexes regIndexes(
      regBased, cvub::kGenericAnalysisEnclosingFunctions);
  std::map<std::string, std::map<std::string, size_t>> regCounts;
  for (const cvub::GenericOperation &operation : regBased.operations)
    ++regCounts[FunctionName(regBased, regIndexes, operation.id)][operation.name];
  Check(regCounts["\"remove_unused_load\""]["hivm.hir.load"] == 0 &&
            regCounts["\"remove_unused_load\""]["memref.alloc"] == 0,
        "reg-based trailing unused HIVM load and allocation must be erased");
  Check(regCounts["\"retain_indirect_load\""]["hivm.hir.load"] == 1,
        "reg-based HIVM load with a later allocation user must remain");

  bool regForwardFailure = false;
  try {
    cvub::GenericModule failure = cvub::ParseGenericIR(
        "ub_overflow_model_cpp/tests/fixtures/"
        "pre_cv_memref_dse_reg_forward_failure.mlir",
        false);
    cvub::ApplyOperationSemanticsToAll(failure.operations);
    (void)cvub::RunPreCVMemrefDeadStoreElimination(std::move(failure));
  } catch (const std::runtime_error &error) {
    regForwardFailure =
        std::string(error.what()).find("reg-based load forwarding aborts") !=
        std::string::npos;
  }
  Check(regForwardFailure,
        "native reg-based forwarded-load abort must fail open explicitly");

  std::cout << "[PASS] pre-CV MemRef DSE mirrors native forwarding and cleanup\n";
  return 0;
}
