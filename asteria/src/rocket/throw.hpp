// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_THROW_HPP_
#define ROCKET_THROW_HPP_

#include "compatibility.h"

namespace rocket {

[[noreturn]] ROCKET_FORMAT_PRINTF(1, 2) extern void throw_invalid_argument(const char *fmt, ...);
[[noreturn]] ROCKET_FORMAT_PRINTF(1, 2) extern void throw_out_of_range(const char *fmt, ...);
[[noreturn]] ROCKET_FORMAT_PRINTF(1, 2) extern void throw_length_error(const char *fmt, ...);
[[noreturn]] ROCKET_FORMAT_PRINTF(1, 2) extern void throw_domain_error(const char *fmt, ...);

}

#endif
