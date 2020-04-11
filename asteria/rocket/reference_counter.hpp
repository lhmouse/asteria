// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFERENCE_COUNTER_HPP_
#define ROCKET_REFERENCE_COUNTER_HPP_

#include "compiler.h"
#include "assert.hpp"
#include <atomic>  // std::atomic<>
#include <exception>  // std::terminate()

namespace rocket {

template<typename valueT = long>
class reference_counter;

template<typename valueT>
class reference_counter
  {
  private:
    ::std::atomic<valueT> m_nref;

  public:
    constexpr
    reference_counter()
    noexcept
      : m_nref(1)
      { }

    explicit constexpr
    reference_counter(valueT nref)
    noexcept
      : m_nref(nref)
      { }

    constexpr
    reference_counter(const reference_counter&)
    noexcept
      : reference_counter()
      { }

    reference_counter&
    operator=(const reference_counter&)
    noexcept
      { return *this;  }

    ~reference_counter()
      { this->do_terminate_if_shared();  }

  private:
    void
    do_terminate_if_shared()
    const
      {
        auto old = this->m_nref.load(::std::memory_order_relaxed);
        if(old > 1)
          ::std::terminate();
      }

  public:
    ROCKET_PURE_FUNCTION
    bool
    unique()
    const
    noexcept
      { return ROCKET_EXPECT(this->m_nref.load(::std::memory_order_relaxed) == 1);  }

    valueT
    get()
    const
    noexcept
      { return this->m_nref.load(::std::memory_order_relaxed);  }

    bool
    try_increment()
    noexcept
      {
        auto old = this->m_nref.load(::std::memory_order_relaxed);
        for(;;)
          if(old == 0)
            return false;
          else if(this->m_nref.compare_exchange_weak(old, old + 1, ::std::memory_order_relaxed))
            return true;
      }

    void
    increment()
    noexcept
      {
        auto old = this->m_nref.fetch_add(1, ::std::memory_order_relaxed);
        ROCKET_ASSERT(old >= 1);
      }

    bool
    decrement()
    noexcept
      {
        auto old = this->m_nref.fetch_sub(1, ::std::memory_order_acq_rel);
        ROCKET_ASSERT(old >= 1);
        return old == 1;
      }
  };

template
class reference_counter<long>;

}  // namespace rocket

#endif
