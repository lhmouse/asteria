// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/compiler/statement_sequence.hpp"
#include "../src/compiler/statement.hpp"
#include "../src/compiler/expression_unit.hpp"
#include "../src/compiler/token_stream.hpp"
#include "../src/runtime/executive_context.hpp"
#include "../src/runtime/reference.hpp"
#include "../src/runtime/variadic_arguer.hpp"
#include "../src/llds/avmc_queue.hpp"

using namespace ::asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(sref(
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
      )__"), tinybuf::open_read);
    Token_Stream tstrm({ });
    tstrm.reload(sref("dummy file"), 16, cbuf);
    Statement_Sequence stmtq({ });
    stmtq.reload(tstrm);
    ASTERIA_TEST_CHECK(cow_vector<Statement>(stmtq).size() == 4);
  }
