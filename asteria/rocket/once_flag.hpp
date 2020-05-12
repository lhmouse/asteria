// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ONCE_FLAG_HPP_
#define ROCKET_ONCE_FLAG_HPP_

#include "assert.hpp"
#include "utilities.hpp"

namespace rocket {

class once_flag;

template<typename funcT, typename... paramsT>
void call_once(once_flag& flag, funcT&& func, paramsT&&... params);

#include "details/once_flag.ipp"

class once_flag
  {
  private:
    details_once_flag::guard m_guard[1] = { };

  public:
    constexpr
    once_flag()
    noexcept
      { }

    once_flag(const once_flag&)
      = delete;

    once_flag&
    operator=(const once_flag&)
      = delete;

  public:
    details_once_flag::guard*
    get_guard()
    noexcept
      {
        return this->m_guard;
      }
  };

template<typename funcT, typename... paramsT>
void call_once(once_flag& flag, funcT&& func, paramsT&&... params)
  {
    details_once_flag::guard* const g = flag.get_guard();

    // Load the first byte with acquire semantics.
    // The value is 0 prior to any initialization and 1 afterwards.
    if(ROCKET_EXPECT(__atomic_load_n(g->bytes, __ATOMIC_ACQUIRE)))
      return;

    // Try acquiring the guard.
    // If 0 is returned, another thread will have finished initialization.
    int status = details_once_flag::__cxa_guard_acquire(g);
    if(ROCKET_EXPECT(status == 0))
      return;

    // Perform initialization now.
    try {
      ::std::forward<funcT>(func)(::std::forward<paramsT>(params)...);
    }
    catch(...) {
      details_once_flag::__cxa_guard_abort(g);
      throw;
    }
    details_once_flag::__cxa_guard_release(g);
  }

}  // namespace rocket

#endif
