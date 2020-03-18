// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_NUMERIC_HPP_
#define ASTERIA_LIBRARY_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

Ival numeric_abs(Ival value);
Rval numeric_abs(Rval value);

Ival numeric_sign(Ival value);
Ival numeric_sign(Rval value);

Bval numeric_is_finite(Ival value);
Bval numeric_is_finite(Rval value);
Bval numeric_is_infinity(Ival value);
Bval numeric_is_infinity(Rval value);
Bval numeric_is_nan(Ival value);
Bval numeric_is_nan(Rval value);

Ival numeric_clamp(Ival value, Ival lower, Ival upper);
Rval numeric_clamp(Rval value, Rval lower, Rval upper);

Ival numeric_round(Ival value);
Rval numeric_round(Rval value);
Ival numeric_floor(Ival value);
Rval numeric_floor(Rval value);
Ival numeric_ceil(Ival value);
Rval numeric_ceil(Rval value);
Ival numeric_trunc(Ival value);
Rval numeric_trunc(Rval value);

Ival numeric_iround(Ival value);
Ival numeric_iround(Rval value);
Ival numeric_ifloor(Ival value);
Ival numeric_ifloor(Rval value);
Ival numeric_iceil(Ival value);
Ival numeric_iceil(Rval value);
Ival numeric_itrunc(Ival value);
Ival numeric_itrunc(Rval value);

Rval numeric_random(Global& global, Ropt limit = { });

Rval numeric_sqrt(Rval x);
Rval numeric_fma(Rval x, Rval y, Rval z);
Rval numeric_remainder(Rval x, Rval y);

pair<Rval, Ival> numeric_frexp(Rval x);
Rval numeric_ldexp(Rval frac, Ival exp);

Ival numeric_addm(Ival x, Ival y);
Ival numeric_subm(Ival x, Ival y);
Ival numeric_mulm(Ival x, Ival y);

Ival numeric_adds(Ival x, Ival y);
Rval numeric_adds(Rval x, Rval y);
Ival numeric_subs(Ival x, Ival y);
Rval numeric_subs(Rval x, Rval y);
Ival numeric_muls(Ival x, Ival y);
Rval numeric_muls(Rval x, Rval y);

Ival numeric_lzcnt(Ival x);
Ival numeric_tzcnt(Ival x);
Ival numeric_popcnt(Ival x);
Ival numeric_rotl(Ival m, Ival x, Ival n);
Ival numeric_rotr(Ival m, Ival x, Ival n);

Sval numeric_format(Ival value, Iopt base = { }, Iopt ebase = { });
Sval numeric_format(Rval value, Iopt base = { }, Iopt ebase = { });
Ival numeric_parse_integer(Sval text);
Rval numeric_parse_real(Sval text, Bopt saturating = { });

// Create an object that is to be referenced as `std.numeric`.
void create_bindings_numeric(Oval& result, API_Version version);

}  // namespace Asteria

#endif
