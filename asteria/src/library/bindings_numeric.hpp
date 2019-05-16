// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_integer std_numeric_abs(const G_integer& value);
extern G_real std_numeric_abs(const G_real& value);

extern G_integer std_numeric_sign(const G_integer& value);
extern G_integer std_numeric_sign(const G_real& value);

extern G_boolean std_numeric_is_finite(const G_integer& value);
extern G_boolean std_numeric_is_finite(const G_real& value);
extern G_boolean std_numeric_is_infinity(const G_integer& value);
extern G_boolean std_numeric_is_infinity(const G_real& value);
extern G_boolean std_numeric_is_nan(const G_integer& value);
extern G_boolean std_numeric_is_nan(const G_real& value);

extern G_integer std_numeric_clamp(const G_integer& value, const G_integer& lower, const G_integer& upper);
extern G_real std_numeric_clamp(const G_real& value, const G_real& lower, const G_real& upper);

extern G_integer std_numeric_round(const G_integer& value);
extern G_real std_numeric_round(const G_real& value);
extern G_integer std_numeric_floor(const G_integer& value);
extern G_real std_numeric_floor(const G_real& value);
extern G_integer std_numeric_ceil(const G_integer& value);
extern G_real std_numeric_ceil(const G_real& value);
extern G_integer std_numeric_trunc(const G_integer& value);
extern G_real std_numeric_trunc(const G_real& value);

extern G_integer std_numeric_iround(const G_integer& value);
extern G_integer std_numeric_iround(const G_real& value);
extern G_integer std_numeric_ifloor(const G_integer& value);
extern G_integer std_numeric_ifloor(const G_real& value);
extern G_integer std_numeric_iceil(const G_integer& value);
extern G_integer std_numeric_iceil(const G_real& value);
extern G_integer std_numeric_itrunc(const G_integer& value);
extern G_integer std_numeric_itrunc(const G_real& value);

extern G_real std_numeric_random(const Global_Context& global, const Opt<G_real>& limit = rocket::nullopt);

extern G_real std_numeric_sqrt(const G_real& x);
extern G_real std_numeric_fma(const G_real& x, const G_real& y, const G_real& z);

extern G_integer std_numeric_addm(const G_integer& x, const G_integer& y);
extern G_integer std_numeric_subm(const G_integer& x, const G_integer& y);
extern G_integer std_numeric_mulm(const G_integer& x, const G_integer& y);

extern G_integer std_numeric_adds(const G_integer& x, const G_integer& y);
extern G_real std_numeric_adds(const G_real& x, const G_real& y);
extern G_integer std_numeric_subs(const G_integer& x, const G_integer& y);
extern G_real std_numeric_subs(const G_real& x, const G_real& y);
extern G_integer std_numeric_muls(const G_integer& x, const G_integer& y);
extern G_real std_numeric_muls(const G_real& x, const G_real& y);

extern G_string std_numeric_format(const G_integer& value, const Opt<G_integer>& base = rocket::nullopt);
extern G_string std_numeric_format(const G_real& value, const Opt<G_integer>& base = rocket::nullopt);
extern G_string std_numeric_format(const G_integer& value, const Opt<G_integer>& base, const G_integer& exp_base);
extern G_string std_numeric_format(const G_real& value, const Opt<G_integer>& base, const G_integer& exp_base);

extern Opt<G_integer> std_numeric_parse_integer(const G_string& text);
extern Opt<G_real> std_numeric_parse_real(const G_string& text, const Opt<G_boolean>& saturating = rocket::nullopt);

// Create an object that is to be referenced as `std.numeric`.
extern void create_bindings_numeric(G_object& result, API_Version version);

}  // namespace Asteria

#endif
