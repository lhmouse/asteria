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

        var obj = {
          a = {
            value = 42;
            get = func() { return this.value;  };
          };
        };

        assert obj.a.get() == 42;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
