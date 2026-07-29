#include "../src/passes/pre_cv_inline_otf_broadcast.hpp"
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

cvub::GenericModule Run(const std::string &path) {
  cvub::GenericModule module = cvub::ParseGenericIR(path, false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  return cvub::RunPreCVInlineOTFBroadcast(std::move(module));
}

} // namespace

int main() {
  cvub::GenericModule module = Run(
      "ub_overflow_model_cpp/tests/fixtures/pre_cv_inline_otf_broadcast.mlir");
  const cvub::GenericModuleAnalysisIndexes indexes(
      module, cvub::kGenericAnalysisEnclosingFunctions);
  std::map<std::string, size_t> broadcastCounts;
  std::map<std::string, const cvub::GenericOperation *> users;
  for (const cvub::GenericOperation &operation : module.operations) {
    const std::string function = FunctionName(module, indexes, operation.id);
    if (operation.name == "hivm.hir.vbrc")
      ++broadcastCounts[function];
    if (operation.name == "hivm.hir.vadd" ||
        operation.name == "hivm.hir.vexp" ||
        operation.name == "hivm.hir.vrelu" ||
        operation.name == "hivm.hir.vshl")
      users[function] = &operation;
  }

  for (const std::string function : {
           "\"last_binary\"", "\"last_unary\"", "\"last_relu\"",
           "\"middle_binary\""})
    Check(broadcastCounts[function] == 0,
          "eligible last/non-last users must inline tensor VBrc");
  for (const std::string function : {
           "\"last_not_whitelisted\"", "\"last_visinf_not_whitelisted\"",
           "\"middle_unary\"",
           "\"last_vabs_i16\"", "\"i64_no_inline\"",
           "\"multi_axis_no_inline\"", "\"partial_inline\"",
           "\"buffer_no_inline\"", "\"shift_non950\""})
    Check(broadcastCounts[function] == 1,
          "native InlineOTFBroadcast rejection branch must retain VBrc");

  Check(cvub::PreCVInlineBroadcastArray(*users["\"last_binary\""],
                                        "broadcast") ==
            std::vector<int64_t>({0, 1}),
        "last-axis inline must merge and sort existing broadcast axes");
  Check(cvub::PreCVInlineBroadcastArray(*users["\"middle_binary\""],
                                        "broadcast") ==
            std::vector<int64_t>({1, 2}),
        "non-last binary inline must merge broadcast axes");
  const cvub::GenericOperation &partial = *users["\"partial_inline\""];
  Check(partial.operands.size() >= 2 &&
            partial.operands[0] == partial.operands[1] &&
            partial.operandTypes[0] == "tensor<4x1xf32>" &&
            partial.operandTypes[1] == "tensor<4x1xf32>",
        "all matching DPS input uses must be replaced while invalid users remain");

  cvub::GenericModule ascend950 = Run(
      "ub_overflow_model_cpp/tests/fixtures/"
      "pre_cv_inline_otf_broadcast_950.mlir");
  size_t shiftBroadcasts = 0;
  const cvub::GenericOperation *shift = nullptr;
  for (const cvub::GenericOperation &operation : ascend950.operations) {
    shiftBroadcasts += operation.name == "hivm.hir.vbrc" ? 1 : 0;
    if (operation.name == "hivm.hir.vshl")
      shift = &operation;
  }
  Check(shiftBroadcasts == 0 && shift &&
            shift->operandTypes.front() == "tensor<4x1xi32>",
        "Ascend950 must enable last-axis VShL/VShR inline");

  std::cout << "[PASS] pre-CV InlineOTFBroadcast mirrors native user rules\n";
  return 0;
}
