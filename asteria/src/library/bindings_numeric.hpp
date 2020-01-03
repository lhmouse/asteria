// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Ival std_numeric_abs(Ival value);
extern Rval std_numeric_abs(Rval value);

extern Ival std_numeric_sign(Ival value);
extern Ival std_numeric_sign(Rval value);

extern Bval std_numeric_is_finite(Ival value);
extern Bval std_numeric_is_finite(Rval value);
extern Bval std_numeric_is_infinity(Ival value);
extern Bval std_numeric_is_infinity(Rval value);
extern Bval std_numeric_is_nan(Ival value);
extern Bval std_numeric_is_nan(Rval value);

extern Ival std_numeric_clamp(Ival value, Ival lower, Ival upper);
extern Rval std_numeric_clamp(Rval value, Rval lower, Rval upper);

extern Ival std_numeric_round(Ival value);
extern Rval std_numeric_round(Rval value);
extern Ival std_numeric_floor(Ival value);
extern Rval std_numeric_floor(Rval value);
extern Ival std_numeric_ceil(Ival value);
extern Rval std_numeric_ceil(Rval value);
extern Ival std_numeric_trunc(Ival value);
extern Rval std_numeric_trunc(Rval value);

extern Ival std_numeric_iround(Ival value);
extern Ival std_numeric_iround(Rval value);
extern Ival std_numeric_ifloor(Ival value);
extern Ival std_numeric_ifloor(Rval value);
extern Ival std_numeric_iceil(Ival value);
extern Ival std_numeric_iceil(Rval value);
extern Ival std_numeric_itrunc(Ival value);
extern Ival std_numeric_itrunc(Rval value);

extern Rval std_numeric_random(Global& global, Ropt limit = ::rocket::clear);

extern Rval std_numeric_sqrt(Rval x);
extern Rval std_numeric_fma(Rval x, Rval y, Rval z);
extern Rval std_numeric_remainder(Rval x, Rval y);

extern pair<Rval, Ival> std_numeric_frexp(Rval x);
extern Rval std_numeric_ldexp(Rval frac, Ival exp);

extern Ival std_numeric_addm(Ival x, Ival y);
extern Ival std_numeric_subm(Ival x, Ival y);
extern Ival std_numeric_mulm(Ival x, Ival y);

extern Ival std_numeric_adds(Ival x, Ival y);
extern Rval std_numeric_adds(Rval x, Rval y);
extern Ival std_numeric_subs(Ival x, Ival y);
extern Rval std_numeric_subs(Rval x, Rval y);
extern Ival std_numeric_muls(Ival x, Ival y);
extern Rval std_numeric_muls(Rval x, Rval y);

extern Ival std_numeric_lzcnt(Ival x);
extern Ival std_numeric_tzcnt(Ival x);
extern Ival std_numeric_popcnt(Ival x);
extern Ival std_numeric_rotl(Ival m, Ival x, Ival n);
extern Ival std_numeric_rotr(Ival m, Ival x, Ival n);

extern Sval std_numeric_format(Ival value, Iopt base = ::rocket::clear, Iopt ebase = ::rocket::clear);
extern Sval std_numeric_format(Rval value, Iopt base = ::rocket::clear, Iopt ebase = ::rocket::clear);
extern Iopt std_numeric_parse_integer(Sval text);
extern Ropt std_numeric_parse_real(Sval text, Bopt saturating = ::rocket::clear);

// Create an object that is to be referenced as `std.numeric`.
extern void create_bindings_numeric(Oval& result, API_Version version);

}  // namespace Asteria

#endif
