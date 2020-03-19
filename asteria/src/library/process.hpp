// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_PROCESS_HPP_
#define ASTERIA_LIBRARY_PROCESS_HPP_

#include "../fwd.hpp"

namespace Asteria {

Ival std_process_execute(Sval path, Aopt argv = { }, Aopt envp = { });
void std_process_daemonize();

// Create an object that is to be referenced as `std.process`.
void create_bindings_process(V_object& result, API_Version version);

}  // namespace Asteria

#endif
