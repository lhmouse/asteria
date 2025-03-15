// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
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
      )__");
    code.execute();
  }
