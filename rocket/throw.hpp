// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_THROW_
#define ROCKET_THROW_

#include "fwd.hpp"
namespace rocket {

// Don't define this template here.
template<typename exceptT>
[[noreturn]] ROCKET_ATTRIBUTE_PRINTF(1, 2)
void
sprintf_and_throw(const char* fmt, ...);

// Declare specializations.
extern template
void
sprintf_and_throw<logic_error>(const char*, ...);

extern template
void
sprintf_and_throw<domain_error>(const char*, ...);

extern template
void
sprintf_and_throw<invalid_argument>(const char*, ...);

extern template
void
sprintf_and_throw<length_error>(const char*, ...);

extern template
void
sprintf_and_throw<out_of_range>(const char*, ...);

extern template
void
sprintf_and_throw<runtime_error>(const char*, ...);

extern template
void
sprintf_and_throw<range_error>(const char*, ...);

extern template
void
sprintf_and_throw<overflow_error>(const char*, ...);

extern template
void
sprintf_and_throw<underflow_error>(const char*, ...);

}  // namespace rocket
#endif
