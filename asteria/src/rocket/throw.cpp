// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "throw.hpp"
#include <stdexcept>
#include <cstdarg>
#include <cstdio>

namespace rocket {

#define ROCKET_DEFINE_VSNPRINTF_AND_THROW(Except_, fmt_)  \
    do {  \
      char msgbuf_[1024];  \
      {  \
        ::std::va_list ap_;  \
        va_start(ap_, fmt_);  \
        ::std::vsnprintf(msgbuf_, sizeof(msgbuf_), (fmt_), ap_);  \
        va_end(ap_);  \
      }  \
      throw Except_(msgbuf_);  \
    } while(false)

[[noreturn]] void throw_invalid_argument(const char *fmt, ...)
  {
    ROCKET_DEFINE_VSNPRINTF_AND_THROW(::std::invalid_argument, fmt);
  }

[[noreturn]] void throw_out_of_range(const char *fmt, ...)
  {
    ROCKET_DEFINE_VSNPRINTF_AND_THROW(::std::out_of_range, fmt);
  }

[[noreturn]] void throw_length_error(const char *fmt, ...)
  {
    ROCKET_DEFINE_VSNPRINTF_AND_THROW(::std::length_error, fmt);
  }

[[noreturn]] void throw_domain_error(const char *fmt, ...)
  {
    ROCKET_DEFINE_VSNPRINTF_AND_THROW(::std::domain_error, fmt);
  }

}
