// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/token.hpp"
#include "../src/parser_result.hpp"

using namespace Asteria;

int main(){
	Vector<Token> tokens;
	auto result = tokenize_line(tokens, 42, Cow_string::shallow("hh+++   if <<<->>>>>\"\\u55b5b喵\"/* - 0x1_7.8:4p+4  .false ;-42e13"));
	ASTERIA_TEST_CHECK(result);
	ASTERIA_TEST_CHECK(tokens.at(0).get<Token::S_identifier>().id == "hh");
	ASTERIA_TEST_CHECK(tokens.at(1).get<Token::S_punctuator>().punct == Token::punctuator_inc);
	ASTERIA_TEST_CHECK(tokens.at(2).get<Token::S_punctuator>().punct == Token::punctuator_add);
	ASTERIA_TEST_CHECK(tokens.at(3).get<Token::S_keyword>().keyword == Token::keyword_if);
	ASTERIA_TEST_CHECK(tokens.at(4).get<Token::S_punctuator>().punct == Token::punctuator_sll);
	ASTERIA_TEST_CHECK(tokens.at(5).get<Token::S_punctuator>().punct == Token::punctuator_sub);
	ASTERIA_TEST_CHECK(tokens.at(6).get<Token::S_punctuator>().punct == Token::punctuator_srl);
	ASTERIA_TEST_CHECK(tokens.at(7).get<Token::S_punctuator>().punct == Token::punctuator_sra);
	ASTERIA_TEST_CHECK(tokens.at(8).get<Token::S_string_literal>().value == "喵\x62\xe5\x96\xb5");
	ASTERIA_TEST_CHECK(tokens.at(9).get<Token::S_punctuator>().punct == Token::punctuator_div);
	ASTERIA_TEST_CHECK(tokens.at(10).get<Token::S_punctuator>().punct == Token::punctuator_mul);
	ASTERIA_TEST_CHECK(tokens.at(11).get<Token::S_punctuator>().punct == Token::punctuator_sub);
	ASTERIA_TEST_CHECK(tokens.at(12).get<Token::S_double_literal>().value == 376.25);
	ASTERIA_TEST_CHECK(tokens.at(13).get<Token::S_punctuator>().punct == Token::punctuator_dot);
	ASTERIA_TEST_CHECK(tokens.at(14).get<Token::S_keyword>().keyword == Token::keyword_false);
	ASTERIA_TEST_CHECK(tokens.at(15).get<Token::S_punctuator>().punct == Token::punctuator_semicolon);
	ASTERIA_TEST_CHECK(tokens.at(16).get<Token::S_integer_literal>().value == -420000000000000);
	ASTERIA_TEST_CHECK(tokens.size() == 17);
}
