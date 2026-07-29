#include "../src/passes/scf_for_loop_canonicalization.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

std::string SymbolName(const cvub::GenericOperation &operation) {
  std::string name = cvub::FindDictionaryValue(operation.properties,
                                                "sym_name");
  if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  return name;
}

} // namespace

int main() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/"
      "scf_for_loop_canonicalization.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunSCFForLoopCanonicalization(std::move(module));

  const cvub::GenericModuleAnalysisIndexes indexes(
      module, cvub::kGenericAnalysisEnclosingFunctions);
  std::map<int, const cvub::GenericOperation *> definitions;
  for (const cvub::GenericOperation &operation : module.operations)
    for (int result : operation.results)
      definitions[result] = &operation;

  bool sawMinStore = false;
  bool sawForallStore = false;
  bool sawPartialApply = false;
  size_t foldedDims = 0;
  for (const cvub::GenericOperation &function : module.operations) {
    if (function.name != "func.func")
      continue;
    const std::string name = SymbolName(function);
    if (name == "scf_for_canonicalize_min") {
      for (const cvub::GenericOperation &operation : module.operations) {
        if (operation.name != "memref.store" || operation.parentId < 0 ||
            operation.operands.empty() ||
            indexes.enclosingFunctionId(operation.id) != function.id)
          continue;
        const auto value = definitions.find(operation.operands.front());
        if (value != definitions.end() &&
            value->second->name == "arith.constant" &&
            cvub::FindDictionaryValue(value->second->properties, "value") ==
                "2 : i64")
          sawMinStore = true;
      }
    } else if (name == "scf_for_canonicalize_partly") {
      for (const cvub::GenericOperation &operation : module.operations)
        if (operation.name == "affine.apply" &&
            indexes.enclosingFunctionId(operation.id) == function.id &&
            cvub::FindDictionaryValue(operation.properties, "map").find(
                "affine_map<(d0) -> (-d0 + 256)>") != std::string::npos)
          sawPartialApply = true;
    } else if (name == "tensor_dim_of_iter_arg_and_result") {
      const cvub::GenericRegion &region = module.regions.at(
          static_cast<size_t>(function.regions.front()));
      const cvub::GenericBlock &entry = module.blocks.at(
          static_cast<size_t>(region.blocks.front()));
      const int initialTensor = entry.arguments.front();
      for (const cvub::GenericOperation &operation : module.operations)
        if (operation.name == "tensor.dim" && !operation.operands.empty() &&
            operation.operands.front() == initialTensor)
          ++foldedDims;
    } else if (name == "scf_forall_canonicalize_min") {
      for (const cvub::GenericOperation &operation : module.operations) {
        if (operation.name != "memref.store" || operation.operands.empty())
          continue;
        if (indexes.enclosingFunctionId(operation.id) != function.id)
          continue;
        const auto value = definitions.find(operation.operands.front());
        if (value != definitions.end() &&
            value->second->name == "arith.constant" &&
            cvub::FindDictionaryValue(value->second->properties, "value") ==
                "64 : i64")
          sawForallStore = true;
      }
    }
  }
  Check(sawMinStore,
        "loop constraints must fold the provable affine.min to i64 two");
  Check(sawPartialApply,
        "loop constraints must retain the selected non-constant expression");
  Check(foldedDims == 2,
        "iter-arg and loop-result dim must both read the initial tensor");
  Check(sawForallStore,
        "forall mixed static/dynamic bounds must constrain affine.min");

  std::cout << "[PASS] SCF for-loop cross-dialect canonicalization parity\n";
  return 0;
}
