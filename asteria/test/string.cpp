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

        assert std.string.replace_slice("hello", -2, 1, "01234", 1) == "hel1234o";
        assert std.string.replace_slice("hello", -2, 1, "01234", 1, 2) == "hel12o";
        assert std.string.replace_slice("hello", -2, 1, "01234", -3) == "hel234o";
        assert std.string.replace_slice("hello", -2, 1, "01234", -3, 2) == "hel23o";

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
        assert std.string.find("hello", "") == 0;
        assert std.string.find("hello", 2, "") == 2;
        assert std.string.find("hello", 2, 0, "") == 2;

        assert std.string.rfind("hello hello world", "lo") == 9;
        assert std.string.rfind("hello hello world", 3, "lo") == 9;
        assert std.string.rfind("hello hello world", 4, "lo") == 9;
        assert std.string.rfind("hello hello world", 3, 7, "lo") == 3;
        assert std.string.rfind("hello hello world", 3, 8, "lo") == 9;
        assert std.string.rfind("hello hello world", 4, 6, "lo") == null;
        assert std.string.rfind("hello hello world", 5, 6, "lo") == 9;
        assert std.string.rfind("hello", "") == 5;
        assert std.string.rfind("hello", 2, "") == 5;
        assert std.string.rfind("hello", 2, 0, "") == 2;

        assert std.string.replace("hello hello world", "llo", "####") == "he#### he#### world";
        assert std.string.replace("hello hello world", 2, "llo", "####") == "he#### he#### world";
        assert std.string.replace("hello hello world", 3, "llo", "####") == "hello he#### world";
        assert std.string.replace("hello hello world", 8, "llo", "####") == "hello he#### world";
        assert std.string.replace("hello hello world", 9, "llo", "####") == "hello hello world";
        assert std.string.replace("hello hello world", 2, 7, "llo", "####") == "he#### hello world";
        assert std.string.replace("hello hello world", 2, 8, "llo", "####") == "he#### hello world";
        assert std.string.replace("hello hello world", 2, 9, "llo", "####") == "he#### he#### world";
        assert std.string.replace("hello hello world", 3, 7, "llo", "####") == "hello hello world";
        assert std.string.replace("hello hello world", 4, 7, "llo", "####") == "hello he#### world";
        assert std.string.replace("abab", "ab", "abab") == "abababab";
        assert std.string.replace("abab", 1, "ab", "abab") == "ababab";
        assert std.string.replace("abab", 2, "ab", "abab") == "ababab";
        assert std.string.replace("abab", 3, "ab", "abab") == "abab";
        assert std.string.replace("hello", "", "X") == "XhXeXlXlXoX";
        assert std.string.replace("hello", 2, "", "X") == "heXlXlXoX";
        assert std.string.replace("hello", 2, 2, "", "X") == "heXlXlXo";

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

        assert std.string.triml("hello") == "hello";
        assert std.string.triml("   hello \t") == "hello \t";
        assert std.string.triml("\t hello   ") == "hello   ";
        assert std.string.triml("hello", "hlo") == "ello";

        assert std.string.trimr("hello") == "hello";
        assert std.string.trimr("   hello \t") == "   hello";
        assert std.string.trimr("\t hello   ") == "\t hello";
        assert std.string.trimr("hello", "hlo") == "he";

        assert std.string.padl("hello", -9, "#") == "hello";
        assert std.string.padl("hello", 10, "#") == "#####hello";
        assert std.string.padl("hello", 10, "#!") == "#!#!hello";

        assert std.string.padr("hello", -9, "#") == "hello";
        assert std.string.padr("hello", 10, "#") == "hello#####";
        assert std.string.padr("hello", 10, "#!") == "hello#!#!";

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
        assert std.string.hex_encode("hello", "|") == "68|65|6C|6C|6F";
        assert std.string.hex_encode("", "|") == "";

        assert std.string.hex_decode("68656c6c6f") == "hello";
        assert std.string.hex_decode("68 65 6c 6c 6f") == "hello";
        assert std.string.hex_decode("68 65 06 0c 6c 6f") == "he\x06\x0clo";
        assert catch( std.string.hex_decode("686") ) != null;
        assert catch( std.string.hex_decode("6865invalid") ) != null;
        assert std.string.hex_decode("") == "";

        assert std.string.base32_encode("hello") == "NBSWY3DP";
        assert std.string.base32_encode("hello!") == "NBSWY3DPEE======";
        assert std.string.base32_encode("hello?!") == "NBSWY3DPH4QQ====";
        assert std.string.base32_encode("") == "";

        assert std.string.base32_decode("NBSWY3DP") == "hello";
        assert catch( std.string.base32_decode("nbSWy3DpE=======") ) != null;
        assert std.string.base32_decode("nbSWy3DpEe======") == "hello!";
        assert catch( std.string.base32_decode("nbSWy3DpH4q=====") ) != null;
        assert std.string.base32_decode("nbSWy3DpH4qQ====") == "hello?!";
        assert std.string.base32_decode("nbSWy3DpIA7SC===") == "hello@?!";
        assert catch( std.string.base32_decode("nbSWy3DpENAD6I==") ) != null;
        assert std.string.base32_decode("nbSWy3DpENAD6II=") == "hello#@?!";
        assert std.string.base32_decode("nbSWy3Dp   ENAD6II=") == "hello#@?!";
        assert catch( std.string.base32_decode("NBSWY3D") ) != null;
        assert catch( std.string.base32_decode("NBSWY3DP!invalid") ) != null;
        assert std.string.base32_decode("") == "";

        assert std.string.base64_encode("hello") == "aGVsbG8=";
        assert std.string.base64_encode("hello!") == "aGVsbG8h";
        assert std.string.base64_encode("hello?!") == "aGVsbG8/IQ==";
        assert std.string.base64_encode("") == "";

        assert std.string.base64_decode("aGVsbG8=") == "hello";
        assert std.string.base64_decode("aGVsbG8h") == "hello!";
        assert catch( std.string.base64_decode("aGVsbG8/I===") ) != null;
        assert std.string.base64_decode("aGVsbG8/IQ==") == "hello?!";
        assert std.string.base64_decode("aGVs  bG8/IQ==") == "hello?!";
        assert std.string.base64_decode("aGVs  bG8/  IQ==") == "hello?!";
        assert catch( std.string.base64_decode("aGVsbG8=!invalid") ) != null;
        assert std.string.base64_decode("") == "";

        assert std.string.url_encode("") == "";
        assert std.string.url_encode("abcdАВГД甲乙丙丁") == "abcd%D0%90%D0%92%D0%93%D0%94%E7%94%B2%E4%B9%99%E4%B8%99%E4%B8%81";
        assert std.string.url_encode(" \t`~!@#$%^&*()_+-={}|[]\\:\";\'<>?,./") == "%20%09%60~%21%40%23%24%25%5E%26%2A%28%29_%2B-%3D%7B%7D%7C%5B%5D%5C%3A%22%3B%27%3C%3E%3F%2C.%2F";

        assert std.string.url_decode("") == "";
        assert std.string.url_decode("abcd1234%D0%90%D0%92%D0%93%D0%94%E7%94%B2%E4%B9%99%E4%B8%99%E4%B8%81") == "abcd1234АВГД甲乙丙丁";
        assert std.string.url_decode(":/?#[]@!$&'()*+,;=-._~") == ":/?#[]@!$&'()*+,;=-._~";
        assert catch( std.string.url_decode("not valid") ) != null;
        assert catch( std.string.url_decode("无效的") ) != null;

        assert std.string.url_encode_query("") == "";
        assert std.string.url_encode_query("abcdАВГД甲乙丙丁") == "abcd%D0%90%D0%92%D0%93%D0%94%E7%94%B2%E4%B9%99%E4%B8%99%E4%B8%81";
        assert std.string.url_encode_query(" \t`~!@#$%^&*()_+-={}|[]\\:\";\'<>?,./") == "+%09%60~!@%23$%25%5E&*()_%2B-%3D%7B%7D%7C%5B%5D%5C:%22;\'%3C%3E?,./";

        assert std.string.url_decode_query("") == "";
        assert std.string.url_decode_query("abcd1234%D0%90%D0%92%D0%93%D0%94%E7%94%B2%E4%B9%99%E4%B8%99%E4%B8%81") == "abcd1234АВГД甲乙丙丁";
        assert std.string.url_decode_query(":/?#[]@!$&'()*+,;=-._~") == ":/?#[]@!$&'()* ,;=-._~";
        assert catch( std.string.url_decode_query("not valid") ) != null;
        assert catch( std.string.url_decode_query("无效的") ) != null;

        assert std.string.translate("hello", "el") == "ho";
        assert std.string.translate("hello", "el", "a") == "hao";

        assert std.string.utf8_validate("abcdАВГД甲乙丙丁") == true;
        assert std.string.utf8_validate("\xC0\x80\x61") == false;
        assert std.string.utf8_validate("\xFF\xFE\x62") == false;

        assert std.string.utf8_encode(30002) == "甲";
        assert catch( std.string.utf8_encode(0xFFFFFF) ) != null;
        assert std.string.utf8_encode(0xFFFFFF, true) == "\uFFFD";
        assert std.string.utf8_encode([ 97, 98, 99, 100, 1040, 1042, 1043, 1044, 30002, 20057, 19993, 19969 ]) == "abcdАВГД甲乙丙丁";
        assert catch( std.string.utf8_encode([ 0xFFFFFF ]) ) != null;
        assert std.string.utf8_encode([ 0xFFFFFF ], true) == "\uFFFD";

        assert std.string.utf8_decode("abcdАВГД甲乙丙丁") == [ 97, 98, 99, 100, 1040, 1042, 1043, 1044, 30002, 20057, 19993, 19969 ];
        assert catch( std.string.utf8_decode("\xC0\x80\x61") ) != null;
        assert std.string.utf8_decode("\xC0\x80\x61", true) == [ 192, 128, 97 ];
        assert catch( std.string.utf8_decode("\xFF\xFE\x62") ) != null;
        assert std.string.utf8_decode("\xFF\xFE\x62", true) == [ 255, 254, 98 ];

        assert std.string.format("1$$2") == "1$2";
        assert std.string.format("hello $1 $2", "world", '!') == "hello world !";
        assert std.string.format("${1} + $1 = ${2}", 5, 10) == "5 + 5 = 10";
        assert std.string.format("funny $0 string") == "funny funny $0 string string";
        assert std.string.format("$2345", 'x', 'y') == "y345";

        assert std.string.pcre_find("a11b2c333d4e555", '\d+\w') == [1,3];
        assert std.string.pcre_find("a11b2c333d4e555", '\d{3}\w') == [6,4];
        assert std.string.pcre_find("a11b2c333d4e555", '\d{34}\w') == null;
        assert std.string.pcre_find("a11b2c333d4e555", '(\w\d+)*') == [0,15];

        assert std.string.pcre_match("a11b2c333d4e555", '\d+\w') == [ "11b" ];
        assert std.string.pcre_match("a11b2c333d4e555", '\d{34}\w') == null;
        assert std.string.pcre_match("a11b2c333d4e555", '(?:\w\d+)*') == [ "a11b2c333d4e555" ];
        assert std.string.pcre_match("a11b2c333d4e555", '(\w\d*)(\w\d+)(\w\d*\w\d+)(\w\d+)') == [ "a11b2c333d4e555", "a11", "b2", "c333d4", "e555" ];
        assert std.string.pcre_match("a11b2c333d4e555", '(\d+\w)(22)?(\d+\w)') == [ "11b2c", "11b", null, "2c" ];

        var m = std.string.pcre_named_match("a11b2c333d4e555", '(?:\w\d+)*');
        assert typeof m == "object";
        assert countof m == 0;

        assert std.string.pcre_named_match("a11b2c333d4e555", '\d{34}\w') == null;

        m = std.string.pcre_named_match("a11b2c333d4e555", '(\w\d*)(?<xx>\w\d+)(\w\d*\w\d+)(?<yy>\w\d+)');
        assert typeof m == "object";
        assert countof m == 2;
        assert m.xx == "b2";
        assert m.yy == "e555";

        m = std.string.pcre_named_match("a11b2c333d4e555", '(?<xx>\d+\w)(?<yy>22)?(?<zz>\d+\w)');
        assert typeof m == "object";
        assert countof m == 3;
        assert m.xx == "11b";
        assert m.yy == null;
        assert m.zz == "2c";

        assert std.string.pcre_replace("a11b2c333d4e555", '\d+\w', '*') == "a*****";
        assert std.string.pcre_replace("a11b2c333d4e555", '(\d{3})(\w)', '$2$1') == "a11b2cd3334e555";
        assert std.string.pcre_replace("a11b2c333d4e555", '\d{34}\w', '#') == "a11b2c333d4e555";

        var M_dw = std.string.PCRE('\d+\w');
        assert M_dw.find("a11b2c333d4e555") == [1,3];
        assert M_dw.match("a11b2c333d4e555") == [ "11b" ];
        assert M_dw.replace("a11b2c333d4e555", '*') == "a*****";

        var M_nn = std.string.PCRE('(?<xx>\d+\w)(?<yy>22)?(?<zz>\d+\w)');
        m = M_nn.named_match("a11b2c333d4e555");
        assert typeof m == "object";
        assert countof m == 3;
        assert m.xx == "11b";
        assert m.yy == null;
        assert m.zz == "2c";

        assert std.string.iconv("UTF-16", "") == "";
        assert catch( std.string.iconv("invalid encoding", "") ) != null;
        assert catch( std.string.iconv("UTF-8", "", "invalid encoding") ) != null;
        assert std.string.iconv("GB18030", "CAT猫") == "\x43\x41\x54\xC3\xA8";
        assert std.string.iconv("SHIFT-JIS", "CAT猫") == "\x43\x41\x54\x94\x4C";
        assert std.string.iconv("UTF-8", "\x43\x41\x54\xC3\xA8", "GB18030") == "CAT猫";
        assert std.string.iconv("UTF-8", "\x43\x41\x54\x94\x4C", "SHIFT-JIS") == "CAT猫";

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
