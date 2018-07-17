// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_THROW_HPP_
#define ROCKET_THROW_HPP_

#include "compatibility.h"

namespace rocket {

ROCKET_FORMAT_CHECK(__printf__, 1, 2) ROCKET_NORETURN extern void throw_invalid_argument(const char *fmt, ...);
ROCKET_FORMAT_CHECK(__printf__, 1, 2) ROCKET_NORETURN extern void throw_out_of_range(const char *fmt, ...);
ROCKET_FORMAT_CHECK(__printf__, 1, 2) ROCKET_NORETURN extern void throw_length_error(const char *fmt, ...);
ROCKET_FORMAT_CHECK(__printf__, 1, 2) ROCKET_NORETURN extern void throw_domain_error(const char *fmt, ...);

}

#endif
