// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_NUMERIC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern D_integer std_numeric_abs(const D_integer& value);
extern D_real std_numeric_abs(const D_real& value);

extern D_integer std_numeric_sign(const D_integer& value);
extern D_integer std_numeric_sign(const D_real& value);

extern D_boolean std_numeric_is_finite(const D_integer& value);
extern D_boolean std_numeric_is_finite(const D_real& value);
extern D_boolean std_numeric_is_infinity(const D_integer& value);
extern D_boolean std_numeric_is_infinity(const D_real& value);
extern D_boolean std_numeric_is_nan(const D_integer& value);
extern D_boolean std_numeric_is_nan(const D_real& value);

extern D_integer std_numeric_clamp(const D_integer& value, const D_integer& lower, const D_integer& upper);
extern D_real std_numeric_clamp(const D_real& value, const D_real& lower, const D_real& upper);

extern D_integer std_numeric_round(const D_integer& value);
extern D_real std_numeric_round(const D_real& value);
extern D_integer std_numeric_floor(const D_integer& value);
extern D_real std_numeric_floor(const D_real& value);
extern D_integer std_numeric_ceil(const D_integer& value);
extern D_real std_numeric_ceil(const D_real& value);
extern D_integer std_numeric_trunc(const D_integer& value);
extern D_real std_numeric_trunc(const D_real& value);

extern D_integer std_numeric_iround(const D_integer& value);
extern D_integer std_numeric_iround(const D_real& value);
extern D_integer std_numeric_ifloor(const D_integer& value);
extern D_integer std_numeric_ifloor(const D_real& value);
extern D_integer std_numeric_iceil(const D_integer& value);
extern D_integer std_numeric_iceil(const D_real& value);
extern D_integer std_numeric_itrunc(const D_integer& value);
extern D_integer std_numeric_itrunc(const D_real& value);

extern D_integer std_numeric_random(const Global_Context& global, const D_integer& upper);
extern D_real std_numeric_random(const Global_Context& global, const Opt<D_real>& upper);
extern D_integer std_numeric_random(const Global_Context& global, const D_integer& lower, const D_integer& upper);
extern D_real std_numeric_random(const Global_Context& global, const D_real& lower, const D_real& upper);

// Create an object that is to be referenced as `std.gc`.
extern void create_bindings_numeric(D_object& result, API_Version version);

}  // namespace Asteria

#endif
