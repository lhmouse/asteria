// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_string std_filesystem_get_working_directory();

extern opt<G_object> std_filesystem_get_information(G_string path);
extern bool std_filesystem_move_from(G_string path_new, G_string path_old);
extern opt<G_integer> std_filesystem_remove_recursive(G_string path);

extern opt<G_object> std_filesystem_directory_list(G_string path);
extern opt<G_integer> std_filesystem_directory_create(G_string path);
extern opt<G_integer> std_filesystem_directory_remove(G_string path);

extern opt<G_string> std_filesystem_file_read(G_string path, opt<G_integer> offset = ::rocket::clear, opt<G_integer> limit = ::rocket::clear);
extern bool std_filesystem_file_stream(Global_Context& global, G_string path, G_function callback, opt<G_integer> offset = ::rocket::clear, opt<G_integer> limit = ::rocket::clear);
extern bool std_filesystem_file_write(G_string path, G_string data, opt<G_integer> offset = ::rocket::clear);
extern bool std_filesystem_file_append(G_string path, G_string data, opt<G_boolean> exclusive = ::rocket::clear);
extern bool std_filesystem_file_copy_from(G_string path_new, G_string path_old);
extern bool std_filesystem_file_remove(G_string path);

// Create an object that is to be referenced as `std.filesystem`.
extern void create_bindings_filesystem(G_object& result, API_Version version);

}  // namespace Asteria

#endif
