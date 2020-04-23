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
///////////////////////////////////////////////////////////////////////////////

        var sth = 10;

        func disp(x) {
          switch(x) {
          case 1:
            var sth = "meow";  // This hides the outer `sth`.
          case 2:
            sth = true;  // This attempts to alter the inner `sth`.
                         // If it has not been initialized, an exception is thrown.
          }
        }

        try {
          disp(2);
          assert false;
        }
        catch(e) {
          std.io.putf("Caught exception: $1\n", e);
          assert std.string.find(e, "bypassed variable `sth`") != null;
        }

///////////////////////////////////////////////////////////////////////////////
      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
