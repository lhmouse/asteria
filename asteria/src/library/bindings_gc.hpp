// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_GC_HPP_
#define ASTERIA_LIBRARY_BINDINGS_GC_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Opt<D_integer> std_gc_get_threshold(const Global_Context& global, const D_integer& generation);
extern Opt<D_integer> std_gc_set_threshold(const Global_Context& global, const D_integer& generation, const D_integer& threshold);
extern D_integer std_gc_collect(const Global_Context& global, const Opt<D_integer>& generation_limit);

// Create an object that is to be referenced as `std.gc`.
extern void create_bindings_gc(D_object& result, API_Version version);

}  // namespace Asteria

#endif
