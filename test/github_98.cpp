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

        var r;
        r = 2 == 3 ? 4 : 5;
        assert r == 5;

        var t;
        r = t = 42;
        assert r == 42;
        assert t == 42;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
