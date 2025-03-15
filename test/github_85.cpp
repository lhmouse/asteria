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

        try {
          var a = a + 1;
          assert false;
        }
        catch(e) {
          assert typeof e == "string";
          assert std.string.find(e, "not initialized");
        }

        var a = 42;
        try {
          var a = a + 1;
          assert false;
        }
        catch(e) {
          assert typeof e == "string";
          assert std.string.find(e, "not initialized");
        }
        assert a == 42;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
