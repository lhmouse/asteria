// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_
#  error Please #include <rocket/compiler.h> instead.
#endif
#undef ROCKET_DETAILS_PLATFORM_COMPILER_IMPLEMENTATION_

#define ROCKET_ATTRIBUTE_PRINTF(...)        __attribute__((__format__(__printf__, __VA_ARGS__)))
#define ROCKET_FUNCSIG                      __PRETTY_FUNCTION__
#define ROCKET_UNREACHABLE()                __builtin_unreachable()
#define ROCKET_SELECTANY                    __attribute__((__weak__))
#define ROCKET_EXPECT(...)                  __builtin_expect((__VA_ARGS__) ? 1 : 0, 1)
#define ROCKET_UNEXPECT(...)                __builtin_expect((__VA_ARGS__) ? 1 : 0, 0)
#define ROCKET_SECTION(...)                 __attribute__((__section__(__VA_ARGS__)))
#define ROCKET_NOINLINE                     __attribute__((__noinline__))
#define ROCKET_PURE_FUNCTION                __attribute__((__pure__))
#define ROCKET_CONST_FUNCTION               __attribute__((__const__))
#define ROCKET_ARTIFICIAL_FUNCTION          __attribute__((__artificial__, __always_inline__))
#define ROCKET_CONSTANT_P(...)              __builtin_constant_p(__VA_ARGS__)
#define ROCKET_FLATTEN_FUNCTION             __attribute__((__flatten__))
#define ROCKET_FORCED_INLINE_FUNCTION       __attribute__((__gnu_inline__, __always_inline__)) extern __inline__

// Check for either libc++ or libstdc++.
#if defined(_LIBCPP_DEBUG) || defined(_GLIBCXX_DEBUG)
#  define ROCKET_DEBUG                      1
#endif
