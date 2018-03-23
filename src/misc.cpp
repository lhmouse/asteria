// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "misc.hpp"

namespace Asteria {

void throw_printf(const char *fmt, ...){
	char what[1024];
	std::va_list ap;
	va_start(ap, fmt);
	std::vsnprintf(what, sizeof(what), fmt, ap);
	va_end(ap);
	DEBUG_PRINTF("Throwing exception: %s\n", what);
	throw std::runtime_error(what);
}

}
