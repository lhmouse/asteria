// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Sval std_filesystem_get_working_directory();

extern Oopt std_filesystem_get_information(Sval path);
extern bool std_filesystem_move_from(Sval path_new, Sval path_old);
extern Iopt std_filesystem_remove_recursive(Sval path);

extern Oopt std_filesystem_directory_list(Sval path);
extern Iopt std_filesystem_directory_create(Sval path);
extern Iopt std_filesystem_directory_remove(Sval path);

extern Sopt std_filesystem_file_read(Sval path, Iopt offset = ::rocket::clear, Iopt limit = ::rocket::clear);
extern bool std_filesystem_file_stream(Global& global, Sval path, Fval callback, Iopt offset = ::rocket::clear, Iopt limit = ::rocket::clear);
extern bool std_filesystem_file_write(Sval path, Sval data, Iopt offset = ::rocket::clear);
extern bool std_filesystem_file_append(Sval path, Sval data, Bopt exclusive = ::rocket::clear);
extern bool std_filesystem_file_copy_from(Sval path_new, Sval path_old);
extern bool std_filesystem_file_remove(Sval path);

// Create an object that is to be referenced as `std.filesystem`.
extern void create_bindings_filesystem(Oval& result, API_Version version);

}  // namespace Asteria

#endif
