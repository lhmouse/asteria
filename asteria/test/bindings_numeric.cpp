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
        assert std.numeric.abs(+42) == 42;
        assert std.numeric.abs(-42) == 42;
        assert typeof std.numeric.abs(42) == "integer";
        assert std.numeric.abs(+42.5) == 42.5;
        assert std.numeric.abs(-42.5) == 42.5;
        assert std.numeric.abs(+infinity) == infinity;
        assert std.numeric.abs(-infinity) == infinity;
        assert std.numeric.abs(+nan) <=> 0 == "<unordered>";
        assert std.numeric.abs(-nan) <=> 0 == "<unordered>";
        assert typeof std.numeric.abs(42.5) == "real";

        assert std.numeric.sign(+42) ==  0;
        assert std.numeric.sign(-42) == -1;
        assert typeof std.numeric.sign(42) == "integer";
        assert std.numeric.sign(+42.5) ==  0;
        assert std.numeric.sign(-42.5) == -1;
        assert std.numeric.sign(+infinity) ==  0;
        assert std.numeric.sign(-infinity) == -1;
        assert std.numeric.sign(+nan) ==  0;
        assert std.numeric.sign(-nan) == -1;
        assert typeof std.numeric.sign(42.5) == "integer";

        assert std.numeric.is_finite(+5) == true;
        assert std.numeric.is_finite(-5) == true;
        assert std.numeric.is_finite(+3.5) == true;
        assert std.numeric.is_finite(-3.5) == true;
        assert std.numeric.is_finite(+3.5) == true;
        assert std.numeric.is_finite(-3.5) == true;
        assert std.numeric.is_finite(+infinity) == false;
        assert std.numeric.is_finite(-infinity) == false;
        assert std.numeric.is_finite(+nan) == false;
        assert std.numeric.is_finite(-nan) == false;

        assert std.numeric.is_infinity(+5) == false;
        assert std.numeric.is_infinity(-5) == false;
        assert std.numeric.is_infinity(+3.5) == false;
        assert std.numeric.is_infinity(-3.5) == false;
        assert std.numeric.is_infinity(+3.5) == false;
        assert std.numeric.is_infinity(-3.5) == false;
        assert std.numeric.is_infinity(+infinity) == true;
        assert std.numeric.is_infinity(-infinity) == true;
        assert std.numeric.is_infinity(+nan) == false;
        assert std.numeric.is_infinity(-nan) == false;
        assert std.numeric.is_infinity(+1.0 / 0);
        assert std.numeric.is_infinity(-1.0 / 0);

        assert std.numeric.is_nan(+5) == false;
        assert std.numeric.is_nan(-5) == false;
        assert std.numeric.is_nan(+3.5) == false;
        assert std.numeric.is_nan(-3.5) == false;
        assert std.numeric.is_nan(+3.5) == false;
        assert std.numeric.is_nan(-3.5) == false;
        assert std.numeric.is_nan(+infinity) == false;
        assert std.numeric.is_nan(-infinity) == false;
        assert std.numeric.is_nan(+nan) == true;
        assert std.numeric.is_nan(-nan) == true;
        assert std.numeric.is_nan(+0.0 / 0);
        assert std.numeric.is_nan(+0.0 / 0);

        assert std.numeric.clamp(1, 2, 3) == 2;
        assert std.numeric.clamp(2, 2, 3) == 2;
        assert std.numeric.clamp(3, 2, 3) == 3;
        assert std.numeric.clamp(4, 2, 3) == 3;
        assert typeof std.numeric.clamp(1, 2, 3) == "integer";
        assert std.numeric.clamp(1.5, 2, 3) == 2.0;
        assert std.numeric.clamp(2.5, 2, 3) == 2.5;
        assert std.numeric.clamp(3.5, 2, 3) == 3.0;
        assert typeof std.numeric.clamp(1.5, 2, 3) == "real";
        assert std.numeric.clamp(1, 2.5, 3.5) == 2.5;
        assert std.numeric.clamp(2, 2.5, 3.5) == 2.5;
        assert std.numeric.clamp(3, 2.5, 3.5) == 3.0;
        assert std.numeric.clamp(4, 2.5, 3.5) == 3.5;
        assert typeof std.numeric.clamp(1, 2.5, 3.5) == "real";
        assert std.numeric.clamp(1.75, 2.5, 3.5) == 2.50;
        assert std.numeric.clamp(2.75, 2.5, 3.5) == 2.75;
        assert std.numeric.clamp(3.75, 2.5, 3.5) == 3.50;
        assert typeof std.numeric.clamp(1.75, 2.5, 3.5) == "real";
        assert std.numeric.is_nan(std.numeric.clamp(+nan, 2.5, 3.5));
        assert std.numeric.is_nan(std.numeric.clamp(-nan, 2.5, 3.5));
        assert std.numeric.clamp(+infinity, 2.5, 3.5) == 3.5;
        assert std.numeric.clamp(-infinity, 2.5, 3.5) == 2.5;
        assert std.numeric.clamp(1, 2, 2) == 2;
        assert std.numeric.clamp(3, 2, 2) == 2;
        assert std.numeric.clamp(1, 2.5, 2.5) == 2.5;
        assert std.numeric.clamp(3, 2.5, 2.5) == 2.5;

        assert std.numeric.round(42) == 42;
        assert typeof std.numeric.round(42) == "integer";
        assert std.numeric.round(42.4) == 42;
        assert typeof std.numeric.round(42.4) == "real";
        assert std.numeric.round(42.5) == 43;
        assert typeof std.numeric.round(42.5) == "real";
        assert std.numeric.round(-42.4) == -42;
        assert typeof std.numeric.round(-42.4) == "real";
        assert std.numeric.round(-42.5) == -43;
        assert typeof std.numeric.round(-42.5) == "real";

        assert std.numeric.floor(42) == 42;
        assert typeof std.numeric.floor(42) == "integer";
        assert std.numeric.floor(42.4) == 42;
        assert typeof std.numeric.floor(42.4) == "real";
        assert std.numeric.floor(42.5) == 42;
        assert typeof std.numeric.floor(42.5) == "real";
        assert std.numeric.floor(-42.4) == -43;
        assert typeof std.numeric.floor(-42.4) == "real";
        assert std.numeric.floor(-42.5) == -43;
        assert typeof std.numeric.floor(-42.5) == "real";

        assert std.numeric.ceil(42) == 42;
        assert typeof std.numeric.ceil(42) == "integer";
        assert std.numeric.ceil(42.4) == 43;
        assert typeof std.numeric.ceil(42.4) == "real";
        assert std.numeric.ceil(42.5) == 43;
        assert typeof std.numeric.ceil(42.5) == "real";
        assert std.numeric.ceil(-42.4) == -42;
        assert typeof std.numeric.ceil(-42.4) == "real";
        assert std.numeric.ceil(-42.5) == -42;
        assert typeof std.numeric.ceil(-42.5) == "real";

        assert std.numeric.trunc(42) == 42;
        assert typeof std.numeric.trunc(42) == "integer";
        assert std.numeric.trunc(42.4) == 42;
        assert typeof std.numeric.trunc(42.4) == "real";
        assert std.numeric.trunc(42.5) == 42;
        assert typeof std.numeric.trunc(42.5) == "real";
        assert std.numeric.trunc(-42.4) == -42;
        assert typeof std.numeric.trunc(-42.4) == "real";
        assert std.numeric.trunc(-42.5) == -42;
        assert typeof std.numeric.trunc(-42.5) == "real";

        assert std.numeric.iround(42) == 42;
        assert typeof std.numeric.iround(42) == "integer";
        assert std.numeric.iround(42.4) == 42;
        assert typeof std.numeric.iround(42.4) == "integer";
        assert std.numeric.iround(42.5) == 43;
        assert typeof std.numeric.iround(42.5) == "integer";
        assert std.numeric.iround(-42.4) == -42;
        assert typeof std.numeric.iround(-42.4) == "integer";
        assert std.numeric.iround(-42.5) == -43;
        assert typeof std.numeric.iround(-42.5) == "integer";

        assert std.numeric.ifloor(42) == 42;
        assert typeof std.numeric.ifloor(42) == "integer";
        assert std.numeric.ifloor(42.4) == 42;
        assert typeof std.numeric.ifloor(42.4) == "integer";
        assert std.numeric.ifloor(42.5) == 42;
        assert typeof std.numeric.ifloor(42.5) == "integer";
        assert std.numeric.ifloor(-42.4) == -43;
        assert typeof std.numeric.ifloor(-42.4) == "integer";
        assert std.numeric.ifloor(-42.5) == -43;
        assert typeof std.numeric.ifloor(-42.5) == "integer";

        assert std.numeric.iceil(42) == 42;
        assert typeof std.numeric.iceil(42) == "integer";
        assert std.numeric.iceil(42.4) == 43;
        assert typeof std.numeric.iceil(42.4) == "integer";
        assert std.numeric.iceil(42.5) == 43;
        assert typeof std.numeric.iceil(42.5) == "integer";
        assert std.numeric.iceil(-42.4) == -42;
        assert typeof std.numeric.iceil(-42.4) == "integer";
        assert std.numeric.iceil(-42.5) == -42;
        assert typeof std.numeric.iceil(-42.5) == "integer";

        assert std.numeric.itrunc(42) == 42;
        assert typeof std.numeric.itrunc(42) == "integer";
        assert std.numeric.itrunc(42.4) == 42;
        assert typeof std.numeric.itrunc(42.4) == "integer";
        assert std.numeric.itrunc(42.5) == 42;
        assert typeof std.numeric.itrunc(42.5) == "integer";
        assert std.numeric.itrunc(-42.4) == -42;
        assert typeof std.numeric.itrunc(-42.4) == "integer";
        assert std.numeric.itrunc(-42.5) == -42;
        assert typeof std.numeric.itrunc(-42.5) == "integer";

        var i = 1000;
        do {
          assert std.numeric.random() >= 0.0;
          assert std.numeric.random() <  1.0;
          assert typeof std.numeric.random() == "real";
          assert std.numeric.random(2) >= 0;
          assert std.numeric.random(2) <  2;
          assert typeof std.numeric.random(2) == "integer";
          assert std.numeric.random(1.5) >= 0.0;
          assert std.numeric.random(1.5) <  1.5;
          assert typeof std.numeric.random(1.5) == "real";
          assert std.numeric.random(-1, 1) >= -1;
          assert std.numeric.random(-1, 1) <   1;
          assert typeof std.numeric.random(-1, 1) == "integer";
          assert std.numeric.random(-1.5, -0.5) >= -1.5;
          assert std.numeric.random(-1.5, -0.5) <  -0.5;
          assert typeof std.numeric.random(-1.5, -0.5) == "real";
        } while(--i != 0)

        assert std.numeric.addm(+1, +2) == +3;
        assert std.numeric.addm(+1, -2) == -1;
        assert std.numeric.addm(std.constants.integer_max, +2) == std.constants.integer_min + 1;
        assert std.numeric.addm(std.constants.integer_min, -2) == std.constants.integer_max - 1;
        assert std.numeric.addm(+2, std.constants.integer_max) == std.constants.integer_min + 1;
        assert std.numeric.addm(-2, std.constants.integer_min) == std.constants.integer_max - 1;

        assert std.numeric.subm(+1, +2) == -1;
        assert std.numeric.subm(+1, -2) == +3;
        assert std.numeric.subm(std.constants.integer_max, -2) == std.constants.integer_min + 1;
        assert std.numeric.subm(std.constants.integer_min, +2) == std.constants.integer_max - 1;
        assert std.numeric.subm(-2, std.constants.integer_max) == std.constants.integer_max;
        assert std.numeric.subm(+2, std.constants.integer_min) == std.constants.integer_min + 2;

        assert std.numeric.mulm(+2, +3) == +6;
        assert std.numeric.mulm(+2, -3) == -6;
        assert std.numeric.mulm(-2, +3) == -6;
        assert std.numeric.mulm(-2, -3) == +6;
        assert std.numeric.mulm(std.constants.integer_max, +3) == std.constants.integer_max - 2;
        assert std.numeric.mulm(std.constants.integer_max, -3) == std.constants.integer_min + 3;
        assert std.numeric.mulm(+3, std.constants.integer_max) == std.constants.integer_max - 2;
        assert std.numeric.mulm(-3, std.constants.integer_max) == std.constants.integer_min + 3;
        assert std.numeric.mulm(std.constants.integer_min, +3) == std.constants.integer_min;
        assert std.numeric.mulm(std.constants.integer_min, -3) == std.constants.integer_min;
        assert std.numeric.mulm(+3, std.constants.integer_min) == std.constants.integer_min;
        assert std.numeric.mulm(-3, std.constants.integer_min) == std.constants.integer_min;
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
