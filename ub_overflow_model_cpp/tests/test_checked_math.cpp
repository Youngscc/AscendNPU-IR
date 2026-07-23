#include "../src/ir/operation_folder.hpp"
#include "../src/support/checked_math.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

} // namespace

int main() {
  constexpr int64_t minimum = std::numeric_limits<int64_t>::min();
  constexpr int64_t maximum = std::numeric_limits<int64_t>::max();

  Check(cvub::CheckedAddInt64(maximum, 1) == std::nullopt,
        "signed addition must reject positive overflow");
  Check(cvub::CheckedAddInt64(minimum, -1) == std::nullopt,
        "signed addition must reject negative overflow");
  Check(cvub::CheckedSubInt64(minimum, 1) == std::nullopt,
        "signed subtraction must reject negative overflow");
  Check(cvub::CheckedSubInt64(maximum, -1) == std::nullopt,
        "signed subtraction must reject positive overflow");
  Check(cvub::CheckedMulInt64(minimum, -1) == std::nullopt,
        "signed multiplication must reject INT64_MIN * -1");
  Check(cvub::CheckedMulInt64(maximum, 2) == std::nullopt,
        "signed multiplication must reject positive overflow");
  Check(cvub::CheckedMulInt64(minimum, 1) == minimum,
        "signed multiplication must preserve INT64_MIN");

  Check(cvub::CheckedMulAddInt64(maximum, 2, minimum) == maximum - 1,
        "mul-add must allow a wide product that cancels into int64 range");
  Check(cvub::CheckedMulAddInt64(minimum, -1, minimum) == 0,
        "mul-add must preserve exact INT64_MIN cancellation");
  Check(cvub::CheckedMulAddInt64(minimum, 2, maximum) == std::nullopt,
        "mul-add must reject a result below INT64_MIN");

  Check(cvub::SignedArithInteger({uint64_t{0xff}, 8}) == -1,
        "i8 all-ones must decode as -1");
  Check(cvub::SignedArithInteger({uint64_t{0x80}, 8}) == -128,
        "i8 sign bit must decode as -128");
  Check(cvub::SignedArithInteger({uint64_t{1} << 63, 64}) == minimum,
        "i64 sign bit must decode as INT64_MIN");
  Check(cvub::SignedArithInteger({std::numeric_limits<uint64_t>::max(), 64}) ==
            -1,
        "i64 all-ones must decode as -1");

  std::cout << "[PASS] portable checked integer arithmetic\n";
  return 0;
}
