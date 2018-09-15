// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "parser_result.hpp"
#include "utilities.hpp"

namespace Asteria {

const char * Parser_result::describe_error(Parser_result::Error error) noexcept
  {
    switch(error) {
      case success: {
        return "The operation succeeded.";
      }
      // Phase 1
      //   I/O stream
      //   UTF-8 decoder
      case error_istream_open_failure: {
        return "The source stream could not be opened for reading.";
      }
      case error_istream_badbit_set: {
        return "An error occurred when reading data from the source stream.";
      }
      case error_utf8_sequence_invalid: {
        return "An invalid UTF-8 byte sequence was encountered.";
      }
      case error_utf8_sequence_incomplete: {
        return "An incomplete UTF-8 byte sequence was encountered.";
      }
      case error_utf_code_point_invalid: {
        return "A UTF-8 byte sequence could not be decoded to a valid UTF code point.";
      }
      // Phase 2
      //   Comment stripper
      //   Tokenizer
      case error_token_character_unrecognized: {
        return "The character could not form a valid token.";
      }
      case error_string_literal_unclosed: {
        return "The string literal was not properly terminated.";
      }
      case error_escape_sequence_unknown: {
        return "An invalid escape sequence was encountered.";
      }
      case error_escape_sequence_incomplete: {
        return "An incomplete escape sequence was encountered.";
      }
      case error_escape_sequence_invalid_hex: {
        return "An invalid hexadecimal digit was encountered.";
      }
      case error_escape_utf_code_point_invalid: {
        return "The value of this escaped UTF code point was reserved or out of range.";
      }
      case error_numeric_literal_incomplete: {
        return "This numeric literal was incomplete.";
      }
      case error_numeric_literal_suffix_disallowed: {
        return "This numeric literal was malformed.";
      }
      case error_numeric_literal_exponent_overflow: {
        return "The exponent of this numeric literal was too large.";
      }
      case error_integer_literal_overflow: {
        return "The exponent of this integer literal was too large.";
      }
      case error_integer_literal_exponent_negative: {
        return "The exponent of this integer literal was negative.";
      }
      case error_real_literal_overflow: {
        return "The value of this real number literal was so large that would yield infinity.";
      }
      case error_real_literal_underflow: {
        return "The value of this real number literal was so small that would yield zero.";
      }
      case error_block_comment_unclosed: {
        return "A block comment was not properly terminated.";
      }
      // Phase 3
      //   Parser
      case error_directive_or_statement_expected: {
        return "An unexpected token was encountered while a directive or statement is expected.";
      }
      case error_identifier_expected: {
        return "An unexpected token was encountered while an identifier is expected.";
      }
      case error_semicolon_expected: {
        return "An unexpected token was encountered while a semicolon is expected.";
      }
      case error_string_literal_expected: {
        return "An unexpected token was encountered while a string literal is expected.";
      }
      case error_statement_expected: {
        return "An unexpected token was encountered while a statement is expected.";
      }
      case error_equals_sign_expected: {
        return "An unexpected token was encountered while an equals sign is expected.";
      }
      case error_expression_expected: {
        return "An unexpected token was encountered while an expression is expected.";
      }
      default: {
        return "No description is available for this error code.";
      }
    }
  }

}
