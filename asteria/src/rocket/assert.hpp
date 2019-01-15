// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_HPP_
#define ROCKET_ASSERT_HPP_

#include "compatibility.h"

namespace rocket {

[[noreturn]] extern void report_assertion_failure(const char *expr, const char *file, long line, const char *msg) noexcept;

template<typename ...paramsT>
  [[noreturn]] ROCKET_ARTIFICIAL_FUNCTION inline bool assertion_failure_handler(paramsT &&...params) noexcept
  {
#ifdef ROCKET_DEBUG
    // Disallow ADL.
    (report_assertion_failure)(static_cast<paramsT &&>(params)...);
#else
    ROCKET_UNREACHABLE();
    // Silence 'unused parameter' warnings.
    static_cast<void>(sizeof...(params));
#endif
  }

// This function template exists to prevent the argument expression of `ROCKET_ASSERT()` from being evaluated more than once.
template<typename valueT, typename ...paramsT>
  ROCKET_ARTIFICIAL_FUNCTION constexpr valueT && asserted_value_wrapper(valueT &&value, paramsT &&...params) noexcept
  {
    return (value ? true : (assertion_failure_handler)(static_cast<paramsT &&>(params)...)),
           static_cast<valueT &&>(value);
  }

}

#define ROCKET_ASSERT_IMPLEMENTATION_(value_, ...)  \
    (ROCKET_CONSTANT_P(value_) ? (((value_) ? true : ::rocket::assertion_failure_handler(__VA_ARGS__)), \
                                  (value_))  \
                               : ::rocket::asserted_value_wrapper((value_), __VA_ARGS__))

#define ROCKET_ASSERT(expr_)              ROCKET_ASSERT_IMPLEMENTATION_(expr_, #expr_, __FILE__, __LINE__, (""))
#define ROCKET_ASSERT_MSG(expr_, m_)      ROCKET_ASSERT_IMPLEMENTATION_(expr_, #expr_, __FILE__, __LINE__, (m_))

#endif
