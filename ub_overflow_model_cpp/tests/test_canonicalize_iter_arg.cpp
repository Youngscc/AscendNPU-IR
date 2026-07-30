#include "../src/passes/canonicalization_hivm_pipeline.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

} // namespace

int main() {
  {
    cvub::GenericModule constants = cvub::ParseGenericIR(
        "ub_overflow_model_cpp/tests/fixtures/"
        "canonicalization_constant_hoist.mlir",
        false);
    cvub::ApplyOperationSemanticsToAll(constants.operations);
    constants =
        cvub::HoistCanonicalizationConstants(std::move(constants));
    const cvub::GenericOperation *function = nullptr;
    size_t constantCount = 0;
    for (const cvub::GenericOperation &operation : constants.operations) {
      if (operation.name == "func.func")
        function = &operation;
      if (operation.name == "arith.constant")
        ++constantCount;
    }
    Check(function != nullptr && !function->regions.empty(),
          "constant-hoist fixture must retain its function");
    const cvub::GenericRegion &functionRegion = constants.regions.at(
        static_cast<size_t>(function->regions.front()));
    const int entryBlock = functionRegion.blocks.front();
    Check(constantCount == 3,
          "nested duplicate constants must be uniqued by value and type");
    for (const cvub::GenericOperation &operation : constants.operations)
      if (operation.name == "arith.constant")
        Check(operation.blockId == entryBlock,
              "greedy canonicalization constants must hoist to the isolated "
              "function entry");
    const cvub::GenericBlock &entry =
        constants.blocks.at(static_cast<size_t>(entryBlock));
    Check(entry.operations.size() >= 3,
          "constant-hoist fixture must retain the entry block");
    for (size_t index = 0; index < 3; ++index)
      Check(constants.operations.at(
                static_cast<size_t>(entry.operations[index])).name ==
                "arith.constant",
            "folder-owned constants must remain a contiguous entry prefix");
  }

  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/canonicalize_iter_arg.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module =
      cvub::RunCanonicalizationHIVMAfterArithToAffine(std::move(module));

  const cvub::GenericOperation *whileOp = nullptr;
  const cvub::GenericOperation *vectorWhileOp = nullptr;
  std::map<std::string, int> markedValues;
  std::map<int, const cvub::GenericOperation *> definitions;
  for (const cvub::GenericOperation &operation : module.operations) {
    Check(operation.name != "scf.for",
          "dead for channels and their side-effect-free loop must be erased");
    for (int result : operation.results)
      definitions[result] = &operation;
    if (operation.name == "scf.while") {
      if (operation.results.size() == 1)
        whileOp = &operation;
      else if (operation.results.size() == 2)
        vectorWhileOp = &operation;
    }
    if (operation.name == "annotation.mark" &&
        operation.operands.size() == 1) {
      std::string label =
          cvub::FindDictionaryValue(operation.attributes, "case");
      if (label.size() >= 2 && label.front() == '"' && label.back() == '"')
        label = label.substr(1, label.size() - 2);
      markedValues[label] = operation.operands.front();
    }
  }

  Check(whileOp != nullptr && whileOp->results.size() == 1 &&
            whileOp->operands.size() == 1 && whileOp->regions.size() == 2,
        "backward while rewrite must retain only the live channel");
  Check(vectorWhileOp != nullptr && vectorWhileOp->results.size() == 2,
        "vector function must keep backward-pattern pruning disabled");
  Check(markedValues.count("for_unchanged") == 1 &&
            definitions.at(markedValues.at("for_unchanged"))->name ==
                "arith.constant",
        "unchanged for result must be replaced by its init");
  Check(markedValues.count("for_external_yield") == 1 &&
            definitions.at(markedValues.at("for_external_yield"))->name ==
                "tensor.empty",
        "external tensor yield must replace the tied for result");
  Check(markedValues.count("for_nested_unchanged") == 1 &&
            definitions.at(markedValues.at("for_nested_unchanged"))->name ==
                "arith.constant",
        "nested SCF equivalence must replace an unchanged for result");
  Check(markedValues.count("while_unchanged") == 1 &&
            markedValues.at("while_unchanged") == whileOp->results.front(),
        "live while result must remain tied to the retained channel");

  for (int regionId : whileOp->regions) {
    const cvub::GenericRegion &region =
        module.regions.at(static_cast<size_t>(regionId));
    Check(region.blocks.size() == 1,
          "canonical while regions must each contain one block");
    const cvub::GenericBlock &block =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    Check(block.arguments.size() == 1,
          "removed while channel must disappear from both region blocks");
  }

  const std::string first = cvub::SerializeGenericModule(module);
  const std::string second = cvub::SerializeGenericModule(
      cvub::RunCanonicalizationHIVMAfterArithToAffine(std::move(module)));
  Check(first == second, "CanonicalizeIterArg stage must be idempotent");
  std::cout << "[PASS] CanonicalizeIterArg for/while/backward parity\n";
  return 0;
}
