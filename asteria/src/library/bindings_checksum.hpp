// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHECKSUM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHECKSUM_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_object std_checksum_crc32_new();
extern G_integer std_checksum_crc32(const G_string& data);

// Create an object that is to be referenced as `std.checksum`.
extern void create_bindings_checksum(G_object& result, API_Version version);

}  // namespace Asteria

#endif
