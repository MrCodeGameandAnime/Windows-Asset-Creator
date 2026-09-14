#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace wac_test {
using TestFunction = void (*)();

struct TestCase {
    std::string name;
    TestFunction function;
};

class Registrar {
public:
    Registrar(char const* name, TestFunction function);
};

class AssertionFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

std::vector<TestCase>& tests();

template <typename Actual, typename Expected>
void RequireEqual(Actual const& actual, Expected const& expected, char const* actual_text,
                  char const* expected_text, char const* file, int line) {
    if (!(actual == expected)) {
        throw AssertionFailure(std::string(file) + ":" + std::to_string(line) + ": expected " +
                               actual_text + " == " + expected_text);
    }
}
}

#define TEST_CASE(name) void name(); static wac_test::Registrar reg_##name{#name, name}; void name()
#define REQUIRE_EQ(actual, expected) wac_test::RequireEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__)
int main();
