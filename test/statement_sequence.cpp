// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/compiler/statement_sequence.hpp"
#include "../asteria/compiler/statement.hpp"
#include "../asteria/compiler/expression_unit.hpp"
#include "../asteria/compiler/token_stream.hpp"
#include "../asteria/runtime/executive_context.hpp"
#include "../asteria/runtime/reference.hpp"
#include "../asteria/runtime/variadic_arguer.hpp"
#include "../asteria/llds/avm_rod.hpp"
using namespace ::asteria;

int main()
  {
    ::rocket::tinyfmt_str cbuf;
    cbuf.set_string(
      &R"__(
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

      )__", tinyfmt::open_read);

    Token_Stream tstrm({ });
    tstrm.reload(&"dummy file", 16, move(cbuf));

    Statement_Sequence stmtq({ });
    stmtq.reload(move(tstrm));
    ASTERIA_TEST_CHECK(stmtq.get_statements().size() == 5);  // implicit return
  }
