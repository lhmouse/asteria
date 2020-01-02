// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHECKSUM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHECKSUM_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_object std_checksum_crc32_new();
extern G_integer std_checksum_crc32(G_string data);

extern G_object std_checksum_fnv1a32_new();
extern G_integer std_checksum_fnv1a32(G_string data);

extern G_object std_checksum_md5_new();
extern G_string std_checksum_md5(G_string data);

extern G_object std_checksum_sha1_new();
extern G_string std_checksum_sha1(G_string data);

extern G_object std_checksum_sha256_new();
extern G_string std_checksum_sha256(G_string data);

// Create an object that is to be referenced as `std.checksum`.
extern void create_bindings_checksum(G_object& result, API_Version version);

}  // namespace Asteria

#endif
