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

        func pack_frames(bt) {
          var str = "";
          for(each k, v -> bt) {
            str += v.frame;
            str += "\n";
          }
          return str;
        }

        func two() {
          throw "boom";
        }

        func one_ptc() {
          try
            two();
          catch(e)
            throw 123;
        }

        func one_nptc() {
          try
            return 1 + two();
          catch(e)
            throw 123;
        }

        var str_ptc = "";
        var str_nptc = "";

        try
          one_ptc();
        catch(e)
          str_ptc = pack_frames(__backtrace);

        try
          one_nptc();
        catch(e)
          str_nptc = pack_frames(__backtrace);

        std.debug.logf("str_ptc:\n$1", str_ptc);
        std.debug.logf("str_nptc:\n$1", str_nptc);
        assert str_ptc == str_nptc;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
