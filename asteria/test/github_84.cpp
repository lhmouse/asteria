// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
///////////////////////////////////////////////////////////////////////////////

        assert std.numeric.parse_real(" 0x1p-100") == +0x1.0p-100;
        assert std.numeric.parse_real("+0x1p-100") == +0x1.0p-100;
        assert std.numeric.parse_real("-0x1p-100") == -0x1.0p-100;

        assert std.numeric.parse_real(" 0x1p-10000") == +0.0;
        assert std.numeric.parse_real("+0x1p-10000") == +0.0;
        assert std.numeric.parse_real("-0x1p-10000") == -0.0;

        try { std.numeric.parse_real(" 0x1p+10000");  assert false;  }
          catch(e) { assert std.string.find(e, "Assertion failure") == null;  }
        try { std.numeric.parse_real("+0x1p+10000");  assert false;  }
          catch(e) { assert std.string.find(e, "Assertion failure") == null;  }
        try { std.numeric.parse_real("-0x1p+10000");  assert false;  }
          catch(e) { assert std.string.find(e, "Assertion failure") == null;  }

        assert std.numeric.parse_real(" 0x1p+10000", true) == +infinity;
        assert std.numeric.parse_real("+0x1p+10000", true) == +infinity;
        assert std.numeric.parse_real("-0x1p+10000", true) == -infinity;

///////////////////////////////////////////////////////////////////////////////
      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, ::rocket::sref(__FILE__), 14);
    Global_Context global;
    code.execute(global);
  }
