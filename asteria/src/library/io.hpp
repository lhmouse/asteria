// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_IO_HPP_
#define ASTERIA_LIBRARY_IO_HPP_

#include "../fwd.hpp"

namespace asteria {

// `std.io.getc`
Opt_integer
std_io_getc();

// `std.io.getln`
Opt_string
std_io_getln();

// `std.io.putc`
Opt_integer
std_io_putc(V_integer value);

Opt_integer
std_io_putc(V_string value);

// `std.io.putln`
Opt_integer
std_io_putln(V_string value);

// `std.io.putf`
Opt_integer
std_io_putf(V_string templ, cow_vector<Value> values);

// `std.io.read`
Opt_string
std_io_read(Opt_integer limit);

// `std.io.write`
Opt_integer
std_io_write(V_string data);

// `std.io.flush`
void
std_io_flush();

// Create an object that is to be referenced as `std.io`.
void
create_bindings_io(V_object& result, API_Version version);

}  // namespace asteria

#endif
