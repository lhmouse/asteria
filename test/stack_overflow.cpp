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

        func recur(n) {
          std.debug.logf("recur($1)", n + 1);
          return recur(n + 1) + 1;
        }
        return recur(0);

///////////////////////////////////////////////////////////////////////////////
      )__");
    ASTERIA_TEST_CHECK_CATCH(code.execute());
  }
