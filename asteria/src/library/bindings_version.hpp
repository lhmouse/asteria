// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_VERSION_HPP_
#define ASTERIA_LIBRARY_BINDINGS_VERSION_HPP_

#include "../fwd.hpp"

namespace Asteria {

// Create an object that is to be referenced as `std.version`.
extern void create_bindings_version(G_object& result, API_Version version);

}  // namespace Asteria

#endif
