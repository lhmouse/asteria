// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_CHECKSUM_HPP_
#define ASTERIA_LIBRARY_CHECKSUM_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.checksum.crc32_new_private`
Pval
std_checksum_crc32_new_private();

// `std.checksum.crc32_new_update`
void
std_checksum_crc32_new_update(Pval& h, Sval data);

// `std.checksum.crc32_new_finish`
Ival
std_checksum_crc32_new_finish(Pval& h);

// `std.checksum.crc32_new`
Oval
std_checksum_crc32_new();

// `std.checksum.crc32`
Ival
std_checksum_crc32(Sval data);

// `std.checksum.fnv1a32_new_private`
Pval
std_checksum_fnv1a32_new_private();

// `std.checksum.fnv1a32_new_update`
void
std_checksum_fnv1a32_new_update(Pval& h, Sval data);

// `std.checksum.fnv1a32_new_finish`
Ival
std_checksum_fnv1a32_new_finish(Pval& h);

// `std.checksum.fnv1a32_new`
Oval
std_checksum_fnv1a32_new();

// `std.checksum.fnv1a32`
Ival
std_checksum_fnv1a32(Sval data);

// `std.checksum.md5_new_private`
Pval
std_checksum_md5_new_private();

// `std.checksum.md5_new_update`
void
std_checksum_md5_new_update(Pval& h, Sval data);

// `std.checksum.md5_new_finish`
Sval
std_checksum_md5_new_finish(Pval& h);

// `std.checksum.md5_new`
Oval
std_checksum_md5_new();

// `std.checksum.md5`
Sval
std_checksum_md5(Sval data);

// `std.checksum.sha1_new_private`
Pval
std_checksum_sha1_new_private();

// `std.checksum.sha1_new_update`
void
std_checksum_sha1_new_update(Pval& h, Sval data);

// `std.checksum.sha1_new_finish`
Sval
std_checksum_sha1_new_finish(Pval& h);

// `std.checksum.sha1_new`
Oval
std_checksum_sha1_new();

// `std.checksum.sha1`
Sval
std_checksum_sha1(Sval data);

// `std.checksum.sha256_new_private`
Pval
std_checksum_sha256_new_private();

// `std.checksum.sha256_new_update`
void
std_checksum_sha256_new_update(Pval& h, Sval data);

// `std.checksum.sha256_new_finish`
Sval
std_checksum_sha256_new_finish(Pval& h);

// `std.checksum.sha256_new`
Oval
std_checksum_sha256_new();

// `std.checksum.sha256`
Sval
std_checksum_sha256(Sval data);

// Create an object that is to be referenced as `std.checksum`.
void
create_bindings_checksum(V_object& result, API_Version version);

}  // namespace Asteria

#endif
