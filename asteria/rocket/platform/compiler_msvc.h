// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_
#  error Please #include <rocket/compiler.h> instead.
#endif
#undef ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_

#define ROCKET_ATTRIBUTE_PRINTF(...)        // not implemented
#define ROCKET_FUNCSIG                      __FUNCSIG__
#define ROCKET_UNREACHABLE()                __assume(0)
#define ROCKET_SELECTANY                    __declspec(selectany)
#define ROCKET_EXPECT(...)                  (__VA_ARGS__)
#define ROCKET_UNEXPECT(...)                (__VA_ARGS__)
#define ROCKET_SECTION(...)                 __declspec(allocate(__VA_ARGS__))
#define ROCKET_NOINLINE                     __declspec(noinline)
#define ROCKET_PURE_FUNCTION                // not implemented
#define ROCKET_ARTIFICIAL_FUNCTION          // not implemented
#define ROCKET_CONSTANT_P(...)              false  // not implemented
#define ROCKET_FLATTEN_FUNCTION             __attribute__((__flatten__))
#define ROCKET_FORCED_INLINE_FUNCTION       __forceinline

// Check for project configuration.
#if defined(_DEBUG)
#  define ROCKET_DEBUG                      1
#endif
