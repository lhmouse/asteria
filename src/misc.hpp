
// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_MISC_HPP_
#define ASTERIA_MISC_HPP_

#include <cstdio>
#include <cstdarg>

#ifdef ENABLE_DEBUG_LOGS
#  define DEBUG_PRINTF(...)   ((void)::std::fprintf(stderr, __VA_ARGS__))
#else
#  define DEBUG_PRINTF(...)   ((void)-1)
#endif

namespace Asteria {

__attribute__((__noreturn__)) extern void throw_printf(const char *fmt, ...);

}

#define THROW_PRINTF(...)    ((void)::Asteria::throw_printf(__VA_ARGS__))

#endif
