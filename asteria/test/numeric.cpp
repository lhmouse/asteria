// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        assert std.numeric.abs(+42) == 42;
        assert std.numeric.abs(-42) == 42;
        assert typeof std.numeric.abs(42) == "integer";
        assert std.numeric.abs(+42.5) == 42.5;
        assert std.numeric.abs(-42.5) == 42.5;
        assert std.numeric.abs(+infinity) == infinity;
        assert std.numeric.abs(-infinity) == infinity;
        assert std.numeric.abs(+nan) <=> 0 == "[unordered]";
        assert std.numeric.abs(-nan) <=> 0 == "[unordered]";
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

        assert std.numeric.max() == null;
        assert std.numeric.max(1) == 1;
        assert std.numeric.max(-1) == -1;
        assert std.numeric.max(0.5) == 0.5;
        assert std.numeric.max(-0.5) == -0.5;
        assert typeof std.numeric.max(1, 1.5) == "real";
        assert std.numeric.max(1, 1.5) == 1.5;
        assert typeof std.numeric.max(1, 1.5, 2) == "integer";
        assert std.numeric.max(1, 1.5, 2) == 2;
        assert typeof std.numeric.max(1, 1.5, 2, 0.5) == "integer";
        assert std.numeric.max(1, 1.5, 2, 0.5) == 2;
        assert std.numeric.max(null, 1, 1.5, 2, 0.5) == 2;
        assert std.numeric.max(1, null, 1.5, 2, 0.5) == 2;
        assert std.numeric.max(1, 1.5, null, 2, 0.5) == 2;
        assert std.numeric.max(1, 1.5, 2, null, 0.5) == 2;
        assert std.numeric.max(1, 1.5, 2, 0.5, null) == 2;

        assert std.numeric.min() == null;
        assert std.numeric.min(1) == 1;
        assert std.numeric.min(-1) == -1;
        assert std.numeric.min(0.5) == 0.5;
        assert std.numeric.min(-0.5) == -0.5;
        assert typeof std.numeric.min(1, 1.5) == "integer";
        assert std.numeric.min(1, 1.5) == 1;
        assert typeof std.numeric.min(1, 1.5, 2) == "integer";
        assert std.numeric.min(1, 1.5, 2) == 1;
        assert typeof std.numeric.min(1, 1.5, 2, 0.5) == "real";
        assert std.numeric.min(1, 1.5, 2, 0.5) == 0.5;
        assert std.numeric.min(null, 1, 1.5, 2, 0.5) == 0.5;
        assert std.numeric.min(1, null, 1.5, 2, 0.5) == 0.5;
        assert std.numeric.min(1, 1.5, null, 2, 0.5) == 0.5;
        assert std.numeric.min(1, 1.5, 2, null, 0.5) == 0.5;
        assert std.numeric.min(1, 1.5, 2, 0.5, null) == 0.5;

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
        assert __isnan std.numeric.clamp(+nan, 2.5, 3.5);
        assert __isnan std.numeric.clamp(-nan, 2.5, 3.5);
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

        assert std.numeric.random() >= 0.0;
        assert std.numeric.random() <  1.0;
        assert typeof std.numeric.random() == "real";
        assert std.numeric.random(+1.5) >= +0.0;
        assert std.numeric.random(+1.5) <  +1.5;
        assert typeof std.numeric.random(+1.5) == "real";
        assert std.numeric.random(-1.5) <= -0.0;
        assert std.numeric.random(-1.5) >  -1.5;
        assert typeof std.numeric.random(-1.5) == "real";

        assert std.numeric.remainder(+6.0, +3.5) == -1.0;
        assert std.numeric.remainder(-6.0, +3.5) == +1.0;
        assert std.numeric.remainder(+6.0, -3.5) == -1.0;
        assert std.numeric.remainder(-6.0, -3.5) == +1.0;

        assert std.numeric.frexp(0) == [0.0,0];
        assert std.numeric.frexp(1) == [0.5,1];
        assert std.numeric.frexp(1.5) == [0.75,1];
        assert std.numeric.frexp(2.0) == [0.50,2];
        assert std.numeric.frexp(+infinity)[0] == +infinity;
        assert std.numeric.frexp(-infinity)[0] == -infinity;
        assert __isnan std.numeric.frexp(nan)[0];

        assert std.numeric.ldexp(0, 0) == 0;
        assert std.numeric.ldexp(1, +1) == 2;
        assert std.numeric.ldexp(1, -1) == 0.5;
        assert std.numeric.ldexp(0, +0x1p40) == 0;
        assert std.numeric.ldexp(1, +0x1p40) == infinity;
        assert std.numeric.ldexp(1, -0x1p40) == 0;
        assert std.numeric.ldexp(infinity, -1) == infinity;
        assert __isnan std.numeric.ldexp(nan, -1);

        assert std.numeric.rotl(12, 0b1111111101111011111, -40) == 0b111110111101;
        assert std.numeric.rotl(12, 0b1111111101111011111, -28) == 0b111110111101;
        assert std.numeric.rotl(12, 0b1111111101111011111, -16) == 0b111110111101;
        assert std.numeric.rotl(12, 0b1111111101111011111,  -4) == 0b111110111101;
        assert std.numeric.rotl(12, 0b1111111101111011111,  -3) == 0b111101111011;
        assert std.numeric.rotl(12, 0b1111111101111011111,  -2) == 0b111011110111;
        assert std.numeric.rotl(12, 0b1111111101111011111,  -1) == 0b110111101111;
        assert std.numeric.rotl(12, 0b1111111101111011111,   0) == 0b101111011111;
        assert std.numeric.rotl(12, 0b1111111101111011111,   1) == 0b011110111111;
        assert std.numeric.rotl(12, 0b1111111101111011111,   2) == 0b111101111110;
        assert std.numeric.rotl(12, 0b1111111101111011111,   3) == 0b111011111101;
        assert std.numeric.rotl(12, 0b1111111101111011111,   4) == 0b110111111011;
        assert std.numeric.rotl(12, 0b1111111101111011111,  16) == 0b110111111011;
        assert std.numeric.rotl(12, 0b1111111101111011111,  28) == 0b110111111011;
        assert std.numeric.rotl(12, 0b1111111101111011111,  40) == 0b110111111011;
        assert std.numeric.rotl(0, 0b1111111101111011111, 40) == 0;
        assert std.numeric.rotl(64, 0x76543210ABCDEF98, 12) == 0x43210ABCDEF98765;

        assert std.numeric.rotr(12, 0b1111111101111011111, -40) == 0b110111111011;
        assert std.numeric.rotr(12, 0b1111111101111011111, -28) == 0b110111111011;
        assert std.numeric.rotr(12, 0b1111111101111011111, -16) == 0b110111111011;
        assert std.numeric.rotr(12, 0b1111111101111011111,  -4) == 0b110111111011;
        assert std.numeric.rotr(12, 0b1111111101111011111,  -3) == 0b111011111101;
        assert std.numeric.rotr(12, 0b1111111101111011111,  -2) == 0b111101111110;
        assert std.numeric.rotr(12, 0b1111111101111011111,  -1) == 0b011110111111;
        assert std.numeric.rotr(12, 0b1111111101111011111,   0) == 0b101111011111;
        assert std.numeric.rotr(12, 0b1111111101111011111,   1) == 0b110111101111;
        assert std.numeric.rotr(12, 0b1111111101111011111,   2) == 0b111011110111;
        assert std.numeric.rotr(12, 0b1111111101111011111,   3) == 0b111101111011;
        assert std.numeric.rotr(12, 0b1111111101111011111,   4) == 0b111110111101;
        assert std.numeric.rotr(12, 0b1111111101111011111,  16) == 0b111110111101;
        assert std.numeric.rotr(12, 0b1111111101111011111,  28) == 0b111110111101;
        assert std.numeric.rotr(12, 0b1111111101111011111,  40) == 0b111110111101;
        assert std.numeric.rotr(0, 0b1111111101111011111, 40) == 0;
        assert std.numeric.rotr(64, 0x76543210ABCDEF98, 12) == -0x6789ABCDEF54322;

        assert std.numeric.format(+1234) == "1234";
        assert std.numeric.format(+0) == "0";
        assert std.numeric.format(-0) == "0";
        assert std.numeric.format(-1234) == "-1234";
        assert std.numeric.format(+9223372036854775807) == "9223372036854775807";
        assert std.numeric.format(-9223372036854775808) == "-9223372036854775808";
        assert std.numeric.format(+0x1234,  2) == "0b1001000110100";
        assert std.numeric.format(-0x1234,  2) == "-0b1001000110100";
        assert std.numeric.format(+0x1234, 10) == "4660";
        assert std.numeric.format(-0x1234, 10) == "-4660";
        assert std.numeric.format(+0x1234, 16) == "0x1234";
        assert std.numeric.format(-0x1234, 16) == "-0x1234";
        assert std.numeric.format(+0x1234,  2,  2) == "0b10010001101p+02";
        assert std.numeric.format(-0x1234,  2,  2) == "-0b10010001101p+02";
        assert std.numeric.format(+0x1234, 16,  2) == "0x48Dp+02";
        assert std.numeric.format(-0x1234, 16,  2) == "-0x48Dp+02";
        assert std.numeric.format(+0x1234, 10, 10) == "466e+01";
        assert std.numeric.format(-0x1234, 10, 10) == "-466e+01";
        assert std.numeric.format(+123.75) == "123.75";
        assert std.numeric.format(+0.0) == "0";
        assert std.numeric.format(-0.0) == "-0";
        assert std.numeric.format(-123.75) == "-123.75";
        assert std.numeric.format(+infinity) == "infinity";
        assert std.numeric.format(-infinity) == "-infinity";
        assert std.numeric.format(+nan) == "nan";
        assert std.numeric.format(-nan) == "-nan";
        assert std.numeric.format(+0x123.C,  2) == "0b100100011.11";
        assert std.numeric.format(-0x123.C,  2) == "-0b100100011.11";
        assert std.numeric.format(+0x123.C, 10) == "291.75";
        assert std.numeric.format(-0x123.C, 10) == "-291.75";
        assert std.numeric.format(+0x123.C, 16) == "0x123.C";
        assert std.numeric.format(-0x123.C, 16) == "-0x123.C";
        assert std.numeric.format(+0x0.123C,  2) == "0b0.00010010001111";
        assert std.numeric.format(-0x0.123C,  2) == "-0b0.00010010001111";
        assert std.numeric.format(+0x0.0123C, 10) == "0.004451751708984375";
        assert std.numeric.format(-0x0.0123C, 10) == "-0.004451751708984375";
        assert std.numeric.format(+0x0.0123C, 16) == "0x0.0123C";
        assert std.numeric.format(-0x0.0123C, 16) == "-0x0.0123C";
        assert std.numeric.format(+0x123.C,  2,  2) == "0b1.0010001111p+08";
        assert std.numeric.format(-0x123.C,  2,  2) == "-0b1.0010001111p+08";
        assert std.numeric.format(+0x123.C, 16,  2) == "0x1.23Cp+08";
        assert std.numeric.format(-0x123.C, 16,  2) == "-0x1.23Cp+08";
        assert std.numeric.format(+0x0.0123C,  2,  2) == "0b1.0010001111p-08";
        assert std.numeric.format(-0x0.0123C,  2,  2) == "-0b1.0010001111p-08";
        assert std.numeric.format(+0x0.0123C, 16,  2) == "0x1.23Cp-08";
        assert std.numeric.format(-0x0.0123C, 16,  2) == "-0x1.23Cp-08";
        assert std.numeric.format(+0x123.C, 10, 10) == "2.9175e+02";
        assert std.numeric.format(-0x123.C, 10, 10) == "-2.9175e+02";

        assert std.numeric.parse_integer(" 1234") == +1234;
        assert std.numeric.parse_integer("+1234") == +1234;
        assert std.numeric.parse_integer("-1234") == -1234;
        assert std.numeric.parse_integer(" 0") == 0;
        assert std.numeric.parse_integer("+0") == 0;
        assert std.numeric.parse_integer("-0") == 0;
        assert std.numeric.parse_integer(" 0b0100001100100001") == +17185;
        assert std.numeric.parse_integer("+0b0100001100100001") == +17185;
        assert std.numeric.parse_integer("-0b0100001100100001") == -17185;
        assert std.numeric.parse_integer(" 0x4321") == +17185;
        assert std.numeric.parse_integer("+0x4321") == +17185;
        assert std.numeric.parse_integer("-0x4321") == -17185;
        try { std.numeric.parse_integer(" 999999999999999999999999999999");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_integer("+999999999999999999999999999999");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_integer("-999999999999999999999999999999");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_integer(" 1e100");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_integer("+1e100");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_integer("-1e100");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }

        assert std.numeric.parse_real(" 1234") == +1234;
        assert std.numeric.parse_real("+1234") == +1234;
        assert std.numeric.parse_real("-1234") == -1234;
        assert std.numeric.parse_real(" 0b101") == +5;
        assert std.numeric.parse_real("+0b101") == +5;
        assert std.numeric.parse_real("-0b101") == -5;
        assert std.numeric.parse_real(" 0x101") == +257;
        assert std.numeric.parse_real("+0x101") == +257;
        assert std.numeric.parse_real("-0x101") == -257;
        assert std.numeric.parse_real(" 123.75") == +123.75;
        assert std.numeric.parse_real("+123.75") == +123.75;
        assert std.numeric.parse_real("-123.75") == -123.75;
        assert std.numeric.parse_real(" 0") == 0;
        assert std.numeric.parse_real("+0") == 0;
        assert __sign std.numeric.parse_real("+0") ==  0;
        assert std.numeric.parse_real("-0") == 0;
        assert __sign std.numeric.parse_real("-0") == -1;
        assert std.numeric.parse_real(" infinity") == +infinity;
        assert std.numeric.parse_real("+infinity") == +infinity;
        assert std.numeric.parse_real("-infinity") == -infinity;
        try { std.numeric.parse_real(" Infinity");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("+Infinity");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("-Infinity");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        assert __isnan std.numeric.parse_real(" nan");
        assert __isnan std.numeric.parse_real("+nan");
        assert __isnan std.numeric.parse_real("-nan");
        try { std.numeric.parse_real(" NaN");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("+NaN");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("-NaN");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        assert std.numeric.parse_real(" 0b01000011.00100001  ") == +67.12890625;
        assert std.numeric.parse_real("+0b01000011.00100001  ") == +67.12890625;
        assert std.numeric.parse_real("-0b01000011.00100001  ") == -67.12890625;
        assert std.numeric.parse_real(" 0b01000011.00100001p3") == +537.03125;
        assert std.numeric.parse_real("+0b01000011.00100001p3") == +537.03125;
        assert std.numeric.parse_real("-0b01000011.00100001p3") == -537.03125;
        assert std.numeric.parse_real(" 0x43.21  ") == +67.12890625;
        assert std.numeric.parse_real("+0x43.21  ") == +67.12890625;
        assert std.numeric.parse_real("-0x43.21  ") == -67.12890625;
        assert std.numeric.parse_real(" 0x43.21p3") == +537.03125;
        assert std.numeric.parse_real("+0x43.21p3") == +537.03125;
        assert std.numeric.parse_real("-0x43.21p3") == -537.03125;
        try { std.numeric.parse_real(" 1e1000000");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("+1e1000000");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        try { std.numeric.parse_real("-1e1000000");  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
        assert std.numeric.parse_real(" 1e1000000", true) ==  +infinity;
        assert std.numeric.parse_real("+1e1000000", true) ==  +infinity;
        assert std.numeric.parse_real("-1e1000000", true) ==  -infinity;
        assert std.numeric.parse_real(" 0e1000000") == 0;
        assert std.numeric.parse_real("+0e1000000") == 0;
        assert std.numeric.parse_real("-0e1000000") == 0;
        assert std.numeric.parse_real("0x10000000000000C000000") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse_real("0x10000000000000A000000") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse_real("0x100000000000009000000") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse_real("0x100000000000008000001") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse_real("0x100000000000008000000") == 0x1.0p80;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
