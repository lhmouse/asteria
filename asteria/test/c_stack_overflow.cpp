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

        var a;
        for(var k = 0; k < 100000; ++k) {
          var b = a;
          a = func() { return b;  };
        }

        a = null;
        std.io.putln("meow");
        std.io.flush();

        std.system.gc_collect();
        std.io.putln("bark");

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
