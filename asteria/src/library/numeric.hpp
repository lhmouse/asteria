// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_NUMERIC_HPP_
#define ASTERIA_LIBRARY_NUMERIC_HPP_

#include "../fwd.hpp"

namespace asteria {

// `std.numeric.abs`
V_integer
std_numeric_abs(V_integer value);

V_real
std_numeric_abs(V_real value);

// `std.numeric.sign`
V_integer
std_numeric_sign(V_integer value);

V_integer
std_numeric_sign(V_real value);

// `std.numeric.is_finite`
V_boolean
std_numeric_is_finite(V_integer value);

V_boolean
std_numeric_is_finite(V_real value);

// `std.numeric.is_infinity`
V_boolean
std_numeric_is_infinity(V_integer value);

V_boolean
std_numeric_is_infinity(V_real value);

// `std.numeric.is_nan`
V_boolean
std_numeric_is_nan(V_integer value);

V_boolean
std_numeric_is_nan(V_real value);

// `std.numeric.max`
Value
std_numeric_max(cow_vector<Value> values);

// `std.numeric.min`
Value
std_numeric_min(cow_vector<Value> values);

// `std.numeric.clamp`
V_integer
std_numeric_clamp(V_integer value, V_integer lower, V_integer upper);

V_real
std_numeric_clamp(V_real value, V_real lower, V_real upper);

// `std.numeric.round`
V_integer
std_numeric_round(V_integer value);

V_real
std_numeric_round(V_real value);

// `std.numeric.iround`
V_integer
std_numeric_iround(V_integer value);

V_integer
std_numeric_iround(V_real value);

// `std.numeric.floor`
V_integer
std_numeric_floor(V_integer value);

V_real
std_numeric_floor(V_real value);

// `std.numeric.ifloor`
V_integer
std_numeric_ifloor(V_integer value);

V_integer
std_numeric_ifloor(V_real value);

// `std.numeric.ceil`
V_integer
std_numeric_ceil(V_integer value);

V_real
std_numeric_ceil(V_real value);

// `std.numeric.iceil`
V_integer
std_numeric_iceil(V_integer value);

V_integer
std_numeric_iceil(V_real value);

// `std.numeric.trunc`
V_integer
std_numeric_trunc(V_integer value);

V_real
std_numeric_trunc(V_real value);

// `std.numeric.itrunc`
V_integer
std_numeric_itrunc(V_integer value);

V_integer
std_numeric_itrunc(V_real value);

// `std.numeric.random`
V_real
std_numeric_random(Global_Context& global, optV_real limit);

// `std.numeric.sqrt`
V_real
std_numeric_sqrt(V_real x);

// `std.numeric.fma`
V_real
std_numeric_fma(V_real x, V_real y, V_real z);

// `std.numeric.remainder`
V_real
std_numeric_remainder(V_real x, V_real y);

// `std.numeric.frexp`
pair<V_real, V_integer>
std_numeric_frexp(V_real x);

// `std.numeric.ldexp`
V_real
std_numeric_ldexp(V_real frac, V_integer exp);

// `std.numeric.addm`
V_integer
std_numeric_addm(V_integer x, V_integer y);

// `std.numeric.subm`
V_integer
std_numeric_subm(V_integer x, V_integer y);

// `std.numeric.mulm`
V_integer
std_numeric_mulm(V_integer x, V_integer y);

// `std.numeric.adds`
V_integer
std_numeric_adds(V_integer x, V_integer y);

V_real
std_numeric_adds(V_real x, V_real y);

// `std.numeric.subs`
V_integer
std_numeric_subs(V_integer x, V_integer y);

V_real
std_numeric_subs(V_real x, V_real y);

// `std.numeric.muls`
V_integer
std_numeric_muls(V_integer x, V_integer y);

V_real
std_numeric_muls(V_real x, V_real y);

// `std.numeric.lzcnt`
V_integer
std_numeric_lzcnt(V_integer x);

// `std.numeric.tzcnt`
V_integer
std_numeric_tzcnt(V_integer x);

// `std.numeric.popcnt`
V_integer
std_numeric_popcnt(V_integer x);

// `std.numeric.rotl`
V_integer
std_numeric_rotl(V_integer m, V_integer x, V_integer n);

// `std.numeric.rotr`
V_integer
std_numeric_rotr(V_integer m, V_integer x, V_integer n);

// `std.numeric.format`
V_string
std_numeric_format(V_integer value, optV_integer base, optV_integer ebase);

// `std.numeric.format`
V_string
std_numeric_format(V_real value, optV_integer base, optV_integer ebase);

// `std.numeric.parse_integer`
V_integer
std_numeric_parse_integer(V_string text);

// `std.numeric.parse_real`
V_real
std_numeric_parse_real(V_string text, optV_boolean saturating);

// Create an object that is to be referenced as `std.numeric`.
void
create_bindings_numeric(V_object& result, API_Version version);

}  // namespace asteria

#endif
