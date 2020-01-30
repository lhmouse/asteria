// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

Ival std_numeric_abs(Ival value);
Rval std_numeric_abs(Rval value);

Ival std_numeric_sign(Ival value);
Ival std_numeric_sign(Rval value);

Bval std_numeric_is_finite(Ival value);
Bval std_numeric_is_finite(Rval value);
Bval std_numeric_is_infinity(Ival value);
Bval std_numeric_is_infinity(Rval value);
Bval std_numeric_is_nan(Ival value);
Bval std_numeric_is_nan(Rval value);

Ival std_numeric_clamp(Ival value, Ival lower, Ival upper);
Rval std_numeric_clamp(Rval value, Rval lower, Rval upper);

Ival std_numeric_round(Ival value);
Rval std_numeric_round(Rval value);
Ival std_numeric_floor(Ival value);
Rval std_numeric_floor(Rval value);
Ival std_numeric_ceil(Ival value);
Rval std_numeric_ceil(Rval value);
Ival std_numeric_trunc(Ival value);
Rval std_numeric_trunc(Rval value);

Ival std_numeric_iround(Ival value);
Ival std_numeric_iround(Rval value);
Ival std_numeric_ifloor(Ival value);
Ival std_numeric_ifloor(Rval value);
Ival std_numeric_iceil(Ival value);
Ival std_numeric_iceil(Rval value);
Ival std_numeric_itrunc(Ival value);
Ival std_numeric_itrunc(Rval value);

Rval std_numeric_random(Global& global, Ropt limit = nullopt);

Rval std_numeric_sqrt(Rval x);
Rval std_numeric_fma(Rval x, Rval y, Rval z);
Rval std_numeric_remainder(Rval x, Rval y);

pair<Rval, Ival> std_numeric_frexp(Rval x);
Rval std_numeric_ldexp(Rval frac, Ival exp);

Ival std_numeric_addm(Ival x, Ival y);
Ival std_numeric_subm(Ival x, Ival y);
Ival std_numeric_mulm(Ival x, Ival y);

Ival std_numeric_adds(Ival x, Ival y);
Rval std_numeric_adds(Rval x, Rval y);
Ival std_numeric_subs(Ival x, Ival y);
Rval std_numeric_subs(Rval x, Rval y);
Ival std_numeric_muls(Ival x, Ival y);
Rval std_numeric_muls(Rval x, Rval y);

Ival std_numeric_lzcnt(Ival x);
Ival std_numeric_tzcnt(Ival x);
Ival std_numeric_popcnt(Ival x);
Ival std_numeric_rotl(Ival m, Ival x, Ival n);
Ival std_numeric_rotr(Ival m, Ival x, Ival n);

Sval std_numeric_format(Ival value, Iopt base = nullopt, Iopt ebase = nullopt);
Sval std_numeric_format(Rval value, Iopt base = nullopt, Iopt ebase = nullopt);
Ival std_numeric_parse_integer(Sval text);
Rval std_numeric_parse_real(Sval text, Bopt saturating = nullopt);

// Create an object that is to be referenced as `std.numeric`.
void create_bindings_numeric(Oval& result, API_Version version);

}  // namespace Asteria

#endif
