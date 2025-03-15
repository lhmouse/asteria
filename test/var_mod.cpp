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

        var obj = {
          a = {
            value = 42;
            get = func() { return this.value;  };
          };
        };

        assert obj.a.get() == 42;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
