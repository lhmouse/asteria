// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

        var ptc;

        ptc = func(n) {
          if(n <= 0) {
            return n;
          }
          return ptc(n-1);  // this may blow the system stack up if non-proper.
        };
        ptc(10000);

        ptc = func(n) {
          if(n <= 0) {
            return ->n;
          }
          ptc(n-1);  // this may blow the system stack up if non-proper.
        };
        ptc(10000);

        ptc = func(n) {
          if(n <= 0) {
            return;
          }
          return ptc(n-1);  // this may blow the system stack up if non-proper.
        };
        ptc(10000);

        ptc = func(n) {
          return (n <= 0) ? n : ptc(n-1);  // this may blow the system stack up if non-proper.
        };
        ptc(10000);

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
