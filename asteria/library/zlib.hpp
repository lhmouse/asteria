// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_ZLIB_
#define ASTERIA_LIBRARY_ZLIB_

#include "../fwd.hpp"
namespace asteria {

// `std.zlib.Deflator`
V_object
std_zlib_Deflator(V_string format, optV_integer level);

V_opaque
std_zlib_Deflator_private(V_string format, optV_integer level);

void
std_zlib_Deflator_update(V_opaque& r, V_string& output, V_string data);

void
std_zlib_Deflator_flush(V_opaque& r, V_string& output);

V_string
std_zlib_Deflator_finish(V_opaque& r, V_string& output);

void
std_zlib_Deflator_clear(V_opaque& r);

// `std.zlib.deflate`
V_string
std_zlib_deflate(V_string data, optV_integer level);

// `std.zlib.gzip`
V_string
std_zlib_gzip(V_string data, optV_integer level);

// `std.zlib.Inflator`
V_object
std_zlib_Inflator(V_string format);

V_opaque
std_zlib_Inflator_private(V_string format);

void
std_zlib_Inflator_update(V_opaque& r, V_string& output, V_string data);

void
std_zlib_Inflator_flush(V_opaque& r, V_string& output);

V_string
std_zlib_Inflator_finish(V_opaque& r, V_string& output);

void
std_zlib_Inflator_clear(V_opaque& r);

// `std.zlib.inflate`
V_string
std_zlib_inflate(V_string data);

// `std.zlib.gunzip`
V_string
std_zlib_gunzip(V_string data);

// Create an object that is to be referenced as `std.zlib`.
void
create_bindings_zlib(V_object& result, API_Version version);

}  // namespace asteria
#endif
