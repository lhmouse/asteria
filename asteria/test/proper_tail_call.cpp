// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
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
      )__"), tinybuf::open_read);
    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
