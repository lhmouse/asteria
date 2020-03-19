// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_GC_HPP_
#define ASTERIA_LIBRARY_GC_HPP_

#include "../fwd.hpp"

namespace Asteria {

Iopt std_gc_tracked_count(Global& global, Ival generation);
Iopt std_gc_get_threshold(Global& global, Ival generation);
Iopt std_gc_set_threshold(Global& global, Ival generation, Ival threshold);
Ival std_gc_collect(Global& global, Iopt generation_limit);

// Create an object that is to be referenced as `std.gc`.
void create_bindings_gc(V_object& result, API_Version version);

}  // namespace Asteria

#endif
