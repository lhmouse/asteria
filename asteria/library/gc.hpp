// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_GC_
#define ASTERIA_LIBRARY_GC_

#include "../fwd.hpp"
namespace asteria {

// `std.gc.count_variables`
V_integer
std_gc_count_variables(Global_Context& global, V_integer generation);

// `std.gc.get_threshold`
V_integer
std_gc_get_threshold(Global_Context& global, V_integer generation);

// `std.gc.set_threshold`
V_integer
std_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold);

// `std.gc.collect`
V_integer
std_gc_collect(Global_Context& global, optV_integer generation_limit);

// Create an object that is to be referenced as `std.gc`.
void
create_bindings_gc(V_object& result, API_Version version);

}  // namespace asteria
#endif
