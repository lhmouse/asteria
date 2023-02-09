// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_VERSION_
#define ASTERIA_LIBRARY_VERSION_

#include "../fwd.hpp"
namespace asteria {

// Create an object that is to be referenced as `std.version`.
void
create_bindings_version(V_object& result, API_Version version);

}  // namespace asteria
#endif
