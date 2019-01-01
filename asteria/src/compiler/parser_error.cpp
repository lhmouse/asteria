// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char * Parser_error::get_code_description(Parser_error::Code xcode) noexcept
  {
    switch(xcode) {
      // Special
      case code_success: {
        return "The operation succeeded.";
      }
      case code_no_data_loaded : {
        return "No data were loaded.";
      }
      // Phase 1
      //   I/O stream
      //   UTF-8 decoder
      case code_istream_open_failure: {
        return "The source stream could not be opened for reading.";
      }
      case code_istream_badbit_set: {
        return "An error occurred when reading data from the source stream.";
      }
      case code_utf8_sequence_invalid: {
        return "An invalid UTF-8 byte sequence was encountered.";
      }
      case code_utf8_sequence_incomplete: {
        return "An incomplete UTF-8 byte sequence was encountered.";
      }
      case code_utf_code_point_invalid: {
        return "A UTF-8 byte sequence could not be decoded to a valid UTF code point.";
      }
      // Phase 2
      //   Comment stripper
      //   Tokenizer
      case code_token_character_unrecognized: {
        return "The character could not form a valid token.";
      }
      case code_string_literal_unclosed: {
        return "The string literal was not properly terminated.";
      }
      case code_escape_sequence_unknown: {
        return "An invalid escape sequence was encountered.";
      }
      case code_escape_sequence_incomplete: {
        return "An incomplete escape sequence was encountered.";
      }
      case code_escape_sequence_invalid_hex: {
        return "An invalid hexadecimal digit was encountered.";
      }
      case code_escape_utf_code_point_invalid: {
        return "The value of this escaped UTF code point was reserved or out of range.";
      }
      case code_numeric_literal_incomplete: {
        return "This numeric literal was incomplete.";
      }
      case code_numeric_literal_suffix_disallowed: {
        return "This numeric literal was not terminated properly.";
      }
      case code_numeric_literal_exponent_overflow: {
        return "The exponent of this numeric literal was too large.";
      }
      case code_integer_literal_overflow: {
        return "The exponent of this integer literal was too large.";
      }
      case code_integer_literal_exponent_negative: {
        return "The exponent of this integer literal was negative.";
      }
      case code_real_literal_overflow: {
        return "The value of this real number literal was so large that would yield infinity.";
      }
      case code_real_literal_underflow: {
        return "The value of this real number literal was so small that would yield zero.";
      }
      case code_block_comment_unclosed: {
        return "A block comment was not properly terminated.";
      }
      // Phase 3
      //   Parser
      case code_identifier_expected: {
        return "Expectation failed while looking for an identifier.";
      }
      case code_semicolon_expected: {
        return "Expectation failed while looking for a `;`.";
      }
      case code_string_literal_expected: {
        return "Expectation failed while looking for a string literal.";
      }
      case code_statement_expected: {
        return "Expectation failed while looking for a statement.";
      }
      case code_equals_sign_expected: {
        return "Expectation failed while looking for an `=`.";
      }
      case code_expression_expected: {
        return "Expectation failed while looking for an expression.";
      }
      case code_open_brace_expected: {
        return "Expectation failed while looking for an `{`.";
      }
      case code_close_brace_or_statement_expected: {
        return "Expectation failed while looking for a `}` or statement.";
      }
      case code_open_parenthesis_expected: {
        return "Expectation failed while looking for an `(`.";
      }
      case code_close_parenthesis_or_parameter_expected: {
        return "Expectation failed while looking for a `)` or identifier.";
      }
      case code_close_parenthesis_expected: {
        return "Expectation failed while looking for a `)`.";
      }
      case code_colon_expected: {
        return "Expectation failed while looking for a `:`.";
      }
      case code_close_brace_or_switch_clause_expected: {
        return "Expectation failed while looking for a `}`, `case` clause or `default` clause.";
      }
      case code_keyword_while_expected: {
        return "Expectation failed while looking for a `while`.";
      }
      case code_keyword_catch_expected: {
        return "Expectation failed while looking for a `catch`.";
      }
      case code_comma_expected: {
        return "Expectation failed while looking for a `,`.";
      }
      case code_for_statement_initializer_expected: {
        return "Expectation failed while looking for an `each`, variable definition or expression statement.";
      }
      case code_close_bracket_or_expression_expected: {
        return "Expectation failed while looking for a `]` or expression.";
      }
      case code_close_brace_or_object_key_expected: {
        return "Expectation failed while looking for a `}`, identifier or string literal.";
      }
      case code_duplicate_object_key: {
        return "A duplicate key was to be created in the same object.";
      }
      case code_close_parenthesis_or_argument_expected: {
        return "Expectation failed while looking for a `)` or expression.";
      }
      case code_close_bracket_expected: {
        return "Expectation failed while looking for a `]`.";
      }
      default: {
        return "No description is available for this error code.";
      }
    }
  }

}
