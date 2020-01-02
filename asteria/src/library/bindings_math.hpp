// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_MATH_HPP_
#define ASTERIA_LIBRARY_BINDINGS_MATH_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_real std_math_exp(G_real y, opt<G_real> base);
extern G_real std_math_expm1(G_real y);
extern G_real std_math_pow(G_real x, G_real y);
extern G_real std_math_log(G_real x, opt<G_real> base);
extern G_real std_math_log1p(G_real x);

extern G_real std_math_sin(G_real x);
extern G_real std_math_cos(G_real x);
extern G_real std_math_tan(G_real x);
extern G_real std_math_asin(G_real x);
extern G_real std_math_acos(G_real x);
extern G_real std_math_atan(G_real x);
extern G_real std_math_atan2(G_real y, G_real x);

extern G_real std_math_hypot(cow_vector<Value> values);

extern G_real std_math_sinh(G_real x);
extern G_real std_math_cosh(G_real x);
extern G_real std_math_tanh(G_real x);
extern G_real std_math_asinh(G_real x);
extern G_real std_math_acosh(G_real x);
extern G_real std_math_atanh(G_real x);

extern G_real std_math_erf(G_real x);
extern G_real std_math_cerf(G_real x);
extern G_real std_math_gamma(G_real x);
extern G_real std_math_lgamma(G_real x);

// Create an object that is to be referenced as `std.math`.
extern void create_bindings_math(G_object& result, API_Version version);

}  // namespace Asteria

#endif
