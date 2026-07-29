#include "../src/passes/pre_cv_cse.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

} // namespace

int main() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/pre_cv_cse.mlir", false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunPreCVCSE(std::move(module));

  size_t addCount = 0;
  size_t loadCount = 0;
  size_t ifCount = 0;
  size_t loopLocalAddCount = 0;
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name == "arith.addi") {
      ++addCount;
      if (!operation.operands.empty() &&
          operation.operands.front() == operation.operands.back()) {
        if (!operation.resultTypes.empty() &&
            operation.resultTypes.front() == "index")
          ++loopLocalAddCount;
      }
    } else if (operation.name == "memref.load") {
      ++loadCount;
    } else if (operation.name == "scf.if") {
      ++ifCount;
    }
  }
  Check(addCount == 4,
        "commutative duplicates and nested dominating duplicate must CSE");
  Check(loopLocalAddCount == 2,
        "available expressions must not escape sibling region scopes");
  Check(loadCount == 2,
        "read-only duplicates must merge only when no write intervenes");
  Check(ifCount == 1,
        "equivalent pure single-block region operations must CSE");
  const cvub::GenericModuleAnalysisIndexes uses(
      module, cvub::kGenericAnalysisUsers);
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "arith.addi")
      Check(!operation.results.empty() && !uses.users(operation.results.front()).empty(),
            "unused side-effect-free operation must be DCE'd");

  std::cout << "[PASS] pre-CV CSE mirrors scoped/effect-aware native CSE\n";
  return 0;
}
