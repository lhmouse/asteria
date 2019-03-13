// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "standard_bindings.hpp"
#include "standard_bindings_debug.hpp"
#include "standard_bindings_chrono.hpp"
#include "../runtime/value.hpp"
#include "../utilities.hpp"

namespace Asteria {

D_object create_standard_bindings(const Rcptr<Generational_Collector> &coll)
  {
    D_object root;
    root.insert_or_assign(rocket::sref("debug"), create_standard_bindings_debug());
    root.insert_or_assign(rocket::sref("chrono"), create_standard_bindings_chrono());
    return root;
  }

}  // namespace Asteria
