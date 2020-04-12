// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_NUMERIC_HPP_
#define ASTERIA_LIBRARY_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.numeric.abs`
Ival
std_numeric_abs(Ival value);

Rval
std_numeric_abs(Rval value);

// `std.numeric.sign`
Ival
std_numeric_sign(Ival value);

Ival
std_numeric_sign(Rval value);

// `std.numeric.is_finite`
Bval
std_numeric_is_finite(Ival value);

Bval
std_numeric_is_finite(Rval value);

// `std.numeric.is_infinity`
Bval
std_numeric_is_infinity(Ival value);

Bval
std_numeric_is_infinity(Rval value);

// `std.numeric.is_nan`
Bval
std_numeric_is_nan(Ival value);

Bval
std_numeric_is_nan(Rval value);

// `std.numeric.clamp`
Ival
std_numeric_clamp(Ival value, Ival lower, Ival upper);

Rval
std_numeric_clamp(Rval value, Rval lower, Rval upper);

// `std.numeric.round`
Ival
std_numeric_round(Ival value);

Rval
std_numeric_round(Rval value);

// `std.numeric.floor`
Ival
std_numeric_floor(Ival value);

Rval
std_numeric_floor(Rval value);

// `std.numeric.ceil`
Ival
std_numeric_ceil(Ival value);

Rval
std_numeric_ceil(Rval value);

// `std.numeric.trunc`
Ival
std_numeric_trunc(Ival value);

Rval
std_numeric_trunc(Rval value);

// `std.numeric.iround`
Ival
std_numeric_iround(Ival value);

Ival
std_numeric_iround(Rval value);

// `std.numeric.ifloor`
Ival
std_numeric_ifloor(Ival value);

Ival
std_numeric_ifloor(Rval value);

// `std.numeric.iceil`
Ival
std_numeric_iceil(Ival value);

Ival
std_numeric_iceil(Rval value);

// `std.numeric.itrunc`
Ival
std_numeric_itrunc(Ival value);

Ival
std_numeric_itrunc(Rval value);

// `std.numeric.random`
Rval
std_numeric_random(Global& global, Ropt limit);

// `std.numeric.sqrt`
Rval
std_numeric_sqrt(Rval x);

// `std.numeric.fma`
Rval
std_numeric_fma(Rval x, Rval y, Rval z);

// `std.numeric.remainder`
Rval
std_numeric_remainder(Rval x, Rval y);

// `std.numeric.frexp`
pair<Rval, Ival>
std_numeric_frexp(Rval x);

// `std.numeric.ldexp`
Rval
std_numeric_ldexp(Rval frac, Ival exp);

// `std.numeric.addm`
Ival
std_numeric_addm(Ival x, Ival y);

// `std.numeric.subm`
Ival
std_numeric_subm(Ival x, Ival y);

// `std.numeric.mulm`
Ival
std_numeric_mulm(Ival x, Ival y);

// `std.numeric.adds`
Ival
std_numeric_adds(Ival x, Ival y);

Rval
std_numeric_adds(Rval x, Rval y);

// `std.numeric.subs`
Ival
std_numeric_subs(Ival x, Ival y);

Rval
std_numeric_subs(Rval x, Rval y);

// `std.numeric.muls`
Ival
std_numeric_muls(Ival x, Ival y);

Rval
std_numeric_muls(Rval x, Rval y);

// `std.numeric.lzcnt`
Ival
std_numeric_lzcnt(Ival x);

// `std.numeric.tzcnt`
Ival
std_numeric_tzcnt(Ival x);

// `std.numeric.popcnt`
Ival
std_numeric_popcnt(Ival x);

// `std.numeric.rotl`
Ival
std_numeric_rotl(Ival m, Ival x, Ival n);

// `std.numeric.rotr`
Ival
std_numeric_rotr(Ival m, Ival x, Ival n);

// `std.numeric.format`
Sval
std_numeric_format(Ival value, Iopt base, Iopt ebase);

// `std.numeric.format`
Sval
std_numeric_format(Rval value, Iopt base, Iopt ebase);

// `std.numeric.parse_integer`
Ival
std_numeric_parse_integer(Sval text);

// `std.numeric.parse_real`
Rval
std_numeric_parse_real(Sval text, Bopt saturating);

// Create an object that is to be referenced as `std.numeric`.
void
create_bindings_numeric(V_object& result, API_Version version);

}  // namespace Asteria

#endif
