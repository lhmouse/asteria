// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/statement_sequence.hpp"
#include "../src/compiler/token_stream.hpp"
#include "../src/runtime/executive_context.hpp"
#include "../src/runtime/reference.hpp"
#include "../src/runtime/variadic_arguer.hpp"

using namespace Asteria;

int main()
  {
    rocket::cow_stringbuf buf;
    buf.set_string(rocket::sref(
      R"__(
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
      )__"), std::ios_base::in);
    Token_Stream tstrm(buf, rocket::sref("dummy file"), { });
    Statement_Sequence stmseq(tstrm, { });
    ASTERIA_TEST_CHECK(stmseq.size() == 4);
  }
