#ifndef CVPIPELINE_UB_MODEL_CPP_TEST_SUPPORT_HPP
#define CVPIPELINE_UB_MODEL_CPP_TEST_SUPPORT_HPP

#include "../src/semantic_ir/generic_ir.hpp"

#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cvub::test {

struct TestCase {
  std::string name;
  std::function<void()> body;
};

inline std::vector<TestCase> &Registry() {
  static std::vector<TestCase> tests;
  return tests;
}

class Registrar {
public:
  Registrar(std::string name, std::function<void()> body) {
    Registry().push_back({std::move(name), std::move(body)});
  }
};

[[noreturn]] inline void Fail(const char *expression, const char *file,
                              int line) {
  throw std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                           ": check failed: " + expression);
}

inline GenericModule ParseFixture(const std::string &name) {
  return ParseGenericIR(fs::path("cvpipeline_ub_model_cpp/tests/fixtures") /
                            name,
                        false);
}

template <typename Callable>
inline void CheckThrowsContains(Callable &&callable, const std::string &needle,
                                const char *expression, const char *file,
                                int line) {
  try {
    std::forward<Callable>(callable)();
  } catch (const std::exception &error) {
    if (std::string(error.what()).find(needle) != std::string::npos)
      return;
    throw std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                             ": " + expression + " threw unexpected error: " +
                             error.what());
  }
  throw std::runtime_error(std::string(file) + ":" + std::to_string(line) +
                           ": " + expression + " did not throw");
}

inline int RunAllTests() {
  int failures = 0;
  for (const TestCase &test : Registry()) {
    try {
      test.body();
      std::cout << "[PASS] " << test.name << '\n';
    } catch (const std::exception &error) {
      ++failures;
      std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
    } catch (...) {
      ++failures;
      std::cerr << "[FAIL] " << test.name << ": unknown exception\n";
    }
  }
  return failures == 0 ? 0 : 1;
}

} // namespace cvub::test

#define CVUB_TEST(name)                                                        \
  static void name();                                                          \
  static const ::cvub::test::Registrar name##_registrar(#name, name);          \
  static void name()

#define CVUB_CHECK(condition)                                                   \
  do {                                                                          \
    if (!(condition))                                                           \
      ::cvub::test::Fail(#condition, __FILE__, __LINE__);                       \
  } while (false)

#define CVUB_CHECK_EQ(actual, expected)                                         \
  do {                                                                          \
    const auto &cvub_actual = (actual);                                         \
    const auto &cvub_expected = (expected);                                     \
    if (!(cvub_actual == cvub_expected))                                        \
      ::cvub::test::Fail(#actual " == " #expected, __FILE__, __LINE__);         \
  } while (false)

#define CVUB_CHECK_THROWS_CONTAINS(expression, needle)                           \
  ::cvub::test::CheckThrowsContains(                                             \
      [&]() { static_cast<void>(expression); }, (needle), #expression, __FILE__, \
      __LINE__)

#endif
