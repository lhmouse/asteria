// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
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
      )__"), tinybuf::open_read);
    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
