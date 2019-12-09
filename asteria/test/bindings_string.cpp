// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    rocket::tinybuf_str cbuf;
    cbuf.set_string(rocket::sref(
      R"__(
        assert std.string.slice("hello", 0) == "hello";
        assert std.string.slice("hello", 1) == "ello";
        assert std.string.slice("hello", 2) == "llo";
        assert std.string.slice("hello", 3) == "lo";
        assert std.string.slice("hello", 4) == "o";
        assert std.string.slice("hello", 5) == "";
        assert std.string.slice("hello", 6) == "";
        assert std.string.slice("hello", 0, 3) == "hel";
        assert std.string.slice("hello", 1, 3) == "ell";
        assert std.string.slice("hello", 2, 3) == "llo";
        assert std.string.slice("hello", 3, 3) == "lo";
        assert std.string.slice("hello", 4, 3) == "o";
        assert std.string.slice("hello", 5, 3) == "";
        assert std.string.slice("hello", 6, 3) == "";
        assert std.string.slice("hello", std.numeric.integer_max) == "";
        assert std.string.slice("hello", std.numeric.integer_max, std.numeric.integer_max) == "";

        assert std.string.slice("hello", -1) == "o";
        assert std.string.slice("hello", -2) == "lo";
        assert std.string.slice("hello", -3) == "llo";
        assert std.string.slice("hello", -4) == "ello";
        assert std.string.slice("hello", -5) == "hello";
        assert std.string.slice("hello", -6) == "hello";
        assert std.string.slice("hello", -1, 3) == "o";
        assert std.string.slice("hello", -2, 3) == "lo";
        assert std.string.slice("hello", -3, 3) == "llo";
        assert std.string.slice("hello", -4, 3) == "ell";
        assert std.string.slice("hello", -5, 3) == "hel";
        assert std.string.slice("hello", -6, 3) == "he";
        assert std.string.slice("hello", -7, 3) == "h";
        assert std.string.slice("hello", -8, 3) == "";
        assert std.string.slice("hello", -9, 3) == "";
        assert std.string.slice("hello", std.numeric.integer_min) == "hello";
        assert std.string.slice("hello", std.numeric.integer_min, std.numeric.integer_max) == "hell";

        assert std.string.replace_slice("hello", 2, "##") == "he##";
        assert std.string.replace_slice("hello", 2, 1, "##") == "he##lo";
        assert std.string.replace_slice("hello", 9, "##") == "hello##";

        assert std.string.replace_slice("hello", -2, "##") == "hel##";
        assert std.string.replace_slice("hello", -2, 1, "##") == "hel##o";
        assert std.string.replace_slice("hello", -9, "##") == "##";
        assert std.string.replace_slice("hello", -9, 1, "##") == "##hello";

        assert std.string.compare("abcd", "abcc") >  0;
        assert std.string.compare("abcd", "abcd") == 0;
        assert std.string.compare("abcd", "abce") <  0;

        assert std.string.compare("abcd", "abcc", 0) == 0;
        assert std.string.compare("abcd", "abcd", 3) == 0;
        assert std.string.compare("abcd", "abce", 4) <  0;

        assert std.string.compare("abcd", "abcc", -1) == 0;
        assert std.string.compare("abcd", "abcd", 99) == 0;

        assert std.string.starts_with("hello", "hel") == true;
        assert std.string.starts_with("hello", "foo") == false;
        assert std.string.starts_with("hello", "") == true;

        assert std.string.ends_with("hello", "llo") == true;
        assert std.string.ends_with("hello", "foo") == false;
        assert std.string.ends_with("hello", "") == true;

        assert std.string.find("hello hello world", "lo") == 3;
        assert std.string.find("hello hello world", 3, "lo") == 3;
        assert std.string.find("hello hello world", 4, "lo") == 9;
        assert std.string.find("hello hello world", 3, 7, "lo") == 3;
        assert std.string.find("hello hello world", 3, 8, "lo") == 3;
        assert std.string.find("hello hello world", 4, 6, "lo") == null;
        assert std.string.find("hello hello world", 5, 6, "lo") == 9;

        assert std.string.rfind("hello hello world", "lo") == 9;
        assert std.string.rfind("hello hello world", 3, "lo") == 9;
        assert std.string.rfind("hello hello world", 4, "lo") == 9;
        assert std.string.rfind("hello hello world", 3, 7, "lo") == 3;
        assert std.string.rfind("hello hello world", 3, 8, "lo") == 9;
        assert std.string.rfind("hello hello world", 4, 6, "lo") == null;
        assert std.string.rfind("hello hello world", 5, 6, "lo") == 9;

        assert std.string.find_and_replace("hello hello world", "llo", "####") == "he#### he#### world";
        assert std.string.find_and_replace("hello hello world", 2, "llo", "####") == "he#### he#### world";
        assert std.string.find_and_replace("hello hello world", 3, "llo", "####") == "hello he#### world";
        assert std.string.find_and_replace("hello hello world", 8, "llo", "####") == "hello he#### world";
        assert std.string.find_and_replace("hello hello world", 9, "llo", "####") == "hello hello world";
        assert std.string.find_and_replace("hello hello world", 2, 7, "llo", "####") == "he#### hello world";
        assert std.string.find_and_replace("hello hello world", 2, 9, "llo", "####") == "he#### hello world";
        assert std.string.find_and_replace("hello hello world", 3, 7, "llo", "####") == "hello hello world";
        assert std.string.find_and_replace("hello hello world", 4, 7, "llo", "####") == "hello he#### world";

        assert std.string.find_any_of("hello", "aeiou") == 1;
        assert std.string.find_any_of("hello", 1, "aeiou") == 1;
        assert std.string.find_any_of("hello", 2, "aeiou") == 4;
        assert std.string.find_any_of("hello", 1, 2, "aeiou") == 1;
        assert std.string.find_any_of("hello", 2, 2, "aeiou") == null;
        assert std.string.find_any_of("hello", 3, 2, "aeiou") == 4;
        assert std.string.find_any_of("hello", "") == null;

        assert std.string.rfind_any_of("hello", "aeiou") == 4;
        assert std.string.rfind_any_of("hello", 1, "aeiou") == 4;
        assert std.string.rfind_any_of("hello", 2, "aeiou") == 4;
        assert std.string.rfind_any_of("hello", 1, 2, "aeiou") == 1;
        assert std.string.rfind_any_of("hello", 2, 2, "aeiou") == null;
        assert std.string.rfind_any_of("hello", 3, 2, "aeiou") == 4;
        assert std.string.rfind_any_of("hello", "") == null;

        assert std.string.find_not_of("hello", "aeiou") == 0;
        assert std.string.find_not_of("hello", 3, "aeiou") == 3;
        assert std.string.find_not_of("hello", 4, "aeiou") == null;
        assert std.string.find_not_of("hello", 1, 2, "aeiou") == 2;
        assert std.string.find_not_of("hello", 2, 2, "aeiou") == 2;
        assert std.string.find_not_of("hello", 4, 2, "aeiou") == null;
        assert std.string.find_not_of("hello", "") == 0;

        assert std.string.rfind_not_of("hello", "aeiou") == 3;
        assert std.string.rfind_not_of("hello", 3, "aeiou") == 3;
        assert std.string.rfind_not_of("hello", 4, "aeiou") == null;
        assert std.string.rfind_not_of("hello", 1, 2, "aeiou") == 2;
        assert std.string.rfind_not_of("hello", 2, 2, "aeiou") == 3;
        assert std.string.rfind_not_of("hello", 4, 2, "aeiou") == null;
        assert std.string.rfind_not_of("hello", "") == 4;

        assert std.string.reverse("") == "";
        assert std.string.reverse("h") == "h";
        assert std.string.reverse("he") == "eh";
        assert std.string.reverse("hel") == "leh";
        assert std.string.reverse("hell") == "lleh";
        assert std.string.reverse("hello") == "olleh";

        assert std.string.trim("hello") == "hello";
        assert std.string.trim("   hello \t") == "hello";
        assert std.string.trim("\t hello   ") == "hello";
        assert std.string.trim("hello", "hlo") == "e";

        assert std.string.ltrim("hello") == "hello";
        assert std.string.ltrim("   hello \t") == "hello \t";
        assert std.string.ltrim("\t hello   ") == "hello   ";
        assert std.string.ltrim("hello", "hlo") == "ello";

        assert std.string.rtrim("hello") == "hello";
        assert std.string.rtrim("   hello \t") == "   hello";
        assert std.string.rtrim("\t hello   ") == "\t hello";
        assert std.string.rtrim("hello", "hlo") == "he";

        assert std.string.lpad("hello", -9, "#") == "hello";
        assert std.string.lpad("hello", 10, "#") == "#####hello";
        assert std.string.lpad("hello", 10, "#!") == "#!#!hello";

        assert std.string.rpad("hello", -9, "#") == "hello";
        assert std.string.rpad("hello", 10, "#") == "hello#####";
        assert std.string.rpad("hello", 10, "#!") == "hello#!#!";

        assert std.string.to_upper("") == "";
        assert std.string.to_upper("hElLo") == "HELLO";
        assert std.string.to_upper("HELLO") == "HELLO";

        assert std.string.to_lower("") == "";
        assert std.string.to_lower("hElLo") == "hello";
        assert std.string.to_lower("hello") == "hello";

        assert std.string.explode("", "``") == [ ];
        assert std.string.explode("aa", "``") == [ "aa" ];
        assert std.string.explode("aa``bb", "``") == [ "aa", "bb" ];
        assert std.string.explode("aa``bb``cc", "``") == [ "aa", "bb", "cc" ];
        assert std.string.explode("", "``", 2) == [ ];
        assert std.string.explode("aa", "``", 2) == [ "aa" ];
        assert std.string.explode("aa``bb", "``", 2) == [ "aa", "bb" ];
        assert std.string.explode("aa``bb``cc", "``", 2) == [ "aa", "bb``cc" ];

        assert std.string.implode([ ], "``") == "";
        assert std.string.implode([ "aa" ], "``") == "aa";
        assert std.string.implode([ "aa", "bb" ], "``") == "aa``bb";
        assert std.string.implode([ "aa", "bb", "cc" ], "``") == "aa``bb``cc";

        assert std.string.hex_encode("hello") == "68656C6C6F";
        assert std.string.hex_encode("hello", true) == "68656c6c6f";
        assert std.string.hex_encode("hello", null, "|") == "68|65|6C|6C|6F";
        assert std.string.hex_encode("hello", true, "|") == "68|65|6c|6c|6f";
        assert std.string.hex_encode("", null, "|") == "";

        assert std.string.hex_decode("68656c6c6f") == "hello";
        assert std.string.hex_decode("68 65 6c 6c 6f") == "hello";
        assert std.string.hex_decode("68 65 06 0c 6c 6f") == "he\x06\x0clo";
        assert std.string.hex_decode("686") == null;
        assert std.string.hex_decode("6865invalid") == null;
        assert std.string.hex_decode("") == "";

        assert std.string.base32_encode("hello") == "NBSWY3DP";
        assert std.string.base32_encode("hello!") == "NBSWY3DPEE======";
        assert std.string.base32_encode("hello?!") == "NBSWY3DPH4QQ====";
        assert std.string.base32_encode("hello", true) == "nbswy3dp";
        assert std.string.base32_encode("hello!", true) == "nbswy3dpee======";
        assert std.string.base32_encode("hello?!", true) == "nbswy3dph4qq====";
        assert std.string.base32_encode("") == "";

        assert std.string.base32_decode("NBSWY3DP") == "hello";
        assert std.string.base32_decode("nbSWy3DpE=======") == null;
        assert std.string.base32_decode("nbSWy3DpEe======") == "hello!";
        assert std.string.base32_decode("nbSWy3DpH4q=====") == null;
        assert std.string.base32_decode("nbSWy3DpH4qQ====") == "hello?!";
        assert std.string.base32_decode("nbSWy3DpIA7SC===") == "hello@?!";
        assert std.string.base32_decode("nbSWy3DpENAD6I==") == null;
        assert std.string.base32_decode("nbSWy3DpENAD6II=") == "hello#@?!";
        assert std.string.base32_decode("nbSWy3Dp   ENAD6II=") == "hello#@?!";
        assert std.string.base32_decode("NBSWY3D") == null;
        assert std.string.base32_decode("NBSWY3DP!invalid") == null;
        assert std.string.base32_decode("") == "";

        assert std.string.base64_encode("hello") == "aGVsbG8=";
        assert std.string.base64_encode("hello!") == "aGVsbG8h";
        assert std.string.base64_encode("hello?!") == "aGVsbG8/IQ==";
        assert std.string.base64_encode("") == "";

        assert std.string.base64_decode("aGVsbG8=") == "hello";
        assert std.string.base64_decode("aGVsbG8h") == "hello!";
        assert std.string.base64_decode("aGVsbG8/I===") == null;
        assert std.string.base64_decode("aGVsbG8/IQ==") == "hello?!";
        assert std.string.base64_decode("aGVs  bG8/IQ==") == "hello?!";
        assert std.string.base64_decode("aGVs  bG8/  IQ==") == "hello?!";
        assert std.string.base64_decode("aGVsbG8=!invalid") == null;
        assert std.string.base64_decode("") == "";

        assert std.string.translate("hello", "el") == "ho";
        assert std.string.translate("hello", "el", "a") == "hao";

        assert std.string.utf8_encode(30002) == "甲";
        assert std.string.utf8_encode(0xFFFFFF) == null;
        assert std.string.utf8_encode(0xFFFFFF, true) == "\uFFFD";
        assert std.string.utf8_encode([ 97, 98, 99, 100, 1040, 1042, 1043, 1044, 30002, 20057, 19993, 19969 ]) == "abcdАВГД甲乙丙丁";
        assert std.string.utf8_encode([ 0xFFFFFF ]) == null;
        assert std.string.utf8_encode([ 0xFFFFFF ], true) == "\uFFFD";

        assert std.string.utf8_decode("abcdАВГД甲乙丙丁") == [ 97, 98, 99, 100, 1040, 1042, 1043, 1044, 30002, 20057, 19993, 19969 ];
        assert std.string.utf8_decode("\xC0\x80\x61") == null;
        assert std.string.utf8_decode("\xC0\x80\x61", true) == [ 192, 128, 97 ];
        assert std.string.utf8_decode("\xFF\xFE\x62") == null;
        assert std.string.utf8_decode("\xFF\xFE\x62", true) == [ 255, 254, 98 ];

        assert std.string.pack8(0x1234) == "\x34";
        assert std.string.pack8([ 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
        assert std.string.unpack8("\x01\x23\x45\x67\x89\xAB\xCD\xEF") == [ 0x01, 0x23, 0x45, 0x67, -0x1p8 | 0x89, -0x1p8 | 0xAB, -0x1p8 | 0xCD, -0x1p8 | 0xEF ];

        assert std.string.pack_16be(0x1234) == "\x12\x34";
        assert std.string.pack_16be([ 0x0123, 0x4567, 0x89AB, 0xCDEF ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
        assert std.string.unpack_16be("\x01\x23\x45\x67\x89\xAB\xCD\xEF") == [ 0x0123, 0x4567, -0x1p16 | 0x89AB, -0x1p16 | 0xCDEF ];

        assert std.string.pack_16le(0x1234) == "\x34\x12";
        assert std.string.pack_16le([ 0x0123, 0x4567, 0x89AB, 0xCDEF ]) == "\x23\x01\x67\x45\xAB\x89\xEF\xCD";
        assert std.string.unpack_16le("\x23\x01\x67\x45\xAB\x89\xEF\xCD") == [ 0x0123, 0x4567, -0x1p16 | 0x89AB, -0x1p16 | 0xCDEF ];

        assert std.string.pack_32be(0x12345678) == "\x12\x34\x56\x78";
        assert std.string.pack_32be([ 0x01234567, 0x89ABCDEF ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF";
        assert std.string.unpack_32be("\x01\x23\x45\x67\x89\xAB\xCD\xEF") == [ 0x01234567, -0x1p32 | 0x89ABCDEF ];

        assert std.string.pack_32le(0x12345678) == "\x78\x56\x34\x12";
        assert std.string.pack_32le([ 0x01234567, 0x89ABCDEF ]) == "\x67\x45\x23\x01\xEF\xCD\xAB\x89";
        assert std.string.unpack_32le("\x67\x45\x23\x01\xEF\xCD\xAB\x89") == [ 0x01234567, -0x1p32 | 0x89ABCDEF ];

        assert std.string.pack_64be(0x123456789ABCDEF0) == "\x12\x34\x56\x78\x9A\xBC\xDE\xF0";
        assert std.string.pack_64be([ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ]) == "\x01\x23\x45\x67\x89\xAB\xCD\xEF\x7E\xDC\xBA\x98\x76\x54\x32\x10";
        assert std.string.unpack_64be("\x01\x23\x45\x67\x89\xAB\xCD\xEF\x7E\xDC\xBA\x98\x76\x54\x32\x10") == [ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ];

        assert std.string.pack_64le(0x123456789ABCDEF0) == "\xF0\xDE\xBC\x9A\x78\x56\x34\x12";
        assert std.string.pack_64le([ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ]) == "\xEF\xCD\xAB\x89\x67\x45\x23\x01\x10\x32\x54\x76\x98\xBA\xDC\x7E";
        assert std.string.unpack_64le("\xEF\xCD\xAB\x89\x67\x45\x23\x01\x10\x32\x54\x76\x98\xBA\xDC\x7E") == [ 0x0123456789ABCDEF, 0x7EDCBA9876543210 ];

        assert std.string.format("1$$2") == "1$2";
        assert std.string.format("hello $1 $2", "world", '!') == "hello world !";
        assert std.string.format("${1} + $1 = ${2}", 5, 10) == "5 + 5 = 10";
        assert std.string.format("funny $0 string") == "funny funny $0 string string";
        assert std.string.format("$2345", 'x', 'y') == "y345";
      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global);
  }
