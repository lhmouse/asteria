// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_HPP_
#define ROCKET_ASSERT_HPP_

#include "compiler.h"

namespace rocket {

// `report_assertion_failure()` is always provided even when assertions are disabled.
[[noreturn]] extern void report_assertion_failure(const char* expr, const char* file, long line, const char* msg) noexcept;

}  // namespace rocket

#ifdef ROCKET_DEBUG
#  define ROCKET_XASSERT_FAIL_(...)        ::rocket::report_assertion_failure(__VA_ARGS__)
#else
#  define ROCKET_XASSERT_FAIL_(...)        ROCKET_UNREACHABLE()
#endif
#define ROCKET_XASSERT_(cond_, str_, m_)   ((cond_) ? void(0) : ROCKET_XASSERT_FAIL_(str_, __FILE__, __LINE__, m_))

#define ROCKET_ASSERT(expr_)               ROCKET_XASSERT_(expr_, #expr_, "")
#define ROCKET_ASSERT_MSG(expr_, m_)       ROCKET_XASSERT_(expr_, #expr_, m_)

#endif
