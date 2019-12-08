// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_MATH_HPP_
#define ASTERIA_LIBRARY_BINDINGS_MATH_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_real std_math_exp(aref<G_real> y, aopt<G_real> base);
extern G_real std_math_expm1(aref<G_real> y);
extern G_real std_math_pow(aref<G_real> x, aref<G_real> y);
extern G_real std_math_log(aref<G_real> x, aopt<G_real> base);
extern G_real std_math_log1p(aref<G_real> x);

extern G_real std_math_sin(aref<G_real> x);
extern G_real std_math_cos(aref<G_real> x);
extern G_real std_math_tan(aref<G_real> x);
extern G_real std_math_asin(aref<G_real> x);
extern G_real std_math_acos(aref<G_real> x);
extern G_real std_math_atan(aref<G_real> x);
extern G_real std_math_atan2(aref<G_real> y, aref<G_real> x);

extern G_real std_math_hypot(const cow_vector<Value>& values);

extern G_real std_math_sinh(aref<G_real> x);
extern G_real std_math_cosh(aref<G_real> x);
extern G_real std_math_tanh(aref<G_real> x);
extern G_real std_math_asinh(aref<G_real> x);
extern G_real std_math_acosh(aref<G_real> x);
extern G_real std_math_atanh(aref<G_real> x);

extern G_real std_math_erf(aref<G_real> x);
extern G_real std_math_cerf(aref<G_real> x);
extern G_real std_math_gamma(aref<G_real> x);
extern G_real std_math_lgamma(aref<G_real> x);

// Create an object that is to be referenced as `std.math`.
extern void create_bindings_math(G_object& result, API_Version version);

}  // namespace Asteria

#endif
