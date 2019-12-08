// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_integer std_numeric_abs(aref<G_integer> value);
extern G_real std_numeric_abs(aref<G_real> value);

extern G_integer std_numeric_sign(aref<G_integer> value);
extern G_integer std_numeric_sign(aref<G_real> value);

extern G_boolean std_numeric_is_finite(aref<G_integer> value);
extern G_boolean std_numeric_is_finite(aref<G_real> value);
extern G_boolean std_numeric_is_infinity(aref<G_integer> value);
extern G_boolean std_numeric_is_infinity(aref<G_real> value);
extern G_boolean std_numeric_is_nan(aref<G_integer> value);
extern G_boolean std_numeric_is_nan(aref<G_real> value);

extern G_integer std_numeric_clamp(aref<G_integer> value, aref<G_integer> lower, aref<G_integer> upper);
extern G_real std_numeric_clamp(aref<G_real> value, aref<G_real> lower, aref<G_real> upper);

extern G_integer std_numeric_round(aref<G_integer> value);
extern G_real std_numeric_round(aref<G_real> value);
extern G_integer std_numeric_floor(aref<G_integer> value);
extern G_real std_numeric_floor(aref<G_real> value);
extern G_integer std_numeric_ceil(aref<G_integer> value);
extern G_real std_numeric_ceil(aref<G_real> value);
extern G_integer std_numeric_trunc(aref<G_integer> value);
extern G_real std_numeric_trunc(aref<G_real> value);

extern G_integer std_numeric_iround(aref<G_integer> value);
extern G_integer std_numeric_iround(aref<G_real> value);
extern G_integer std_numeric_ifloor(aref<G_integer> value);
extern G_integer std_numeric_ifloor(aref<G_real> value);
extern G_integer std_numeric_iceil(aref<G_integer> value);
extern G_integer std_numeric_iceil(aref<G_real> value);
extern G_integer std_numeric_itrunc(aref<G_integer> value);
extern G_integer std_numeric_itrunc(aref<G_real> value);

extern G_real std_numeric_random(const Global_Context& global, aopt<G_real> limit = rocket::clear);

extern G_real std_numeric_sqrt(aref<G_real> x);
extern G_real std_numeric_fma(aref<G_real> x, aref<G_real> y, aref<G_real> z);
extern G_real std_numeric_remainder(aref<G_real> x, aref<G_real> y);

extern pair<G_real, G_integer> std_numeric_frexp(aref<G_real> x);
extern G_real std_numeric_ldexp(aref<G_real> frac, aref<G_integer> exp);

extern G_integer std_numeric_addm(aref<G_integer> x, aref<G_integer> y);
extern G_integer std_numeric_subm(aref<G_integer> x, aref<G_integer> y);
extern G_integer std_numeric_mulm(aref<G_integer> x, aref<G_integer> y);

extern G_integer std_numeric_adds(aref<G_integer> x, aref<G_integer> y);
extern G_real std_numeric_adds(aref<G_real> x, aref<G_real> y);
extern G_integer std_numeric_subs(aref<G_integer> x, aref<G_integer> y);
extern G_real std_numeric_subs(aref<G_real> x, aref<G_real> y);
extern G_integer std_numeric_muls(aref<G_integer> x, aref<G_integer> y);
extern G_real std_numeric_muls(aref<G_real> x, aref<G_real> y);

extern G_integer std_numeric_lzcnt(aref<G_integer> x);
extern G_integer std_numeric_tzcnt(aref<G_integer> x);
extern G_integer std_numeric_popcnt(aref<G_integer> x);

extern G_string std_numeric_format(aref<G_integer> value, aopt<G_integer> base = rocket::clear, aopt<G_integer> ebase = rocket::clear);
extern G_string std_numeric_format(aref<G_real> value, aopt<G_integer> base = rocket::clear, aopt<G_integer> ebase = rocket::clear);
extern opt<G_integer> std_numeric_parse_integer(aref<G_string> text);
extern opt<G_real> std_numeric_parse_real(aref<G_string> text, aopt<G_boolean> saturating = rocket::clear);

// Create an object that is to be referenced as `std.numeric`.
extern void create_bindings_numeric(G_object& result, API_Version version);

}  // namespace Asteria

#endif
