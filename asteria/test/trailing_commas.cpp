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
    Global_Context global;
    code.execute(global);
  }
