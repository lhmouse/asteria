// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

        var data = [];
        data[0] = data;

        var foo = [];
        foo[0] ||= foo;

        var bar = [];
        bar[0] ??= bar;

        std.gc.collect();

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
