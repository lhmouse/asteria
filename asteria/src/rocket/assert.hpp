// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_HPP_
#define ROCKET_ASSERT_HPP_

#include "compatibility.h"

namespace rocket {

// `report_assertion_failure()` is always provided even when assertions are disabled.
[[noreturn]] extern void report_assertion_failure(const char *expr, const char *file, long line, const char *msg) noexcept;

// When an assertion fails, do this.
[[noreturn]] ROCKET_ARTIFICIAL_FUNCTION inline bool assertion_failure_handler(const char *expr, const char *file, long line, const char *msg) noexcept
  {
#ifdef ROCKET_DEBUG
    report_assertion_failure(expr, file, line, msg);
#else
    ROCKET_UNREACHABLE();
#endif
    // Silence 'unused parameter' warnings.
    (void(expr - file + line + msg));
  }

// This function template exists to prevent the argument expression of `ROCKET_ASSERT()` from being evaluated more than once.
template<typename valueT>
  ROCKET_ARTIFICIAL_FUNCTION constexpr valueT && asserted_value_wrapper(valueT &&value, const char *expr, const char *file, long line, const char *msg) noexcept
  {
    return void(bool(value) || assertion_failure_handler(expr, file, line, msg)),
           static_cast<valueT &&>(value);
  }

}

#define ROCKET_ASSERT_IMPLEMENTATION_(value_, ...)  \
    (ROCKET_CONSTANT_P(value_)  \
     ? void(bool(value_) || ::rocket::assertion_failure_handler(__VA_ARGS__)),  \
       (value_)  \
     : ::rocket::asserted_value_wrapper(value_, __VA_ARGS__))

#define ROCKET_ASSERT(expr_)              ROCKET_ASSERT_IMPLEMENTATION_(expr_, #expr_, __FILE__, __LINE__, (""))
#define ROCKET_ASSERT_MSG(expr_, m_)      ROCKET_ASSERT_IMPLEMENTATION_(expr_, #expr_, __FILE__, __LINE__, (m_))

#endif
