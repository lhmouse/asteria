// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/parser.hpp"
#include "../src/token_stream.hpp"
#include "../src/executive_context.hpp"
#include "../src/reference.hpp"
#include "../src/exception.hpp"
#include "../src/backtracer.hpp"
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
      first();
    )__");
    Token_stream tis;
    ASTERIA_TEST_CHECK(tis.load(iss, String::shallow("dummy file")));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis));
    const auto code = pr.extract_document();

    Vector<Backtracer> btv;
    Executive_context ctx;
    try {
      try {
        auto res = code.execute_as_function_in_place(ctx, nullptr);
        ASTERIA_TEST_CHECK(res.read().check<D_integer>() == -2);
      } catch(...) {
        Backtracer::unpack_and_rethrow(btv, std::current_exception());
      }
    } catch(Exception &e) {
      ASTERIA_DEBUG_LOG("caught: ", e.get_value());
    }
  }
