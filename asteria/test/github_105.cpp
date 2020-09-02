// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      ::rocket::sref(__FILE__), __LINE__, ::rocket::sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        var obj = { };
        ref x -> obj.meow;
        x = x ?? 42;
        assert obj.meow == 42;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    code.execute(global);
  }
