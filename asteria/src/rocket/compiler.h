// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPILER_H_
#define ROCKET_COMPILER_H_

#ifdef __cplusplus
#  include <cstdbool>  // standard library configuration macros
#else
#  include <stdbool.h>  // bool, true, false
#endif

#define ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_ 1

#if defined(__clang__)
#  include "platform/compiler_clang.h"
#elif defined(__GNUC__)
#  include "platform/compiler_gcc.h"
#elif defined(_MSC_VER)
#  include "platform/compiler_msvc.h"
#endif

#ifdef ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_
#  error "`ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_` must have been `#undef`'d here."
#endif

#endif
