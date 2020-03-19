// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_CHECKSUM_HPP_
#define ASTERIA_LIBRARY_CHECKSUM_HPP_

#include "../fwd.hpp"

namespace Asteria {

Pval std_checksum_crc32_new_private();
Ival std_checksum_crc32_new_write(Pval& h, Sval data);
Ival std_checksum_crc32_new_finish(Pval& h);
Oval std_checksum_crc32_new();
Ival std_checksum_crc32(Sval data);

Pval std_checksum_fnv1a32_new_private();
Ival std_checksum_fnv1a32_new_write(Pval& h, Sval data);
Ival std_checksum_fnv1a32_new_finish(Pval& h);
Oval std_checksum_fnv1a32_new();
Ival std_checksum_fnv1a32(Sval data);

Pval std_checksum_md5_new_private();
Ival std_checksum_md5_new_write(Pval& h, Sval data);
Sval std_checksum_md5_new_finish(Pval& h);
Oval std_checksum_md5_new();
Sval std_checksum_md5(Sval data);

Pval std_checksum_sha1_new_private();
Ival std_checksum_sha1_new_write(Pval& h, Sval data);
Sval std_checksum_sha1_new_finish(Pval& h);
Oval std_checksum_sha1_new();
Sval std_checksum_sha1(Sval data);

Pval std_checksum_sha256_new_private();
Ival std_checksum_sha256_new_write(Pval& h, Sval data);
Sval std_checksum_sha256_new_finish(Pval& h);
Oval std_checksum_sha256_new();
Sval std_checksum_sha256(Sval data);

// Create an object that is to be referenced as `std.checksum`.
void create_bindings_checksum(Oval& result, API_Version version);

}  // namespace Asteria

#endif
