// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COMPATIBILITY_CLANG_H_
#define ROCKET_COMPATIBILITY_CLANG_H_

// Check for compiler support.
#define ROCKET_ATTRIBUTE_PRINTF(...)        __attribute__((__format__(__printf__, __VA_ARGS__)))
#define ROCKET_FUNCSIG                      __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()                __builtin_unreachable()
#define ROCKET_SELECTANY                    __attribute__((__weak__))
#define ROCKET_EXPECT(...)                  __builtin_expect((__VA_ARGS__) ? 1 : 0, 1)
#define ROCKET_UNEXPECT(...)                __builtin_expect((__VA_ARGS__) ? 1 : 0, 0)
#define ROCKET_SECTION(...)                 __attribute__((__section__(__VA_ARGS__)))
#define ROCKET_NOINLINE                     __attribute__((__noinline__))
#define ROCKET_PURE_FUNCTION                __attribute__((__pure__))

// Check for either libc++ or libstdc++.
#if defined(_LIBCPP_DEBUG) || defined(_GLIBCXX_DEBUG)
#  define ROCKET_DEBUG                      1
#endif

// Check for libc++ or libstdc++ from GCC 5.
#if defined(_LIBCPP_VERSION) || defined(_GLIBCXX14_CONSTEXPR)
#  define ROCKET_HAS_TRIVIALITY_TRAITS      1
#endif

#endif
