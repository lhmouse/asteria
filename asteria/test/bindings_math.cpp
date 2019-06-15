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
        func lt_1ups(x, y) {
          return (y == 0) ? (x == 0) : (__abs(1-__abs(x/y)) <= 0x1.0p-52);
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
        assert std.math.log(9, 3) == 2;
        assert std.math.log(3, 9) == 0.5;
        assert std.math.log(0, 10) == -infinity;
        assert __isnan std.math.log(-1, 10);
        assert __isnan std.math.log(10,  0);

        assert std.math.log1p(0) == 0;
        assert std.math.log1p(-1) == -infinity;
        assert __isnan std.math.log1p(-2);
        assert lt_1ups(std.math.log1p(1.7182818284590452), 1);
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
