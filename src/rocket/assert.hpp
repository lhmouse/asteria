// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_HPP_
#define ROCKET_ASSERT_HPP_

#include <exception> // std::terminate()
#include <cstdio> // std::cerr, std::fprintf()

namespace rocket {

__attribute__((__noreturn__))
inline int on_assert_fail(const char *expr, const char *file, unsigned long line, const char *msg){
	::std::fprintf(stderr, "ASSERTION FAILED !!\n"
	                       "  Expression: %s\n"
	                       "  File:       %s\n"
	                       "  Line:       %lu\n"
	                       "  Message:    %s\n"
	                       , expr, file, line, msg);
	::std::terminate();
}

}

#endif

#undef ROCKET_ASSERT
#undef ROCKET_ASSERT_MSG

#if !defined(NDEBUG) || defined(_DEBUG) || defined(_GLIBCXX_DEBUG)
#  define ROCKET_ASSERT(expr_)                (static_cast<void>(((expr_) ? 1 : 0) || ::rocket::on_assert_fail(#expr_, __FILE__, __LINE__, (""  ))))
#  define ROCKET_ASSERT_MSG(expr_, msg_)      (static_cast<void>(((expr_) ? 1 : 0) || ::rocket::on_assert_fail(#expr_, __FILE__, __LINE__, (msg_))))
#else
#  define ROCKET_ASSERT(expr_)                (static_cast<void>(((expr_) ? 1 : 0) || (__builtin_unreachable(), 1)))
#  define ROCKET_ASSERT_MSG(expr_, msg_)      (static_cast<void>(((expr_) ? 1 : 0) || (__builtin_unreachable(), 1)))
#endif
