// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SYNTAX_ENUMS_HPP_
#define ASTERIA_SYNTAX_ENUMS_HPP_

#include "../fwd.hpp"

namespace Asteria {

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
    precedence_relational      =  4,
    precedence_equality        =  5,
    precedence_bitwise_and     =  6,
    precedence_bitwise_xor     =  7,
    precedence_bitwise_or      =  8,
    precedence_logical_and     =  9,
    precedence_logical_or      = 10,
    precedence_coalescence     = 11,
    precedence_assignment      = 12,
    precedence_lowest          = 99,
  };

// Operators
enum Xop : uint8_t
  {
    xop_inc_post  =  0,  // ++ (postfix)
    xop_dec_post  =  1,  // -- (postfix)
    xop_subscr    =  2,  // []
    xop_pos       =  3,  // +
    xop_neg       =  4,  // -
    xop_notb      =  5,  // ~
    xop_notl      =  6,  // !
    xop_inc_pre   =  7,  // ++ (prefix)
    xop_dec_pre   =  8,  // -- (prefix)
    xop_unset     =  9,  // unset
    xop_lengthof  = 10,  // lengthof
    xop_typeof    = 11,  // typeof
    xop_sqrt      = 12,  // __sqrt
    xop_isnan     = 13,  // __isnan
    xop_isinf     = 14,  // __isinf
    xop_abs       = 15,  // __abs
    xop_signb     = 16,  // __signb
    xop_round     = 17,  // __round
    xop_floor     = 18,  // __floor
    xop_ceil      = 19,  // __ceil
    xop_trunc     = 20,  // __trunc
    xop_iround    = 21,  // __iround
    xop_ifloor    = 22,  // __ifloor
    xop_iceil     = 23,  // __iceil
    xop_itrunc    = 24,  // __itrunc
    xop_cmp_eq    = 25,  // ==
    xop_cmp_ne    = 26,  // !=
    xop_cmp_lt    = 27,  // <
    xop_cmp_gt    = 28,  // >
    xop_cmp_lte   = 29,  // <=
    xop_cmp_gte   = 30,  // >=
    xop_cmp_3way  = 31,  // <=>
    xop_add       = 32,  // +
    xop_sub       = 33,  // -
    xop_mul       = 34,  // *
    xop_div       = 35,  // /
    xop_mod       = 36,  // %
    xop_sll       = 37,  // <<<
    xop_srl       = 38,  // >>>
    xop_sla       = 39,  // <<
    xop_sra       = 40,  // >>
    xop_andb      = 41,  // &
    xop_orb       = 42,  // |
    xop_xorb      = 43,  // ^
    xop_assign    = 44,  // =
  };

}  // namespace Asteria

#endif
