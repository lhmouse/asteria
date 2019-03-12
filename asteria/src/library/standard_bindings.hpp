// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_STANDARD_BINDINGS_HPP_
#define ASTERIA_LIBRARY_STANDARD_BINDINGS_HPP_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"
#include "../runtime/value.hpp"

namespace Asteria {

// `std.debug`
extern D_boolean std_debug_print(const Cow_Vector<Value> &values);
extern D_boolean std_debug_var_dump(const Value &value, std::size_t indent_increment);

// Create an object that is to be referenced as `std`.
extern D_object create_standard_bindings(const Rcptr<Generational_Collector> &coll);

}  // namespace Asteria

#endif
