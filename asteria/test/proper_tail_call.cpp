// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    rocket::cow_stringbuf buf(rocket::sref(
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
            return& n;
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
          return (n <= 0) ? n
                          : ptc(n-1);  // this may blow the system stack up if non-proper.
        };
        ptc(10000);
      )__"));
    Simple_Script code(buf, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global);
  }
