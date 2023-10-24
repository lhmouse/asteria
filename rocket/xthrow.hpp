// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XTHROW_
#define ROCKET_XTHROW_

#include "fwd.hpp"
namespace rocket {

template<typename exceptT>
[[noreturn]] ROCKET_ATTRIBUTE_PRINTF(1, 2)
void
sprintf_and_throw(const char* fmt, ...);

extern template void sprintf_and_throw<logic_error>(const char*, ...);
extern template void sprintf_and_throw<domain_error>(const char*, ...);
extern template void sprintf_and_throw<invalid_argument>(const char*, ...);
extern template void sprintf_and_throw<length_error>(const char*, ...);
extern template void sprintf_and_throw<out_of_range>(const char*, ...);
extern template void sprintf_and_throw<runtime_error>(const char*, ...);
extern template void sprintf_and_throw<range_error>(const char*, ...);
extern template void sprintf_and_throw<overflow_error>(const char*, ...);
extern template void sprintf_and_throw<underflow_error>(const char*, ...);

}  // namespace rocket
#endif
