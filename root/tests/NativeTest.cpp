#include "NativeTest.h"

#include <exception>
#include <iostream>

namespace wac_test {
std::vector<TestCase>& tests() {
    static std::vector<TestCase> registered_tests;
    return registered_tests;
}

Registrar::Registrar(char const* name, TestFunction function) {
    tests().push_back({name, function});
}
}

int main() {
    int failures = 0;
    for (auto const& test : wac_test::tests()) {
        try {
            test.function();
            std::cout << "PASS " << test.name << '\n';
        } catch (std::exception const& error) {
            ++failures;
            std::cout << "FAIL " << test.name << ": " << error.what() << '\n';
        } catch (...) {
            ++failures;
            std::cout << "FAIL " << test.name << ": unknown exception\n";
        }
    }
    return failures == 0 ? 0 : 1;
}
