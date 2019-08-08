// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "fwd.hpp"
#include "runtime/value.hpp"

namespace Asteria {

const char* describe_frame_type(Frame_Type type) noexcept
  {
    switch(type) {
    case frame_type_native:
      {
        return "native";
      }
    case frame_type_throw:
      {
        return "throw";
      }
    case frame_type_catch:
      {
        return "catch";
      }
    case frame_type_func:
      {
        return "func";
      }
    default:
      return "<unknown frame type>";
    }
  }

const char* describe_gtype(Gtype gtype) noexcept
  {
    switch(gtype) {
    case gtype_null:
      {
        return "null";
      }
    case gtype_boolean:
      {
        return "boolean";
      }
    case gtype_integer:
      {
        return "integer";
      }
    case gtype_real:
      {
        return "real";
      }
    case gtype_string:
      {
        return "string";
      }
    case gtype_opaque:
      {
        return "opaque";
      }
    case gtype_function:
      {
        return "function";
      }
    case gtype_array:
      {
        return "array";
      }
    case gtype_object:
      {
        return "object";
      }
    default:
      return "<unknown data type>";
    }
  }

const char* stringify_punctuator(Punctuator punct) noexcept
  {
    switch(punct) {
    case punctuator_add:
      {
        return "+";
      }
    case punctuator_add_eq:
      {
        return "+=";
      }
    case punctuator_sub:
      {
        return "-";
      }
    case punctuator_sub_eq:
      {
        return "-=";
      }
    case punctuator_mul:
      {
        return "*";
      }
    case punctuator_mul_eq:
      {
        return "*=";
      }
    case punctuator_div:
      {
        return "/";
      }
    case punctuator_div_eq:
      {
        return "/=";
      }
    case punctuator_mod:
      {
        return "%";
      }
    case punctuator_mod_eq:
      {
        return "%=";
      }
    case punctuator_inc:
      {
        return "++";
      }
    case punctuator_dec:
      {
        return "--";
      }
    case punctuator_sll:
      {
        return "<<<";
      }
    case punctuator_sll_eq:
      {
        return "<<<=";
      }
    case punctuator_srl:
      {
        return ">>>";
      }
    case punctuator_srl_eq:
      {
        return ">>>=";
      }
    case punctuator_sla:
      {
        return "<<";
      }
    case punctuator_sla_eq:
      {
        return "<<=";
      }
    case punctuator_sra:
      {
        return ">>";
      }
    case punctuator_sra_eq:
      {
        return ">>=";
      }
    case punctuator_andb:
      {
        return "&";
      }
    case punctuator_andb_eq:
      {
        return "&=";
      }
    case punctuator_andl:
      {
        return "&&";
      }
    case punctuator_andl_eq:
      {
        return "&&=";
      }
    case punctuator_orb:
      {
        return "|";
      }
    case punctuator_orb_eq:
      {
        return "|=";
      }
    case punctuator_orl:
      {
        return "||";
      }
    case punctuator_orl_eq:
      {
        return "||=";
      }
    case punctuator_xorb:
      {
        return "^";
      }
    case punctuator_xorb_eq:
      {
        return "^=";
      }
    case punctuator_notb:
      {
        return "~";
      }
    case punctuator_notl:
      {
        return "!";
      }
    case punctuator_cmp_eq:
      {
        return "==";
      }
    case punctuator_cmp_ne:
      {
        return "!=";
      }
    case punctuator_cmp_lt:
      {
        return "<";
      }
    case punctuator_cmp_gt:
      {
        return ">";
      }
    case punctuator_cmp_lte:
      {
        return "<=";
      }
    case punctuator_cmp_gte:
      {
        return ">=";
      }
    case punctuator_dot:
      {
        return ".";
      }
    case punctuator_quest:
      {
        return "?";
      }
    case punctuator_quest_eq:
      {
        return "?=";
      }
    case punctuator_assign:
      {
        return "=";
      }
    case punctuator_parenth_op:
      {
        return "(";
      }
    case punctuator_parenth_cl:
      {
        return ")";
      }
    case punctuator_bracket_op:
      {
        return "[";
      }
    case punctuator_bracket_cl:
      {
        return "]";
      }
    case punctuator_brace_op:
      {
        return "{";
      }
    case punctuator_brace_cl:
      {
        return "}";
      }
    case punctuator_comma:
      {
        return ",";
      }
    case punctuator_colon:
      {
        return ":";
      }
    case punctuator_semicol:
      {
        return ";";
      }
    case punctuator_spaceship:
      {
        return "<=>";
      }
    case punctuator_coales:
      {
        return "?\?";
      }
    case punctuator_coales_eq:
      {
        return "?\?=";
      }
    case punctuator_ellipsis:
      {
        return "...";
      }
    default:
      return "<unknown punctuator>";
    }
  }

const char* stringify_keyword(Keyword kwrd) noexcept
  {
    switch(kwrd) {
    case keyword_var:
      {
        return "var";
      }
    case keyword_const:
      {
        return "const";
      }
    case keyword_func:
      {
        return "func";
      }
    case keyword_if:
      {
        return "if";
      }
    case keyword_else:
      {
        return "else";
      }
    case keyword_switch:
      {
        return "switch";
      }
    case keyword_case:
      {
        return "case";
      }
    case keyword_default:
      {
        return "default";
      }
    case keyword_do:
      {
        return "do";
      }
    case keyword_while:
      {
        return "while";
      }
    case keyword_for:
      {
        return "for";
      }
    case keyword_each:
      {
        return "each";
      }
    case keyword_try:
      {
        return "try";
      }
    case keyword_catch:
      {
        return "catch";
      }
    case keyword_defer:
      {
        return "defer";
      }
    case keyword_break:
      {
        return "break";
      }
    case keyword_continue:
      {
        return "continue";
      }
    case keyword_throw:
      {
        return "throw";
      }
    case keyword_return:
      {
        return "return";
      }
    case keyword_null:
      {
        return "null";
      }
    case keyword_true:
      {
        return "true";
      }
    case keyword_false:
      {
        return "false";
      }
    case keyword_nan:
      {
        return "nan";
      }
    case keyword_infinity:
      {
        return "infinity";
      }
    case keyword_this:
      {
        return "this";
      }
    case keyword_unset:
      {
        return "unset";
      }
    case keyword_lengthof:
      {
        return "lengthof";
      }
    case keyword_typeof:
      {
        return "typeof";
      }
    case keyword_and:
      {
        return "and";
      }
    case keyword_or:
      {
        return "or";
      }
    case keyword_not:
      {
        return "not";
      }
    case keyword_assert:
      {
        return "assert";
      }
    case keyword_sqrt:
      {
        return "__sqrt";
      }
    case keyword_isnan:
      {
        return "__isnan";
      }
    case keyword_isinf:
      {
        return "__isinf";
      }
    case keyword_abs:
      {
        return "__abs";
      }
    case keyword_signb:
      {
        return "__signb";
      }
    case keyword_round:
      {
        return "__round";
      }
    case keyword_floor:
      {
        return "__floor";
      }
    case keyword_ceil:
      {
        return "__ceil";
      }
    case keyword_trunc:
      {
        return "__trunc";
      }
    case keyword_iround:
      {
        return "__iround";
      }
    case keyword_ifloor:
      {
        return "__ifloor";
      }
    case keyword_iceil:
      {
        return "__iceil";
      }
    case keyword_itrunc:
      {
        return "__itrunc";
      }
    case keyword_fma:
      {
        return "__fma";
      }
    default:
      return "<unknown keyword>";
    }
  }

const char* describe_parser_status(Parser_Status status) noexcept
  {
    switch(status) {
    case parser_status_success:
      {
        return "The operation succeeded.";
      }
    case parser_status_no_data_loaded :
      {
        return "No data were loaded.";
      }
    case parser_status_istream_open_failure:
      {
        return "The source stream could not be opened for reading.";
      }
    case parser_status_istream_badbit_set:
      {
        return "An error occurred when reading data from the source stream.";
      }
    case parser_status_utf8_sequence_invalid:
      {
        return "An invalid UTF-8 byte sequence was encountered.";
      }
    case parser_status_utf8_sequence_incomplete:
      {
        return "An incomplete UTF-8 byte sequence was encountered.";
      }
    case parser_status_utf_code_point_invalid:
      {
        return "A UTF-8 byte sequence could not be decoded to a valid UTF code point.";
      }
    case parser_status_null_character_disallowed:
      {
        return "Null characters are not allowed in source data.";
      }
    case parser_status_token_character_unrecognized:
      {
        return "The character could not form a valid token.";
      }
    case parser_status_string_literal_unclosed:
      {
        return "The string literal was not properly terminated.";
      }
    case parser_status_escape_sequence_unknown:
      {
        return "An invalid escape sequence was encountered.";
      }
    case parser_status_escape_sequence_incomplete:
      {
        return "An incomplete escape sequence was encountered.";
      }
    case parser_status_escape_sequence_invalid_hex:
      {
        return "An invalid hexadecimal digit was encountered.";
      }
    case parser_status_escape_utf_code_point_invalid:
      {
        return "The value of this escaped UTF code point was reserved or out of range.";
      }
    case parser_status_numeric_literal_incomplete:
      {
        return "This numeric literal was incomplete.";
      }
    case parser_status_numeric_literal_suffix_disallowed:
      {
        return "This numeric literal was not terminated properly.";
      }
    case parser_status_numeric_literal_exponent_overflow:
      {
        return "The exponent of this numeric literal was too large.";
      }
    case parser_status_integer_literal_overflow:
      {
        return "This integer literal was too large and could not be represented.";
      }
    case parser_status_integer_literal_exponent_negative:
      {
        return "The exponent of this integer literal was negative.";
      }
    case parser_status_real_literal_overflow:
      {
        return "The value of this real literal was so large that would yield infinity.";
      }
    case parser_status_real_literal_underflow:
      {
        return "The value of this real literal was so small that would yield zero.";
      }
    case parser_status_block_comment_unclosed:
      {
        return "A block comment was not properly terminated.";
      }
    case parser_status_digit_separator_following_nondigit:
      {
        return "A digit separator must follow a digit.";
      }
    case parser_status_identifier_expected:
      {
        return "Expectation failed while looking for an identifier.";
      }
    case parser_status_semicolon_expected:
      {
        return "Expectation failed while looking for a `;`.";
      }
    case parser_status_string_literal_expected:
      {
        return "Expectation failed while looking for a string literal.";
      }
    case parser_status_statement_expected:
      {
        return "Expectation failed while looking for a statement.";
      }
    case parser_status_equals_sign_expected:
      {
        return "Expectation failed while looking for an `=`.";
      }
    case parser_status_expression_expected:
      {
        return "Expectation failed while looking for an expression.";
      }
    case parser_status_open_brace_expected:
      {
        return "Expectation failed while looking for an `{`.";
      }
    case parser_status_closed_brace_or_statement_expected:
      {
        return "Expectation failed while looking for a `}` or statement.";
      }
    case parser_status_open_parenthesis_expected:
      {
        return "Expectation failed while looking for an `(`.";
      }
    case parser_status_parameter_or_ellipsis_expected:
      {
        return "Expectation failed while looking for an identifier or `...`.";
      }
    case parser_status_closed_parenthesis_expected:
      {
        return "Expectation failed while looking for a `)`.";
      }
    case parser_status_colon_expected:
      {
        return "Expectation failed while looking for a `:`.";
      }
    case parser_status_closed_brace_or_switch_clause_expected:
      {
        return "Expectation failed while looking for a `}`, `case` clause or `default` clause.";
      }
    case parser_status_keyword_while_expected:
      {
        return "Expectation failed while looking for a `while`.";
      }
    case parser_status_keyword_catch_expected:
      {
        return "Expectation failed while looking for a `catch`.";
      }
    case parser_status_comma_expected:
      {
        return "Expectation failed while looking for a `,`.";
      }
    case parser_status_for_statement_initializer_expected:
      {
        return "Expectation failed while looking for an `each`, `;`, variable definition or expression statement.";
      }
    case parser_status_semicolon_or_expression_expected:
      {
        return "Expectation failed while looking for a `;` or expression.";
      }
    case parser_status_closed_brace_expected:
      {
        return "Expectation failed while looking for a `}`.";
      }
    case parser_status_duplicate_object_key:
      {
        return "A duplicate key was to be created in the same object.";
      }
    case parser_status_closed_parenthesis_or_argument_expected:
      {
        return "Expectation failed while looking for a `)` or expression.";
      }
    case parser_status_closed_bracket_expected:
      {
        return "Expectation failed while looking for a `]`.";
      }
    case parser_status_open_brace_or_equal_initializer_expected:
      {
        return "Expectation failed while looking for an `{` or `=`.";
      }
    case parser_status_equals_sign_or_colon_expected:
      {
        return "Expectation failed while looking for an `=` or `:`.";
      }
    default:
      {
        return "No description is available for this error code.";
      }
    }
  }

// We assume that a all-bit-zero struct represents the `null` value.
// This is effectively undefined behavior. Don't play with this at home!
alignas(Value) const unsigned char null_value_storage[sizeof(Value)] = { };

}  // namespace Asteria
