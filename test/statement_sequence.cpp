// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/compiler/statement_sequence.hpp"
#include "../asteria/compiler/statement.hpp"
#include "../asteria/compiler/expression_unit.hpp"
#include "../asteria/compiler/token_stream.hpp"
#include "../asteria/runtime/executive_context.hpp"
#include "../asteria/runtime/reference.hpp"
#include "../asteria/runtime/variadic_arguer.hpp"
#include "../asteria/llds/avmc_queue.hpp"
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
    tstrm.reload(sref("dummy file"), 16, ::std::move(cbuf));

    Statement_Sequence stmtq({ });
    stmtq.reload(::std::move(tstrm));
    ASTERIA_TEST_CHECK(cow_vector<Statement>(stmtq).size() == 4);
  }
