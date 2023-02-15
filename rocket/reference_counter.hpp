// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFERENCE_COUNTER_
#define ROCKET_REFERENCE_COUNTER_

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
    static_assert(is_integral<valueT>::value && !is_same<valueT, bool>::value,
        "invalid reference counter value type");

  public:
    using value_type  = valueT;

  private:
    ::std::atomic<value_type> m_nref;

  public:
    constexpr
    reference_counter() noexcept
      : m_nref(1)  { }

    explicit constexpr
    reference_counter(value_type nref) noexcept
      : m_nref(nref)  { }

    constexpr
    reference_counter(const reference_counter&) noexcept
      : reference_counter()  { }

    reference_counter&
    operator=(const reference_counter&) & noexcept
      { return *this;  }

    ~reference_counter()
      {
        auto old = this->m_nref.load(memory_order_relaxed);
        if(old > 1)
          ::std::terminate();
      }

  public:
    bool
    unique() const noexcept
      { return this->m_nref.load(memory_order_relaxed) == 1;  }

    value_type
    get() const noexcept
      { return this->m_nref.load(memory_order_relaxed);  }

    // Increment the counter only if it is non-zero, and return its new value.
    long
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

    // Provide some convenient operators.
    operator
    value_type() const noexcept
      { return this->get();  }

    value_type
    operator++() noexcept
      { return this->increment();  }

    value_type
    operator--() noexcept
      { return this->decrement();  }

    value_type
    operator++(int) noexcept
      { return this->increment() - 1;  }

    value_type
    operator--(int) noexcept
      { return this->decrement() + 1;  }
  };

template
class reference_counter<long>;

}  // namespace rocket
#endif
