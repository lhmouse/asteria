// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/parser.hpp"
#include "../asteria/src/compiler/token_stream.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include "../asteria/src/runtime/executive_context.hpp"
#include "../asteria/src/runtime/reference.hpp"
#include "../asteria/src/runtime/exception.hpp"
#include "../asteria/src/runtime/function_header.hpp"
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
    Function_header head(rocket::cow_string::shallow("file again"), 42, rocket::cow_string::shallow("<top level>"));
    code.execute_as_function(res, global, std::move(head), nullptr, { }, { });
    ASTERIA_TEST_CHECK(res.read().check<D_string>() == "meow");
  }
