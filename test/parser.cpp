// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/parser.hpp"
#include "../asteria/src/token_stream.hpp"
#include "../asteria/src/global_context.hpp"
#include "../asteria/src/executive_context.hpp"
#include "../asteria/src/reference.hpp"
#include "../asteria/src/exception.hpp"
#include "../asteria/src/function_header.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      func third() {
        throw "meow";
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
        return e;
      }
    )__");
    Token_stream tis;
    ASTERIA_TEST_CHECK(tis.load(iss, rocket::cow_string::shallow("dummy file")));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis));
    const auto code = pr.extract_document();

    Reference res;
    Global_context global;
    code.execute_as_function(res, global, Function_header(rocket::cow_string::shallow("file again"), 42, rocket::cow_string::shallow("<top level>"), { }), nullptr, { }, { });
    ASTERIA_TEST_CHECK(res.read().check<D_string>() == "meow");
  }
