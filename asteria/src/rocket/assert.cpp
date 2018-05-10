// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "assert.hpp"
#include <exception>
#include <cstdio>

namespace rocket {

void on_assert_fail(const char *expr, const char *file, unsigned long line, const char *msg) noexcept {
	::std::fprintf(stderr, "===============================================================================\n"
	                       "ASSERTION FAILED !!\n"
	                       "  Expression: %s\n"
	                       "  File:       %s\n"
	                       "  Line:       %lu\n"
	                       "  Message:    %s\n"
	                       "===============================================================================\n"
	                       , expr, file, line, msg);
	::std::terminate();
}

}
