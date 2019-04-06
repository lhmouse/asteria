// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_
#define ASTERIA_LIBRARY_BINDINGS_ARRAY_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Value std_array_max_of(const D_array& data);
extern Value std_array_min_of(const D_array& data);

extern D_boolean std_array_is_sorted(const Global_Context& global, const D_array& data, const Opt<D_function>& comparator);

// Create an object that is to be referenced as `std.array`.
extern void create_bindings_array(D_object& result, API_Version version);

}  // namespace Asteria

#endif
