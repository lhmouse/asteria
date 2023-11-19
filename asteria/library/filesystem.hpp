// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_FILESYSTEM_
#define ASTERIA_LIBRARY_FILESYSTEM_

#include "../fwd.hpp"
namespace asteria {

// `std.filesystem.get_working_directory`
V_string
std_filesystem_get_working_directory();

// `std.filesystem.get_real_path`
V_string
std_filesystem_get_real_path(V_string path);

// `std.filesystem.get_properties`
optV_object
std_filesystem_get_properties(V_string path);

// `std.filesystem.move`
void
std_filesystem_move(V_string path_new, V_string path_old);

// `std.filesystem.remove_recursive`
V_integer
std_filesystem_remove_recursive(V_string path);

// `std.filesystem.glob`
V_array
std_filesystem_glob(V_string pattern);

// `std.filesystem.directory_list`
V_object
std_filesystem_list(V_string path);

// `std.filesystem.directory_create`
V_integer
std_filesystem_create_directory(V_string path);

// `std.filesystem.directory_remove`
V_integer
std_filesystem_remove_directory(V_string path);

// `std.filesystem.read`
V_string
std_filesystem_read(V_string path, optV_integer offset, optV_integer limit);

// `std.filesystem.stream`
V_integer
std_filesystem_stream(Global_Context& global, V_string path, V_function callback, optV_integer offset, optV_integer limit);

// `std.filesystem.write`
void
std_filesystem_write(V_string path, optV_integer offset, V_string data);

// `std.filesystem.append`
void
std_filesystem_append(V_string path, V_string data, optV_boolean exclusive);

// `std.filesystem.copy_file`
void
std_filesystem_copy_file(V_string path_new, V_string path_old);

// `std.filesystem.remove_file`
V_integer
std_filesystem_remove_file(V_string path);

// Create an object that is to be referenced as `std.filesystem`.
void
create_bindings_filesystem(V_object& result, API_Version version);

}  // namespace asteria
#endif
