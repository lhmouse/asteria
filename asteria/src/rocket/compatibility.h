// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPATIBILITY_H_
#define ROCKET_COMPATIBILITY_H_

#if defined(__clang__)
#  define ROCKET_CLANG       1
#elif defined(__GNUC__)
#  define ROCKET_GCC         1
#elif defined(_MSC_VER)
#  define ROCKET_MSVC        1
#else
#  error This compiler is not supported.
#endif

//======================================================================
// Clang specific
//======================================================================
#ifdef ROCKET_CLANG

#if !defined(ROCKET_DEBUG) && defined(_LIBCPP_DEBUG)
#  define ROCKET_DEBUG                   1
#endif

#define ROCKET_ATTRIBUTE_PRINTF(...)     __attribute__((__format__(__printf__, __VA_ARGS__)))
#define ROCKET_FUNCSIG                   __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()             __builtin_unreachable()
#define ROCKET_SELECTANY                 __attribute__((__weak__))
#define ROCKET_TRIVIAL_TYPE_TRAITS       1

#endif // ROCKET_CLANG

//======================================================================
// GCC specific
//======================================================================
#ifdef ROCKET_GCC

#if !defined(ROCKET_DEBUG) && defined(_GLIBCXX_DEBUG)
#  define ROCKET_DEBUG                   1
#endif

#define ROCKET_ATTRIBUTE_PRINTF(...)     __attribute__((__format__(__gnu_printf__, __VA_ARGS__)))
#define ROCKET_FUNCSIG                   __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()             __builtin_unreachable()
#define ROCKET_SELECTANY                 __attribute__((__weak__))
#define ROCKET_TRIVIAL_TYPE_TRAITS       (__GNUG__ >= 5)

#endif // ROCKET_GCC

//======================================================================
// Microsoft specific
//======================================================================
#ifdef ROCKET_MSVC

#if !defined(ROCKET_DEBUG) && defined(_DEBUG)
#  define ROCKET_DEBUG                   1
#endif

#define ROCKET_ATTRIBUTE_PRINTF(...)     // not implemented
#define ROCKET_FUNCSIG                   __FUNCSIG__
#define ROCKET_UNREACHABLE()             __assume(0)
#define ROCKET_SELECTANY                 __declspec(selectany)
#define ROCKET_TRIVIAL_TYPE_TRAITS       1

#endif // ROCKET_MSVC

#endif
