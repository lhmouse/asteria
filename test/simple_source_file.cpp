// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/simple_source_file.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      var one = 1;
      const two = 2;
      func fib(n) {
        return n <= one ? one : fib(n - one) + fib(n - two);
      }
      var con = { };
      con["value"] = fib(10);
      con["const"] = one;
      return con.value + con.const;
    )__");
    Simple_source_file code(iss, std::ref("my_file"));
    Global_context global;
    auto res = code.execute(global, { });
    ASTERIA_TEST_CHECK(res.read().check<D_integer>() == 90);
  }
