#include "../src/passes/module_extended_canonicalizer.hpp"

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
      "ub_overflow_model_cpp/tests/fixtures/"
      "module_extended_canonicalizer.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunModuleExtendedCanonicalizer(std::move(module));

  const cvub::GenericOperation *mark = nullptr;
  std::map<int, const cvub::GenericOperation *> definitions;
  for (const cvub::GenericOperation &operation : module.operations) {
    for (int result : operation.results)
      definitions[result] = &operation;
    if (operation.name == "annotation.mark")
      mark = &operation;
  }
  Check(mark != nullptr && mark->operands.size() == 3,
        "fixture result marker must survive");
  Check(definitions.at(mark->operands[0])->name == "affine.apply",
        "identity affine.apply must fold to its composed producer");
  Check(definitions.at(mark->operands[1])->name == "arith.constant",
        "constant affine.apply must materialize an index constant");
  const cvub::GenericOperation &semi = *definitions.at(mark->operands[2]);
  const std::string map =
      cvub::FindDictionaryValue(semi.properties, "map");
  Check(map.find("s2 + s3 * s1") != std::string::npos,
        "semi-affine local terms must use native reconstruction order");

  const std::string first = cvub::SerializeGenericModule(module);
  const std::string second = cvub::SerializeGenericModule(
      cvub::RunModuleExtendedCanonicalizer(std::move(module)));
  Check(first == second, "module ExtendedCanonicalizer must be idempotent");
  std::cout << "[PASS] module ExtendedCanonicalizer affine fixed point parity\n";
  return 0;
}
