// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern std::int64_t std_chrono_utc_now();
extern std::int64_t std_chrono_local_now();
extern double std_chrono_hires_now();
extern std::int64_t std_chrono_steady_now();

extern std::int64_t std_chrono_local_from_utc(std::int64_t time_utc);
extern std::int64_t std_chrono_utc_from_local(std::int64_t time_local);

extern bool std_chrono_parse_datetime(std::int64_t &time_point_out, const Cow_String &time_str);
extern void std_chrono_format_datetime(Cow_String &time_str_out, std::int64_t time_point, bool with_ms);
extern Cow_String std_chrono_min_datetime(bool with_ms);
extern Cow_String std_chrono_max_datetime(bool with_ms);

// Create an object that is to be referenced as `std.chrono`.
extern D_object create_bindings_chrono();

}  // namespace Asteria

#endif
