// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/token_stream.hpp"
#include "../src/compiler/token.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    Token_Stream ts;
    std::stringbuf buf(
      R"__(#!some shebang
        hh+++
        if <<<->>>>>"\u55b5b喵"/
        * - 0x01`7.8`4p+4  // comments
        .false/*more
        comments*/;/*yet more*/-42e13
      )__");
    auto r = ts.load(buf, rocket::sref("dummy_file"), { });
    ASTERIA_TEST_CHECK(r);
    ASTERIA_TEST_CHECK(buf.sgetc() == std::char_traits<char>::eof());

    auto p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_identifier() == "hh");
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_inc);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_add);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_keyword() == Token::keyword_if);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_sll);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_sub);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_srl);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_sra);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_string_literal() == "喵\x62\xe5\x96\xb5");
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_div);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_mul);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_sub);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_real_literal() == 376.25);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_dot);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_keyword() == Token::keyword_false);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_punctuator() == Token::punctuator_semicol);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(p);
    ASTERIA_TEST_CHECK(p->as_integer_literal() == -420000000000000);
    ts.shift();

    p = ts.peek_opt();
    ASTERIA_TEST_CHECK(!p);
  }
