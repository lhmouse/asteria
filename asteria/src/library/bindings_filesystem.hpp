// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_string std_filesystem_get_working_directory();

extern opt<G_object> std_filesystem_get_information(aref<G_string> path);
extern bool std_filesystem_move_from(aref<G_string> path_new, aref<G_string> path_old);
extern opt<G_integer> std_filesystem_remove_recursive(aref<G_string> path);

extern opt<G_object> std_filesystem_directory_list(aref<G_string> path);
extern opt<G_integer> std_filesystem_directory_create(aref<G_string> path);
extern opt<G_integer> std_filesystem_directory_remove(aref<G_string> path);

extern opt<G_string> std_filesystem_file_read(aref<G_string> path, aopt<G_integer> offset = rocket::clear, aopt<G_integer> limit = rocket::clear);
extern bool std_filesystem_file_stream(const Global_Context& global, aref<G_string> path, aref<G_function> callback, aopt<G_integer> offset = rocket::clear, aopt<G_integer> limit = rocket::clear);
extern bool std_filesystem_file_write(aref<G_string> path, aref<G_string> data, aopt<G_integer> offset = rocket::clear);
extern bool std_filesystem_file_append(aref<G_string> path, aref<G_string> data, aopt<G_boolean> exclusive = rocket::clear);
extern bool std_filesystem_file_copy_from(aref<G_string> path_new, aref<G_string> path_old);
extern bool std_filesystem_file_remove(aref<G_string> path);

// Create an object that is to be referenced as `std.filesystem`.
extern void create_bindings_filesystem(G_object& result, API_Version version);

}  // namespace Asteria

#endif
