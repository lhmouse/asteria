// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

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
    template<typename funcT, typename... paramsT>
    void
    call(funcT&& func, paramsT&&... params)
      {
        // Load the first byte with acquire semantics.
        // The value is 0 prior to any initialization and 1 afterwards.
        if(__atomic_load_n((char*) this->m_guard, __ATOMIC_ACQUIRE))
          return;

        // Try acquiring the guard.
        // If 0 is returned, another thread will have finished initialization.
        int status = ::__cxxabiv1::__cxa_guard_acquire(this->m_guard);
        if(ROCKET_EXPECT(status == 0))
          return;

        // Perform initialization the now.
        try {
          ::std::forward<funcT>(func)(::std::forward<paramsT>(params)...);
        }
        catch(...) {
          ::__cxxabiv1::__cxa_guard_abort(this->m_guard);
          throw;
        }
        ::__cxxabiv1::__cxa_guard_release(this->m_guard);
      }
  };

}  // namespace rocket
#endif
