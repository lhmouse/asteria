// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_integer std_chrono_utc_now();
extern G_integer std_chrono_local_now();
extern G_real std_chrono_hires_now();
extern G_integer std_chrono_steady_now();

extern G_integer std_chrono_local_from_utc(aref<G_integer> time_utc);
extern G_integer std_chrono_utc_from_local(aref<G_integer> time_local);
extern G_string std_chrono_utc_format(aref<G_integer> time_point, aopt<G_boolean> with_ms = rocket::clear);
extern opt<G_integer> std_chrono_utc_parse(aref<G_string> time_str);

// Initialize an object that is to be referenced as `std.chrono`.
extern void create_bindings_chrono(G_object& result, API_Version version);

}  // namespace Asteria

#endif
