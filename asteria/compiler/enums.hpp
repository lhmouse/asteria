// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_ENUMS_
#define ASTERIA_COMPILER_ENUMS_

#include "../fwd.hpp"
namespace asteria {

// Punctuators
enum Punctuator : uint8_t
  {
    punctuator_add         =  0,  // +
    punctuator_add_eq      =  1,  // +=
    punctuator_sub         =  2,  // -
    punctuator_sub_eq      =  3,  // -=
    punctuator_mul         =  4,  // *
    punctuator_mul_eq      =  5,  // *=
    punctuator_div         =  6,  // /
    punctuator_div_eq      =  7,  // /=
    punctuator_mod         =  8,  // %
    punctuator_mod_eq      =  9,  // %=
    punctuator_inc         = 10,  // ++
    punctuator_dec         = 11,  // --
    punctuator_sll         = 12,  // <<<
    punctuator_sll_eq      = 13,  // <<<=
    punctuator_srl         = 14,  // >>>
    punctuator_srl_eq      = 15,  // >>>=
    punctuator_sla         = 16,  // <<
    punctuator_sla_eq      = 17,  // <<=
    punctuator_sra         = 18,  // >>
    punctuator_sra_eq      = 19,  // >>=
    punctuator_andb        = 20,  // &
    punctuator_andb_eq     = 21,  // &=
    punctuator_andl        = 22,  // &&
    punctuator_andl_eq     = 23,  // &&=
    punctuator_orb         = 24,  // |
    punctuator_orb_eq      = 25,  // |=
    punctuator_orl         = 26,  // ||
    punctuator_orl_eq      = 27,  // ||=
    punctuator_xorb        = 28,  // ^
    punctuator_xorb_eq     = 29,  // ^=
    punctuator_notb        = 30,  // ~
    punctuator_notl        = 31,  // !
    punctuator_cmp_eq      = 32,  // ==
    punctuator_cmp_ne      = 33,  // !=
    punctuator_cmp_lt      = 34,  // <
    punctuator_cmp_gt      = 35,  // >
    punctuator_cmp_lte     = 36,  // <=
    punctuator_cmp_gte     = 37,  // >=
    punctuator_dot         = 38,  // .
    punctuator_quest       = 39,  // ?
    punctuator_quest_eq    = 40,  // ?=
    punctuator_assign      = 41,  // =
    punctuator_parenth_op  = 42,  // (
    punctuator_parenth_cl  = 43,  // )
    punctuator_bracket_op  = 44,  // [
    punctuator_bracket_cl  = 45,  // ]
    punctuator_brace_op    = 46,  // {
    punctuator_brace_cl    = 47,  // }
    punctuator_comma       = 48,  // ,
    punctuator_colon       = 49,  // :
    punctuator_semicol     = 50,  // ;
    punctuator_cmp_3way    = 51,  // <=>
    punctuator_coales      = 52,  // ??
    punctuator_coales_eq   = 53,  // ??=
    punctuator_ellipsis    = 54,  // ...
    punctuator_head        = 55,  // [^]
    punctuator_tail        = 56,  // [$]
    punctuator_arrow       = 57,  // ->
    punctuator_scope       = 58,  // ::
    punctuator_random      = 59,  // [?]
    punctuator_cmp_un      = 60,  // </>
  };

ROCKET_CONST
const char*
stringify_punctuator(Punctuator punct) noexcept;

// Keywords
enum Keyword : uint8_t
  {
    keyword_var       =  0,
    keyword_const     =  1,
    keyword_func      =  2,
    keyword_if        =  3,
    keyword_else      =  4,
    keyword_switch    =  5,
    keyword_case      =  6,
    keyword_default   =  7,
    keyword_do        =  8,
    keyword_while     =  9,
    keyword_for       = 10,
    keyword_each      = 11,
    keyword_try       = 12,
    keyword_catch     = 13,
    keyword_defer     = 14,
    keyword_break     = 15,
    keyword_continue  = 16,
    keyword_throw     = 17,
    keyword_return    = 18,
    keyword_null      = 19,
    keyword_true      = 20,
    keyword_false     = 21,
    keyword_import    = 22,
    keyword_ref       = 23,
    keyword_this      = 24,
    keyword_unset     = 25,
    keyword_countof   = 26,
    keyword_typeof    = 27,
    keyword_and       = 28,
    keyword_or        = 29,
    keyword_not       = 30,
    keyword_assert    = 31,
    keyword_sqrt      = 32,  // __sqrt
    keyword_isnan     = 33,  // __isnan
    keyword_isinf     = 34,  // __isinf
    keyword_abs       = 35,  // __abs
    keyword_sign      = 36,  // __sign
    keyword_round     = 37,  // __round
    keyword_floor     = 38,  // __floor
    keyword_ceil      = 39,  // __ceil
    keyword_trunc     = 40,  // __trunc
    keyword_iround    = 41,  // __iround
    keyword_ifloor    = 42,  // __ifloor
    keyword_iceil     = 43,  // __iceil
    keyword_itrunc    = 44,  // __itrunc
    keyword_fma       = 45,  // __fma
    keyword_extern    = 46,  // extern
    keyword_vcall     = 47,  // __vcall
    keyword_lzcnt     = 48,  // __lzcnt
    keyword_tzcnt     = 49,  // __tzcnt
    keyword_popcnt    = 50,  // __popcnt
    keyword_addm      = 51,  // __addm
    keyword_subm      = 52,  // __subm
    keyword_mulm      = 53,  // __mulm
    keyword_adds      = 54,  // __adds
    keyword_subs      = 55,  // __subs
    keyword_muls      = 56,  // __muls
  };

ROCKET_CONST
const char*
stringify_keyword(Keyword kwrd) noexcept;

// Target of jump statements
enum Jump_Target : uint8_t
  {
    jump_target_unspec  = 0,
    jump_target_switch  = 1,
    jump_target_while   = 2,
    jump_target_for     = 3,
  };

// Infix operator precedences
enum Precedence : uint8_t
  {
    precedence_multiplicative  =  1,
    precedence_additive        =  2,
    precedence_shift           =  3,
    precedence_bitwise_and     =  4,
    precedence_bitwise_or      =  5,
    precedence_relational      =  6,
    precedence_equality        =  7,
    precedence_logical_and     =  8,
    precedence_logical_or      =  9,
    precedence_coalescence     = 10,
    precedence_assignment      = 11,
    precedence_lowest          = 99,
  };

}  // namespace asteria
#endif
