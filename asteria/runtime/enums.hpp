// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ENUMS_
#define ASTERIA_RUNTIME_ENUMS_

#include "../fwd.hpp"
namespace asteria {

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
    xop_countof   = 10,  // countof
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
    xop_lzcnt     = 48,  // __lzcnt
    xop_tzcnt     = 49,  // __tzcnt
    xop_popcnt    = 50,  // __popcnt
    xop_addm      = 51,  // __addm
    xop_subm      = 52,  // __subm
    xop_mulm      = 53,  // __mulm
    xop_adds      = 54,  // __adds
    xop_subs      = 55,  // __subs
    xop_muls      = 56,  // __muls
    xop_random    = 57,  // [?]
    xop_cmp_un    = 58,  // </>
  };

ROCKET_CONST
const char*
describe_xop(Xop xop) noexcept;

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

// Tail call optimization (PTC) awareness bitmask
enum PTC_Aware : int8_t
  {
    ptc_aware_none    =  0,  // allow PTC only in `return`
    ptc_aware_void    = -1,  // call and discard results
    ptc_aware_by_ref  =  1,  // call and forward results by reference
    ptc_aware_by_val  =  3,  // call and forward results by value
  };

}  // namespace asteria
#endif
