// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CONSTANTS_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CONSTANTS_HPP_

#include "../fwd.hpp"

namespace Asteria {

// Create an object that is to be referenced as `std.constants`.
extern void create_bindings_constants(D_object& result, API_Version version);

}  // namespace Asteria

#endif
