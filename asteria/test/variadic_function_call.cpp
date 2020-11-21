// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        var num = 4;

        var obj = {
          value = "hello";

          do_imuladd = func(x, y, z) {
            assert this.value == "hello";  // member function

            x = x * y + z;
            return "ifma success";
          };

          generator = func(x) {
            assert this.value == "hello";  // member function

            if(x == null)
              return 3;     // num of args := 3
            if(x == 0)
              return ->num;  // arg 0 := the variable
            if(x == 1)
              return 2;     // arg 1 := 2
            if(x == 2)
              return 5;     // arg 2 := 5

            assert false;  // unreachable
          };
        };

        assert __vcall(obj.do_imuladd, obj.generator) == "ifma success";
        assert num == 13;  // 4 * 2 + 5
        assert __vcall(obj.do_imuladd, obj.generator) == "ifma success";
        assert num == 31;  // 13 * 2 + 5
        assert __vcall(obj.do_imuladd, obj.generator) == "ifma success";
        assert num == 67;  // 31 * 2 + 5

        var obj = {
          value = "meow";

          do_sum = func(x, y, z) {
            this.value = x + y + z;
            return "sum success";
          };
        };

        assert __vcall(obj.do_sum, [ 1, 2, 3 ]) == "sum success";
        assert obj.value == 6;
        assert __vcall(obj.do_sum, [ 7, 8, 9 ]) == "sum success";
        assert obj.value == 24;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    code.execute(global);
  }
