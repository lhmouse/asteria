// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "throw.hpp"
#include <stdlib.h>  // ::free()
#include <stdarg.h>  // ::va_list
#include <stdio.h>  // ::vasprintf()

namespace rocket {
namespace {

class vasprintf_guard
  {
  private:
    char* m_str;

  public:
    vasprintf_guard(const char* fmt, ::va_list ap)
      {
        if(::vasprintf(&(this->m_str), fmt, ap) < 0)
          throw ::std::bad_alloc();
      }
    ~vasprintf_guard()
      {
        ::free(this->m_str);
      }
    vasprintf_guard(const vasprintf_guard&)
      = delete;
    vasprintf_guard& operator=(const vasprintf_guard&)
      = delete;

  public:
    const char* c_str() const noexcept
      {
        return this->m_str;
      }
  };

}  // namespace

// Define the main template.
template<typename exceptT> void sprintf_and_throw(const char* fmt, ...)
  {
    // Compose the error message in allocated storage.
    ::va_list ap;
    va_start(ap, fmt);
    vasprintf_guard msg(fmt, ap);
    va_end(ap);
    // Construct the exception object and throw it.
    throw exceptT(msg.c_str());
  }

// Define specializations.
template void sprintf_and_throw<logic_error>(const char* fmt, ...);
template void sprintf_and_throw<domain_error>(const char* fmt, ...);
template void sprintf_and_throw<invalid_argument>(const char* fmt, ...);
template void sprintf_and_throw<length_error>(const char* fmt, ...);
template void sprintf_and_throw<out_of_range>(const char* fmt, ...);

template void sprintf_and_throw<runtime_error>(const char* fmt, ...);
template void sprintf_and_throw<range_error>(const char* fmt, ...);
template void sprintf_and_throw<overflow_error>(const char* fmt, ...);
template void sprintf_and_throw<underflow_error>(const char* fmt, ...);

}  // namespace rocket
