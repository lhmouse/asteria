// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_GC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_GC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern opt<G_integer> std_gc_tracked_count(const Global_Context& global, aref<G_integer> generation);
extern opt<G_integer> std_gc_get_threshold(const Global_Context& global, aref<G_integer> generation);
extern opt<G_integer> std_gc_set_threshold(const Global_Context& global, aref<G_integer> generation, aref<G_integer> threshold);
extern G_integer std_gc_collect(const Global_Context& global, aopt<G_integer> generation_limit);

// Create an object that is to be referenced as `std.gc`.
extern void create_bindings_gc(G_object& result, API_Version version);

}  // namespace Asteria

#endif
