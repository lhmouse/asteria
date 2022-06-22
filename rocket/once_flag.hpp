// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ONCE_FLAG_
#define ROCKET_ONCE_FLAG_

#include "fwd.hpp"
#include "assert.hpp"

namespace rocket {

class once_flag;

#include "details/once_flag.ipp"

class once_flag
  {
  private:
    details_once_flag::guard m_guard[1] = { };

  public:
    constexpr
    once_flag() noexcept
      { }

    once_flag(const once_flag&)
      = delete;

    once_flag&
    operator=(const once_flag&)
      = delete;

  public:
    template<typename funcT, typename... paramsT>
    void
    call(funcT&& func, paramsT&&... params)
      {
        // Load the first byte with acquire semantics.
        // The value is 0 prior to any initialization and 1 afterwards.
        if(ROCKET_EXPECT(__atomic_load_n(this->m_guard->bytes, __ATOMIC_ACQUIRE)))
          return;

        // Try acquiring the guard.
        // If 0 is returned, another thread will have finished initialization.
        int status = details_once_flag::__cxa_guard_acquire(this->m_guard);
        if(ROCKET_EXPECT(status == 0))
          return;

        // Perform initialization the now.
        try {
          ::std::forward<funcT>(func)(::std::forward<paramsT>(params)...);
        }
        catch(...) {
          details_once_flag::__cxa_guard_abort(this->m_guard);
          throw;
        }
        details_once_flag::__cxa_guard_release(this->m_guard);
      }
  };

}  // namespace rocket

#endif
