// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_IO_HPP_
#define ASTERIA_LIBRARY_IO_HPP_

#include "../fwd.hpp"

namespace Asteria {

Iopt std_io_getc();
Sopt std_io_getln();
Iopt std_io_putc(Ival value);
Iopt std_io_putc(Sval value);
Iopt std_io_putln(Sval value);

Sopt std_io_read(Iopt limit);
Iopt std_io_write(Sval data);

void std_io_flush();

// Create an object that is to be referenced as `std.io`.
void create_bindings_io(V_object& result, API_Version version);

}  // namespace Asteria

#endif
