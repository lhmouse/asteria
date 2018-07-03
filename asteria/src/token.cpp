// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "token.hpp"
#include "parser_result.hpp"
#include "utilities.hpp"
#include <algorithm>
#include <limits>
#include <cfloat>
#include <cmath>

namespace Asteria {

Token::Token(Token &&) noexcept = default;
Token & Token::operator=(Token &&) noexcept = default;
Token::~Token() = default;

namespace {
	Parser_result do_validate_code_point_utf8(std::size_t line, Cow_string_cref str, std::size_t column){
		// Get a UTF code point.
		static constexpr unsigned char s_length_table[32] = {
			1, 1, 1, 1, 1, 1, 1, 1, // 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
			1, 1, 1, 1, 1, 1, 1, 1, // 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78,
			0, 0, 0, 0, 0, 0, 0, 0, // 0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8,
			2, 2, 2, 2, 3, 3, 4, 0, // 0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8,
		};
		char32_t code = static_cast<unsigned char>(str.at(column));
		const unsigned length = s_length_table[code >> 3];
		if(length == 0){
			// This leading character is not valid.
			return Parser_result(line, column, 1, Parser_result::error_code_utf8_code_unit_invalid);
		}
		if(length > 1){
			// Unset bits that are not part of the payload.
			code &= static_cast<char32_t>(0xFF) >> length;
			// Accumulate trailing code units.
			if(str.size() - column < length){
				// No enough characters are provided.
				return Parser_result(line, column, str.size() - column, Parser_result::error_code_utf8_code_point_truncated);
			}
			for(unsigned i = 1; i < length; ++i){
				const char32_t next = static_cast<unsigned char>(str.at(column + i));
				if((next & 0xC0) != 0x80){
					// This trailing character is not valid.
					return Parser_result(line, column, i + 1, Parser_result::error_code_utf8_code_unit_invalid);
				}
				code = (code << 6) | (next & 0x3F);
			}
			if((0xD800 <= code) && (code < 0xE000)){
				// Surrogates are not allowed.
				return Parser_result(line, column, length, Parser_result::error_code_utf8_surrogates_disallowed);
			}
			if(code >= 0x110000){
				// Code point value is too large.
				return Parser_result(line, column, length, Parser_result::error_code_utf_code_point_value_too_large);
			}
			// Re-encode it and check for overlong sequences.
			unsigned len_min;
			if(code < 0x80){
				len_min = 1;
			} else if(code < 0x800){
				len_min = 2;
			} else if(code < 0x10000){
				len_min = 3;
			} else {
				len_min = 4;
			}
			if(length != len_min){
				// Overlong sequences are not allowed.
				return Parser_result(line, column, length, Parser_result::error_code_utf8_encoding_overlong);
			}
		}
		return Parser_result(line, column, length, Parser_result::error_code_success);
	}

	bool do_merge_sign(Vector<Token> &tokens_inout, std::size_t line, std::size_t column){
		// The last token must be a positive or negative sign.
		if(tokens_inout.empty()){
			return false;
		}
		const auto &token_sign = tokens_inout.back();
		auto cand_punct = token_sign.get_opt<Token::S_punctuator>();
		if(cand_punct == nullptr){
			return false;
		}
		if((cand_punct->punct != Token::punctuator_add) && (cand_punct->punct != Token::punctuator_sub)){
			return false;
		}
		const bool sign = cand_punct->punct == Token::punctuator_sub;
		// Don't merge them if they are not contiguous.
		if(token_sign.get_source_line() != line){
			return false;
		}
		if(token_sign.get_source_column() + token_sign.get_source_length() != column){
			return false;
		}
		// Don't merge them if the sign token follows a non-punctuator or a punctuator that terminates a postfix expression.
		if(tokens_inout.size() >= 2){
			const auto &token_prev = tokens_inout.rbegin()[1];
			cand_punct = token_prev.get_opt<Token::S_punctuator>();
			if(cand_punct == nullptr){
				return false;
			}
			if((cand_punct->punct == Token::punctuator_inc) || (cand_punct->punct == Token::punctuator_dec)){
				return false;
			}
			if((cand_punct->punct == Token::punctuator_parenth_cl) || (cand_punct->punct == Token::punctuator_bracket_cl)){
				return false;
			}
		}
		// Drop the sign token.
		tokens_inout.pop_back();
		return sign;
	}

