// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

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

// `std.system.random_uuid`
V_string
std_system_random_uuid();

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
std_system_call(V_string cmd, optV_array argv, optV_array envp);

// `std.system.pipe`
optV_string
std_system_pipe(V_string cmd, optV_array argv, optV_array envp, optV_string input);

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
