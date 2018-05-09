// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPATIBILITY_HPP_
#define ROCKET_COMPATIBILITY_HPP_

#if defined(_MSC_VER)
#  define ROCKET_MSVC          1
#elif defined(__clang__)
#  define ROCKET_CLANG         1
#elif defined(__GNUC__)
#  define ROCKET_GCC           1
#else
#  error This compiler is not supported.
#endif

//======================================================================
// Microsoft specific
//======================================================================
#ifdef ROCKET_MSVC

#ifdef _DEBUG
#  define ROCKET_DEBUG     1
#endif

#define ROCKET_NORETURN    __declspec(noreturn)

#endif // ROCKET_MSVC

//======================================================================
// Clang specific
//======================================================================
#ifdef ROCKET_CLANG

#ifdef _LIBCPP_DEBUG
#  define ROCKET_DEBUG     1
#endif

#define ROCKET_NORETURN    __attribute__((__noreturn__))

#endif // ROCKET_CLANG

//======================================================================
// GCC specific
//======================================================================
#ifdef ROCKET_GCC

#ifdef _GLIBCXX_DEBUG
#  define ROCKET_DEBUG     1
#endif

#define ROCKET_NORETURN    __attribute__((__noreturn__))

#endif // ROCKET_GCC

#endif
