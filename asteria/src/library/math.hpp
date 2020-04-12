// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_MATH_HPP_
#define ASTERIA_LIBRARY_MATH_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.math.exp`
Rval
std_math_exp(Rval y, Ropt base);

// `std.math.expm1`
Rval
std_math_expm1(Rval y);

// `std.math.pow`
Rval
std_math_pow(Rval x, Rval y);

// `std.math.log`
Rval
std_math_log(Rval x, Ropt base);

// `std.math.log1p`
Rval
std_math_log1p(Rval x);

// `std.math.sin`
Rval
std_math_sin(Rval x);

// `std.math.cos`
Rval
std_math_cos(Rval x);

// `std.math.tan`
Rval
std_math_tan(Rval x);

// `std.math.asin`
Rval
std_math_asin(Rval x);

// `std.math.acos`
Rval
std_math_acos(Rval x);

// `std.math.atan`
Rval
std_math_atan(Rval x);

// `std.math.atan2`
Rval
std_math_atan2(Rval y, Rval x);

// `std.math.hypot`
Rval
std_math_hypot(cow_vector<Value> values);

// `std.math.sinh`
Rval
std_math_sinh(Rval x);

// `std.math.cosh`
Rval
std_math_cosh(Rval x);

// `std.math.tanh`
Rval
std_math_tanh(Rval x);

// `std.math.asinh`
Rval
std_math_asinh(Rval x);

// `std.math.acosh`
Rval
std_math_acosh(Rval x);

// `std.math.atanh`
Rval
std_math_atanh(Rval x);

// `std.math.erf`
Rval
std_math_erf(Rval x);

// `std.math.cerf`
Rval
std_math_cerf(Rval x);

// `std.math.gamma`
Rval
std_math_gamma(Rval x);

// `std.math.lgamma`
Rval
std_math_lgamma(Rval x);

// Create an object that is to be referenced as `std.math`.
void
create_bindings_math(V_object& result, API_Version version);

}  // namespace Asteria

#endif
