// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "fwd.hpp"
#include "value.hpp"

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
    case punctuator_tail:
      {
        return "[$]";
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
    case keyword_sign:
      {
        return "__sign";
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
        return "operation succeeded";
      }
    case parser_status_utf8_sequence_invalid:
      {
        return "UTF-8 sequence invalid";
      }
    case parser_status_utf8_sequence_incomplete:
      {
        return "UTF-8 sequence incomplete";
      }
    case parser_status_utf_code_point_invalid:
      {
        return "UTF code point invalid";
      }
    case parser_status_null_character_disallowed:
      {
        return "null character disallowed in source code";
      }
    case parser_status_token_character_unrecognized:
      {
        return "character invalid in source code";
      }
    case parser_status_string_literal_unclosed:
      {
        return "string literal unclosed";
      }
    case parser_status_escape_sequence_unknown:
      {
        return "escape sequence invalid";
      }
    case parser_status_escape_sequence_incomplete:
      {
        return "escape sequence incomplete";
      }
    case parser_status_escape_sequence_invalid_hex:
      {
        return "hexadecimal digit expected";
      }
    case parser_status_escape_utf_code_point_invalid:
      {
        return "UTF code point value invalid";
      }
    case parser_status_numeric_literal_invalid:
      {
        return "numeric literal invalid";
      }
    case parser_status_integer_literal_overflow:
      {
        return "integer literal too large";
      }
    case parser_status_integer_literal_inexact:
      {
        return "fraction as an integer literal";
      }
    case parser_status_real_literal_overflow:
      {
        return "real literal too large";
      }
    case parser_status_real_literal_underflow:
      {
        return "real literal truncated to zero";
      }
    case parser_status_numeric_literal_suffix_invalid:
      {
        return "numeric literal suffix invalid";
      }
    case parser_status_block_comment_unclosed:
      {
        return "block comment unclosed";
      }
    case parser_status_digit_separator_following_nondigit:
      {
        return "digit separator not following a digit";
      }
    case parser_status_identifier_expected:
      {
        return "identifier expected";
      }
    case parser_status_semicolon_expected:
      {
        return "`;` expected";
      }
    case parser_status_string_literal_expected:
      {
        return "string literal expected";
      }
    case parser_status_statement_expected:
      {
        return "statement expected";
      }
    case parser_status_equals_sign_expected:
      {
        return "`=` expected";
      }
    case parser_status_expression_expected:
      {
        return "expression expected";
      }
    case parser_status_open_brace_expected:
      {
        return "`{` expected";
      }
    case parser_status_closed_brace_or_statement_expected:
      {
        return "`}` or statement expected";
      }
    case parser_status_open_parenthesis_expected:
      {
        return "`(` expected";
      }
    case parser_status_parameter_or_ellipsis_expected:
      {
        return "identifier or `...` expected";
      }
    case parser_status_closed_parenthesis_expected:
      {
        return "`)` expected";
      }
    case parser_status_colon_expected:
      {
        return "`:` expected";
      }
    case parser_status_closed_brace_or_switch_clause_expected:
      {
        return "`}`, `case` or `default` expected";
      }
    case parser_status_keyword_while_expected:
      {
        return "`while` expected";
      }
    case parser_status_keyword_catch_expected:
      {
        return "`catch` expected";
      }
    case parser_status_comma_expected:
      {
        return "`,` expected";
      }
    case parser_status_for_statement_initializer_expected:
      {
        return "`each`, `;`, variable definition or expression statement expected";
      }
    case parser_status_semicolon_or_expression_expected:
      {
        return "`;` or expression expected";
      }
    case parser_status_closed_brace_expected:
      {
        return "`}` expected";
      }
    case parser_status_too_many_array_elements:
      {
        return "array elements too many";
      }
    case parser_status_closed_parenthesis_or_argument_expected:
      {
        return "`)` or expression expected";
      }
    case parser_status_closed_bracket_expected:
      {
        return "`]` expected";
      }
    case parser_status_open_brace_or_equal_initializer_expected:
      {
        return "`{` or `=` expected";
      }
    case parser_status_equals_sign_or_colon_expected:
      {
        return "`=` or `:` expected";
      }
    case parser_status_closed_brace_or_name_expected:
      {
        return "`}`, identifier or string literal expected";
      }
    default:
      {
        return "<unknown parser error>";
      }
    }
  }

// We assume that a all-bit-zero struct represents the `null` value.
// This is effectively undefined behavior. Don't play with this at home!
alignas(Value) const unsigned char null_value_storage[sizeof(Value)] = { };

}  // namespace Asteria
