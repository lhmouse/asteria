// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/simple_source_file.hpp"
#include "../src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    static constexpr char s_source[] =
      R"__(
        // tail call, non-proper
        func nptc(n) {
          if(n <= 0) {
            throw "boom";
          }
          return nptc(n-1);
        }
        try {
          nptc(10);
        }
        catch(e) {
          std.debug.dump(__backtrace);
          assert lengthof __backtrace == 13;  // 1 throw, 11 frames, 1 catch
        }

        // tail call, proper
        func ptc(n) {
          if(n <= 0) {
            throw "boom";
          }
          return& ptc(n-1);
        }
        try {
          ptc(10);
        }
        catch(e) {
          std.debug.dump(__backtrace);
          assert lengthof __backtrace == 3;  // 1 throw, 1 frame, 1 catch
        }
      )__";

    Cow_isstream iss(rocket::sref(s_source));
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
