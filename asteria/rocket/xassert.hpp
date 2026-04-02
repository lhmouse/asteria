// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ROCKET_XASSERT_
#define ASTERIA_ROCKET_XASSERT_

#include "fwd.hpp"
namespace asteria {

// This is always declared even when assertions are disabled.
[[noreturn]]
void
assert_fail(const char* expr, const char* file, long line, const char* msg_opt)
  noexcept;

#ifdef ASTERIA_DEBUG
#  define ASTERIA_ASSERT_FAIL(...)    ::asteria::assert_fail(__VA_ARGS__)
#else
#  define ASTERIA_ASSERT_FAIL(...)    ASTERIA_UNREACHABLE()
#endif

#define ASTERIA_ASSERT_MSG(expr, msg)  \
          ((expr) ? (void) 0 : ASTERIA_ASSERT_FAIL(#expr, __FILE__, __LINE__, (msg)))

#define ASTERIA_ASSERT(expr)  \
          ((expr) ? (void) 0 : ASTERIA_ASSERT_FAIL(#expr, __FILE__, __LINE__, ""))

}  // namespace asteria
#endif
