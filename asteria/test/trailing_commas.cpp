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

        func foo() { }
        func foo(a) { }
        func foo(a,) { }
        func foo(a,b) { }
        func foo(a,b,) { }

        func foo(...) { }

        foo();
        foo(1);
        foo(1,);
        foo(1,2);
        foo(1,2,);

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
