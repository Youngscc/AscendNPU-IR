#include "../src/passes/func_extended_canonicalizer.hpp"

#include <iostream>
#include <set>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

} // namespace

int main() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/"
      "func_extended_canonicalizer.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunFirstFuncExtendedCanonicalizer(std::move(module));

  const cvub::GenericModuleAnalysisIndexes indexes(
      module, cvub::kGenericAnalysisEnclosingFunctions);
  std::set<int> constantFunctions;
  size_t identityApplyCount = 0;
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name == "arith.constant" &&
        cvub::FindDictionaryValue(operation.properties, "value") ==
            "64 : index")
      constantFunctions.insert(indexes.enclosingFunctionId(operation.id));
    if (operation.name == "affine.apply" &&
        cvub::FindDictionaryValue(operation.properties, "map") ==
            "affine_map<()[s0] -> (s0)>")
      ++identityApplyCount;
  }
  Check(constantFunctions.size() == 2,
        "operation-folder constants must remain isolated per function");
  Check(identityApplyCount == 0,
        "func-scoped greedy worklists must fold each identity apply");

  std::cout << "[PASS] func ExtendedCanonicalizer preserves function scope\n";
  return 0;
}
