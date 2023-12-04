// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_SYSTEM_
#define ASTERIA_LIBRARY_SYSTEM_

#include "../fwd.hpp"
namespace asteria {

// `std.system.get_working_directory`
V_string
std_system_get_working_directory();

// `std.system.get_environment_variable`
optV_string
std_system_get_environment_variable(V_string name);

// `std.system.get_environment_variables`
V_object
std_system_get_environment_variables();

// `std.system.get_properties`
V_object
std_system_get_properties();

// `std.system.generate_uuid`
V_string
std_system_generate_uuid(Global_Context& global);

// `std.system.get_pid()`
V_integer
std_system_get_pid();

// `std.system.get_ppid()`
V_integer
std_system_get_ppid();

// `std.system.get_uid()`
V_integer
std_system_get_uid();

// `std.system.get_euid()`
V_integer
std_system_get_euid();

// `std.system.call`
V_integer
std_system_call(V_string path, optV_array argv, optV_array envp);

// `std.system.daemonize`
void
std_system_daemonize();

// `std.system.sleep`
V_real
std_system_sleep(V_real duration);

// `std.system.load_conf`
V_object
std_system_load_conf(V_string path);

// Create an object that is to be referenced as `std.system`.
void
create_bindings_system(V_object& result, API_Version version);

}  // namespace asteria
#endif
