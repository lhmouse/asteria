// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include <cstdio>
#include <cstdlib>

#define TEST_CHECK(expr_)	\
	((void)((expr_) ? 1 :	\
		(::std::fprintf(stderr, "Test check failed: %s\n  File: %s\n  Line: %lu\n",	\
			#expr_, __FILE__, (unsigned long)__LINE__), ::std::abort(), 1)	\
		))
