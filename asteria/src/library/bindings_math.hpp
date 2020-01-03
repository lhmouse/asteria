// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_MATH_HPP_
#define ASTERIA_LIBRARY_BINDINGS_MATH_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Rval std_math_exp(Rval y, Ropt base);
extern Rval std_math_expm1(Rval y);
extern Rval std_math_pow(Rval x, Rval y);
extern Rval std_math_log(Rval x, Ropt base);
extern Rval std_math_log1p(Rval x);

extern Rval std_math_sin(Rval x);
extern Rval std_math_cos(Rval x);
extern Rval std_math_tan(Rval x);
extern Rval std_math_asin(Rval x);
extern Rval std_math_acos(Rval x);
extern Rval std_math_atan(Rval x);
extern Rval std_math_atan2(Rval y, Rval x);

extern Rval std_math_hypot(cow_vector<Value> values);

extern Rval std_math_sinh(Rval x);
extern Rval std_math_cosh(Rval x);
extern Rval std_math_tanh(Rval x);
extern Rval std_math_asinh(Rval x);
extern Rval std_math_acosh(Rval x);
extern Rval std_math_atanh(Rval x);

extern Rval std_math_erf(Rval x);
extern Rval std_math_cerf(Rval x);
extern Rval std_math_gamma(Rval x);
extern Rval std_math_lgamma(Rval x);

// Create an object that is to be referenced as `std.math`.
extern void create_bindings_math(Oval& result, API_Version version);

}  // namespace Asteria

#endif
