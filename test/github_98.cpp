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

        var r;
        r = 2 == 3 ? 4 : 5;
        assert r == 5;

        var t;
        r = t = 42;
        assert r == 42;
        assert t == 42;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
