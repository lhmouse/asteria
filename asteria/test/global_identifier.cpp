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

        var std = "meow";
        return typeof std + "/" + typeof __global std;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    ASTERIA_TEST_CHECK(code.execute().dereference_readonly().as_string() == "string/object");
  }
