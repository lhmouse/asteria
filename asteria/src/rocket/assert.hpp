// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_HPP_
#define ROCKET_ASSERT_HPP_

#include "compatibility.hpp"

namespace rocket {

ROCKET_NORETURN extern void on_assert_fail(const char *expr, const char *file, unsigned long line, const char *msg) noexcept;

}

#undef ROCKET_ASSERT
#undef ROCKET_ASSERT_MSG

#define ROCKET_ASSERT(expr_)              (static_cast<void>(	\
                                            (expr_) ? 1 : (ROCKET_DETAILS_ON_ASSERT_FAIL(#expr_, __FILE__, __LINE__, ("")), 0)	\
                                          ))
#define ROCKET_ASSERT_MSG(expr_, m_)      (static_cast<void>(	\
                                            (expr_) ? 1 : (ROCKET_DETAILS_ON_ASSERT_FAIL(#expr_, __FILE__, __LINE__, (m_)), 0)	\
                                          ))

#endif // ROCKET_ASSERT_HPP_

#ifdef ROCKET_DEBUG
#  define ROCKET_DETAILS_ON_ASSERT_FAIL(...)      (::rocket::on_assert_fail(__VA_ARGS__))
#else
#  define ROCKET_DETAILS_ON_ASSERT_FAIL(...)      (ROCKET_UNREACHABLE)
#endif
