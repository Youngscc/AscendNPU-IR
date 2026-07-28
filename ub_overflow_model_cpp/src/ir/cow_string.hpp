#ifndef UB_OVERFLOW_MODEL_CPP_COW_STRING_HPP
#define UB_OVERFLOW_MODEL_CPP_COW_STRING_HPP

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace cvub {

// Exact string value with lazy materialization and copy-on-write ownership.
// MLIR-backed base nodes may defer printing uniqued Attributes; projection
// clones share the printed value until a modeled pass actually overrides it.
class CowString {
public:
  using size_type = std::string::size_type;
  static constexpr size_type npos = std::string::npos;

  CowString() : state_(std::make_shared<State>()) {}
  CowString(const char *value) : CowString(std::string(value)) {}
  CowString(std::string value) : state_(std::make_shared<State>()) {
    state_->value = std::move(value);
    state_->materialized = true;
  }

  static CowString Deferred(std::function<std::string()> materializer) {
    CowString result;
    result.state_->materializer = std::move(materializer);
    return result;
  }

  CowString &operator=(const char *value) {
    return *this = std::string(value);
  }
  CowString &operator=(std::string value) {
    state_ = std::make_shared<State>();
    state_->value = std::move(value);
    state_->materialized = true;
    return *this;
  }

  const std::string &get() const {
    materialize();
    return state_->value;
  }
  std::string &mutate() {
    materialize();
    if (state_.use_count() != 1) {
      auto detached = std::make_shared<State>();
      detached->value = state_->value;
      detached->materialized = true;
      state_ = std::move(detached);
    }
    return state_->value;
  }

  operator const std::string &() const { return get(); }
  operator std::string &() { return mutate(); }
  operator std::string_view() const { return get(); }

  bool empty() const { return get().empty(); }
  size_type size() const { return get().size(); }
  char front() const { return get().front(); }
  char back() const { return get().back(); }
  const char *c_str() const { return get().c_str(); }
  size_type find(const std::string &value, size_type position = 0) const {
    return get().find(value, position);
  }
  size_type find(const char *value, size_type position = 0) const {
    return get().find(value, position);
  }
  size_type find(char value, size_type position = 0) const {
    return get().find(value, position);
  }
  size_type find(std::string_view value, size_type position = 0) const {
    return std::string_view(get()).find(value, position);
  }
  size_type rfind(const std::string &value, size_type position = npos) const {
    return get().rfind(value, position);
  }
  size_type rfind(char value, size_type position = npos) const {
    return get().rfind(value, position);
  }
  std::string substr(size_type position = 0, size_type count = npos) const {
    return get().substr(position, count);
  }
  void clear() { mutate().clear(); }
  void resize(size_type size) { mutate().resize(size); }
  CowString &replace(size_type position, size_type count,
                     const std::string &replacement) {
    mutate().replace(position, count, replacement);
    return *this;
  }
  CowString &insert(size_type position, const std::string &value) {
    mutate().insert(position, value);
    return *this;
  }
  CowString &operator+=(const std::string &suffix) {
    mutate() += suffix;
    return *this;
  }

  auto begin() { return mutate().begin(); }
  auto end() { return mutate().end(); }
  auto begin() const { return get().begin(); }
  auto end() const { return get().end(); }

  friend bool operator==(const CowString &lhs, const CowString &rhs) {
    return lhs.get() == rhs.get();
  }
  friend bool operator!=(const CowString &lhs, const CowString &rhs) {
    return !(lhs == rhs);
  }
  friend bool operator==(const CowString &lhs, const std::string &rhs) {
    return lhs.get() == rhs;
  }
  friend bool operator==(const std::string &lhs, const CowString &rhs) {
    return lhs == rhs.get();
  }
  friend bool operator!=(const CowString &lhs, const std::string &rhs) {
    return !(lhs == rhs);
  }
  friend bool operator==(const CowString &lhs, const char *rhs) {
    return lhs.get() == rhs;
  }
  friend bool operator==(const char *lhs, const CowString &rhs) {
    return rhs == lhs;
  }
  friend bool operator!=(const CowString &lhs, const char *rhs) {
    return !(lhs == rhs);
  }
  friend bool operator!=(const char *lhs, const CowString &rhs) {
    return !(rhs == lhs);
  }
  friend std::string operator+(const CowString &lhs, const CowString &rhs) {
    return lhs.get() + rhs.get();
  }
  friend std::string operator+(const CowString &lhs, const std::string &rhs) {
    return lhs.get() + rhs;
  }
  friend std::string operator+(const std::string &lhs, const CowString &rhs) {
    return lhs + rhs.get();
  }
  friend std::string operator+(const CowString &lhs, const char *rhs) {
    return lhs.get() + rhs;
  }
  friend std::string operator+(const char *lhs, const CowString &rhs) {
    return lhs + rhs.get();
  }
  friend std::ostream &operator<<(std::ostream &output,
                                  const CowString &value) {
    return output << value.get();
  }

private:
  struct State {
    mutable std::string value;
    mutable std::function<std::string()> materializer;
    mutable bool materialized = false;
  };

  void materialize() const {
    if (state_->materialized)
      return;
    state_->value = state_->materializer ? state_->materializer()
                                         : std::string();
    state_->materializer = {};
    state_->materialized = true;
  }

  std::shared_ptr<State> state_;
};

} // namespace cvub

#endif
