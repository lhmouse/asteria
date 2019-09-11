// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::istringstream iss(
      R"__(
        var one = 1;
        const two = 2;
        func fib(n) {
          return n <= one ? n : fib(n - one) + fib(n - two);
        }
        var con = { };
        con["value"] = fib(11);
        con["const"] = one;
        return con.value + con.const;
      )__");

    Simple_Script code;
    ASTERIA_TEST_CHECK(iss >> code);
    auto res = code.execute();
    ASTERIA_TEST_CHECK(res.read().as_integer() == 90);
  }
