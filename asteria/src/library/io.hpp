// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_IO_HPP_
#define ASTERIA_LIBRARY_IO_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.io.getc`
Iopt
std_io_getc();

// `std.io.getln`
Sopt
std_io_getln();

// `std.io.putc`
Iopt
std_io_putc(Ival value);

Iopt
std_io_putc(Sval value);

// `std.io.putln`
Iopt
std_io_putln(Sval value);

// `std.io.putf`
Iopt
std_io_putf(Sval templ, cow_vector<Value> values);

// `std.io.read`
Sopt
std_io_read(Iopt limit);

// `std.io.write`
Iopt
std_io_write(Sval data);

// `std.io.flush`
void
std_io_flush();

// Create an object that is to be referenced as `std.io`.
void
create_bindings_io(V_object& result, API_Version version);

}  // namespace Asteria

#endif
