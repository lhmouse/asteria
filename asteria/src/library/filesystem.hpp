// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace asteria {

// `std.filesystem.get_working_directory`
V_string
std_filesystem_get_working_directory();

// `std.filesystem.get_real_path`
V_string
std_filesystem_get_real_path(V_string path);

// `std.filesystem.get_information`
Opt_object
std_filesystem_get_information(V_string path);

// `std.filesystem.move_from`
void
std_filesystem_move_from(V_string path_new, V_string path_old);

// `std.filesystem.remove_recursive`
V_integer
std_filesystem_remove_recursive(V_string path);

// `std.filesystem.directory_list`
V_object
std_filesystem_dir_list(V_string path);

// `std.filesystem.directory_create`
V_integer
std_filesystem_dir_create(V_string path);

// `std.filesystem.directory_remove`
V_integer
std_filesystem_dir_remove(V_string path);

// `std.filesystem.file_read`
V_string
std_filesystem_file_read(V_string path, Opt_integer offset, Opt_integer limit);

// `std.filesystem.file_stream`
V_integer
std_filesystem_file_stream(Global_Context& global, V_string path, V_function callback,
                           Opt_integer offset, Opt_integer limit);

// `std.filesystem.file_write`
void
std_filesystem_file_write(V_string path, Opt_integer offset, V_string data);


// `std.filesystem.file_append`
void
std_filesystem_file_append(V_string path, V_string data, Opt_boolean exclusive);

// `std.filesystem.file_copy_from`
void
std_filesystem_file_copy_from(V_string path_new, V_string path_old);

// `std.filesystem.file_remove`
V_integer
std_filesystem_file_remove(V_string path);

// Create an object that is to be referenced as `std.filesystem`.
void
create_bindings_filesystem(V_object& result, API_Version version);

}  // namespace asteria

#endif
