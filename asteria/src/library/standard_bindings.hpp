// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_STANDARD_BINDINGS_HPP_
#define ASTERIA_LIBRARY_STANDARD_BINDINGS_HPP_

#include "../fwd.hpp"
#include "standard_bindings_debug.hpp"
#include "standard_bindings_chrono.hpp"

namespace Asteria {

// Create an object that is to be referenced as `std`.
extern D_object create_standard_bindings(const Rcptr<Generational_Collector> &coll);

}  // namespace Asteria

#endif
