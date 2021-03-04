// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFERENCE_COUNTER_HPP_
#define ROCKET_REFERENCE_COUNTER_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include <atomic>  // std::atomic<>
#include <exception>  // std::terminate()

namespace rocket {

template<typename valueT = long>
class reference_counter;

template<typename valueT>
class reference_counter
  {
    static_assert(
        is_integral<valueT>::value && !is_same<valueT, bool>::value,
        "Invalid reference counter value type");

  public:
    using value_type  = valueT;

  private:
    ::std::atomic<value_type> m_nref;

  public:
    constexpr
    reference_counter()
      noexcept
      : m_nref(1)
      { }

    explicit constexpr
    reference_counter(value_type nref)
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
      {
        auto old = this->m_nref.load(::std::memory_order_relaxed);
        if(old > 1)
          ::std::terminate();
      }

  public:
    bool
    unique()
      const noexcept
      { return this->m_nref.load(::std::memory_order_relaxed) == 1;  }

    value_type
    get()
      const noexcept
      { return this->m_nref.load(::std::memory_order_relaxed);  }

    bool
    try_increment()
      noexcept
      {
        auto old = this->m_nref.load(::std::memory_order_relaxed);
        for(;;)
          if(old == 0)
            return false;
          else if(this->m_nref.compare_exchange_weak(old, old + 1,
                           ::std::memory_order_relaxed))
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
