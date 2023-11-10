// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XCOMPILER_
#  error Please #include <rocket/xcompiler.h> instead.
#endif

#define ROCKET_ATTRIBUTE_PRINTF(...)        // not implemented
#define ROCKET_SELECTANY                    __declspec(selectany)
#define ROCKET_SECTION(...)                 __declspec(allocate(__VA_ARGS__))
#define ROCKET_NEVER_INLINE                 __declspec(noinline)
#define ROCKET_PURE                         // not implemented
#define ROCKET_CONST                        // not implemented
#define ROCKET_ARTIFICIAL                   // not implemented
#define ROCKET_FLATTEN                      // not implemented
#define ROCKET_ALWAYS_INLINE                __forceinline
#define ROCKET_COLD                         // not implemented
#define ROCKET_HOT                          // not implemented

#define ROCKET_UNREACHABLE()                __assume(0)
#define ROCKET_EXPECT(...)                  (__VA_ARGS__)  // not implemented
#define ROCKET_UNEXPECT(...)                (__VA_ARGS__)  // not implemented
#define ROCKET_CONSTANT_P(...)              false  // not implemented

#define ROCKET_FUNCSIG                      __FUNCSIG__

#define ROCKET_LZCNT32(...)                 ((int) _lzcnt_u32(__VA_ARGS__))
#define ROCKET_LZCNT64(...)                 ((int) _lzcnt_u64(__VA_ARGS__))
#define ROCKET_TZCNT32(...)                 ((int) _tzcnt_u32(__VA_ARGS__))
#define ROCKET_TZCNT64(...)                 ((int) _tzcnt_u64(__VA_ARGS__))
#define ROCKET_POPCNT32(...)                ((int) _mm_popcnt_u32(__VA_ARGS__))
#define ROCKET_POPCNT64(...)                ((int) _mm_popcnt_u64(__VA_ARGS__))

#define ROCKET_ADD_OVERFLOW(x,y,r)          `not implemented`
#define ROCKET_SUB_OVERFLOW(x,y,r)          `not implemented`
#define ROCKET_MUL_OVERFLOW(x,y,r)          `not implemented`

// MSVC does not support any big-endian platforms.
#define ROCKET_HTOBE16(x)                   (_byteswap_ushort(x))
#define ROCKET_HTOBE32(x)                   (_byteswap_ulong(x))
#define ROCKET_HTOBE64(x)                   (_byteswap_uint64(x))
#define ROCKET_HTOLE16(x)                   (x)
#define ROCKET_HTOLE32(x)                   (x)
#define ROCKET_HTOLE64(x)                   (x)
#define ROCKET_BETOH16(x)                   (_byteswap_ushort(x))
#define ROCKET_BETOH32(x)                   (_byteswap_ulong(x))
#define ROCKET_BETOH64(x)                   (_byteswap_uint64(x))
#define ROCKET_LETOH16(x)                   (x)
#define ROCKET_LETOH32(x)                   (x)
#define ROCKET_LETOH64(x)                   (x)

// Check for project configuration.
#if defined(_DEBUG)
#  define ROCKET_DEBUG                      1
#endif
