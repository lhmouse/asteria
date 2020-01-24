// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_MATH_HPP_
#define ASTERIA_LIBRARY_BINDINGS_MATH_HPP_

#include "../fwd.hpp"

namespace Asteria {

Rval std_math_exp(Rval y, Ropt base);
Rval std_math_expm1(Rval y);
Rval std_math_pow(Rval x, Rval y);
Rval std_math_log(Rval x, Ropt base);
Rval std_math_log1p(Rval x);

Rval std_math_sin(Rval x);
Rval std_math_cos(Rval x);
Rval std_math_tan(Rval x);
Rval std_math_asin(Rval x);
Rval std_math_acos(Rval x);
Rval std_math_atan(Rval x);
Rval std_math_atan2(Rval y, Rval x);

Rval std_math_hypot(cow_vector<Value> values);

Rval std_math_sinh(Rval x);
Rval std_math_cosh(Rval x);
Rval std_math_tanh(Rval x);
Rval std_math_asinh(Rval x);
Rval std_math_acosh(Rval x);
Rval std_math_atanh(Rval x);

Rval std_math_erf(Rval x);
Rval std_math_cerf(Rval x);
Rval std_math_gamma(Rval x);
Rval std_math_lgamma(Rval x);

// Create an object that is to be referenced as `std.math`.
void create_bindings_math(Oval& result, API_Version version);

}  // namespace Asteria

#endif
