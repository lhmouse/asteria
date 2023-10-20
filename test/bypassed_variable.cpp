// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        var sth = 10;

        func disp(x) {
          switch(x) {
          case 1:
            var sth = "meow";  // This hides the outer `sth`.
          case 2:
            sth = true;  // This attempts to alter the inner `sth`.
                         // If it has not been initialized, an exception is thrown.
          }
        }

        try {
          sth = disp(1);
          assert false;
        }
        catch(e) {
          std.io.putf("Caught exception: $1\n", e);
          assert std.string.find(e, "Void reference not dereferenceable") != null;
        }

        assert sth == 10;

        try {
          disp(2);
          assert false;
        }
        catch(e) {
          std.io.putf("Caught exception: $1\n", e);
          assert std.string.find(e, "`sth` not initialized") != null;
        }

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
