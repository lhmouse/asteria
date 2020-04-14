// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
        func recur(n) {
          std.debug.logf("recur($1)", n + 1);
          return recur(n + 1) + 1;
        }
        return recur(0);
      )__"), tinybuf::open_read);
    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    ASTERIA_TEST_CHECK_CATCH(code.execute(global));
  }
