// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "assert.hpp"
#include <exception>
#include <cstdio>
namespace rocket {

void
assert_fail(const char* expr, const char* file, long line, const char* msg) noexcept
  {
    ::fprintf(stderr,
      "=========================\n"
      "*** ASSERTION FAILURE ***\n"
      "=========================\n"
      "Expression: %s\n"
      "Location:   %s:%ld\n"
      "Message:    %s\n"
      "=========================\n",
      expr, file, line, msg);

    ::fflush(nullptr);
    ::std::terminate();
  }

}  // namespace rocket
