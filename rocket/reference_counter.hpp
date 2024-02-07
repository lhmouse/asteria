// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFERENCE_COUNTER_
#define ROCKET_REFERENCE_COUNTER_

#include "fwd.hpp"
#include "xassert.hpp"
#include <atomic>  // std::atomic<>
namespace rocket {

template<typename valueT = int>
class reference_counter
  {
    static_assert(is_integral<valueT>::value && !is_same<valueT, bool>::value);

  public:
    using value_type  = valueT;

  private:
    ::std::atomic<value_type> m_nref;

  public:
    constexpr reference_counter() noexcept
      : m_nref(1)  { }

    constexpr reference_counter(const reference_counter&) noexcept
      : m_nref(1)  { }

    constexpr
    reference_counter&
    operator=(const reference_counter&) & noexcept
      { return *this;  }

  public:
    bool
    unique() const noexcept
      { return this->m_nref.load(memory_order_relaxed) == 1;  }

    value_type
    get() const noexcept
      { return this->m_nref.load(memory_order_relaxed);  }

    // Increment the counter only if it is non-zero, and return its new value.
    value_type
    try_increment() noexcept
      {
        auto old = this->m_nref.load(memory_order_relaxed);
        for(;;)
          if(old == 0)
            return old;
          else if(this->m_nref.compare_exchange_weak(old, old + 1,
                                 memory_order_relaxed))
            return old + 1;
      }

    // Increment the counter and return its new value.
    value_type
    increment() noexcept
      {
        auto old = this->m_nref.fetch_add(1, memory_order_relaxed);
        ROCKET_ASSERT(old >= 1);
        return old + 1;
      }

    // Decrement the counter and return its new value.
    // This has to be done with acquire-release semantics, because the
    // operation which has decremented the counter to zero shall synchronize
    // with other write operations to associated shared resources.
    value_type
    decrement() noexcept
      {
        auto old = this->m_nref.fetch_sub(1, memory_order_acq_rel);
        ROCKET_ASSERT(old >= 1);
        return old - 1;
      }
  };

}  // namespace rocket
#endif
