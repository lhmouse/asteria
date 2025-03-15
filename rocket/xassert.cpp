// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "xassert.hpp"
#include <exception>
#include <cstdio>
namespace rocket {

void
assert_fail(const char* expr, const char* file, long line, const char* msg) noexcept
  {
    ::fprintf(stderr,
        "FATAL: Assertion `%s` at '%s:%ld' failed: %s\n"
        "The program cannot continue. Please file a bug report.\n",
        expr, file, line, msg);

    ::fflush(nullptr);
    ::std::terminate();
  }

}  // namespace rocket
