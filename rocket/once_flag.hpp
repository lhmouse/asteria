// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ONCE_FLAG_
#define ROCKET_ONCE_FLAG_

#include "fwd.hpp"
#include "assert.hpp"
#include <cxxabi.h>
namespace rocket {

class once_flag
  {
  private:
    ::__cxxabiv1::__guard m_guard[1] = { };

  public:
    constexpr once_flag() noexcept = default;

    once_flag(const once_flag&) = delete;
    once_flag& operator=(const once_flag&) = delete;

  public:
    bool
    test() const noexcept
      {
        return __atomic_load_n((const bool*) this->m_guard, __ATOMIC_RELAXED);
      }

    template<typename funcT, typename... paramsT>
    void
    call(funcT&& func, paramsT&&... params)
      {
        // Try acquiring the guard. If `__atomic_load_n()` returns zero, another
        // thread will have finished initialization.
        if(__atomic_load_n((const bool*) this->m_guard, __ATOMIC_ACQUIRE)
            || (::__cxxabiv1::__cxa_guard_acquire(this->m_guard) == 0))
          return;

        try {
          ::std::forward<funcT>(func) (::std::forward<paramsT>(params)...);
          ::__cxxabiv1::__cxa_guard_release(this->m_guard);
        }
        catch(...) {
          ::__cxxabiv1::__cxa_guard_abort(this->m_guard);
          throw;
        }
      }
  };

}  // namespace rocket
#endif
