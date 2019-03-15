// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings.hpp"
#include "bindings_debug.hpp"
#include "bindings_chrono.hpp"
#include "../runtime/value.hpp"
#include "../utilities.hpp"

namespace Asteria {

D_object create_bindings(const Rcptr<Generational_Collector> &coll)
  {
    D_object root;
    root.insert_or_assign(rocket::sref("debug"), create_bindings_debug());
    root.insert_or_assign(rocket::sref("chrono"), create_bindings_chrono());
    return root;
  }

}  // namespace Asteria
