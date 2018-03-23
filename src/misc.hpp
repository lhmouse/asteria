
// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_MISC_HPP_
#define ASTERIA_MISC_HPP_

#include <cstdarg>

namespace Asteria {

__attribute__((__noreturn__)) extern void throw_printf(const char *fmt, ...);

}

#define THROW_PRINTF(...)    (::Asteria::throw_printf(__VA_ARGS__))

#endif
