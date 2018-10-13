// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/simple_source_file.hpp"
#include "../asteria/src/global_context.hpp"
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
      return fib(28) + one;
    )__");
    Simple_source_file code(iss, String::shallow("my_file"));
    Global_context global;
    auto res = code.execute(global, { });
//    ASTERIA_TEST_CHECK(res.read().check<D_integer>() == 90);
  }
