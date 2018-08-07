// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPATIBILITY_H_
#define ROCKET_COMPATIBILITY_H_

#if defined(_MSC_VER)
#  define ROCKET_MSVC        1
#elif defined(__clang__)
#  define ROCKET_CLANG       1
#elif defined(__GNUC__)
#  define ROCKET_GCC         1
#else
#  error This compiler is not supported.
#endif

//======================================================================
// Microsoft specific
//======================================================================
#ifdef ROCKET_MSVC

#ifdef _DEBUG
#  define ROCKET_DEBUG                1
#endif

#define ROCKET_NORETURN               __declspec(noreturn)
#define ROCKET_FORMAT_CHECK(...)      // not implemented
#define ROCKET_FUNCSIG                __FUNCSIG__
#define ROCKET_UNREACHABLE()          __assume(0)
#define ROCKET_SELECTANY              __declspec(selectany)

#endif // ROCKET_MSVC

//======================================================================
// Clang specific
//======================================================================
#ifdef ROCKET_CLANG

#ifdef _LIBCPP_DEBUG
#  define ROCKET_DEBUG                1
#endif

#define ROCKET_NORETURN               __attribute__((__noreturn__))
#define ROCKET_FORMAT_CHECK(...)      __attribute__((__format__(__VA_ARGS__)))
#define ROCKET_FUNCSIG                __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()          __builtin_unreachable()
#define ROCKET_SELECTANY              __attribute__((__weak__))

#endif // ROCKET_CLANG

//======================================================================
// GCC specific
//======================================================================
#ifdef ROCKET_GCC

#ifdef _GLIBCXX_DEBUG
#  define ROCKET_DEBUG                1
#endif

#define ROCKET_NORETURN               __attribute__((__noreturn__))
#define ROCKET_FORMAT_CHECK(...)      __attribute__((__format__(__VA_ARGS__)))
#define ROCKET_FUNCSIG                __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()          __builtin_unreachable()
#define ROCKET_SELECTANY              __attribute__((__weak__))

#endif // ROCKET_GCC

#endif
