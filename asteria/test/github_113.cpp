// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        var data = [];
        data[0] = data;

        var foo = [];
        foo[0] ||= foo;

        var bar = [];
        bar[0] ??= bar;

        std.system.gc_collect();

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    code.execute(global);
  }
