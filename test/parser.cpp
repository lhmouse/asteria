// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/parser.hpp"
#include "../src/token_stream.hpp"
#include "../src/global_context.hpp"
#include "../src/executive_context.hpp"
#include "../src/reference.hpp"
#include "../src/exception.hpp"
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
      return fib(10) + one;
    )__");
    Token_stream tis;
    ASTERIA_TEST_CHECK(tis.load(iss, String::shallow("my_file")));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis));
    const auto code = pr.extract_document();

    Global_context global;
    Reference result;
    Executive_context ctx;
    code.execute_in_place(result, ctx, global);
    ASTERIA_TEST_CHECK(result.read().check<D_integer>() == 90);
  }
