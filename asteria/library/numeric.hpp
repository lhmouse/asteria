// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_NUMERIC_
#define ASTERIA_LIBRARY_NUMERIC_

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

// `std.numeric.remainder`
V_real
std_numeric_remainder(V_real x, V_real y);

// `std.numeric.frexp`
pair<V_real, V_integer>
std_numeric_frexp(V_real x);

// `std.numeric.ldexp`
V_real
std_numeric_ldexp(V_real frac, V_integer exp);

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

// `std.numeric.parse`
Value
std_numeric_parse(V_string text);

// `std.numeric.pack_i8`
V_string
std_numeric_pack_i8(V_integer value);

V_string
std_numeric_pack_i8(V_array values);

// `std.numeric.unpack_i8`
V_array
std_numeric_unpack_i8(V_string text);

// `std.numeric.pack_i16be`
V_string
std_numeric_pack_i16be(V_integer value);

V_string
std_numeric_pack_i16be(V_array values);

// `std.numeric.unpack_i16be`
V_array
std_numeric_unpack_i16be(V_string text);

// `std.numeric.pack_i16le`
V_string
std_numeric_pack_i16le(V_integer value);

V_string
std_numeric_pack_i16le(V_array values);

// `std.numeric.unpack_i16le`
V_array
std_numeric_unpack_i16le(V_string text);

// `std.numeric.pack_i32be`
V_string
std_numeric_pack_i32be(V_integer value);

V_string
std_numeric_pack_i32be(V_array values);

// `std.numeric.unpack_i32be`
V_array
std_numeric_unpack_i32be(V_string text);

// `std.numeric.pack_i32le`
V_string
std_numeric_pack_i32le(V_integer value);

V_string
std_numeric_pack_i32le(V_array values);

// `std.numeric.unpack_i32le`
V_array
std_numeric_unpack_i32le(V_string text);

// `std.numeric.pack_i64be`
V_string
std_numeric_pack_i64be(V_integer value);

V_string
std_numeric_pack_i64be(V_array values);

// `std.numeric.unpack_i64be`
V_array
std_numeric_unpack_i64be(V_string text);

// `std.numeric.pack_i64le`
V_string
std_numeric_pack_i64le(V_integer value);

V_string
std_numeric_pack_i64le(V_array values);

// `std.numeric.unpack_i64le`
V_array
std_numeric_unpack_i64le(V_string text);

// `std.numeric.pack_f32be`
V_string
std_numeric_pack_f32be(V_real value);

V_string
std_numeric_pack_f32be(V_array values);

// `std.numeric.unpack_f32be`
V_array
std_numeric_unpack_f32be(V_string text);

// `std.numeric.pack_f32le`
V_string
std_numeric_pack_f32le(V_real value);

V_string
std_numeric_pack_f32le(V_array values);

// `std.numeric.unpack_f32le`
V_array
std_numeric_unpack_f32le(V_string text);

// `std.numeric.pack_f64be`
V_string
std_numeric_pack_f64be(V_real value);

V_string
std_numeric_pack_f64be(V_array values);

// `std.numeric.unpack_f64be`
V_array
std_numeric_unpack_f64be(V_string text);

// `std.numeric.pack_f64le`
V_string
std_numeric_pack_f64le(V_real value);

V_string
std_numeric_pack_f64le(V_array values);

// `std.numeric.unpack_f64le`
V_array
std_numeric_unpack_f64le(V_string text);

// Create an object that is to be referenced as `std.numeric`.
void
create_bindings_numeric(V_object& result, API_Version version);

}  // namespace asteria
#endif
