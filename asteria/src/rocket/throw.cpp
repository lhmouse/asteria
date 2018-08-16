// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "throw.hpp"
#include <stdexcept>
#include <cstdarg>
#include <cstdio>

namespace rocket
{

#define VSNPRINTF(buf_, fmt_)    \
    do {    \
      ::std::va_list ap_;    \
      va_start(ap_, fmt_);    \
      ::std::vsnprintf((buf_), sizeof(buf_), (fmt_), ap_);    \
      va_end(ap_);    \
    } while(0)

void throw_invalid_argument(const char *fmt, ...)
  {
    char str[1024];
    VSNPRINTF(str, fmt);
    throw ::std::invalid_argument(str);
  }
void throw_out_of_range(const char *fmt, ...)
  {
    char str[1024];
    VSNPRINTF(str, fmt);
    throw ::std::out_of_range(str);
  }
void throw_length_error(const char *fmt, ...)
  {
    char str[1024];
    VSNPRINTF(str, fmt);
    throw ::std::length_error(str);
  }
void throw_domain_error(const char *fmt, ...)
  {
    char str[1024];
    VSNPRINTF(str, fmt);
    throw ::std::domain_error(str);
  }

}
