// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPILER_H_
#define ROCKET_COMPILER_H_

#define ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_  1
// Clang may pretend to be GCC or MSVC so has to be checked very first.
#if defined(__clang__)
#  include "platform/compiler_clang.h"
#elif defined(__GNUC__)
#  include "platform/compiler_gcc.h"
#elif defined(_MSC_VER)
#  include "platform/compiler_msvc.h"
#endif

// Ensure all requested macros are defined.
#if !(1  \
        && defined(ROCKET_ATTRIBUTE_PRINTF)  \
        && defined(ROCKET_FUNCSIG)  \
        && defined(ROCKET_UNREACHABLE)  \
        && defined(ROCKET_SELECTANY)  \
        && defined(ROCKET_EXPECT)  \
        && defined(ROCKET_UNEXPECT)  \
        && defined(ROCKET_SECTION)  \
        && defined(ROCKET_NOINLINE)  \
        && defined(ROCKET_PURE_FUNCTION)  \
        && defined(ROCKET_CONST_FUNCTION)  \
        && defined(ROCKET_ARTIFICIAL_FUNCTION)  \
        && defined(ROCKET_CONSTANT_P)  \
        && defined(ROCKET_FLATTEN_FUNCTION)  \
        && defined(ROCKET_FORCED_INLINE_FUNCTION)  \
      )
#  error Some macros are missing.
#endif

#endif
