// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_MATH_HPP_
#define ASTERIA_LIBRARY_BINDINGS_MATH_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_real std_math_exp(const G_real& y, const Opt<G_real>& base);
extern G_real std_math_expm1(const G_real& y);
extern G_real std_math_pow(const G_real& x, const G_real& y);
extern G_real std_math_log(const G_real& x, const Opt<G_real>& base);
extern G_real std_math_log1p(const G_real& x);

// Create an object that is to be referenced as `std.math`.
extern void create_bindings_math(G_object& result, API_Version version);

}  // namespace Asteria

#endif