	Parser_result do_get_token(Vector<Token> &tokens_out, std::size_t line, Cow_string_cref str, std::size_t column){
		const auto char_head = str.at(column);
		switch(char_head){
		case ' ':  case '\t':  case '\v':  case '\f':  case '\r':  case '\n': {
			// Ignore a series of spaces.
			const auto pos = str.find_first_not_of(" \t\v\f\r\n", column + 1);
			const auto length = std::min(pos, str.size()) - column;
			return Parser_result(line, column, length, Parser_result::error_code_success); }

		case '!':  case '%':  case '&':  case '(':  case ')':  case '*':  case '+':  case ',':
		case '-':  case '.':  case '/':  case ':':  case ';':  case '<':  case '=':  case '>':
		case '?':  case '[':  case ']':  case '^':  case '{':  case '|':  case '}':  case '~': {
			// Get a punctuator.
			struct Punctuator_element {
				char first[7];
				Token::Punctuator second;
			};
			static_assert(sizeof(Punctuator_element) % 8 == 0, "`Punctuator_element` is misaligned.");
			// Remember to keep this table sorted!
			static constexpr Punctuator_element s_punctuator_table[] = {
				{ "!",     Token::punctuator_notl        },
				{ "!=",    Token::punctuator_cmp_ne      },
				{ "%",     Token::punctuator_mod         },
				{ "%=",    Token::punctuator_mod_eq      },
				{ "&",     Token::punctuator_andb        },
				{ "&&",    Token::punctuator_andl        },
				{ "&&=",   Token::punctuator_andl_eq     },
				{ "&=",    Token::punctuator_andb_eq     },
				{ "(",     Token::punctuator_parenth_op  },
				{ ")",     Token::punctuator_parenth_cl  },
				{ "*",     Token::punctuator_mul         },
				{ "*=",    Token::punctuator_mul_eq      },
				{ "+",     Token::punctuator_add         },
				{ "++",    Token::punctuator_inc         },
				{ "+=",    Token::punctuator_add_eq      },
				{ ",",     Token::punctuator_comma       },
				{ "-",     Token::punctuator_sub         },
				{ "--",    Token::punctuator_dec         },
				{ "-=",    Token::punctuator_sub_eq      },
				{ ".",     Token::punctuator_dot         },
				{ "/",     Token::punctuator_div         },
				{ "/=",    Token::punctuator_div_eq      },
				{ ":",     Token::punctuator_colon       },
				{ ";",     Token::punctuator_semicolon   },
				{ "<",     Token::punctuator_cmp_lt      },
				{ "<<",    Token::punctuator_sla         },
				{ "<<<",   Token::punctuator_sll         },
				{ "<<<=",  Token::punctuator_sll_eq      },
				{ "<<=",   Token::punctuator_sla_eq      },
				{ "<=",    Token::punctuator_cmp_lte     },
				{ "=",     Token::punctuator_assign      },
				{ "==",    Token::punctuator_cmp_eq      },
				{ ">",     Token::punctuator_cmp_gt      },
				{ ">=",    Token::punctuator_cmp_gte     },
				{ ">>",    Token::punctuator_sra         },
				{ ">>=",   Token::punctuator_sra_eq      },
				{ ">>>",   Token::punctuator_srl         },
				{ ">>>=",  Token::punctuator_srl_eq      },
				{ "?",     Token::punctuator_condition   },
				{ "[",     Token::punctuator_bracket_op  },
				{ "]",     Token::punctuator_bracket_cl  },
				{ "^",     Token::punctuator_xorb        },
				{ "^=",    Token::punctuator_xorb_eq     },
				{ "{",     Token::punctuator_brace_op    },
				{ "|",     Token::punctuator_orb         },
				{ "|=",    Token::punctuator_orb_eq      },
				{ "||",    Token::punctuator_orl         },
				{ "||=",   Token::punctuator_orl_eq      },
				{ "}",     Token::punctuator_brace_cl    },
				{ "~",     Token::punctuator_notb        },
			};
			struct Punctuator_comparator {
				bool operator()(const Punctuator_element &lhs, const Punctuator_element &rhs) const noexcept {
					return std::char_traits<char>::compare(lhs.first, rhs.first, sizeof(lhs.first)) < 0;
				}
				bool operator()(char lhs, const Punctuator_element &rhs) const noexcept {
					return std::char_traits<char>::lt(lhs, rhs.first[0]);
				}
				bool operator()(const Punctuator_element &lhs, char rhs) const noexcept {
					return std::char_traits<char>::lt(lhs.first[0], rhs);
				}
			};
#ifdef ROCKET_DEBUG
			ROCKET_ASSERT(std::is_sorted(std::begin(s_punctuator_table), std::end(s_punctuator_table), Punctuator_comparator()));
#endif
			const auto range = std::equal_range(std::begin(s_punctuator_table), std::end(s_punctuator_table), char_head, Punctuator_comparator());
			// For two elements X and Y, if X is in front of Y, then X is potential a prefix of Y.
			// Traverse the range backwards to prevent premature matches, as a token is defined to be the longest valid character sequence.
			for(auto it = range.second; it != range.first; --it){
				const auto &pair = it[-1];
				const auto length = std::char_traits<char>::length(pair.first);
				if((str.size() - column >= length) && (std::char_traits<char>::compare(pair.first, str.data() + column, length) == 0)){
					// Push a punctuator.
					Token::S_punctuator token_p = { pair.second };
					tokens_out.emplace_back(line, column, length, std::move(token_p));
					return Parser_result(line, column, length, Parser_result::error_code_success);
				}
			}
			// There must be a bug in the punctuator table.
			ASTERIA_DEBUG_LOG("The punctuator `", char_head, "` is not handled. This is probably a bug. Please report.");
			std::terminate(); }

		case '\"':  case '\'': {
			// Get a string literal.
			const auto body_begin = column + 1;
			auto body_end = str.find(char_head, body_begin);
			Cow_string value;
			if(char_head == '\''){
				// Escape sequences do not have special meanings inside single quotation marks.
				if(body_end == str.npos){
					return Parser_result(line, column, str.size() - column, Parser_result::error_code_string_literal_unclosed);
				}
				// Copy the substring as is.
				value.assign(str, body_begin, body_end - body_begin);
			} else {
				// Escape characters have to be treated specifically, inside double quotation marks.
				// Consequently, this is only an estimated length.
				value.reserve(body_end - body_begin);
				// Translate escape sequences as needed.
				body_end = body_begin;
				for(;;){
					if(body_end >= str.size()){
						return Parser_result(line, column, str.size() - column, Parser_result::error_code_string_literal_unclosed);
					}
					auto char_next = str.at(body_end);
					if(char_next == char_head){
						// Got end of string literal. Finish.
						break;
					}
					if(char_next != '\\'){
						// This character does not start an escape sequence. Copy it as is.
						value.push_back(char_next);
						body_end += 1;
						continue;
					}
					// This character starts an escape sequence.
					unsigned seq_len = 2;
					if(str.size() - body_end < seq_len){
						return Parser_result(line, body_end, str.size() - body_end, Parser_result::error_code_escape_sequence_incomplete);
					}
					char_next = str.at(body_end + 1);
					switch(char_next){
					case '\'':
						value.push_back('\'');
						break;
					case '\"':
						value.push_back('\"');
						break;
					case '\\':
						value.push_back('\\');
						break;
					case '?':
						value.push_back('?');
						break;
					case 'a':
						value.push_back('\a');
						break;
					case 'b':
						value.push_back('\b');
						break;
					case 'f':
						value.push_back('\f');
						break;
					case 'n':
						value.push_back('\n');
						break;
					case 'r':
						value.push_back('\r');
						break;
					case 't':
						value.push_back('\t');
						break;
					case 'v':
						value.push_back('\v');
						break;
					case '0':
						value.push_back(0);
						break;
					case 'Z':
						value.push_back(0x1A);
						break;
					case 'e':
						value.push_back(0x1B);
						break;

					case 'U': {
						seq_len += 2;
						// Fallthrough.
					case 'u':
						seq_len += 2;
						// Fallthrough.
					case 'x':
						seq_len += 2;
						// Read hex digits.
						if(str.size() - body_end < seq_len){
							return Parser_result(line, body_end, str.size() - body_end, Parser_result::error_code_escape_sequence_incomplete);
						}
						char32_t code = 0;
						for(unsigned i = 2; i < seq_len; ++i){
							const auto digit = str.at(body_end + i);
							static constexpr char s_hex_table[] = "00112233445566778899AaBbCcDdEeFf";
							const auto ptr = std::char_traits<char>::find(s_hex_table, 32, digit);
							if(ptr == nullptr){
								return Parser_result(line, body_end, i, Parser_result::error_code_escape_sequence_invalid_hex);
							}
							const auto digit_value = static_cast<unsigned char>((ptr - s_hex_table) / 2);
							code = (code << 4) | digit_value;
						}
						if((0xD800 <= code) && (code < 0xE000)){
							// Surrogates are not allowed.
							return Parser_result(line, body_end, seq_len, Parser_result::error_code_utf8_surrogates_disallowed);
						}
						if(code >= 0x110000){
							// Code point value is too large.
							return Parser_result(line, body_end, seq_len, Parser_result::error_code_utf_code_point_value_too_large);
						}
						// Encode it.
						const auto encode_one = [&](unsigned shift, unsigned mask){
							value.push_back(static_cast<char>((~mask << 1) | ((code >> shift) & mask)));
						};
						if(code < 0x80){
							encode_one( 0, 0xFF);
						} else if(code < 0x800){
							encode_one( 6, 0x1F);
							encode_one( 0, 0x3F);
						} else if(code < 0x10000){
							encode_one(12, 0x0F);
							encode_one( 6, 0x3F);
							encode_one( 0, 0x3F);
						} else {
							encode_one(18, 0x07);
							encode_one(12, 0x3F);
							encode_one( 6, 0x3F);
							encode_one( 0, 0x3F);
						}
						break; }

					default:
						// Fail if this escape sequence cannot be identified.
						return Parser_result(line, body_end, seq_len, Parser_result::error_code_escape_sequence_unknown);
					}
					body_end += seq_len;
				}
			}
			const auto length = body_end + 1 - column;
			// Push the string unescaped.
			Token::S_string_literal token_s = { std::move(value) };
			tokens_out.emplace_back(line, column, length, std::move(token_s));
			return Parser_result(line, column, length, Parser_result::error_code_success); }

		case '0':  case '1':  case '2':  case '3':  case '4':
		case '5':  case '6':  case '7':  case '8':  case '9': {
			// Get a numeric literal.
			static constexpr char s_numeric_table[] = "_:00112233445566778899AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
			static constexpr unsigned s_delim_count = 2;
			// Declare everything that will be calculated latter.
			// 0) The integral part is required. The fractional and exponent parts are optional.
			// 1) If `frac_begin` equals `int_end` then there is no fractional part.
			// 2) If `exp_begin` equals `frac_end` then there is no exponent part.
			unsigned radix;
			std::size_t int_begin, int_end;
			std::size_t frac_begin, frac_end;
			unsigned exp_base;
			bool exp_sign;
			std::size_t exp_begin, exp_end;
			// Check for radix prefixes.
			radix = 10;
			int_begin = column;
			char char_next;
			if(char_head == '0'){
				// Do not use `str.at()` here as `int_begin + 1` may exceed `str.size()`.
				char_next = str[int_begin + 1];
				if((char_next == 'B') || (char_next == 'b')){
					radix = 2;
					int_begin += 2;
				} else if((char_next == 'X') || (char_next == 'x')){
					radix = 16;
					int_begin += 2;
				}
			}
			auto pos = str.find_first_not_of(s_numeric_table, int_begin, s_delim_count + radix * 2);
			int_end = std::min(pos, str.size());
			if(int_begin == int_end){
				return Parser_result(line, column, int_end - column, Parser_result::error_code_numeric_literal_incomplete);
			}
			// Look for the fractional part.
			frac_begin = int_end;
			frac_end = frac_begin;
			char_next = str[int_end];
			if(char_next == '.'){
				frac_begin += 1;
				pos = str.find_first_not_of(s_numeric_table, frac_begin, s_delim_count + radix * 2);
				frac_end = std::min(pos, str.size());
				if(frac_begin == frac_end){
					return Parser_result(line, column, frac_end - column, Parser_result::error_code_numeric_literal_incomplete);
				}
			}
			// Look for the exponent.
			exp_base = 0;
			exp_sign = false;
			exp_begin = frac_end;
			exp_end = exp_begin;
			char_next = str[frac_end];
			if((char_next == 'E') || (char_next == 'e')){
				exp_base = 10;
				exp_begin += 1;
			} else if((char_next == 'P') || (char_next == 'p')){
				exp_base = 2;
				exp_begin += 1;
			}
			if(exp_base != 0){
				char_next = str[exp_begin];
				if(char_next == '+'){
					exp_sign = false;
					exp_begin += 1;
				} else if(char_next == '-'){
					exp_sign = true;
					exp_begin += 1;
				}
				pos = str.find_first_not_of(s_numeric_table, exp_begin, s_delim_count + 20);
				exp_end = std::min(pos, str.size());
				if(exp_begin == exp_end){
					return Parser_result(line, column, exp_end - column, Parser_result::error_code_numeric_literal_incomplete);
				}
			}
			// Disallow suffixes. Suffixes such as `ll`, `u` and `f` are used in C and C++ to specify the types of numeric literals.
			// Since we make no use of them, we just reserve them for further use for good.
			pos = str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.", exp_end);
			pos = std::min(pos, str.size());
			if(pos != exp_end){
				return Parser_result(line, exp_end, pos - exp_end, Parser_result::error_code_numeric_literal_suffixes_disallowed);
			}
			const auto length = pos - column;
			// Parse the exponent.
			std::int32_t exp = 0;
			for(pos = exp_begin; pos != exp_end; ++pos){
				const auto ptr = std::char_traits<char>::find(s_numeric_table + s_delim_count, 20, str.at(pos));
				if(ptr == nullptr){
					continue;
				}
				const auto digit_value = static_cast<unsigned char>((ptr - s_numeric_table - s_delim_count) / 2);
				const auto bound = (std::numeric_limits<std::int32_t>::max() - digit_value) / 10;
				if(exp > bound){
					return Parser_result(line, column, length, Parser_result::error_code_numeric_literal_exponent_overflow);
				}
				exp *= 10;
				exp += digit_value;
			}
			if(exp_sign){
				exp = -exp;
			}
			if(frac_begin == int_end){
				// Parse the literal as an integer.
				Unsigned_integer value = 0;
				for(pos = int_begin; pos != int_end; ++pos){
					const auto ptr = std::char_traits<char>::find(s_numeric_table + s_delim_count, radix * 2, str.at(pos));
					if(ptr == nullptr){
						continue;
					}
					const auto digit_value = static_cast<unsigned char>((ptr - s_numeric_table - s_delim_count) / 2);
					const auto bound = (std::numeric_limits<Unsigned_integer>::max() - digit_value) / radix;
					if(value > bound){
						return Parser_result(line, column, length, Parser_result::error_code_integer_literal_overflow);
					}
					value *= radix;
					value += digit_value;
				}
				if(value != 0){
					if(exp < 0){
						return Parser_result(line, column, length, Parser_result::error_code_integer_literal_exponent_negative);
					}
					for(std::int32_t i = 0; i < exp; ++i){
						const auto bound = std::numeric_limits<decltype(value)>::max() / exp_base;
						if(value > bound){
							return Parser_result(line, column, length, Parser_result::error_code_integer_literal_overflow);
						}
						value *= exp_base;
					}
				}
				if(do_merge_sign(tokens_out, line, column)){
					value = -value;
				}
				Token::S_integer_literal token_i = { static_cast<Signed_integer>(value) };
				tokens_out.emplace_back(line, column, length, std::move(token_i));
				return Parser_result(line, column, length, Parser_result::error_code_success);
			}
			// Parse the literal as a floating-point number.
			Double_precision value = 0;
			bool zero = true;
			for(pos = int_begin; pos != int_end; ++pos){
				const auto ptr = std::char_traits<char>::find(s_numeric_table + s_delim_count, radix * 2, str.at(pos));
				if(ptr == nullptr){
					continue;
				}
				const auto digit_value = static_cast<unsigned char>((ptr - s_numeric_table - s_delim_count) / 2);
				if(digit_value != 0){
					zero = false;
				}
				value *= radix;
				value += digit_value;
			}
			int value_class = std::fpclassify(value);
			if(value_class == FP_INFINITE){
				return Parser_result(line, column, length, Parser_result::error_code_integer_literal_overflow);
			}
			Double_precision mantissa = 0;
			for(pos = frac_end; pos != frac_begin; --pos){
				const auto ptr = std::char_traits<char>::find(s_numeric_table + s_delim_count, radix * 2, str.at(pos - 1));
				if(ptr == nullptr){
					continue;
				}
				const auto digit_value = static_cast<unsigned char>((ptr - s_numeric_table - s_delim_count) / 2);
				if(digit_value != 0){
					zero = false;
				}
				mantissa += digit_value;
				mantissa /= radix;
			}
			value += mantissa;
			if(exp_base == FLT_RADIX){
				value = std::scalbn(value, exp);
			} else {
				value = value * std::pow(static_cast<int>(exp_base), exp);
			}
			value_class = std::fpclassify(value);
			if(value_class == FP_INFINITE){
				return Parser_result(line, column, length, Parser_result::error_code_double_literal_overflow);
			}
			if((value_class == FP_ZERO) && (zero == false)){
				return Parser_result(line, column, length, Parser_result::error_code_double_literal_underflow);
			}
			if(do_merge_sign(tokens_out, line, column)){
				value = -value;
			}
			Token::S_double_literal token_d = { value };
			tokens_out.emplace_back(line, column, length, std::move(token_d));
			return Parser_result(line, column, length, Parser_result::error_code_success); }

		case 'A':  case 'B':  case 'C':  case 'D':  case 'E':  case 'F':  case 'G':
		case 'H':  case 'I':  case 'J':  case 'K':  case 'L':  case 'M':  case 'N':
		case 'O':  case 'P':  case 'Q':  case 'R':  case 'S':  case 'T':
		case 'U':  case 'V':  case 'W':  case 'X':  case 'Y':  case 'Z':
		case 'a':  case 'b':  case 'c':  case 'd':  case 'e':  case 'f':  case 'g':
		case 'h':  case 'i':  case 'j':  case 'k':  case 'l':  case 'm':  case 'n':
		case 'o':  case 'p':  case 'q':  case 'r':  case 's':  case 't':
		case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
		case '_': {
			// Get an identifier, then check whether it is a keyword.
			struct Keyword_element {
				char first[15];
				Token::Keyword second;
			};
			static_assert(sizeof(Keyword_element) % 8 == 0, "`Keyword_element` is misaligned.");
			// Remember to keep this table sorted!
			static constexpr Keyword_element s_keyword_table[] = {
				{ "break",     Token::keyword_break     },
				{ "case",      Token::keyword_case      },
				{ "catch",     Token::keyword_catch     },
				{ "const",     Token::keyword_const     },
				{ "continue",  Token::keyword_continue  },
				{ "default",   Token::keyword_default   },
				{ "defer",     Token::keyword_defer     },
				{ "do",        Token::keyword_do        },
				{ "each",      Token::keyword_each      },
				{ "else",      Token::keyword_else      },
				{ "false",     Token::keyword_false     },
				{ "for",       Token::keyword_for       },
				{ "function",  Token::keyword_function  },
				{ "if",        Token::keyword_if        },
				{ "infinity",  Token::keyword_infinity  },
				{ "nan",       Token::keyword_nan       },
				{ "null",      Token::keyword_null      },
				{ "return",    Token::keyword_return    },
				{ "switch",    Token::keyword_switch    },
				{ "throw",     Token::keyword_throw     },
				{ "true",      Token::keyword_true      },
				{ "try",       Token::keyword_try       },
				{ "var",       Token::keyword_var       },
				{ "while",     Token::keyword_while     },
			};
			struct Keyword_comparator {
				bool operator()(const Keyword_element &lhs, const Keyword_element &rhs) const noexcept {
					return std::char_traits<char>::compare(lhs.first, rhs.first, sizeof(lhs.first)) < 0;
				}
				bool operator()(char lhs, const Keyword_element &rhs) const noexcept {
					return std::char_traits<char>::lt(lhs, rhs.first[0]);
				}
				bool operator()(const Keyword_element &lhs, char rhs) const noexcept {
					return std::char_traits<char>::lt(lhs.first[0], rhs);
				}
			};
#ifdef ROCKET_DEBUG
			ROCKET_ASSERT(std::is_sorted(std::begin(s_keyword_table), std::end(s_keyword_table), Keyword_comparator()));
#endif
			const auto pos = str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_", column + 1);
			const auto length = std::min(pos, str.size()) - column;
			// Check whether this matches a keyword.
			const auto range = std::equal_range(std::begin(s_keyword_table), std::end(s_keyword_table), char_head, Keyword_comparator());
			for(auto it = range.first; it != range.second; ++it){
				const auto &pair = it[0];
				if((std::char_traits<char>::length(pair.first) == length) && (std::char_traits<char>::compare(pair.first, str.data() + column, length) == 0)){
					// Push a keyword.
					Token::S_keyword token_kw = { pair.second };
					tokens_out.emplace_back(line, column, length, std::move(token_kw));
					return Parser_result(line, column, length, Parser_result::error_code_success);
				}
			}
			// Push a trivial identifier.
			Token::S_identifier token_id = { str.substr(column, length) };
			tokens_out.emplace_back(line, column, length, std::move(token_id));
			return Parser_result(line, column, length, Parser_result::error_code_success); }

		default:
			// Fail to find a valid token.
			ASTERIA_DEBUG_LOG("The character at the beginning of `", str.substr(column, 25), "` was not handled.");
			return Parser_result(line, column, 1, Parser_result::error_code_token_character_unrecognized);
		}
	}
}

Parser_result tokenize_line(Vector<Token> &tokens_out, std::size_t line, Cow_string_cref str){
	// Ensure the source string is valid UTF-8.
	std::size_t column = 0;
	while(column < str.size()){
		const auto result = do_validate_code_point_utf8(line, str, column);
		ROCKET_ASSERT(result.get_length() <= str.size() - column);
		if(result.get_error_code() != Parser_result::error_code_success){
			ASTERIA_DEBUG_LOG("Invalid UTF-8 string: line = ", line, ", substr = `", str.substr(column, result.get_length()), "`, error_code = ", result.get_error_code());
			return result;
		}
		ROCKET_ASSERT(result.get_length() > 0);
		column += result.get_length();
	}
	// Get tokens one by one.
	column = 0;
	while(column < str.size()){
		const auto result = do_get_token(tokens_out, line, str, column);
		ROCKET_ASSERT(result.get_length() <= str.size() - column);
		if(result.get_error_code() != Parser_result::error_code_success){
			ASTERIA_DEBUG_LOG("Parser error: line = ", line, ", substr = `", str.substr(column, result.get_length()), "`, error_code = ", result.get_error_code());
			return result;
		}
		ROCKET_ASSERT(result.get_length() > 0);
		column += result.get_length();
	}
	return Parser_result(line, column, 0, Parser_result::error_code_success);
}

}
