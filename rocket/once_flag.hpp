// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ONCE_FLAG_
#define ROCKET_ONCE_FLAG_

#include "fwd.hpp"
#include "xassert.hpp"
#include <cxxabi.h>
namespace rocket {

class once_flag
  {
  private:
    union Guard
      {
        unsigned long long ull;
        unsigned long ul;
        unsigned int ui;
        long long ll;
        long l;
        int i;
        bool b;

        operator unsigned long long*() noexcept { return &(this->ull);  }
        operator unsigned long*() noexcept { return &(this->ul);  }
        operator unsigned int*() noexcept { return &(this->ui);  }
        operator long long*() noexcept { return &(this->ll);  }
        operator long*() noexcept { return &(this->l);  }
        operator int*() noexcept { return &(this->i);  }
      };

    struct Sentry
      {
        decltype(::abi::__cxa_guard_abort)* dtor;
        Guard* guard;

        ~Sentry() { (* this->dtor) (* this->guard);  }
      };

    Guard m_guard = { 0 };

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
        return __atomic_load_n(&(this->m_guard.b), __ATOMIC_RELAXED);
      }

    template<typename funcT, typename... paramsT>
    void
    call(funcT&& func, paramsT&&... params)
      {
        // Try acquiring the guard. If `__atomic_load_n()` returns zero,
        // another thread will have finished initialization.
        if(__atomic_load_n(&(this->m_guard.b), __ATOMIC_ACQUIRE)
            || (::abi::__cxa_guard_acquire(this->m_guard) == 0))
          return;

        Sentry sentry = { ::abi::__cxa_guard_abort, &(this->m_guard) };
        forward<funcT>(func) (forward<paramsT>(params)...);
        sentry.dtor = ::abi::__cxa_guard_release;
      }
  };

}  // namespace rocket
#endif
