// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern D_integer std_numeric_abs(const D_integer& value);
extern D_real std_numeric_abs(const D_real& value);

extern D_integer std_numeric_signbit(const D_integer& value);
extern D_integer std_numeric_signbit(const D_real& value);

extern D_integer std_numeric_clamp(const D_integer& value, const D_integer& lower, const D_integer& upper);
extern D_real std_numeric_clamp(const D_integer& value, const D_real& lower, const D_real& upper);
extern D_real std_numeric_clamp(const D_real& value, const D_integer& lower, const D_integer& upper);
extern D_real std_numeric_clamp(const D_real& value, const D_real& lower, const D_real& upper);

extern D_real std_numeric_random();
extern D_integer std_numeric_random(const D_integer& upper);
extern D_real std_numeric_random(const D_real& upper);
extern D_integer std_numeric_random(const D_integer& lower, const D_integer& upper);
extern D_real std_numeric_random(const D_real& lower, const D_real& upper);

// Create an object that is to be referenced as `std.gc`.
extern void create_bindings_numeric(D_object& result, API_Version version);

}  // namespace Asteria

#endif
