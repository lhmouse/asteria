// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_integer std_numeric_abs(G_integer value);
extern G_real std_numeric_abs(G_real value);

extern G_integer std_numeric_sign(G_integer value);
extern G_integer std_numeric_sign(G_real value);

extern G_boolean std_numeric_is_finite(G_integer value);
extern G_boolean std_numeric_is_finite(G_real value);
extern G_boolean std_numeric_is_infinity(G_integer value);
extern G_boolean std_numeric_is_infinity(G_real value);
extern G_boolean std_numeric_is_nan(G_integer value);
extern G_boolean std_numeric_is_nan(G_real value);

extern G_integer std_numeric_clamp(G_integer value, G_integer lower, G_integer upper);
extern G_real std_numeric_clamp(G_real value, G_real lower, G_real upper);

extern G_integer std_numeric_round(G_integer value);
extern G_real std_numeric_round(G_real value);
extern G_integer std_numeric_floor(G_integer value);
extern G_real std_numeric_floor(G_real value);
extern G_integer std_numeric_ceil(G_integer value);
extern G_real std_numeric_ceil(G_real value);
extern G_integer std_numeric_trunc(G_integer value);
extern G_real std_numeric_trunc(G_real value);

extern G_integer std_numeric_iround(G_integer value);
extern G_integer std_numeric_iround(G_real value);
extern G_integer std_numeric_ifloor(G_integer value);
extern G_integer std_numeric_ifloor(G_real value);
extern G_integer std_numeric_iceil(G_integer value);
extern G_integer std_numeric_iceil(G_real value);
extern G_integer std_numeric_itrunc(G_integer value);
extern G_integer std_numeric_itrunc(G_real value);

extern G_real std_numeric_random(Global_Context& global, opt<G_real> limit = ::rocket::clear);

extern G_real std_numeric_sqrt(G_real x);
extern G_real std_numeric_fma(G_real x, G_real y, G_real z);
extern G_real std_numeric_remainder(G_real x, G_real y);

extern pair<G_real, G_integer> std_numeric_frexp(G_real x);
extern G_real std_numeric_ldexp(G_real frac, G_integer exp);

extern G_integer std_numeric_addm(G_integer x, G_integer y);
extern G_integer std_numeric_subm(G_integer x, G_integer y);
extern G_integer std_numeric_mulm(G_integer x, G_integer y);

extern G_integer std_numeric_adds(G_integer x, G_integer y);
extern G_real std_numeric_adds(G_real x, G_real y);
extern G_integer std_numeric_subs(G_integer x, G_integer y);
extern G_real std_numeric_subs(G_real x, G_real y);
extern G_integer std_numeric_muls(G_integer x, G_integer y);
extern G_real std_numeric_muls(G_real x, G_real y);

extern G_integer std_numeric_lzcnt(G_integer x);
extern G_integer std_numeric_tzcnt(G_integer x);
extern G_integer std_numeric_popcnt(G_integer x);
extern G_integer std_numeric_rotl(G_integer m, G_integer x, G_integer n);
extern G_integer std_numeric_rotr(G_integer m, G_integer x, G_integer n);

extern G_string std_numeric_format(G_integer value, opt<G_integer> base = ::rocket::clear, opt<G_integer> ebase = ::rocket::clear);
extern G_string std_numeric_format(G_real value, opt<G_integer> base = ::rocket::clear, opt<G_integer> ebase = ::rocket::clear);
extern opt<G_integer> std_numeric_parse_integer(G_string text);
extern opt<G_real> std_numeric_parse_real(G_string text, opt<G_boolean> saturating = ::rocket::clear);

// Create an object that is to be referenced as `std.numeric`.
extern void create_bindings_numeric(G_object& result, API_Version version);

}  // namespace Asteria

#endif
