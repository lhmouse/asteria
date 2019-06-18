// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_HASH_HPP_
#define ASTERIA_LIBRARY_BINDINGS_HASH_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_object std_hash_crc32_new();
extern G_integer std_hash_crc32(const G_string& data);

// Create an object that is to be referenced as `std.hash`.
extern void create_bindings_hash(G_object& result, API_Version version);

}  // namespace Asteria

#endif
