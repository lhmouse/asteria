// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_

#include "../fwd.hpp"

namespace Asteria {

Ival chrono_utc_now();
Ival chrono_local_now();
Rval chrono_hires_now();
Ival chrono_steady_now();

Ival chrono_local_from_utc(Ival time_utc);
Ival chrono_utc_from_local(Ival time_local);
Sval chrono_utc_format(Ival time_point, Bopt with_ms = { });
Ival chrono_utc_parse(Sval time_str);

// Initialize an object that is to be referenced as `std.chrono`.
void create_bindings_chrono(Oval& result, API_Version version);

}  // namespace Asteria

#endif
