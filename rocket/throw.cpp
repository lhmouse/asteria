// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "throw.hpp"
#include "unique_ptr.hpp"
#include <stdlib.h>  // ::free()
#include <stdarg.h>  // ::va_list
#include <stdio.h>  // ::vasprintf()
namespace rocket {

// Define the main template.
template<typename exceptT>
void
sprintf_and_throw(const char* fmt, ...)
  {
    // Compose the error message in allocated storage.
    ::va_list ap;
    va_start(ap, fmt);
    char* str;
    int ret = ::vasprintf(&str, fmt, ap);
    va_end(ap);

    if(ret < 0)
      throw ::std::bad_alloc();

    // Remove trailing new line characters.
    size_t ep = (uint32_t) ret;
    while((ep != 0) && (str[--ep] == '\n'))
      str[ep] = 0;

    // Construct the exception object and throw it.
    unique_ptr<char, void (void*)> uptr(str, ::free);
    throw exceptT(str);
  }

// Define specializations.
template
void
sprintf_and_throw<logic_error>(const char*, ...);

template
void
sprintf_and_throw<domain_error>(const char*, ...);

template
void
sprintf_and_throw<invalid_argument>(const char*, ...);

template
void
sprintf_and_throw<length_error>(const char*, ...);

template
void
sprintf_and_throw<out_of_range>(const char*, ...);

template
void
sprintf_and_throw<runtime_error>(const char*, ...);

template
void
sprintf_and_throw<range_error>(const char*, ...);

template
void
sprintf_and_throw<overflow_error>(const char*, ...);

template
void
sprintf_and_throw<underflow_error>(const char*, ...);

}  // namespace rocket
