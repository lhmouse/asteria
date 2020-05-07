// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_CHECKSUM_HPP_
#define ASTERIA_LIBRARY_CHECKSUM_HPP_

#include "../fwd.hpp"

namespace asteria {

// members of `std.checksum.crc32_new()`
V_opaque
std_checksum_CRC32_private();

void
std_checksum_CRC32_update(V_opaque& h, V_string data);

V_integer
std_checksum_CRC32_finish(V_opaque& h);

// `std.checksum.crc32_new`
V_object
std_checksum_crc32_new();

// `std.checksum.crc32`
V_integer
std_checksum_crc32(V_string data);

// `std.checksum.crc32_file`
V_integer
std_checksum_crc32_file(V_string path);

// members of `std.checksum.fnv1a32_new()`
V_opaque
std_checksum_FNV1a32_private();

void
std_checksum_FNV1a32_update(V_opaque& h, V_string data);

V_integer
std_checksum_FNV1a32_finish(V_opaque& h);

// `std.checksum.fnv1a32_new`
V_object
std_checksum_fnv1a32_new();

// `std.checksum.fnv1a32`
V_integer
std_checksum_fnv1a32(V_string data);

// `std.checksum.fnv1a32_file`
V_integer
std_checksum_fnv1a32_file(V_string path);

// members of `std.checksum.md5_new()`
V_opaque
std_checksum_MD5_private();

void
std_checksum_MD5_update(V_opaque& h, V_string data);

V_string
std_checksum_MD5_finish(V_opaque& h);

// `std.checksum.md5_new`
V_object
std_checksum_md5_new();

// `std.checksum.md5`
V_string
std_checksum_md5(V_string data);

// `std.checksum.md5_file`
V_string
std_checksum_md5_file(V_string path);

// members of `std.checksum.sha1_new()`
V_opaque
std_checksum_SHA1_private();

void
std_checksum_SHA1_update(V_opaque& h, V_string data);

V_string
std_checksum_SHA1_finish(V_opaque& h);

// `std.checksum.sha1_new`
V_object
std_checksum_sha1_new();

// `std.checksum.sha1`
V_string
std_checksum_sha1(V_string data);

// `std.checksum.sha1_file`
V_string
std_checksum_sha1_file(V_string path);

// members of `std.checksum.sha256_new()`
V_opaque
std_checksum_SHA256_private();

void
std_checksum_SHA256_update(V_opaque& h, V_string data);

V_string
std_checksum_SHA256_finish(V_opaque& h);

// `std.checksum.sha256_new`
V_object
std_checksum_sha256_new();

// `std.checksum.sha256`
V_string
std_checksum_sha256(V_string data);

// `std.checksum.sha256_file`
V_string
std_checksum_sha256_file(V_string path);

// Create an object that is to be referenced as `std.checksum`.
void
create_bindings_checksum(V_object& result, API_Version version);

}  // namespace asteria

#endif
