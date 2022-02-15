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

        try {
          var a = a + 1;
          assert false;
        }
        catch(e) {
          assert typeof e == "string";
          assert std.string.find(e, "uninitialized");
        }

        var a = 42;
        try {
          var a = a + 1;
          assert false;
        }
        catch(e) {
          assert typeof e == "string";
          assert std.string.find(e, "uninitialized");
        }
        assert a == 42;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
