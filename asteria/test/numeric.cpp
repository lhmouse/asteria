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

        assert std.numeric.parse(" 1234") == +1234;
        assert typeof std.numeric.parse(" 1234") == "integer";
        assert std.numeric.parse("+1234") == +1234;
        assert typeof std.numeric.parse("+1234") == "integer";
        assert std.numeric.parse("-1234") == -1234;
        assert typeof std.numeric.parse("-1234") == "integer";
        assert std.numeric.parse(" 0") == 0;
        assert typeof std.numeric.parse(" 0") == "integer";
        assert std.numeric.parse("+0") == 0;
        assert typeof std.numeric.parse("+0") == "integer";
        assert std.numeric.parse("-0") == 0;
        assert typeof std.numeric.parse("-0") == "integer";
        assert std.numeric.parse(" 0b0100001100100001") == +17185;
        assert typeof std.numeric.parse(" 0b0100001100100001") == "integer";
        assert std.numeric.parse("+0b0100001100100001") == +17185;
        assert typeof std.numeric.parse("+0b0100001100100001") == "integer";
        assert std.numeric.parse("-0b0100001100100001") == -17185;
        assert typeof std.numeric.parse("-0b0100001100100001") == "integer";
        assert std.numeric.parse(" 0x4321") == +17185;
        assert typeof std.numeric.parse(" 0x4321") == "integer";
        assert std.numeric.parse("+0x4321") == +17185;
        assert typeof std.numeric.parse("+0x4321") == "integer";
        assert std.numeric.parse("-0x4321") == -17185;
        assert typeof std.numeric.parse("-0x4321") == "integer";
        assert typeof std.numeric.parse(" 999999999999999999999999999999") == "real";
        assert typeof std.numeric.parse("+999999999999999999999999999999") == "real";
        assert typeof std.numeric.parse("-999999999999999999999999999999") == "real";
        assert typeof std.numeric.parse(" 1e100") == "real";
        assert typeof std.numeric.parse("+1e100") == "real";
        assert typeof std.numeric.parse("-1e100") == "real";
        assert std.numeric.parse(" 1234.5") == +1234.5;
        assert typeof std.numeric.parse(" 1234.5") == "real";
        assert std.numeric.parse("+1234.5") == +1234.5;
        assert typeof std.numeric.parse("+1234.5") == "real";
        assert std.numeric.parse("-1234.5") == -1234.5;
        assert typeof std.numeric.parse("-1234.5") == "real";
        assert std.numeric.parse(" 0b0100001100100001.11") == +17185.75;
        assert typeof std.numeric.parse(" 0b0100001100100001.11") == "real";
        assert std.numeric.parse("+0b0100001100100001.11") == +17185.75;
        assert typeof std.numeric.parse("+0b0100001100100001.11") == "real";
        assert std.numeric.parse("-0b0100001100100001.11") == -17185.75;
        assert typeof std.numeric.parse("-0b0100001100100001.11") == "real";
        assert std.numeric.parse(" 0x4321.4") == +17185.25;
        assert typeof std.numeric.parse(" 0x4321.4") == "real";
        assert std.numeric.parse("+0x4321.4") == +17185.25;
        assert typeof std.numeric.parse("+0x4321.4") == "real";
        assert std.numeric.parse("-0x4321.4") == -17185.25;
        assert typeof std.numeric.parse("-0x4321.4") == "real";
        assert __sign std.numeric.parse(" 1e-10000") ==  0;
        assert __sign std.numeric.parse("+1e-10000") ==  0;
        assert __sign std.numeric.parse("-1e-10000") == -1;
        assert __sign std.numeric.parse(" 0.0") ==  0;
        assert __sign std.numeric.parse("+0.0") ==  0;
        assert __sign std.numeric.parse("-0.0") == -1;
        assert std.numeric.parse(" infinity") == +infinity;
        assert std.numeric.parse("+infinity") == +infinity;
        assert std.numeric.parse("-infinity") == -infinity;
        assert __isnan std.numeric.parse(" nan") == true;
        assert __isnan std.numeric.parse("+nan") == true;
        assert __isnan std.numeric.parse("-nan") == true;
        assert catch( std.numeric.parse(" NaN") ) != null;
        assert catch( std.numeric.parse("+NaN") ) != null;
        assert catch( std.numeric.parse("-NaN") ) != null;
        assert std.numeric.parse(" 0x43.21  ") == +67.12890625;
        assert std.numeric.parse("+0x43.21  ") == +67.12890625;
        assert std.numeric.parse("-0x43.21  ") == -67.12890625;
        assert std.numeric.parse(" 0x43.21p3") == +537.03125;
        assert std.numeric.parse("+0x43.21p3") == +537.03125;
        assert std.numeric.parse("-0x43.21p3") == -537.03125;
        assert std.numeric.parse(" 1e1000000") ==  +infinity;
        assert std.numeric.parse("+1e1000000") ==  +infinity;
        assert std.numeric.parse("-1e1000000") ==  -infinity;
        assert std.numeric.parse(" 0e1000000") == 0;
        assert std.numeric.parse("+0e1000000") == 0;
        assert std.numeric.parse("-0e1000000") == 0;
        assert std.numeric.parse("0x10000000000000C000000") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse("0x10000000000000A000000") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse("0x100000000000009000000") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse("0x100000000000008000001") == 0x1.0p80 + 0x1.0p28;
        assert std.numeric.parse("0x100000000000008000000") == 0x1.0p80;

        assert std.numeric.pack_i8(0x1234) == "\x34";
        assert std.numeric.pack_i8([ 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
        assert std.numeric.unpack_i8("\x01\x23\x45\x67\x89\xAB\xCD\xEF") == [ 0x01, 0x23, 0x45, 0x67, -0x1p8 | 0x89, -0x1p8 | 0xAB, -0x1p8 | 0xCD, -0x1p8 | 0xEF ];

        assert std.numeric.pack_i16be(0x1234) == "\x12\x34";
        assert std.numeric.pack_i16be([ 0x0123, 0x4567, 0x89AB, 0xCDEF ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
        assert std.numeric.unpack_i16be("\x01\x23\x45\x67\x89\xAB\xCD\xEF") == [ 0x0123, 0x4567, -0x1p16 | 0x89AB, -0x1p16 | 0xCDEF ];

        assert std.numeric.pack_i16le(0x1234) == "\x34\x12";
        assert std.numeric.pack_i16le([ 0x0123, 0x4567, 0x89AB, 0xCDEF ]) == "\x23\x01\x67\x45\xAB\x89\xEF\xCD";
        assert std.numeric.unpack_i16le("\x23\x01\x67\x45\xAB\x89\xEF\xCD") == [ 0x0123, 0x4567, -0x1p16 | 0x89AB, -0x1p16 | 0xCDEF ];

        assert std.numeric.pack_i32be(0x12345678) == "\x12\x34\x56\x78";
        assert std.numeric.pack_i32be([ 0x01234567, 0x89ABCDEF ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
        assert std.numeric.unpack_i32be("\x01\x23\x45\x67\x89\xAB\xCD\xEF") == [ 0x01234567, -0x1p32 | 0x89ABCDEF ];

        assert std.numeric.pack_i32le(0x12345678) == "\x78\x56\x34\x12";
        assert std.numeric.pack_i32le([ 0x01234567, 0x89ABCDEF ]) == "\x67\x45\x23\x01\xEF\xCD\xAB\x89";
        assert std.numeric.unpack_i32le("\x67\x45\x23\x01\xEF\xCD\xAB\x89") == [ 0x01234567, -0x1p32 | 0x89ABCDEF ];

        assert std.numeric.pack_i64be(0x123456789ABCDEF0) == "\x12\x34\x56\x78\x9A\xBC\xDE\xF0";
        assert std.numeric.pack_i64be([ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF\x7E\xDC\xBA\x98\x76\x54\x32\x10";
        assert std.numeric.unpack_i64be("\x01\x23\x45\x67\x89\xAB\xCD\xEF\x7E\xDC\xBA\x98\x76\x54\x32\x10") == [ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ];

        assert std.numeric.pack_i64le(0x123456789ABCDEF0) == "\xF0\xDE\xBC\x9A\x78\x56\x34\x12";
        assert std.numeric.pack_i64le([ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ]) == "\xEF\xCD\xAB\x89\x67\x45\x23\x01\x10\x32\x54\x76\x98\xBA\xDC\x7E";
        assert std.numeric.unpack_i64le("\xEF\xCD\xAB\x89\x67\x45\x23\x01\x10\x32\x54\x76\x98\xBA\xDC\x7E") == [ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ];

        assert std.numeric.pack_f32be(1.1) == "\x3F\x8C\xCC\xCD";
        assert std.numeric.pack_f32be([ 1.1, 1.3 ]) == "\x3F\x8C\xCC\xCD\x3F\xA6\x66\x66";
        assert std.numeric.unpack_f32be("\x3F\x8C\xCC\xCD\x3F\xA6\x66\x66") == [ 1.1000000238418579, 1.2999999523162842 ];

        assert std.numeric.pack_f32le(1.1) == "\xCD\xCC\x8C\x3F";
        assert std.numeric.pack_f32le([ 1.1, 1.3 ]) == "\xCD\xCC\x8C\x3F\x66\x66\xA6\x3F";
        assert std.numeric.unpack_f32le("\xCD\xCC\x8C\x3F\x66\x66\xA6\x3F") == [ 1.1000000238418579, 1.2999999523162842 ];

        assert std.numeric.pack_f64be(1.1) == "\x3F\xF1\x99\x99\x99\x99\x99\x9A";
        assert std.numeric.pack_f64be([ 1.1, 1.3 ]) == "\x3F\xF1\x99\x99\x99\x99\x99\x9A\x3F\xF4\xCC\xCC\xCC\xCC\xCC\xCD";
        assert std.numeric.unpack_f64be("\x3F\xF1\x99\x99\x99\x99\x99\x9A\x3F\xF4\xCC\xCC\xCC\xCC\xCC\xCD") == [ 1.1, 1.3 ];

        assert std.numeric.pack_f64le(1.1) == "\x9A\x99\x99\x99\x99\x99\xF1\x3F";
        assert std.numeric.pack_f64le([ 1.1, 1.3 ]) == "\x9A\x99\x99\x99\x99\x99\xF1\x3F\xCD\xCC\xCC\xCC\xCC\xCC\xF4\x3F";
        assert std.numeric.unpack_f64le("\x9A\x99\x99\x99\x99\x99\xF1\x3F\xCD\xCC\xCC\xCC\xCC\xCC\xF4\x3F") == [ 1.1, 1.3 ];

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
