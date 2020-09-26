// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      ::rocket::sref(__FILE__), __LINE__, ::rocket::sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        var std = "meow";
        return typeof std + "/" + typeof __global std;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    ASTERIA_TEST_CHECK(code.execute(global).read().as_string() == "string/object");
  }
