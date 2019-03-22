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

    auto p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_identifier>().name == "hh");
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_inc);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_add);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_keyword>().keyword == Token::keyword_if);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_sll);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_sub);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_srl);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_sra);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_string_literal>().value == "喵\x62\xe5\x96\xb5");
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_div);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_mul);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_sub);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_real_literal>().value == 376.25);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_dot);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_keyword>().keyword == Token::keyword_false);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_punctuator>().punct == Token::punctuator_semicol);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->check<Token::S_integer_literal>().value == -420000000000000);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(!p);
  }
