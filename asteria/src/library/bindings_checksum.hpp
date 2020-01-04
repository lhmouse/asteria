// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHECKSUM_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHECKSUM_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Pval std_checksum_crc32_new_private();
extern void std_checksum_crc32_new_write(Pval& h, Sval data);
extern Ival std_checksum_crc32_new_finish(Pval& h);
extern Oval std_checksum_crc32_new();
extern Ival std_checksum_crc32(Sval data);

extern Pval std_checksum_fnv1a32_new_private();
extern void std_checksum_fnv1a32_new_write(Pval& h, Sval data);
extern Ival std_checksum_fnv1a32_new_finish(Pval& h);
extern Oval std_checksum_fnv1a32_new();
extern Ival std_checksum_fnv1a32(Sval data);

extern Pval std_checksum_md5_new_private();
extern void std_checksum_md5_new_write(Pval& h, Sval data);
extern Sval std_checksum_md5_new_finish(Pval& h);
extern Oval std_checksum_md5_new();
extern Sval std_checksum_md5(Sval data);

extern Pval std_checksum_sha1_new_private();
extern void std_checksum_sha1_new_write(Pval& h, Sval data);
extern Sval std_checksum_sha1_new_finish(Pval& h);
extern Oval std_checksum_sha1_new();
extern Sval std_checksum_sha1(Sval data);

extern Pval std_checksum_sha256_new_private();
extern void std_checksum_sha256_new_write(Pval& h, Sval data);
extern Sval std_checksum_sha256_new_finish(Pval& h);
extern Oval std_checksum_sha256_new();
extern Sval std_checksum_sha256(Sval data);

// Create an object that is to be referenced as `std.checksum`.
extern void create_bindings_checksum(Oval& result, API_Version version);

}  // namespace Asteria

#endif
