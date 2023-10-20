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

        var result = 0;

        func fma(a, b, c) {
          if(a == null) {
            result = "a null";
            return;
          }

          if(b == null) {
            result = "b null";
            return;
          }

          if(c == null) {
            result = "c null";
            return;
          }

          result = a * b + c;
        }

        __vcall(fma, null);
        assert result == "a null";

        __vcall(fma, []);
        assert result == "a null";

        __vcall(fma, [5]);
        assert result == "b null";

        __vcall(fma, [4,5]);
        assert result == "c null";

        __vcall(fma, [2,3,4]);
        assert result == 10;

        // too many arguments
        assert catch(__vcall(fma, [4,5,6,7]));

        func va_gen(n) {
          switch(n) {
            case null:  return 3;  // #args
            case 0:  return 10;
            case 1:  return  3;
            case 2:  return 19;
            default:  assert false;
          }
        }

        __vcall(fma, va_gen);
        assert result == 49;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
