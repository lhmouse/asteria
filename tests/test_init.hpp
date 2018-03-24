// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_INIT_HPP_
#define ASTERIA_TEST_INIT_HPP_

#include "../src/precompiled.hpp"
#include "../src/fwd.hpp"

#define ASTERIA_TEST_CHECK(expr_)	\
	((void)((expr_) ? 1 :	\
		(::std::fprintf(stderr, "Test check failed: %s\n  File: %s\n  Line: %lu\n",	\
			#expr_, __FILE__, (unsigned long)__LINE__), ::std::abort(), 1)	\
		))

#endif
