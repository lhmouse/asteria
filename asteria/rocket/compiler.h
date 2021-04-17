// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPILER_H_
#define ROCKET_COMPILER_H_

// Clang may pretend to be GCC or MSVC so has to be checked very first.
#if defined(__clang__)
#  include "details/compiler_clang.i"
#elif defined(__GNUC__)
#  include "details/compiler_gcc.i"
#elif defined(_MSC_VER)
#  include "details/compiler_msvc.i"
#endif

// Ensure all requested macros are defined.
#if !(1  \
    && defined(ROCKET_ATTRIBUTE_PRINTF)  \
    && defined(ROCKET_SELECTANY)  \
    && defined(ROCKET_SECTION)  \
    && defined(ROCKET_NOINLINE)  \
    && defined(ROCKET_PURE_FUNCTION)  \
    && defined(ROCKET_CONST_FUNCTION)  \
    && defined(ROCKET_ARTIFICIAL_FUNCTION)  \
    && defined(ROCKET_FLATTEN_FUNCTION)  \
    && defined(ROCKET_FORCED_INLINE_FUNCTION)  \
    \
    && defined(ROCKET_UNREACHABLE)  \
    && defined(ROCKET_EXPECT)  \
    && defined(ROCKET_UNEXPECT)  \
    && defined(ROCKET_CONSTANT_P)  \
    \
    && defined(ROCKET_FUNCSIG)  \
    \
    && defined(ROCKET_LZCNT32)  \
    && defined(ROCKET_LZCNT64)  \
    && defined(ROCKET_TZCNT32)  \
    && defined(ROCKET_TZCNT64)  \
    && defined(ROCKET_POPCNT32)  \
    && defined(ROCKET_POPCNT64)  \
  )
#  error Some macros are missing.
#endif

#endif
