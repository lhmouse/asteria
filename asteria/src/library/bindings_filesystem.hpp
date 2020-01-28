// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

Sval std_filesystem_get_working_directory();

Oopt std_filesystem_get_information(Sval path);
void std_filesystem_move_from(Sval path_new, Sval path_old);
Ival std_filesystem_remove_recursive(Sval path);

Oopt std_filesystem_directory_list(Sval path);
Bval std_filesystem_directory_create(Sval path);
Bval std_filesystem_directory_remove(Sval path);

Sopt std_filesystem_file_read(Sval path, Iopt offset = nullopt, Iopt limit = nullopt);
Iopt std_filesystem_file_stream(Global& global, Sval path, Fval callback, Iopt offset = nullopt, Iopt limit = nullopt);
bool std_filesystem_file_write(Sval path, Sval data, Iopt offset = nullopt);
bool std_filesystem_file_append(Sval path, Sval data, Bopt exclusive = nullopt);
void std_filesystem_file_copy_from(Sval path_new, Sval path_old);
Bval std_filesystem_file_remove(Sval path);

// Create an object that is to be referenced as `std.filesystem`.
void create_bindings_filesystem(Oval& result, API_Version version);

}  // namespace Asteria

#endif
