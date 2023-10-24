// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XCOMPILER_
#define ROCKET_XCOMPILER_

// Clang may pretend to be GCC or MSVC so has to be checked very first.
#if defined(__clang__)
#  include "details/xcompiler_clang.i"
#elif defined(__GNUC__)
#  include "details/xcompiler_gcc.i"
#elif defined(_MSC_VER)
#  include "details/xcompiler_msvc.i"
#endif

// Ensure all requested macros are defined.
#if !(1  \
    && defined(ROCKET_ATTRIBUTE_PRINTF)  \
    && defined(ROCKET_SELECTANY)  \
    && defined(ROCKET_SECTION)  \
    && defined(ROCKET_NEVER_INLINE)  \
    && defined(ROCKET_PURE)  \
    && defined(ROCKET_CONST)  \
    && defined(ROCKET_ARTIFICIAL)  \
    && defined(ROCKET_FLATTEN)  \
    && defined(ROCKET_ALWAYS_INLINE)  \
    && defined(ROCKET_COLD)  \
    && defined(ROCKET_HOT)  \
    \
    && defined(ROCKET_UNREACHABLE)  \
    && defined(ROCKET_EXPECT)  \
    && defined(ROCKET_UNEXPECT)  \
    && defined(ROCKET_CONSTANT_P)  \
    \
    && defined(ROCKET_FUNCSIG)  \
    \
    && defined(ROCKET_LZCNT32)  \
    && defined(ROCKET_TZCNT32)  \
    && defined(ROCKET_LZCNT64)  \
    && defined(ROCKET_TZCNT64)  \
    && defined(ROCKET_POPCNT32)  \
    && defined(ROCKET_POPCNT64)  \
    \
    && defined(ROCKET_ADD_OVERFLOW)  \
    && defined(ROCKET_SUB_OVERFLOW)  \
    && defined(ROCKET_MUL_OVERFLOW)  \
  )
#  error Some macros are missing.
#endif

// Disable standard C assertions by default.
#if !defined(NDEBUG) && !defined(ROCKET_DEBUG)
#  define NDEBUG  1
#endif

#endif  // ROCKET_XCOMPILER_
