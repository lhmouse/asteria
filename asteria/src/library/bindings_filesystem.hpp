// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_FILESYSTEM_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_string std_filesystem_get_working_directory();

extern Opt<G_object> std_filesystem_get_information(const G_string& path);
extern bool std_filesystem_move_from(const G_string& path_new, const G_string& path_old);
extern Opt<G_integer> std_filesystem_remove_recursive(const G_string& path);

extern Opt<G_object> std_filesystem_directory_list(const G_string& path);
extern Opt<G_integer> std_filesystem_directory_create(const G_string& path);
extern Opt<G_integer> std_filesystem_directory_remove(const G_string& path);

extern Opt<G_string> std_filesystem_file_read(const G_string& path, const Opt<G_integer>& offset = rocket::nullopt, const Opt<G_integer>& limit = rocket::nullopt);
extern bool std_filesystem_file_stream(const Global_Context& global, const G_string& path, const G_function& callback, const Opt<G_integer>& offset = rocket::nullopt, const Opt<G_integer>& limit = rocket::nullopt);
extern bool std_filesystem_file_write(const G_string& path, const G_string& data, const Opt<G_integer>& offset = rocket::nullopt);
extern bool std_filesystem_file_append(const G_string& path, const G_string& data, const Opt<G_boolean>& exclusive = rocket::nullopt);
extern bool std_filesystem_file_copy_from(const G_string& path_new, const G_string& path_old);
extern bool std_filesystem_file_remove(const G_string& path);

// Create an object that is to be referenced as `std.filesystem`.
extern void create_bindings_filesystem(G_object& result, API_Version version);

}  // namespace Asteria

#endif
