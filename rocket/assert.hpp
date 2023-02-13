// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ASSERT_
#define ROCKET_ASSERT_
namespace rocket {

// `report_assertion_failure()` is always provided even when assertions are disabled.
[[noreturn]] void
report_assertion_failure(const char* expr, const char* file, long line, const char* msg) noexcept;

#ifdef ROCKET_DEBUG
#  define ROCKET_XASSERT_FAIL_(...)     ::rocket::report_assertion_failure(__VA_ARGS__)
#else
#  define ROCKET_XASSERT_FAIL_(...)     ROCKET_UNREACHABLE()
#endif

#define ROCKET_XASSERT_(cond, str, mt)  \
    ((cond) ? void(0) : ROCKET_XASSERT_FAIL_(str, __FILE__, __LINE__, mt))

#define ROCKET_ASSERT(expr)             ROCKET_XASSERT_(expr, #expr, "")
#define ROCKET_ASSERT_MSG(expr, mt)     ROCKET_XASSERT_(expr, #expr, mt)

}  // namespace rocket
#endif
