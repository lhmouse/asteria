// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "fwd.hpp"
#include "value.hpp"

namespace Asteria {

const char* describe_frame_type(Frame_Type type) noexcept
  {
    switch(type) {
    case frame_type_native: {
        return "native";
      }
    case frame_type_throw: {
        return "throw";
      }
    case frame_type_catch: {
        return "catch";
      }
    case frame_type_call: {
        return "call";
      }
    case frame_type_func: {
        return "function";
      }
    case frame_type_defer: {
        return "defer";
      }
    default:
      return "<unknown frame type>";
    }
  }

const char* describe_vtype(Vtype vtype) noexcept
  {
    switch(vtype) {
    case vtype_null: {
        return "null";
      }
    case vtype_boolean: {
        return "boolean";
      }
    case vtype_integer: {
        return "integer";
      }
    case vtype_real: {
        return "real";
      }
    case vtype_string: {
        return "string";
      }
    case vtype_opaque: {
        return "opaque";
      }
    case vtype_function: {
        return "function";
      }
    case vtype_array: {
        return "array";
      }
    case vtype_object: {
        return "object";
      }
    default:
      return "<unknown data type>";
    }
  }

const char* describe_parser_status(Parser_Status status) noexcept
  {
    switch(status) {
    case parser_status_success: {
        return "operation succeeded";
      }
    case parser_status_utf8_sequence_invalid: {
        return "UTF-8 sequence invalid";
      }
    case parser_status_utf8_sequence_incomplete: {
        return "UTF-8 sequence incomplete";
      }
    case parser_status_utf_code_point_invalid: {
        return "UTF code point invalid";
      }
    case parser_status_null_character_disallowed: {
        return "null character disallowed in source code";
      }
    case parser_status_token_character_unrecognized: {
        return "character invalid in source code";
      }
    case parser_status_string_literal_unclosed: {
        return "string literal unclosed";
      }
    case parser_status_escape_sequence_unknown: {
        return "escape sequence invalid";
      }
    case parser_status_escape_sequence_incomplete: {
        return "escape sequence incomplete";
      }
    case parser_status_escape_sequence_invalid_hex: {
        return "hexadecimal digit expected";
      }
    case parser_status_escape_utf_code_point_invalid: {
        return "UTF code point value invalid";
      }
    case parser_status_numeric_literal_invalid: {
        return "numeric literal invalid";
      }
    case parser_status_integer_literal_overflow: {
        return "integer literal too large";
      }
    case parser_status_integer_literal_inexact: {
        return "fraction as an integer literal";
      }
    case parser_status_real_literal_overflow: {
        return "real literal too large";
      }
    case parser_status_real_literal_underflow: {
        return "real literal truncated to zero";
      }
    case parser_status_numeric_literal_suffix_invalid: {
        return "numeric literal suffix invalid";
      }
    case parser_status_block_comment_unclosed: {
        return "block comment unclosed";
      }
    case parser_status_digit_separator_following_nondigit: {
        return "digit separator not following a digit";
      }
    case parser_status_identifier_expected: {
        return "identifier expected";
      }
    case parser_status_semicolon_expected: {
        return "`;` expected";
      }
    case parser_status_string_literal_expected: {
        return "string literal expected";
      }
    case parser_status_statement_expected: {
        return "statement expected";
      }
    case parser_status_equals_sign_expected: {
        return "`=` expected";
      }
    case parser_status_expression_expected: {
        return "expression expected";
      }
    case parser_status_open_brace_expected: {
        return "`{` expected";
      }
    case parser_status_closed_brace_or_statement_expected: {
        return "`}` or statement expected";
      }
    case parser_status_open_parenthesis_expected: {
        return "`(` expected";
      }
    case parser_status_parameter_or_ellipsis_expected: {
        return "identifier or `...` expected";
      }
    case parser_status_closed_parenthesis_expected: {
        return "`)` expected";
      }
    case parser_status_colon_expected: {
        return "`:` expected";
      }
    case parser_status_closed_brace_or_switch_clause_expected: {
        return "`}`, `case` or `default` expected";
      }
    case parser_status_keyword_while_expected: {
        return "`while` expected";
      }
    case parser_status_keyword_catch_expected: {
        return "`catch` expected";
      }
    case parser_status_comma_expected: {
        return "`,` expected";
      }
    case parser_status_for_statement_initializer_expected: {
        return "`each`, `;`, variable definition or expression statement expected";
      }
    case parser_status_semicolon_or_expression_expected: {
        return "`;` or expression expected";
      }
    case parser_status_closed_brace_expected: {
        return "`}` expected";
      }
    case parser_status_too_many_elements: {
        return "max number of elements exceeded";
      }
    case parser_status_closed_bracket_expected: {
        return "`]` expected";
      }
    case parser_status_open_brace_or_equal_initializer_expected: {
        return "`{` or `=` expected";
      }
    case parser_status_equals_sign_or_colon_expected: {
        return "`=` or `:` expected";
      }
    case parser_status_closed_bracket_or_comma_expected: {
        return "`]` or `,` expected";
      }
    case parser_status_closed_brace_or_comma_expected: {
        return "`}` or `,` expected";
      }
    case parser_status_closed_bracket_or_expression_expected: {
        return "`]` or expression expected";
      }
    case parser_status_closed_brace_or_json5_key_expected: {
        return "`}`, identifier or string literal expected";
      }
    default:
      return "<unknown parser error>";
    }
  }

// We assume that a all-bit-zero struct represents the `null` value.
// This is effectively undefined behavior. Don't play with this at home!
alignas(Value) const unsigned char null_value_storage[sizeof(Value)] = { };

}  // namespace Asteria
