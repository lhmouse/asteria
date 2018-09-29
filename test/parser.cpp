// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/parser.hpp"
#include "../src/token_stream.hpp"
#include "../src/executive_context.hpp"
#include "../src/reference.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      var a = 0;
      for(var i = 1; i <= 5; ++i) {
        a += i + a * i;
      }
      // assert(a == 719);
      return 42;
    )__");
    Token_stream tis;
    ASTERIA_TEST_CHECK(tis.load(iss, String::shallow("dummy file")));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis));
    const auto code = pr.extract_document();

    Executive_context ctx(nullptr);
    auto res = code.execute_as_function_in_place(ctx);
    ASTERIA_TEST_CHECK(res.read().check<D_integer>() == 42);
    auto qr = ctx.get_named_reference_opt(String::shallow("a"));
    ASTERIA_TEST_CHECK(qr);
    ASTERIA_TEST_CHECK(qr->read().check<D_integer>() == 719);
  }
