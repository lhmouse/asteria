// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_DEBUG_
#define ASTERIA_LIBRARY_DEBUG_

#include "../fwd.hpp"
namespace asteria {

// `std.debug.logf`
optV_integer
std_debug_logf(V_string templ, cow_vector<Value> values);

// `std.debug.dump`
optV_integer
std_debug_dump(Value value, optV_integer indent);

// Create an object that is to be referenced as `std.debug`.
void
create_bindings_debug(V_object& result, API_Version version);

}  // namespace asteria
#endif
