// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SYSTEM_HPP_
#define ASTERIA_LIBRARY_SYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.system.gc_count_variables`
Iopt
std_system_gc_count_variables(Global& global, Ival generation);

// `std.system.gc_get_threshold`
Iopt
std_system_gc_get_threshold(Global& global, Ival generation);

// `std.system.gc_set_threshold`
Iopt
std_system_gc_set_threshold(Global& global, Ival generation, Ival threshold);

// `std.system.gc_collect`
Ival
std_system_gc_collect(Global& global, Iopt generation_limit);

// `std.system.execute`
Ival
std_system_execute(Sval path, Aopt argv, Aopt envp);

// `std.system.daemonize`
void
std_system_daemonize();

// `std.system.env_get_variable`
Sopt
std_system_env_get_variable(Sval name);

// `std.system.env_get_variables`
Oval
std_system_env_get_variables();

// Create an object that is to be referenced as `std.system`.
void
create_bindings_system(V_object& result, API_Version version);

}  // namespace Asteria

#endif
