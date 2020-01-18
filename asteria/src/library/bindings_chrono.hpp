// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Ival std_chrono_utc_now();
extern Ival std_chrono_local_now();
extern Rval std_chrono_hires_now();
extern Ival std_chrono_steady_now();

extern Ival std_chrono_local_from_utc(Ival time_utc);
extern Ival std_chrono_utc_from_local(Ival time_local);
extern Sval std_chrono_utc_format(Ival time_point, Bopt with_ms = emptyc);
extern Iopt std_chrono_utc_parse(Sval time_str);

// Initialize an object that is to be referenced as `std.chrono`.
extern void create_bindings_chrono(Oval& result, API_Version version);

}  // namespace Asteria

#endif
