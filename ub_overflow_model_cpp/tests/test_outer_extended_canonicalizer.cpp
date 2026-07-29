#include "../src/passes/outer_extended_canonicalizer.hpp"

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
      "ub_overflow_model_cpp/tests/fixtures/outer_extended_canonicalizer.mlir",
      false);
  module = cvub::RunOuterExtendedCanonicalizer(std::move(module));

  const cvub::GenericOperation *mark = nullptr;
  const cvub::GenericOperation *brcMark = nullptr;
  const cvub::GenericOperation *reduce = nullptr;
  const cvub::GenericBlock *entry = nullptr;
  std::map<int, const cvub::GenericOperation *> definitions;
  for (const cvub::GenericOperation &operation : module.operations) {
    Check(operation.name != "arith.muli",
          "muli(x, 1) must fold through the native Arith fold");
    Check(operation.name != "arith.extsi" &&
              operation.name != "arith.trunci",
          "trunci(extsi(x)) must fold to x");
    Check(operation.name != "hivm.hir.vbrc",
          "redundant vbrc and vreduce fill chain must be erased");
    for (int result : operation.results)
      definitions[result] = &operation;
    if (operation.name == "annotation.mark") {
      const std::string caseName =
          cvub::FindDictionaryValue(operation.attributes, "case");
      if (caseName == "\"folded\"")
        mark = &operation;
      else if (caseName == "\"redundant_vbrc\"")
        brcMark = &operation;
    }
    if (operation.name == "hivm.hir.vreduce")
      reduce = &operation;
    if (operation.name == "func.func" && !operation.regions.empty()) {
      const cvub::GenericRegion &region =
          module.regions.at(static_cast<size_t>(operation.regions.front()));
      entry = &module.blocks.at(static_cast<size_t>(region.blocks.front()));
    }
  }
  Check(mark != nullptr && entry != nullptr && mark->operands.size() == 1,
        "fixture mark and function entry must survive");
  Check(!entry->arguments.empty() && mark->operands.front() == entry->arguments.front(),
        "folded use must refer directly to the original function argument");
  Check(brcMark != nullptr && brcMark->operands.size() == 1,
        "redundant vbrc user must survive");
  const auto brcSource = definitions.find(brcMark->operands.front());
  Check(brcSource != definitions.end() &&
            brcSource->second->name == "tensor.empty",
        "redundant tensor vbrc must be replaced by its source");
  Check(reduce != nullptr && reduce->operands.size() >= 2,
        "vreduce fixture must survive");
  const auto reduceInit = definitions.find(reduce->operands[1]);
  Check(reduceInit != definitions.end() &&
            reduceInit->second->name == "tensor.empty",
        "neutral vreduce init must be replaced by tensor.empty");

  const std::string first = cvub::SerializeGenericModule(module);
  const std::string second = cvub::SerializeGenericModule(
      cvub::RunOuterExtendedCanonicalizer(std::move(module)));
  Check(first == second, "outer canonicalizer must be idempotent");
  std::cout << "[PASS] outer ExtendedCanonicalizer Arith and greedy parity\n";
  return 0;
}
