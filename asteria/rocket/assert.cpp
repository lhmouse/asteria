// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "assert.hpp"
#include <exception>
#include <cstdio>

namespace rocket {

void report_assertion_failure(const char* expr, const char* file, long line, const char* msg) noexcept
  {
    // Write a message to the standard error stream.
    ::std::fprintf(stderr,
                   "========================\n"
                   "*** ASSERTION FAILED***\n"
                   "\tExpression: %s\n"
                   "\tFile:       %s\n"
                   "\tLine:       %ld\n"
                   "\tMessage:    %s\n"
                   "========================\n",
                   expr, file, line, msg);
    // Prefer `std::terminate()` to `std::abort()`.
    ::std::terminate();
  }

}  // namespace rocket
