// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_MATH_
#define ASTERIA_LIBRARY_MATH_

#include "../fwd.hpp"
namespace asteria {

// `std.math.exp`
V_real
std_math_exp(V_real y);

V_real
std_math_exp(V_real base, V_real y);

// `std.math.log`
V_real
std_math_log(V_real x);

V_real
std_math_log(V_real base, V_real x);

// `std.math.expm1`
V_real
std_math_expm1(V_real y);

// `std.math.log1p`
V_real
std_math_log1p(V_real x);

// `std.math.sin`
V_real
std_math_sin(V_real x);

// `std.math.cos`
V_real
std_math_cos(V_real x);

// `std.math.sincos`
pair<V_real, V_real>
std_math_sincos(V_real x);

// `std.math.tan`
V_real
std_math_tan(V_real x);

// `std.math.asin`
V_real
std_math_asin(V_real x);

// `std.math.acos`
V_real
std_math_acos(V_real x);

// `std.math.atan`
V_real
std_math_atan(V_real x);

// `std.math.atan2`
V_real
std_math_atan2(V_real y, V_real x);

// `std.math.hypot`
V_real
std_math_hypot(cow_vector<Value> values);

// `std.math.sinh`
V_real
std_math_sinh(V_real x);

// `std.math.cosh`
V_real
std_math_cosh(V_real x);

// `std.math.tanh`
V_real
std_math_tanh(V_real x);

// `std.math.asinh`
V_real
std_math_asinh(V_real x);

// `std.math.acosh`
V_real
std_math_acosh(V_real x);

// `std.math.atanh`
V_real
std_math_atanh(V_real x);

// `std.math.erf`
V_real
std_math_erf(V_real x);

// `std.math.cerf`
V_real
std_math_cerf(V_real x);

// `std.math.gamma`
V_real
std_math_gamma(V_real x);

// `std.math.lgamma`
V_real
std_math_lgamma(V_real x);

// Create an object that is to be referenced as `std.math`.
void
create_bindings_math(V_object& result, API_Version version);

}  // namespace asteria
#endif
