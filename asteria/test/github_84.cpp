// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        assert std.numeric.parse(" 0x1p-100") == +0x1.0p-100;
        assert std.numeric.parse("+0x1p-100") == +0x1.0p-100;
        assert std.numeric.parse("-0x1p-100") == -0x1.0p-100;

        assert std.numeric.parse(" 0x1p-10000") == +0.0;
        assert std.numeric.parse("+0x1p-10000") == +0.0;
        assert std.numeric.parse("-0x1p-10000") == -0.0;

        assert std.numeric.parse(" 0x1p+10000") == +infinity;
        assert std.numeric.parse("+0x1p+10000") == +infinity;
        assert std.numeric.parse("-0x1p+10000") == -infinity;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
