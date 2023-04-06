// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_
#define ROCKET_ASSERT_
namespace rocket {

// This is always declared even when assertions are disabled.
[[noreturn]]
void
assert_fail(const char* expr, const char* file, long line, const char* msg) noexcept;

#ifdef ROCKET_DEBUG
#  define ROCKET_ASSERT_FAIL(...)    ::rocket::assert_fail(__VA_ARGS__)
#else
#  define ROCKET_ASSERT_FAIL(...)    ROCKET_UNREACHABLE()
#endif

#define ROCKET_ASSERT_MSG(expr, msg)  \
          ((expr) ? (void) 0 : ROCKET_ASSERT_FAIL(#expr, __FILE__, __LINE__, (msg)))

#define ROCKET_ASSERT(expr)  \
          ((expr) ? (void) 0 : ROCKET_ASSERT_FAIL(#expr, __FILE__, __LINE__, ""))

}  // namespace rocket
#endif
