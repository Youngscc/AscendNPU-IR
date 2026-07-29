#include "../src/passes/affine_min_max_canonicalization.hpp"

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
      "ub_overflow_model_cpp/tests/fixtures/arith_to_affine.mlir", false);
  module = cvub::RunArithToAffineConversionPass(std::move(module));

  size_t applyCount = 0;
  size_t maxCount = 0;
  size_t minCount = 0;
  size_t i32AddCount = 0;
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name == "affine.apply")
      ++applyCount;
    else if (operation.name == "affine.max")
      ++maxCount;
    else if (operation.name == "affine.min")
      ++minCount;
    else if (operation.name == "arith.addi" &&
             operation.operandTypes == std::vector<std::string>{"i32", "i32"})
      ++i32AddCount;
    if (operation.name == "affine.apply" || operation.name == "affine.max" ||
        operation.name == "affine.min")
      Check(cvub::startsWith(
                cvub::FindDictionaryValue(operation.properties, "map"),
                "affine_map<"),
            "converted operation must carry a native affine_map");
  }
  Check(applyCount == 6, "six binary index operations must become affine.apply");
  Check(maxCount == 2 && minCount == 2,
        "signed and unsigned index min/max must become affine min/max");
  Check(i32AddCount == 1, "non-index arithmetic must remain legal and unchanged");

  const std::string first = cvub::SerializeGenericModule(module);
  const std::string second = cvub::SerializeGenericModule(
      cvub::RunArithToAffineConversionPass(std::move(module)));
  Check(first == second, "ArithToAffine must be idempotent");
  std::cout << "[PASS] ArithToAffine native index conversion parity\n";
  return 0;
}
