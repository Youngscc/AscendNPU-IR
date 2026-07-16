#ifndef CVPIPELINE_UB_MODEL_CPP_CHECKED_MATH_HPP
#define CVPIPELINE_UB_MODEL_CPP_CHECKED_MATH_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace cvub {

inline uint64_t CheckedAdd(uint64_t lhs, uint64_t rhs,
                           const std::string &context) {
  if (rhs > std::numeric_limits<uint64_t>::max() - lhs)
    throw std::runtime_error("PlanMemory exact blocker: uint64 overflow in " +
                             context);
  return lhs + rhs;
}

inline uint64_t CheckedMul(uint64_t lhs, uint64_t rhs,
                           const std::string &context) {
  if (lhs != 0 && rhs > std::numeric_limits<uint64_t>::max() / lhs)
    throw std::runtime_error("PlanMemory exact blocker: uint64 overflow in " +
                             context);
  return lhs * rhs;
}

inline uint64_t AlignUp(uint64_t value, uint64_t alignment) {
  if (alignment == 0)
    return value;
  const uint64_t remainder = value % alignment;
  return remainder == 0
             ? value
             : CheckedAdd(value, alignment - remainder, "AlignUp");
}

inline uint64_t BitsToBytes(uint64_t bits) {
  return bits / 8 + (bits % 8 == 0 ? 0 : 1);
}

} // namespace cvub

#endif
