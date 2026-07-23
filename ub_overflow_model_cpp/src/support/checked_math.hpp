#ifndef CVPIPELINE_UB_MODEL_CPP_CHECKED_MATH_HPP
#define CVPIPELINE_UB_MODEL_CPP_CHECKED_MATH_HPP

#include <cstdint>
#include <limits>
#include <optional>
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

inline uint64_t UnsignedMagnitude(int64_t value) {
  return value >= 0
             ? static_cast<uint64_t>(value)
             : static_cast<uint64_t>(-(value + 1)) + uint64_t{1};
}

inline std::optional<int64_t> SignedFromMagnitude(bool negative,
                                                  uint64_t magnitude) {
  constexpr uint64_t minMagnitude = uint64_t{1} << 63;
  if (!negative) {
    if (magnitude > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
      return std::nullopt;
    return static_cast<int64_t>(magnitude);
  }
  if (magnitude > minMagnitude)
    return std::nullopt;
  if (magnitude == minMagnitude)
    return std::numeric_limits<int64_t>::min();
  return -static_cast<int64_t>(magnitude);
}

inline std::optional<int64_t> CheckedAddInt64(int64_t lhs, int64_t rhs) {
  if ((rhs > 0 && lhs > std::numeric_limits<int64_t>::max() - rhs) ||
      (rhs < 0 && lhs < std::numeric_limits<int64_t>::min() - rhs))
    return std::nullopt;
  return lhs + rhs;
}

inline std::optional<int64_t> CheckedSubInt64(int64_t lhs, int64_t rhs) {
  if ((rhs > 0 && lhs < std::numeric_limits<int64_t>::min() + rhs) ||
      (rhs < 0 && lhs > std::numeric_limits<int64_t>::max() + rhs))
    return std::nullopt;
  return lhs - rhs;
}

inline std::optional<int64_t> CheckedMulInt64(int64_t lhs, int64_t rhs) {
  const uint64_t lhsMagnitude = UnsignedMagnitude(lhs);
  const uint64_t rhsMagnitude = UnsignedMagnitude(rhs);
  if (lhsMagnitude != 0 &&
      rhsMagnitude > std::numeric_limits<uint64_t>::max() / lhsMagnitude)
    return std::nullopt;
  const uint64_t magnitude = lhsMagnitude * rhsMagnitude;
  const bool negative = magnitude != 0 && ((lhs < 0) != (rhs < 0));
  return SignedFromMagnitude(negative, magnitude);
}

inline std::optional<int64_t> CheckedMulAddInt64(int64_t lhs, int64_t rhs,
                                                int64_t addend) {
  const uint64_t lhsMagnitude = UnsignedMagnitude(lhs);
  const uint64_t rhsMagnitude = UnsignedMagnitude(rhs);
  if (lhsMagnitude != 0 &&
      rhsMagnitude > std::numeric_limits<uint64_t>::max() / lhsMagnitude)
    return std::nullopt;

  const uint64_t productMagnitude = lhsMagnitude * rhsMagnitude;
  const bool productNegative =
      productMagnitude != 0 && ((lhs < 0) != (rhs < 0));
  const uint64_t addendMagnitude = UnsignedMagnitude(addend);
  const bool addendNegative = addend < 0;

  uint64_t resultMagnitude = 0;
  bool resultNegative = false;
  if (productNegative == addendNegative) {
    if (addendMagnitude >
        std::numeric_limits<uint64_t>::max() - productMagnitude)
      return std::nullopt;
    resultMagnitude = productMagnitude + addendMagnitude;
    resultNegative = productNegative;
  } else if (productMagnitude >= addendMagnitude) {
    resultMagnitude = productMagnitude - addendMagnitude;
    resultNegative = productNegative;
  } else {
    resultMagnitude = addendMagnitude - productMagnitude;
    resultNegative = addendNegative;
  }
  return SignedFromMagnitude(resultNegative, resultMagnitude);
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
