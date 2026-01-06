// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "xthrow.hpp"
#include <stdarg.h>  // ::va_list
#include <stdio.h>  // ::vsnprintf()
namespace rocket {

template<typename exceptT>
void
sprintf_and_throw(const char* fmt, ...)
  {
    // Compose the error message in temporary storage.
    ::va_list ap;
    va_start(ap, fmt);
    char strbuf[2048];
    int ret = ::vsnprintf(strbuf, sizeof strbuf, fmt, ap);
    va_end(ap);

    // Remove trailing line breaks.
    auto eptr = strbuf + clamp_cast<uint32_t>(ret, 0, (int) sizeof strbuf - 1);
    while((eptr != strbuf) && (*eptr == '\n'))
      *(eptr --) = 0;

    // Throw an exception with a copy of the formatted message...
    // Can we make use of the reference-counting string in standard
    // exceptions directly?
    throw exceptT(strbuf);
  }

template void sprintf_and_throw<logic_error>(const char*, ...);
template void sprintf_and_throw<domain_error>(const char*, ...);
template void sprintf_and_throw<invalid_argument>(const char*, ...);
template void sprintf_and_throw<length_error>(const char*, ...);
template void sprintf_and_throw<out_of_range>(const char*, ...);
template void sprintf_and_throw<runtime_error>(const char*, ...);
template void sprintf_and_throw<range_error>(const char*, ...);
template void sprintf_and_throw<overflow_error>(const char*, ...);
template void sprintf_and_throw<underflow_error>(const char*, ...);

}  // namespace rocket
