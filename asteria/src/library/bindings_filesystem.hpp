// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {


// Create an object that is to be referenced as `std.filesystem`.
extern void create_bindings_filesystem(G_object& result, API_Version version);

}  // namespace Asteria

#endif
