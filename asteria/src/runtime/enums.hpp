// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ENUMS_HPP_
#define ASTERIA_RUNTIME_ENUMS_HPP_

#include "../fwd.hpp"

namespace Asteria {

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
    xop_sign      = 16,  // __sign
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
    xop_fma       = 45,  // __fma()
    xop_head      = 46,  // [^]
    xop_tail      = 47,  // [$]
  };

ROCKET_PURE_FUNCTION extern const char* describe_xop(Xop xop) noexcept;

// IR status codes
enum AIR_Status : uint8_t
  {
    air_status_next             = 0,
    air_status_return_void      = 1,
    air_status_return_ref       = 2,
    air_status_break_unspec     = 3,
    air_status_break_switch     = 4,
    air_status_break_while      = 5,
    air_status_break_for        = 6,
    air_status_continue_unspec  = 7,
    air_status_continue_while   = 8,
    air_status_continue_for     = 9,
  };

// Tail call optimization (PTC) awareness
enum PTC_Aware : uint8_t
  {
    ptc_aware_none    = 0,  // Proper tail call is not allowed.
    ptc_aware_by_ref  = 1,  // The result is forwarded by reference.
    ptc_aware_by_val  = 2,  // The result is forwarded by value.
    ptc_aware_void    = 3,  // The call is forwarded but its result is discarded.
  };

}  // namespace Asteria

#endif
