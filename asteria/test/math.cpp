// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
        func lt_1ups(x, y) {
          return ((y == 0) ? __abs(x) : __abs(1-x/y)) <= 0x1.0p-52;
        }

        func lt_3ups(x, y) {
          return ((y == 0) ? __abs(x) : __abs(1-x/y)) <= 0x1.0p-50;
        }

        assert lt_1ups(std.math.exp(1), 2.7182818284590452);
        assert std.math.exp(3, 2) == 8;
        assert std.math.exp(2, 3) == 9;
        assert std.math.exp(0, nan) == 1;
        assert __isnan std.math.exp(2, nan);
        assert std.math.exp(nan, 1) == 1;
        assert __isnan std.math.exp(nan, 2);

        assert std.math.expm1(0) == 0;
        assert lt_1ups(std.math.expm1(1), 1.7182818284590452);

        assert std.math.pow(2, 3) == 8;
        assert std.math.pow(3, 2) == 9;
        assert std.math.pow(nan, 0) == 1;
        assert __isnan std.math.pow(nan, 2);
        assert std.math.pow(1, nan) == 1;
        assert __isnan std.math.pow(2, nan);

        assert std.math.log(std.math.e) == 1;
        assert lt_1ups(std.math.log(9, 3), 2.0);
        assert lt_1ups(std.math.log(3, 9), 0.5);
        assert std.math.log(0, 10) == -infinity;
        assert __isnan std.math.log(-1, 10);
        assert __isnan std.math.log(10,  0);

        assert std.math.log1p(0) == 0;
        assert std.math.log1p(-1) == -infinity;
        assert __isnan std.math.log1p(-2);
        assert lt_1ups(std.math.log1p(1.7182818284590452), 1);

        assert std.math.sin(0) == 0;
        assert lt_1ups(std.math.sin(std.math.pi / 2), 1);
        assert lt_3ups(std.math.sin(std.math.pi *  1 / 6), +0.5);
        assert lt_3ups(std.math.sin(std.math.pi *  5 / 6), +0.5);
        assert lt_3ups(std.math.sin(std.math.pi *  7 / 6), -0.5);
        assert lt_3ups(std.math.sin(std.math.pi * 11 / 6), -0.5);

        assert std.math.cos(0) == 1;
        assert lt_1ups(std.math.cos(std.math.pi / 2), 0);
        assert lt_3ups(std.math.cos(std.math.pi *  2 / 6), +0.5);
        assert lt_3ups(std.math.cos(std.math.pi *  4 / 6), -0.5);
        assert lt_3ups(std.math.cos(std.math.pi *  8 / 6), -0.5);
        assert lt_3ups(std.math.cos(std.math.pi * 10 / 6), +0.5);

        assert std.math.tan(0) == 0;
        //assert __isnan std.math.tan(std.math.pi / 2);
        assert lt_3ups(std.math.tan(std.math.pi *  1 / 4), +1.0);
        assert lt_3ups(std.math.tan(std.math.pi *  3 / 4), -1.0);
        assert lt_3ups(std.math.tan(std.math.pi *  5 / 4), +1.0);
        assert lt_3ups(std.math.tan(std.math.pi *  7 / 4), -1.0);

        assert std.math.asin(0) == 0;
        assert lt_1ups(std.math.asin(1), std.math.pi / 2);
        assert lt_3ups(std.math.asin(+0.5), + std.math.pi / 6);
        assert lt_3ups(std.math.asin(-0.5), - std.math.pi / 6);

        assert lt_1ups(std.math.acos(0), std.math.pi / 2);
        assert std.math.acos(1) == 0;
        assert lt_3ups(std.math.acos(+0.5), std.math.pi * 1 / 3);
        assert lt_3ups(std.math.acos(-0.5), std.math.pi * 2 / 3);

        assert std.math.atan(0) == 0;
        assert lt_3ups(std.math.atan(+1.0), + std.math.pi / 4);
        assert lt_3ups(std.math.atan(-1.0), - std.math.pi / 4);

        assert std.math.atan2(0, 0) == 0;
        assert lt_1ups(std.math.atan2( 0, +1), std.math.pi * +0.00);
        assert lt_3ups(std.math.atan2(+1, +1), std.math.pi * +0.25);
        assert lt_1ups(std.math.atan2(+1,  0), std.math.pi * +0.50);
        assert lt_3ups(std.math.atan2(+1, -1), std.math.pi * +0.75);
        assert lt_1ups(std.math.atan2( 0, -1), std.math.pi * +1.00);
        assert lt_3ups(std.math.atan2(-1, -1), std.math.pi * -0.75);
        assert lt_1ups(std.math.atan2(-1,  0), std.math.pi * -0.50);
        assert lt_3ups(std.math.atan2(-1, +1), std.math.pi * -0.25);

        assert std.math.hypot() == 0;
        assert std.math.hypot(+3) == 3;
        assert std.math.hypot(-3) == 3;
        assert std.math.hypot(+3, +4) == 5;
        assert std.math.hypot(+3, -4) == 5;
        assert std.math.hypot(-3, +4) == 5;
        assert std.math.hypot(-3, -4) == 5;
        assert std.math.hypot(+3, +4, +12) == 13;
        assert std.math.hypot(+3, +4, -12) == 13;
        assert std.math.hypot(+3, -4, +12) == 13;
        assert std.math.hypot(+3, -4, -12) == 13;
        assert std.math.hypot(-3, +4, +12) == 13;
        assert std.math.hypot(-3, +4, -12) == 13;
        assert std.math.hypot(-3, -4, +12) == 13;
        assert std.math.hypot(-3, -4, -12) == 13;
        assert std.math.hypot(+infinity, +nan) == infinity;
        assert std.math.hypot(-infinity, +nan) == infinity;
        assert std.math.hypot(+nan, +infinity) == infinity;
        assert std.math.hypot(+nan, -infinity) == infinity;
        assert std.math.hypot(nan, 1, infinity) == infinity;
        assert __isnan std.math.hypot(nan, 1, nan);
      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
