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

        func noop(x) { }
        var obj = { };
        noop(->obj[1]);  // `obj[1]` is not a valid reference because `obj` is not an array.

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    ASTERIA_TEST_CHECK_CATCH(code.execute(global));
  }
