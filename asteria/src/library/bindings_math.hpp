// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_MATH_HPP_
#define ASTERIA_LIBRARY_BINDINGS_MATH_HPP_

#include "../fwd.hpp"

namespace Asteria {

Rval math_exp(Rval y, Ropt base);
Rval math_expm1(Rval y);
Rval math_pow(Rval x, Rval y);
Rval math_log(Rval x, Ropt base);
Rval math_log1p(Rval x);

Rval math_sin(Rval x);
Rval math_cos(Rval x);
Rval math_tan(Rval x);
Rval math_asin(Rval x);
Rval math_acos(Rval x);
Rval math_atan(Rval x);
Rval math_atan2(Rval y, Rval x);

Rval math_hypot(cow_vector<Value> values);

Rval math_sinh(Rval x);
Rval math_cosh(Rval x);
Rval math_tanh(Rval x);
Rval math_asinh(Rval x);
Rval math_acosh(Rval x);
Rval math_atanh(Rval x);

Rval math_erf(Rval x);
Rval math_cerf(Rval x);
Rval math_gamma(Rval x);
Rval math_lgamma(Rval x);

// Create an object that is to be referenced as `std.math`.
void create_bindings_math(Oval& result, API_Version version);

}  // namespace Asteria

#endif
