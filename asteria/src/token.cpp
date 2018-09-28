// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "token.hpp"
#include "utilities.hpp"

namespace Asteria {

const char * Token::get_keyword(Keyword keyword) noexcept
  {
    switch(keyword) {
      case keyword_var: {
        return "var";
      }
      case keyword_const: {
        return "const";
      }
      case keyword_func: {
        return "func";
      }
      case keyword_if: {
        return "if";
      }
      case keyword_else: {
        return "else";
      }
      case keyword_switch: {
        return "switch";
      }
      case keyword_case: {
        return "case";
      }
      case keyword_default: {
        return "default";
      }
      case keyword_do: {
        return "do";
      }
      case keyword_while: {
        return "while";
      }
      case keyword_for: {
        return "for";
      }
      case keyword_each: {
        return "each";
      }
      case keyword_try: {
        return "try";
      }
      case keyword_catch: {
        return "catch";
      }
      case keyword_defer: {
        return "defer";
      }
      case keyword_break: {
        return "break";
      }
      case keyword_continue: {
        return "continue";
      }
      case keyword_throw: {
        return "throw";
      }
      case keyword_return: {
        return "return";
      }
      case keyword_null: {
        return "null";
      }
      case keyword_true: {
        return "true";
      }
      case keyword_false: {
        return "false";
      }
      case keyword_nan: {
        return "nan";
      }
      case keyword_infinity: {
        return "infinity";
      }
      case keyword_this: {
        return "this";
      }
      case keyword_unset: {
        return "unset";
      }
      case keyword_export: {
        return "export";
      }
      case keyword_import: {
        return "import";
      }
      default: {
        return "<unknown>";
      }
    }
  }

const char * Token::get_punctuator(Punctuator punct) noexcept
  {
    switch(punct) {
      case punctuator_add: {
        return "+";
      }
      case punctuator_add_eq: {
        return "+=";
      }
      case punctuator_sub: {
        return "-";
      }
      case punctuator_sub_eq: {
        return "-=";
      }
      case punctuator_mul: {
        return "*";
      }
      case punctuator_mul_eq: {
        return "*=";
      }
      case punctuator_div: {
        return "/";
      }
      case punctuator_div_eq: {
        return "/=";
      }
      case punctuator_mod: {
        return "%";
      }
      case punctuator_mod_eq: {
        return "%=";
      }
      case punctuator_inc: {
        return "++";
      }
      case punctuator_dec: {
        return "--";
      }
      case punctuator_sll: {
        return "<<<";
      }
      case punctuator_sll_eq: {
        return "<<<=";
      }
      case punctuator_srl: {
        return ">>>";
      }
      case punctuator_srl_eq: {
        return ">>>=";
      }
      case punctuator_sla: {
        return "<<";
      }
      case punctuator_sla_eq: {
        return "<<=";
      }
      case punctuator_sra: {
        return ">>";
      }
      case punctuator_sra_eq: {
        return ">>=";
      }
      case punctuator_andb: {
        return "&";
      }
      case punctuator_andb_eq: {
        return "&=";
      }
      case punctuator_andl: {
        return "&&";
      }
      case punctuator_andl_eq: {
        return "&&=";
      }
      case punctuator_orb: {
        return "|";
      }
      case punctuator_orb_eq: {
        return "|=";
      }
      case punctuator_orl: {
        return "||";
      }
      case punctuator_orl_eq: {
        return "||=";
      }
      case punctuator_xorb: {
        return "^";
      }
      case punctuator_xorb_eq: {
        return "^=";
      }
      case punctuator_notb: {
        return "~";
      }
      case punctuator_notl: {
        return "!";
      }
      case punctuator_cmp_eq: {
        return "==";
      }
      case punctuator_cmp_ne: {
        return "!=";
      }
      case punctuator_cmp_lt: {
        return "<";
      }
      case punctuator_cmp_gt: {
        return ">";
      }
      case punctuator_cmp_lte: {
        return "<=";
      }
      case punctuator_cmp_gte: {
        return ">=";
      }
      case punctuator_dot: {
        return ".";
      }
      case punctuator_quest: {
        return "?";
      }
      case punctuator_quest_eq: {
        return "?=";
      }
      case punctuator_assign: {
        return "=";
      }
      case punctuator_parenth_op: {
        return "(";
      }
      case punctuator_parenth_cl: {
        return ")";
      }
      case punctuator_bracket_op: {
        return "[";
      }
      case punctuator_bracket_cl: {
        return "]";
      }
      case punctuator_brace_op: {
        return "{";
      }
      case punctuator_brace_cl: {
        return "}";
      }
      case punctuator_comma: {
        return ",";
      }
      case punctuator_colon: {
        return ":";
      }
      case punctuator_semicol: {
        return ";";
      }
      case punctuator_spaceship: {
        return "<=>";
      }
      case punctuator_coales: {
        return "?\?";
      }
      case punctuator_coales_eq: {
        return "?\?=";
      }
      default: {
        return "<unknown>";
      }
    }
  }

Token::~Token()
  {
  }

void Token::dump(std::ostream &os) const
  {
    switch(this->index()) {
      case index_keyword: {
        const auto &alt = this->check<S_keyword>();
        // keyword `if`
        os <<"keyword `" <<get_keyword(alt.keyword) <<"`";
        return;
      }
      case index_punctuator: {
        const auto &alt = this->check<S_punctuator>();
        // punctuator `;`
        os <<"punctuator `" <<get_punctuator(alt.punct) <<"`";
        return;
      }
      case index_identifier: {
        const auto &alt = this->check<S_identifier>();
        // identifier `meow`
        os <<"identifier `" <<alt.name <<"`";
        return;
      }
      case index_integer_literal: {
        const auto &alt = this->check<S_integer_literal>();
        // integer-literal `42`
        os <<"integer-literal `" <<std::dec <<alt.value <<"`";
        return;
      }
      case index_real_literal: {
        const auto &alt = this->check<S_real_literal>();
        // real-number-literal `123.456`
        os <<"real-number-literal `" <<std::dec <<std::nouppercase <<std::setprecision(std::numeric_limits<Xfloat>::max_digits10) <<alt.value <<"`";
        return;
      }
      case index_string_literal: {
        const auto &alt = this->check<S_string_literal>();
        // string-literal "hello world"
        os <<"string-literal `" <<quote(alt.value) <<"`";
        return;
      }
      default: {
        ASTERIA_TERMINATE("An unknown token type enumeration `", this->index(), "` has been encountered.");
      }
    }
  }

}
