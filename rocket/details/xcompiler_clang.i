// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XCOMPILER_
#  error Please #include <rocket/xcompiler.h> instead.
#endif

#define ROCKET_ATTRIBUTE_PRINTF(...)        __attribute__((__format__(__printf__, __VA_ARGS__)))
#define ROCKET_SELECTANY                    __attribute__((__weak__))
#define ROCKET_SECTION(...)                 __attribute__((__section__(__VA_ARGS__)))
#define ROCKET_NEVER_INLINE                 __attribute__((__noinline__))
#define ROCKET_PURE                         __attribute__((__pure__))
#define ROCKET_CONST                        __attribute__((__const__))
#define ROCKET_ARTIFICIAL                   __attribute__((__artificial__, __always_inline__))
#define ROCKET_FLATTEN                      __attribute__((__flatten__))
#define ROCKET_ALWAYS_INLINE                __attribute__((__always_inline__)) __inline__
#define ROCKET_COLD                         __attribute__((__cold__))
#define ROCKET_HOT                          __attribute__((__hot__))

#define ROCKET_UNREACHABLE()                __builtin_unreachable()
#define ROCKET_EXPECT(...)                  __builtin_expect(!!(__VA_ARGS__), 1)
#define ROCKET_UNEXPECT(...)                __builtin_expect(!!(__VA_ARGS__), 0)
#define ROCKET_CONSTANT_P(...)              __builtin_constant_p(__VA_ARGS__)

#define ROCKET_FUNCSIG                      __PRETTY_FUNCTION__

#define ROCKET_LZCNT32(...)                 ((__VA_ARGS__) ? (__builtin_clz(__VA_ARGS__) & 31) : 32)
#define ROCKET_LZCNT64(...)                 ((__VA_ARGS__) ? (__builtin_clzll(__VA_ARGS__) & 63) : 64)
#define ROCKET_TZCNT32(...)                 ((__VA_ARGS__) ? (__builtin_ctz(__VA_ARGS__) & 31) : 32)
#define ROCKET_TZCNT64(...)                 ((__VA_ARGS__) ? (__builtin_ctzll(__VA_ARGS__) & 63) : 64)
#define ROCKET_POPCNT32(...)                __builtin_popcount(__VA_ARGS__)
#define ROCKET_POPCNT64(...)                __builtin_popcountll(__VA_ARGS__)

#define ROCKET_ADD_OVERFLOW(x,y,r)          __builtin_add_overflow(x,y,r)
#define ROCKET_SUB_OVERFLOW(x,y,r)          __builtin_sub_overflow(x,y,r)
#define ROCKET_MUL_OVERFLOW(x,y,r)          __builtin_mul_overflow(x,y,r)

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define ROCKET_BSWAP_TO_BE(w,x)           (x)
#  define ROCKET_BSWAP_TO_LE(w,x)           (__builtin_bswap##w(x))
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define ROCKET_BSWAP_TO_BE(w,x)           (__builtin_bswap##w(x))
#  define ROCKET_BSWAP_TO_LE(w,x)           (x)
#else
#  error unknown byte order
#endif

#define ROCKET_HTOBE16(x)                   ROCKET_BSWAP_TO_BE(16,x)
#define ROCKET_HTOBE32(x)                   ROCKET_BSWAP_TO_BE(32,x)
#define ROCKET_HTOBE64(x)                   ROCKET_BSWAP_TO_BE(64,x)
#define ROCKET_HTOLE16(x)                   ROCKET_BSWAP_TO_LE(16,x)
#define ROCKET_HTOLE32(x)                   ROCKET_BSWAP_TO_LE(32,x)
#define ROCKET_HTOLE64(x)                   ROCKET_BSWAP_TO_LE(64,x)
#define ROCKET_BETOH16(x)                   ROCKET_BSWAP_TO_BE(16,x)
#define ROCKET_BETOH32(x)                   ROCKET_BSWAP_TO_BE(32,x)
#define ROCKET_BETOH64(x)                   ROCKET_BSWAP_TO_BE(64,x)
#define ROCKET_LETOH16(x)                   ROCKET_BSWAP_TO_LE(16,x)
#define ROCKET_LETOH32(x)                   ROCKET_BSWAP_TO_LE(32,x)
#define ROCKET_LETOH64(x)                   ROCKET_BSWAP_TO_LE(64,x)

// Check for either libc++ or libstdc++.
#if defined(_LIBCPP_DEBUG) || defined(_GLIBCXX_DEBUG)
#  define ROCKET_DEBUG                      1
#endif

#if defined(__SSE2__) && (__clang_major__ < 8)
#  define _mm_storeu_si32(ptr, val)         _mm_store_ss((float*) (ptr), _mm_castsi128_ps(val))
#  define _mm_storeu_si64(ptr, val)         _mm_store_sd((double*) (ptr), _mm_castsi128_pd(val))
#endif
