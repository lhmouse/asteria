// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_CHRONO_
#define ASTERIA_LIBRARY_CHRONO_

#include "../fwd.hpp"
namespace asteria {

// `std.chrono.now`
V_integer
std_chrono_now();

// `std.chrono.format`
V_string
std_chrono_format(V_integer time_point, optV_boolean with_ms, optV_integer utc_offset);

// `std.chrono.parse`
V_integer
std_chrono_parse(V_string time_str);

// `std.chrono.hires_now`
V_real
std_chrono_hires_now();

// `std.chrono.steady_now`
V_integer
std_chrono_steady_now();

// Initialize an object that is to be referenced as `std.chrono`.
void
create_bindings_chrono(V_object& result, API_Version version);

}  // namespace asteria
#endif
