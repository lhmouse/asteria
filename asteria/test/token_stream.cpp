// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/token_stream.hpp"
#include "../asteria/src/compiler/token.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    Token_Stream ts;
    std::istringstream iss("#!meow \n"
                           "hh+++ \n"
                           " if <<<->>>>>\"\\u55b5b喵\"/\n"
                           "* - 0x`01`7.8`4p+4  // comments\n"
                           ".false/*more\n"
                           "comments*/;/*yet more*/-42e13");
    auto r = ts.load(iss, rocket::sref("dummy_file"), Parser_Options());
    ASTERIA_TEST_CHECK(r);
    ASTERIA_TEST_CHECK(iss.eof());
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_identifier>().name == "hh");
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_inc);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_add);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_keyword>().keyword == Token::keyword_if);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_sll);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_sub);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_srl);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_sra);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_string_literal>().value == "喵\x62\xe5\x96\xb5");
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_div);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_mul);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_sub);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_real_literal>().value == 376.25);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_dot);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_keyword>().keyword == Token::keyword_false);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_punctuator>().punct == Token::punctuator_semicol);
    ASTERIA_TEST_CHECK(ts.shift().check<Token::S_integer_literal>().value == -420000000000000);
    ASTERIA_TEST_CHECK(ts.peek_opt() == nullptr);
  }
