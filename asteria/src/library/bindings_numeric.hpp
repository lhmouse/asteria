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

extern G_integer std_numeric_random(const Global_Context& global, const G_integer& upper);
extern G_real std_numeric_random(const Global_Context& global, const Opt<G_real>& upper);
extern G_integer std_numeric_random(const Global_Context& global, const G_integer& lower, const G_integer& upper);
extern G_real std_numeric_random(const Global_Context& global, const G_real& lower, const G_real& upper);

// Create an object that is to be referenced as `std.gc`.
extern void create_bindings_numeric(G_object& result, API_Version version);

}  // namespace Asteria

#endif
