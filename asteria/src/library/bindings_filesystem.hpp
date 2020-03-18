// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

Sval filesystem_get_working_directory();

Oopt filesystem_get_information(Sval path);
void filesystem_move_from(Sval path_new, Sval path_old);
Ival filesystem_remove_recursive(Sval path);

Oopt filesystem_directory_list(Sval path);
Bval filesystem_directory_create(Sval path);
Bval filesystem_directory_remove(Sval path);

Sopt filesystem_file_read(Sval path, Iopt offset = { }, Iopt limit = { });
Iopt filesystem_file_stream(Global& global, Sval path, Fval callback, Iopt offset = { }, Iopt limit = { });
void filesystem_file_write(Sval path, Sval data, Iopt offset = { });
void filesystem_file_append(Sval path, Sval data, Bopt exclusive = { });
void filesystem_file_copy_from(Sval path_new, Sval path_old);
Bval filesystem_file_remove(Sval path);

// Create an object that is to be referenced as `std.filesystem`.
void create_bindings_filesystem(Oval& result, API_Version version);

}  // namespace Asteria

#endif
