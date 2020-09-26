// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      ::rocket::sref(__FILE__), __LINE__, ::rocket::sref(R"__(
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
      )__"));
    Global_Context global;
    code.execute(global);
  }
