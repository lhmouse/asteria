// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPATIBILITY_MSVC_H_
#define ROCKET_COMPATIBILITY_MSVC_H_

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

#endif
