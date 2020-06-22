// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ATOMIC_HPP_
#define ROCKET_ATOMIC_HPP_

#include "compiler.h"
#include <atomic>  // std::atomic<>

namespace rocket {

template<typename valueT, ::std::memory_order memorderT = ::std::memory_order_acq_rel>
class atomic;

/* Differences from `std::atomic`:
 * 1. The default constructor value-initializes the object.
 * 2. Memory order is a template argument and cannot be specified elsewhere.
**/

template<typename valueT, ::std::memory_order memorderT>
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

    atomic(const atomic&)
      = delete;

    atomic&
    operator=(const atomic&)
      = delete;

  public:
    // load/store operations
    value_type
    load()
    const noexcept
      {
        switch(memorderT) {
          case ::std::memory_order_relaxed:
          case ::std::memory_order_release:
            return this->m_val.load(::std::memory_order_relaxed);

          case ::std::memory_order_acquire:
          case ::std::memory_order_acq_rel:
            return this->m_val.load(::std::memory_order_acquire);

          case ::std::memory_order_consume:
          case ::std::memory_order_seq_cst:
            return this->m_val.load(memorderT);

          default:
            ROCKET_ASSERT(false);
        }
      }

    atomic&
    store(value_type val)
    noexcept
      {
        this->m_val.store(val, memorderT);
        return *this;
      }

    // exchange operations
    value_type
    exchange(value_type val)
    noexcept
      { return this->m_val.exchange(val, memorderT);  }

    bool
    compare_exchange(value_type& cmp, value_type xchg)
    noexcept
      { return this->m_val.compare_exchange_weak(cmp, xchg, memorderT);  }

    // arithmetic operations
    template<typename otherT>
    value_type
    fetch_add(otherT other)
    noexcept
      { return this->m_val.fetch_add(other, memorderT);  }

    template<typename otherT>
    value_type
    fetch_sub(otherT other)
    noexcept
      { return this->m_val.fetch_sub(other, memorderT);  }

    template<typename otherT>
    value_type
    fetch_and(otherT other)
    noexcept
      { return this->m_val.fetch_and(other, memorderT);  }

    template<typename otherT>
    value_type
    fetch_or(otherT other)
    noexcept
      { return this->m_val.fetch_or(other, memorderT);  }

    template<typename otherT>
    value_type
    fetch_xor(otherT other)
    noexcept
      { return this->m_val.fetch_xor(other, memorderT);  }

    value_type
    increment()
    noexcept
      { return this->m_val.fetch_add(1, memorderT) + 1;  }

    value_type
    decrement()
    noexcept
      { return this->m_val.fetch_sub(1, memorderT) - 1;  }
  };

template<typename valueT>
using atomic_relaxed = atomic<valueT, ::std::memory_order_relaxed>;

template<typename valueT>
using atomic_acq_rel = atomic<valueT, ::std::memory_order_acq_rel>;

template<typename valueT>
using atomic_seq_cst = atomic<valueT, ::std::memory_order_seq_cst>;

}  // namespace rocket

#endif
