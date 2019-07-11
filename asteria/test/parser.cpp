// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/parser.hpp"
#include "../src/compiler/token_stream.hpp"
#include "../src/runtime/global_context.hpp"
#include "../src/runtime/executive_context.hpp"
#include "../src/runtime/reference.hpp"
#include "../src/runtime/variadic_arguer.hpp"
#include "../src/syntax/source_location.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::stringbuf buf(R"__(
      func third() {
        const f = func(p) = p + "ow";
        throw f("me");
      }
      func second() {
        return third();
      }
      func first() {
        return second();
      }
      try {
        first();
      }
      catch(e) {
        return typeof(e) + ":" + e;
      }
    )__");
    Token_Stream tis;
    ASTERIA_TEST_CHECK(tis.load(buf, rocket::sref("dummy file"), Parser_Options()));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis, Parser_Options()));
    auto code = pr.get_statements();
    ASTERIA_TEST_CHECK(code.size() == 4);
  }
