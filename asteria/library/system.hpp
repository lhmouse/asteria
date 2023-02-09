// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SYSTEM_
#define ASTERIA_LIBRARY_SYSTEM_

#include "../fwd.hpp"
namespace asteria {

// `std.system.gc_count_variables`
V_integer
std_system_gc_count_variables(Global_Context& global, V_integer generation);

// `std.system.gc_get_threshold`
V_integer
std_system_gc_get_threshold(Global_Context& global, V_integer generation);

// `std.system.gc_set_threshold`
V_integer
std_system_gc_set_threshold(Global_Context& global, V_integer generation, V_integer threshold);

// `std.system.gc_collect`
V_integer
std_system_gc_collect(Global_Context& global, optV_integer generation_limit);

// `std.system.env_get_variable`
optV_string
std_system_env_get_variable(V_string name);

// `std.system.env_get_variables`
V_object
std_system_env_get_variables();

// `std.system.uuid`
V_string
std_system_uuid(Global_Context& global);

// `std.system.proc_get_pid()`
V_integer
std_system_proc_get_pid();

// `std.system.proc_get_ppid()`
V_integer
std_system_proc_get_ppid();

// `std.system.proc_get_uid()`
V_integer
std_system_proc_get_uid();

// `std.system.proc_get_euid()`
V_integer
std_system_proc_get_euid();

// `std.system.proc_invoke`
V_integer
std_system_proc_invoke(V_string path, optV_array argv, optV_array envp);

// `std.system.proc_daemonize`
void
std_system_proc_daemonize();

// `std.system.conf_load_file`
V_object
std_system_conf_load_file(V_string path);

// Create an object that is to be referenced as `std.system`.
void
create_bindings_system(V_object& result, API_Version version);

}  // namespace asteria
#endif
