// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_HPP_
#define ROCKET_ASSERT_HPP_

#include "compatibility.h"

namespace rocket {

// `report_assertion_failure()` is always provided even when assertions are disabled.
[[noreturn]] extern bool report_assertion_failure(const char *expr, const char *file, long line, const char *msg) noexcept;

}

#ifdef ROCKET_DEBUG
#  define ROCKET_DETAILS_REPORT_ASSERTION_FAILURE_(...)    ::rocket::report_assertion_failure(__VA_ARGS__)
#else
#  define ROCKET_DETAILS_REPORT_ASSERTION_FAILURE_(...)    ROCKET_UNREACHABLE(::rocket::report_assertion_failure(__VA_ARGS__))
#endif

#define ROCKET_DETAILS_IMPLEMENT_ASSERT_(pred_, xstr_, msg_)     static_cast<void>(bool(pred_) || ::rocket::report_assertion_failure(xstr_, __FILE__, __LINE__, msg_))

#define ROCKET_ASSERT(expr_)              ROCKET_DETAILS_IMPLEMENT_ASSERT_(expr_, #expr_, "")
#define ROCKET_ASSERT_MSG(expr_, m_)      ROCKET_DETAILS_IMPLEMENT_ASSERT_(expr_, #expr_, m_)

#endif
