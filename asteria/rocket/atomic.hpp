// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ATOMIC_HPP_
#define ROCKET_ATOMIC_HPP_

#include "compiler.h"
#include <atomic>  // std::atomic<>

namespace rocket {

template<typename valueT, ::std::memory_order morderT = ::std::memory_order_acq_rel>
class atomic;

/* Differences from `std::atomic`:
 * 1. The default constructor value-initializes the object.
 * 2. Memory order is a template argument and cannot be specified elsewhere.
**/

template<typename valueT, ::std::memory_order morderT>
class atomic
  {
  public:
    using value_type  = valueT;

  private:
    ::std::atomic<value_type> m_val;

  public:
    constexpr
    atomic(value_type val = { })
    noexcept
      : m_val(val)
      { }

    atomic&
    operator=(value_type val)
    volatile noexcept
      { return this->store(val);  }

    atomic(const atomic&)
      = delete;

    atomic&
    operator=(const atomic&)
      = delete;

  public:
    // load/store operations
    value_type
    load()
    const volatile noexcept
      {
        return this->m_val.load(morderT);
      }

    atomic&
    store(value_type val)
    volatile noexcept
      {
        this->m_val.store(val, morderT);
        return *this;
      }

    operator
    value_type()
    const volatile noexcept
      { return this->m_val.load(morderT);  }

    // exchange operations
    value_type
    exchange(value_type val)
    volatile noexcept
      { return this->m_val.exchange(val, morderT);  }

    bool
    compare_exchange(value_type& cmp, value_type xchg)
    volatile noexcept
      { return this->m_val.compare_exchange_weak(cmp, xchg, morderT);  }

    // arithmetic operations
    template<typename otherT>
    value_type
    fetch_add(otherT other)
    volatile noexcept
      { return this->m_val.fetch_add(other, morderT);  }

    template<typename otherT>
    value_type
    fetch_sub(otherT other)
    volatile noexcept
      { return this->m_val.fetch_sub(other, morderT);  }

    template<typename otherT>
    value_type
    fetch_and(otherT other)
    volatile noexcept
      { return this->m_val.fetch_and(other, morderT);  }

    template<typename otherT>
    value_type
    fetch_or(otherT other)
    volatile noexcept
      { return this->m_val.fetch_or(other, morderT);  }

    template<typename otherT>
    value_type
    fetch_xor(otherT other)
    volatile noexcept
      { return this->m_val.fetch_xor(other, morderT);  }

    value_type
    increment()
    volatile noexcept
      { return this->m_val.fetch_add(1, morderT) + 1;  }

    value_type
    decrement()
    volatile noexcept
      { return this->m_val.fetch_sub(1, morderT) - 1;  }
  };

template<typename valueT>
using atomic_relaxed = atomic<valueT, ::std::memory_order_relaxed>;

template<typename valueT>
using atomic_acq_rel = atomic<valueT, ::std::memory_order_acq_rel>;

template<typename valueT>
using atomic_seq_cst = atomic<valueT, ::std::memory_order_seq_cst>;

}  // namespace rocket

#endif
