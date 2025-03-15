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

        var a = "meow";
        for(var k = 0; k < 100000; ++k)
          a = [ a, a ];

        std.io.putln("meow");
        std.io.flush();

        std.gc.collect();
        a = null;
        std.gc.collect();

        std.io.putln("bark");
        std.io.flush();

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
