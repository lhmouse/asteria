// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/parser.hpp"
#include "../asteria/src/compiler/token_stream.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include "../asteria/src/runtime/executive_context.hpp"
#include "../asteria/src/runtime/reference.hpp"
#include "../asteria/src/runtime/variadic_arguer.hpp"
#include "../asteria/src/syntax/source_location.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      func third() {
        const f = func(p) p + "ow";
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
      } catch(e) {
        return typeof(e) + ":" + e;
      }
    )__");
    Token_Stream tis;
    ASTERIA_TEST_CHECK(tis.load(iss, rocket::sref("dummy file"), Parser_Options()));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis, Parser_Options()));
    const auto code = pr.extract_block();

    Reference res;
    Global_Context global;
    Rcobj<Variadic_Arguer> zvarg(Variadic_Arguer(Source_Location(rocket::sref("file"), 42), rocket::sref("scope")));
    code.execute_as_function(res, zvarg, { }, global, { });
    ASTERIA_TEST_CHECK(res.read().check<D_string>() == "string:meow");
  }
