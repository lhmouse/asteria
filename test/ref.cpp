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

        var a = 1;
        ref b -> a;

        b++;
        assert a == 2;
        assert b == 2;

        a++;
        assert a == 3;
        assert b == 3;

        var f = func() = a;  // return by value
        try
          f() = 4;
        catch(e)
          ;
        assert a == 3;

        f = func() -> a;  // return by ref
        f() = 5;
        assert a == 5;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
