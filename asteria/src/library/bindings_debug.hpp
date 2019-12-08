// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_DEBUG_HPP_
#define ASTERIA_LIBRARY_BINDINGS_DEBUG_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern bool std_debug_print(const cow_vector<Value>& values);
extern bool std_debug_dump(const Value& value, aopt<G_integer> indent = rocket::clear);

// Create an object that is to be referenced as `std.debug`.
extern void create_bindings_debug(G_object& result, API_Version version);

}  // namespace Asteria

#endif
