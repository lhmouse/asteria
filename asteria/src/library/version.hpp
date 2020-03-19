// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_VERSION_HPP_
#define ASTERIA_LIBRARY_VERSION_HPP_

#include "../fwd.hpp"

namespace Asteria {

// Create an object that is to be referenced as `std.version`.
void create_bindings_version(V_object& result, API_Version version);

}  // namespace Asteria

#endif
