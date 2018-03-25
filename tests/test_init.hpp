// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_INIT_HPP_
#define ASTERIA_TEST_INIT_HPP_

#include "../src/precompiled.hpp"
#include "../src/fwd.hpp"
#include <iostream>

#define ASTERIA_TEST_CHECK(expr_)	\
	((void)((expr_) ? 1 : ((::std::cerr	\
		<<"ASTERIA_TEST_CHECK() failed: " <<#expr_ <<'\n'	\
		<<"  File: " <<__FILE__ <<'\n'	\
		<<"  Line: " <<__LINE__ <<'\n'	\
		<< ::std::flush), ::std::terminate(), 0)))

#endif
