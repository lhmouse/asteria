// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPATIBILITY_H_
#define ROCKET_COMPATIBILITY_H_

#ifdef __cplusplus
#  include <cstdbool>  // standard library configuration macros
#else
#  include <stdbool.h>  // bool, true, false
#endif

#define ROCKET_DETAILS_COMPATIBILITY_IMPLEMENTATION_  1

// Check for GCC.
#if !defined(__clang__) && defined(__GNUC__)
#  include "_compatibility_gcc.h"
#endif

// Check for Clang.
#if defined(__clang__)
#  include "_compatibility_clang.h"
#endif

// Check for MSVC.
#if !defined(__clang__) && defined(_MSC_VER)
#  include "_compatibility_msvc.h"
#endif

#endif
