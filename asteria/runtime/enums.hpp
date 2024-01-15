// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ENUMS_
#define ASTERIA_RUNTIME_ENUMS_

#include "../fwd.hpp"
namespace asteria {

enum Xop : uint8_t
  {
    xop_inc       =  0,  // ++
    xop_dec       =  1,  // --
    xop_index     =  2,  // []
    xop_pos       =  3,  // +
    xop_neg       =  4,  // -
    xop_notb      =  5,  // ~
    xop_notl      =  6,  // !
    xop_unset     =  7,  // unset
    xop_countof   =  8,  // countof
    xop_typeof    =  9,  // typeof
    xop_sqrt      = 10,  // __sqrt
    xop_isnan     = 11,  // __isnan
    xop_isinf     = 12,  // __isinf
    xop_abs       = 13,  // __abs
    xop_sign      = 14,  // __sign
    xop_round     = 15,  // __round
    xop_floor     = 16,  // __floor
    xop_ceil      = 17,  // __ceil
    xop_trunc     = 18,  // __trunc
    xop_iround    = 19,  // __iround
    xop_ifloor    = 20,  // __ifloor
    xop_iceil     = 21,  // __iceil
    xop_itrunc    = 22,  // __itrunc
    xop_cmp_eq    = 23,  // ==
    xop_cmp_ne    = 24,  // !=
    xop_cmp_lt    = 25,  // <
    xop_cmp_gt    = 26,  // >
    xop_cmp_lte   = 27,  // <=
    xop_cmp_gte   = 28,  // >=
    xop_cmp_3way  = 29,  // <=>
    xop_cmp_un    = 30,  // </>
    xop_add       = 31,  // +
    xop_sub       = 32,  // -
    xop_mul       = 33,  // *
    xop_div       = 34,  // /
    xop_mod       = 35,  // %
    xop_sll       = 36,  // <<<
    xop_srl       = 37,  // >>>
    xop_sla       = 38,  // <<
    xop_sra       = 39,  // >>
    xop_andb      = 40,  // &
    xop_orb       = 41,  // |
    xop_xorb      = 42,  // ^
    xop_assign    = 43,  // =
    xop_fma       = 44,  // __fma()
    xop_head      = 45,  // [^]
    xop_tail      = 46,  // [$]
    xop_lzcnt     = 47,  // __lzcnt
    xop_tzcnt     = 48,  // __tzcnt
    xop_popcnt    = 49,  // __popcnt
    xop_addm      = 50,  // __addm
    xop_subm      = 51,  // __subm
    xop_mulm      = 52,  // __mulm
    xop_adds      = 53,  // __adds
    xop_subs      = 54,  // __subs
    xop_muls      = 55,  // __muls
    xop_random    = 56,  // [?]
    xop_isvoid    = 57,  // __isvoid
  };

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

enum PTC_Aware : uint8_t
  {
    ptc_aware_none    = 0,
    ptc_aware_by_ref  = 1,
    ptc_aware_by_val  = 2,
    ptc_aware_void    = 3,
  };

}  // namespace asteria
#endif
