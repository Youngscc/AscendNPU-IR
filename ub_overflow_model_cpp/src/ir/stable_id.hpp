#ifndef UB_OVERFLOW_MODEL_CPP_STABLE_ID_HPP
#define UB_OVERFLOW_MODEL_CPP_STABLE_ID_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace cvub {

template <typename Tag> class StableId {
public:
  using Storage = uint32_t;
  static constexpr Storage kInvalid = std::numeric_limits<Storage>::max();

  constexpr StableId() = default;
  explicit constexpr StableId(Storage value) : value_(value) {}

  static StableId fromIndex(size_t index) {
    if (index >= static_cast<size_t>(kInvalid))
      throw std::overflow_error("stable ID space exhausted");
    return StableId(static_cast<Storage>(index));
  }

  constexpr bool valid() const { return value_ != kInvalid; }
  explicit constexpr operator bool() const { return valid(); }
  constexpr Storage raw() const { return value_; }
  constexpr size_t index() const { return static_cast<size_t>(value_); }

  friend constexpr bool operator==(StableId lhs, StableId rhs) {
    return lhs.value_ == rhs.value_;
  }
  friend constexpr bool operator!=(StableId lhs, StableId rhs) {
    return !(lhs == rhs);
  }
  friend constexpr bool operator<(StableId lhs, StableId rhs) {
    return lhs.value_ < rhs.value_;
  }

private:
  Storage value_ = kInvalid;
};

struct OpIdTag;
struct ValueIdTag;
struct BlockIdTag;
struct RegionIdTag;
struct BufferIdTag;

using OpId = StableId<OpIdTag>;
using ValueId = StableId<ValueIdTag>;
using BlockId = StableId<BlockIdTag>;
using RegionId = StableId<RegionIdTag>;
using BufferId = StableId<BufferIdTag>;

} // namespace cvub

#endif
