// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "enums.hpp"

namespace asteria {

const char*
stringify_punctuator(Punctuator punct) noexcept
  {
    switch(punct) {
      case punctuator_add:
        return "+";

      case punctuator_add_eq:
        return "+=";

      case punctuator_sub:
        return "-";

      case punctuator_sub_eq:
        return "-=";

      case punctuator_mul:
        return "*";

      case punctuator_mul_eq:
        return "*=";

      case punctuator_div:
        return "/";

      case punctuator_div_eq:
        return "/=";

      case punctuator_mod:
        return "%";

      case punctuator_mod_eq:
        return "%=";

      case punctuator_inc:
        return "++";

      case punctuator_dec:
        return "--";

      case punctuator_sll:
        return "<<<";

      case punctuator_sll_eq:
        return "<<<=";

      case punctuator_srl:
        return ">>>";

      case punctuator_srl_eq:
        return ">>>=";

      case punctuator_sla:
        return "<<";

      case punctuator_sla_eq:
        return "<<=";

      case punctuator_sra:
        return ">>";

      case punctuator_sra_eq:
        return ">>=";

      case punctuator_andb:
        return "&";

      case punctuator_andb_eq:
        return "&=";

      case punctuator_andl:
        return "&&";

      case punctuator_andl_eq:
        return "&&=";

      case punctuator_orb:
        return "|";

      case punctuator_orb_eq:
        return "|=";

      case punctuator_orl:
        return "||";

      case punctuator_orl_eq:
        return "||=";

      case punctuator_xorb:
        return "^";

      case punctuator_xorb_eq:
        return "^=";

      case punctuator_notb:
        return "~";

      case punctuator_notl:
        return "!";

      case punctuator_cmp_eq:
        return "==";

      case punctuator_cmp_ne:
        return "!=";

      case punctuator_cmp_lt:
        return "<";

      case punctuator_cmp_gt:
        return ">";

      case punctuator_cmp_lte:
        return "<=";

      case punctuator_cmp_gte:
        return ">=";

      case punctuator_dot:
        return ".";

      case punctuator_quest:
        return "?";

      case punctuator_quest_eq:
        return "?=";

      case punctuator_assign:
        return "=";

      case punctuator_parenth_op:
        return "(";

      case punctuator_parenth_cl:
        return ")";

      case punctuator_bracket_op:
        return "[";

      case punctuator_bracket_cl:
        return "]";

      case punctuator_brace_op:
        return "{";

      case punctuator_brace_cl:
        return "}";

      case punctuator_comma:
        return ",";

      case punctuator_colon:
        return ":";

      case punctuator_semicol:
        return ";";

      case punctuator_cmp_3way:
        return "<=>";

      case punctuator_coales:
        return "?\?";

      case punctuator_coales_eq:
        return "?\?=";

      case punctuator_ellipsis:
        return "...";

      case punctuator_head:
        return "[^]";

      case punctuator_tail:
        return "[$]";

      case punctuator_arrow:
        return "->";

      case punctuator_scope:
        return "::";

      case punctuator_random:
        return "[?]";

      case punctuator_cmp_un:
        return "</>";

      default:
        return "[unknown punctuator]";
    }
  }

const char*
stringify_keyword(Keyword kwrd) noexcept
  {
    switch(kwrd) {
      case keyword_var:
        return "var";

      case keyword_const:
        return "const";

      case keyword_func:
        return "func";

      case keyword_if:
        return "if";

      case keyword_else:
        return "else";

      case keyword_switch:
        return "switch";

      case keyword_case:
        return "case";

      case keyword_default:
        return "default";

      case keyword_do:
        return "do";

      case keyword_while:
        return "while";

      case keyword_for:
        return "for";

      case keyword_each:
        return "each";

      case keyword_try:
        return "try";

      case keyword_catch:
        return "catch";

      case keyword_defer:
        return "defer";

      case keyword_break:
        return "break";

      case keyword_continue:
        return "continue";

      case keyword_throw:
        return "throw";

      case keyword_return:
        return "return";

      case keyword_null:
        return "null";

      case keyword_true:
        return "true";

      case keyword_false:
        return "false";

      case keyword_import:
        return "import";

      case keyword_ref:
        return "ref";

      case keyword_this:
        return "this";

      case keyword_unset:
        return "unset";

      case keyword_countof:
        return "countof";

      case keyword_typeof:
        return "typeof";

      case keyword_and:
        return "and";

      case keyword_or:
        return "or";

      case keyword_not:
        return "not";

      case keyword_assert:
        return "assert";

      case keyword_sqrt:
        return "__sqrt";

      case keyword_isnan:
        return "__isnan";

      case keyword_isinf:
        return "__isinf";

      case keyword_abs:
        return "__abs";

      case keyword_sign:
        return "__sign";

      case keyword_round:
        return "__round";

      case keyword_floor:
        return "__floor";

      case keyword_ceil:
        return "__ceil";

      case keyword_trunc:
        return "__trunc";

      case keyword_iround:
        return "__iround";

      case keyword_ifloor:
        return "__ifloor";

      case keyword_iceil:
        return "__iceil";

      case keyword_itrunc:
        return "__itrunc";

      case keyword_fma:
        return "__fma";

      case keyword_extern:
        return "extern";

      case keyword_vcall:
        return "__vcall";

      case keyword_lzcnt:
        return "__lzcnt";

      case keyword_tzcnt:
        return "__tzcnt";

      case keyword_popcnt:
        return "__popcnt";

      case keyword_addm:
        return "__addm";

      case keyword_subm:
        return "__subm";

      case keyword_mulm:
        return "__mulm";

      case keyword_adds:
        return "__adds";

      case keyword_subs:
        return "__subs";

      case keyword_muls:
        return "__muls";

      default:
        return "[unknown keyword]";
    }
  }

}  // namespace asteria
