// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_IO_
#define ASTERIA_LIBRARY_IO_

#include "../fwd.hpp"
namespace asteria {

// `std.io.getc`
optV_integer
std_io_getc();

// `std.io.getln`
optV_string
std_io_getln();

// `std.io.putc`
optV_integer
std_io_putc(V_integer value);

optV_integer
std_io_putc(V_string value);

// `std.io.putln`
optV_integer
std_io_putln(V_string value);

// `std.io.putf`
optV_integer
std_io_putf(V_string templ, cow_vector<Value> values);

// `std.io.putfln`
optV_integer
std_io_putfln(V_string templ, cow_vector<Value> values);

// `std.io.read`
optV_string
std_io_read(optV_integer limit);

// `std.io.write`
optV_integer
std_io_write(V_string data);

// `std.io.flush`
void
std_io_flush();

// Create an object that is to be referenced as `std.io`.
void
create_bindings_io(V_object& result, API_Version version);

}  // namespace asteria
#endif
