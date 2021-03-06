// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        assert std.numeric.parse_real(" 0x1p-100") == +0x1.0p-100;
        assert std.numeric.parse_real("+0x1p-100") == +0x1.0p-100;
        assert std.numeric.parse_real("-0x1p-100") == -0x1.0p-100;

        assert std.numeric.parse_real(" 0x1p-10000") == +0.0;
        assert std.numeric.parse_real("+0x1p-10000") == +0.0;
        assert std.numeric.parse_real("-0x1p-10000") == -0.0;

        try { std.numeric.parse_real(" 0x1p+10000");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("+0x1p+10000");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("-0x1p+10000");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }

        assert std.numeric.parse_real(" 0x1p+10000", true) == +infinity;
        assert std.numeric.parse_real("+0x1p+10000", true) == +infinity;
        assert std.numeric.parse_real("-0x1p+10000", true) == -infinity;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
