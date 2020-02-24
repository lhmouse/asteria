// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
///////////////////////////////////////////////////////////////////////////////

        try {
          var a = a + 1;
          assert false;
        }
        catch(e) {
          assert typeof e == "string";
          assert std.string.find(e, "uninitialized");
        }

        var a = 42;
        try {
          var a = a + 1;
          assert false;
        }
        catch(e) {
          assert typeof e == "string";
          assert std.string.find(e, "uninitialized");
        }
        assert a == 42;

///////////////////////////////////////////////////////////////////////////////
      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
