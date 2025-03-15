// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
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
      )__");
    code.execute();
  }
