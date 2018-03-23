// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PRECOMPILED_HPP_
#define ASTERIA_PRECOMPILED_HPP_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <iterator>
#include <algorithm>
#include <utility>
#include <functional>
#include <iomanip>
#include <ostream>
#include <exception>
#include <memory>

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cctype>

#ifdef ENABLE_DEBUG_LOGS
#  define DEBUG_PRINTF(...)   (::std::fprintf(stderr, __VA_ARGS__))
#else
#  define DEBUG_PRINTF(...)   (-1)
#endif

namespace Asteria {
	//
}

#endif
