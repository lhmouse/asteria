// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

        func noop(x) { }
        var obj = { };
        noop(->obj[1]);  // `obj[1]` is not a valid reference because `obj` is not an array.

///////////////////////////////////////////////////////////////////////////////
      )__");
    ASTERIA_TEST_CHECK_CATCH(code.execute());
  }
