#include "../src/passes/pre_cv_hivm_opt_single_point.hpp"

#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

std::string FunctionName(const cvub::GenericModule &module, int functionId) {
  if (functionId < 0)
    return "";
  const cvub::GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  return cvub::FindDictionaryValue(function.properties, "sym_name");
}

} // namespace

int main() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/"
      "pre_cv_hivm_opt_single_point.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunPreCVHIVMOptSinglePoint(std::move(module));

  const cvub::GenericModuleAnalysisIndexes indexes(
      module, cvub::kGenericAnalysisEnclosingFunctions);
  std::map<std::string, std::map<std::string, size_t>> operationsByFunction;
  std::set<std::string> scalarOperations;
  size_t scalarLoads = 0;
  size_t scalarStores = 0;
  for (const cvub::GenericOperation &operation : module.operations) {
    const std::string function =
        FunctionName(module, indexes.enclosingFunctionId(operation.id));
    ++operationsByFunction[function][operation.name];
    if (cvub::startsWith(operation.name, "arith.") ||
        cvub::startsWith(operation.name, "math."))
      scalarOperations.insert(operation.name);
    if (operation.name == "memref.load")
      ++scalarLoads;
    if (operation.name == "memref.store")
      ++scalarStores;
  }

  const auto count = [&](const std::string &function,
                         const std::string &operation) {
    return operationsByFunction[function][operation];
  };
  Check(count("\"single_point_success\"", "hivm.hir.vbrc") == 0 &&
            count("\"single_point_success\"", "hivm.hir.copy") == 0 &&
            count("\"single_point_success\"", "hivm.hir.vadd") == 0,
        "eligible f32, broadcast, and copy operations must scalarize");
  Check(count("\"single_point_integer\"", "hivm.hir.vadd") == 0 &&
            count("\"single_point_integer\"", "hivm.hir.vdiv") == 0,
        "eligible i64 operations must scalarize");
  Check(count("\"single_point_no_match\"", "hivm.hir.vadd") == 2,
        "non-unit and tensor-semantics operations must remain");
  Check(count("\"single_point_type_and_space_no_match\"",
              "hivm.hir.vadd") == 1 &&
            count("\"single_point_type_and_space_no_match\"",
                  "hivm.hir.copy") == 1,
        "unsupported scalar type and missing memory spaces must remain");
  Check(count("\"single_point_load\"", "hivm.hir.load") == 0 &&
            count("\"single_point_load_without_no_alias\"",
                  "hivm.hir.load") == 1 &&
            count("\"single_point_invalid_memory_user\"",
                  "hivm.hir.load") == 1,
        "load scalarization must require no-IO-alias and read-only users");
  Check(count("\"single_point_host\"", "hivm.hir.vadd") == 1,
        "host functions must remain unchanged");

  static const std::set<std::string> expectedScalarOperations = {
      "arith.addf",     "arith.addi",     "arith.divf",
      "arith.divsi",    "arith.maximumf", "arith.maxsi",
      "arith.minimumf", "arith.minsi",    "arith.mulf",
      "arith.muli",     "arith.subf",     "arith.subi",
      "math.absf",      "math.absi",      "math.sqrt"};
  for (const std::string &operation : expectedScalarOperations)
    Check(scalarOperations.count(operation) != 0,
          "expected scalar operation was not materialized");
  Check(scalarLoads != 0 && scalarStores != 0,
        "scalarization must materialize memref loads and stores");

  bool unsignedFailure = false;
  try {
    cvub::GenericModule unsignedModule = cvub::ParseGenericIR(
        "ub_overflow_model_cpp/tests/fixtures/"
        "pre_cv_hivm_opt_single_point_unsigned_failure.mlir",
        false);
    cvub::ApplyOperationSemanticsToAll(unsignedModule.operations);
    (void)cvub::RunPreCVHIVMOptSinglePoint(std::move(unsignedModule));
  } catch (const std::runtime_error &error) {
    unsignedFailure =
        std::string(error.what()).find("unsigned i64 max/min") !=
        std::string::npos;
  }
  Check(unsignedFailure,
        "native-invalid unsigned i64 scalarization must fail open");

  std::cout << "[PASS] pre-CV HIVMOptSinglePoint mirrors native patterns\n";
  return 0;
}
