// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/simple_source_file.hpp"
#include "../asteria/src/runtime/global_context.hpp"
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

        assert std.numeric.signbit(+42) ==  0;
        assert std.numeric.signbit(-42) == -1;
        assert typeof std.numeric.signbit(42) == "integer";
        assert std.numeric.signbit(+42.5) ==  0;
        assert std.numeric.signbit(-42.5) == -1;
        assert std.numeric.signbit(+infinity) ==  0;
        assert std.numeric.signbit(-infinity) == -1;
        assert std.numeric.signbit(+nan) ==  0;
        assert std.numeric.signbit(-nan) == -1;
        assert typeof std.numeric.signbit(42.5) == "integer";

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
          assert std.numeric.random(2) >= 0;
          assert std.numeric.random(2) <  2;
          assert std.numeric.random(1.5) >= 0.0;
          assert std.numeric.random(1.5) <  1.5;
          assert std.numeric.random(-1, 1) >= -1;
          assert std.numeric.random(-1, 1) <   1;
          assert std.numeric.random(-1.5, -0.5) >= -1.5;
          assert std.numeric.random(-1.5, -0.5) <  -0.5;
        } while(--i != 0)
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
