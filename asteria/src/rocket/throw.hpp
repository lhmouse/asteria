// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_THROW_HPP_
#define ROCKET_THROW_HPP_

#include "compatibility.hpp"

namespace rocket {

ROCKET_NORETURN extern void throw_invalid_argument(const char *fmt, ...);
ROCKET_NORETURN extern void throw_out_of_range(const char *fmt, ...);
ROCKET_NORETURN extern void throw_length_error(const char *fmt, ...);

ROCKET_NORETURN extern void rethrow_current_exception();

}

#endif
