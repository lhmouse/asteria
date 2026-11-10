// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ROCKET_ONCE_FLAG_
#define ASTERIA_ROCKET_ONCE_FLAG_

#include "fwd.hpp"
#include "xassert.hpp"
#include "unique_ptr.hpp"
#include <cxxabi.h>
namespace asteria {

class once_flag
  {
  private:
#if defined _CXXABI_TWEAKS_H
    using guard_type = int64_t;
#else
    using guard_type = uint64_t;
#endif
    using sentry_type = unique_ptr<guard_type, decltype(::abi::__cxa_guard_release)*>;
    guard_type m_guard = 0;

  public:
    constexpr
    once_flag()
      noexcept = default;

    once_flag(const once_flag&) = delete;
    once_flag& operator=(const once_flag&) = delete;

  public:
    bool
    test()
      const noexcept
      {
        return __atomic_load_n(&(this->m_guard), __ATOMIC_RELAXED);
      }

    template<typename funcT, typename... paramsT>
    void
    call(funcT&& func, paramsT&&... params)
      {
        // Try acquiring the guard. If `__atomic_load_n()` returns zero,
        // another thread will have finished initialization.
        if(__atomic_load_n(reinterpret_cast<char*>(&(this->m_guard)), __ATOMIC_ACQUIRE)
            || (::abi::__cxa_guard_acquire(&(this->m_guard)) == 0))
          return;

        sentry_type sentry(&(this->m_guard), ::abi::__cxa_guard_abort);
        forward<funcT>(func) (forward<paramsT>(params)...);
        sentry.get_deleter() = ::abi::__cxa_guard_release;
      }
  };

}  // namespace asteria
#endif
