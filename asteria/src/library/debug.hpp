// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_DEBUG_HPP_
#define ASTERIA_LIBRARY_DEBUG_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.debug.logf`
Iopt
std_debug_logf(Sval templ, cow_vector<Value> values);

// `std.debug.dump`
Iopt
std_debug_dump(Value value, Iopt indent);

// Create an object that is to be referenced as `std.debug`.
void
create_bindings_debug(V_object& result, API_Version version);

}  // namespace Asteria

#endif
