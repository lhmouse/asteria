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
    ASTERIA_TEST_CHECK(tis.load(iss, String::shallow("dummy file")));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis));
    const auto code = pr.extract_document();

    Global_context global;
    auto res = code.execute_as_function(global, String::shallow("file again"), 42, String::shallow("<top level>"), { }, { }, { });
    ASTERIA_TEST_CHECK(res.read().check<D_string>() == "meow");
  }
