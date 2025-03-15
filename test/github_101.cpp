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

        var a = 1;
        func two() { return 2;  }
        func check() { return a &&= two();  }
        check();
        std.debug.logf("a = $1\n", a);

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
