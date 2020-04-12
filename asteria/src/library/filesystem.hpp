// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.filesystem.get_working_directory`
Sval
std_filesystem_get_working_directory();

// `std.filesystem.get_real_path`
Sval
std_filesystem_get_real_path(Sval path);

// `std.filesystem.get_information`
Oopt
std_filesystem_get_information(Sval path);

// `std.filesystem.move_from`
void
std_filesystem_move_from(Sval path_new, Sval path_old);

// `std.filesystem.remove_recursive`
Ival
std_filesystem_remove_recursive(Sval path);

// `std.filesystem.directory_list`
Oopt
std_filesystem_directory_list(Sval path);

// `std.filesystem.directory_create`
Bval
std_filesystem_directory_create(Sval path);

// `std.filesystem.directoryr_remove`
Bval
std_filesystem_directory_remove(Sval path);

// `std.filesystem.file_read`
Sopt
std_filesystem_file_read(Sval path, Iopt offset, Iopt limit);

// `std.filesystem.file_stream`
Iopt
std_filesystem_file_stream(Global& global, Sval path, Fval callback, Iopt offset, Iopt limit);

// `std.filesystem.file_write`
void
std_filesystem_file_write(Sval path, Sval data, Iopt offset);

// `std.filesystem.file_append`
void
std_filesystem_file_append(Sval path, Sval data, Bopt exclusive);

// `std.filesystem.file_copy_from`
void
std_filesystem_file_copy_from(Sval path_new, Sval path_old);

// `std.filesystem.file_remove`
Bval
std_filesystem_file_remove(Sval path);

// Create an object that is to be referenced as `std.filesystem`.
void
create_bindings_filesystem(V_object& result, API_Version version);

}  // namespace Asteria

#endif
