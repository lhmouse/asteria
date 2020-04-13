// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SYSTEM_HPP_
#define ASTERIA_LIBRARY_SYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.system.gc_count_variables`
optV_integer
std_system_gc_count_variables(Global_Context& global, V_integer generation);

// `std.system.gc_get_threshold`
optV_integer
std_system_gc_get_threshold(Global_Context& global, V_integer generation);

// `std.system.gc_set_threshold`
optV_integer
std_system_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold);

// `std.system.gc_collect`
V_integer
std_system_gc_collect(Global_Context& global, optV_integer generation_limit);

// `std.system.execute`
V_integer
std_system_execute(V_string path, optV_array argv, optV_array envp);

// `std.system.daemonize`
void
std_system_daemonize();

// `std.system.env_get_variable`
optV_string
std_system_env_get_variable(V_string name);

// `std.system.env_get_variables`
V_object
std_system_env_get_variables();

// Create an object that is to be referenced as `std.system`.
void
create_bindings_system(V_object& result, API_Version version);

}  // namespace Asteria

#endif
