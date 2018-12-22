// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPATIBILITY_H_
#define ROCKET_COMPATIBILITY_H_

#include <cstdbool>  // standard library configuration macros

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

// Check for compiler support.
#define ROCKET_ATTRIBUTE_PRINTF(...)        __attribute__((__format__(__printf__, __VA_ARGS__)))
#define ROCKET_FUNCSIG                      __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()                __builtin_unreachable()
#define ROCKET_SELECTANY                    __attribute__((__weak__))
#define ROCKET_EXPECT(...)                  __builtin_expect((__VA_ARGS__) ? 1 : 0, 1)
#define ROCKET_UNEXPECT(...)                __builtin_expect((__VA_ARGS__) ? 1 : 0, 0)
#define ROCKET_SECTION(...)                 __attribute__((__section__(__VA_ARGS__)))
#define ROCKET_NOINLINE                     __attribute__((__noinline__))

// Check for either libc++ or libstdc++.
#if defined(_LIBCPP_DEBUG) || defined(_GLIBCXX_DEBUG)
#  define ROCKET_DEBUG                      1
#endif

// Check for libc++ or libstdc++ from GCC 5.
#if defined(_LIBCPP_VERSION) || defined(_GLIBCXX14_CONSTEXPR)
#  define ROCKET_HAS_TRIVIALITY_TRAITS      1
#endif

#endif // ROCKET_CLANG

//======================================================================
// GCC specific
//======================================================================
#ifdef ROCKET_GCC

// Check for compiler support.
#define ROCKET_ATTRIBUTE_PRINTF(...)        __attribute__((__format__(__gnu_printf__, __VA_ARGS__)))
#define ROCKET_FUNCSIG                      __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()                __builtin_unreachable()
#define ROCKET_SELECTANY                    __attribute__((__weak__))
#define ROCKET_EXPECT(...)                  __builtin_expect((__VA_ARGS__) ? 1 : 0, 1)
#define ROCKET_UNEXPECT(...)                __builtin_expect((__VA_ARGS__) ? 1 : 0, 0)
#define ROCKET_SECTION(...)                 __attribute__((__section__(__VA_ARGS__)))
#define ROCKET_NOINLINE                     __attribute__((__noinline__))

// Check for libstdc++.
#if defined(_GLIBCXX_DEBUG)
#  define ROCKET_DEBUG                      1
#endif

// Check for libstdc++ from GCC 5.
#if defined(_GLIBCXX14_CONSTEXPR)
#  define ROCKET_HAS_TRIVIALITY_TRAITS      1
#endif

#endif // ROCKET_GCC

//======================================================================
// Microsoft specific
//======================================================================
#ifdef ROCKET_MSVC

// Check for compiler support.
#define ROCKET_ATTRIBUTE_PRINTF(...)        // not implemented
#define ROCKET_FUNCSIG                      __FUNCSIG__
#define ROCKET_UNREACHABLE()                __assume(0)
#define ROCKET_SELECTANY                    __declspec(selectany)
#define ROCKET_EXPECT(...)                  (__VA_ARGS__)
#define ROCKET_UNEXPECT(...)                (__VA_ARGS__)
#define ROCKET_SECTION(...)                 __declspec(allocate(__VA_ARGS__))
#define ROCKET_NOINLINE                     __declspec((noinline))

// Check for project configuration.
#if defined(_DEBUG)
#  define ROCKET_DEBUG                      1
#endif

// Check for the version of... the compiler?
#if _MSC_VER >= 1700
#  define ROCKET_HAS_TRIVIALITY_TRAITS      1
#endif

#endif // ROCKET_MSVC

#endif
