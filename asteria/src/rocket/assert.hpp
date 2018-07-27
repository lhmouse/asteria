// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_HPP_
#define ROCKET_ASSERT_HPP_

#include "compatibility.h"

namespace rocket
{

ROCKET_NORETURN extern void on_assert_fail(const char *expr, const char *file, unsigned long line, const char *msg) noexcept;

}

#ifdef ROCKET_DEBUG
#  define ROCKET_DETAILS_ASSERT(expr_, str_, m_)    ((expr_) ? (void)0 : ::rocket::on_assert_fail(str_, __FILE__, __LINE__, m_))
#else
#  define ROCKET_DETAILS_ASSERT(expr_, str_, m_)    ((expr_) ? (void)0 : ROCKET_UNREACHABLE())
#endif

#define ROCKET_ASSERT(expr_)              ROCKET_DETAILS_ASSERT(expr_, #expr_, "")
#define ROCKET_ASSERT_MSG(expr_, m_)      ROCKET_DETAILS_ASSERT(expr_, #expr_, m_)

#endif
