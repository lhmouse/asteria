// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "throw.hpp"
#include <cstdarg>

namespace rocket {

// Define the main template.
template<typename exceptT> [[noreturn]] ROCKET_ATTRIBUTE_PRINTF(1, 2) void sprintf_and_throw(const char *fmt, ...)
  {
    char msg[1024];
    ::std::va_list ap;
    va_start(ap, fmt);
    ::std::vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    throw exceptT(msg);
  }

// Define specializations.
template void sprintf_and_throw<logic_error>(const char *fmt, ...);
template void sprintf_and_throw<domain_error>(const char *fmt, ...);
template void sprintf_and_throw<invalid_argument>(const char *fmt, ...);
template void sprintf_and_throw<length_error>(const char *fmt, ...);
template void sprintf_and_throw<out_of_range>(const char *fmt, ...);

template void sprintf_and_throw<runtime_error>(const char *fmt, ...);
template void sprintf_and_throw<range_error>(const char *fmt, ...);
template void sprintf_and_throw<overflow_error>(const char *fmt, ...);
template void sprintf_and_throw<underflow_error>(const char *fmt, ...);

}  // namespace rocket
