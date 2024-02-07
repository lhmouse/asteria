// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/compiler/token_stream.hpp"
#include "../asteria/compiler/statement_sequence.hpp"
#include "../asteria/compiler/statement.hpp"
#include "../asteria/runtime/air_node.hpp"
#include "../asteria/runtime/global_context.hpp"
#include "../asteria/runtime/analytic_context.hpp"
#include "../asteria/runtime/enums.hpp"
using namespace ::asteria;

int main()
  {
    Compiler_Options opts = { };
    Token_Stream tstrm(opts);
    ::rocket::tinybuf_str cbuf(&R"__(
///////////////////////////////////////////////////////////////////////////////

        var a = 1.5;

        {
          a = 5.5;
          for(;;)
            break;
        }

        {
          var b = a;
          a = 4.5;
        }

        {
          defer a = 6.5;
        }

///////////////////////////////////////////////////////////////////////////////
      )__", tinybuf::open_read);
    tstrm.reload(&__FILE__, 13, move(cbuf));
    Statement_Sequence sseq(opts);
    sseq.reload(move(tstrm));

    const auto& stmts = sseq.get_statements();
    ASTERIA_TEST_CHECK(stmts.size() == 5);   // implicit return

    Global_Context global;
    Analytic_Context ctx(xtc_plain, global);

    cow_vector<AIR_Node> code;
    cow_vector<phsh_string> names;
    stmts.at(0).generate_code(code, ctx, &names, global, opts, ptc_aware_none);
    ASTERIA_TEST_CHECK(code.size() == 4);  // clear, declare, 1.5, initialize
    ASTERIA_TEST_CHECK(names.size() == 1);  // "a"

    code.clear();
    names.clear();
    stmts.at(1).generate_code(code, ctx, &names, global, opts, ptc_aware_none);
    ASTERIA_TEST_CHECK(code.size() == 5);  // clear, "a", 5.5, operator `=`, `for` statement
    ASTERIA_TEST_CHECK(names.size() == 0);

    code.clear();
    names.clear();
    stmts.at(2).generate_code(code, ctx, &names, global, opts, ptc_aware_none);
    ASTERIA_TEST_CHECK(code.size() == 1);  // block
    ASTERIA_TEST_CHECK(names.size() == 0);

    code.clear();
    names.clear();
    stmts.at(3).generate_code(code, ctx, &names, global, opts, ptc_aware_none);
    ASTERIA_TEST_CHECK(code.size() == 1);  // block
    ASTERIA_TEST_CHECK(names.size() == 0);
  }
