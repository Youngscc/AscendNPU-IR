#include "../src/pipeline/cvpipelining_ub_pipeline.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

} // namespace

int main() {
  const cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  const cvub::ModulePlanResult result =
      cvub::RunUBModuleFromAfterCVPipelining(module, {});

  Check(result.precision == cvub::ModulePlanPrecision::Exact,
        "two independently supported AIV functions must remain exact");
  Check(result.functions.size() == 2,
        "each AIV function must have an independent plan");
  Check(result.peakBits == 98304,
        "module peak must be the maximum function peak");
  Check(result.requiredBits == 98304,
        "module required bits must be the maximum function requirement");
  Check(result.functions[0].function != result.functions[1].function,
        "function plans must retain distinct identities");

  std::cout << "[PASS] module plans AIV functions independently\n";
  return 0;
}
