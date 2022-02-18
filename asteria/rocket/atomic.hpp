// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ATOMIC_HPP_
#define ROCKET_ATOMIC_HPP_

#include "assert.hpp"
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
    atomic(value_type val = { }) noexcept
      : m_val(val)
      { }

    atomic(const atomic&)
      = delete;

    atomic&
    operator=(const atomic&)
      = delete;

  private:
    static constexpr ::std::memory_order
    do_order_acquire() noexcept
      {
        switch(memorderT) {
          case ::std::memory_order_consume:
            return ::std::memory_order_consume;

          case ::std::memory_order_acquire:
          case ::std::memory_order_release:
          case ::std::memory_order_acq_rel:
            return ::std::memory_order_acquire;

          default:  // relaxed || seq_cst
            return memorderT;
        }
      }

    static constexpr ::std::memory_order
    do_order_release() noexcept
      {
        switch(memorderT) {
          case ::std::memory_order_consume:
          case ::std::memory_order_acquire:
          case ::std::memory_order_release:
          case ::std::memory_order_acq_rel:
            return ::std::memory_order_release;

          default:  // relaxed || seq_cst
            return memorderT;
        }
      }

    static constexpr ::std::memory_order
    do_order_acq_rel() noexcept
      {
        switch(memorderT) {
          case ::std::memory_order_consume:
          case ::std::memory_order_acquire:
          case ::std::memory_order_release:
          case ::std::memory_order_acq_rel:
            return ::std::memory_order_acq_rel;

          default:  // relaxed || seq_cst
            return memorderT;
        }
      }

  public:
    // load/store operations
    value_type
    load() const noexcept
      { return this->m_val.load(this->do_order_acquire());  }

    atomic&
    store(value_type val) noexcept
      { this->m_val.store(val, this->do_order_release());  return *this;  }

    // exchange operations
    value_type
    exchange(value_type val) noexcept
      { return this->m_val.exchange(val, this->do_order_acq_rel());  }

    bool
    compare_exchange(value_type& cmp, value_type xchg) noexcept
      { return this->m_val.compare_exchange_weak(cmp, xchg, this->do_order_acq_rel());  }

    // arithmetic operations
    template<typename otherT>
    value_type
    fetch_add(otherT other) noexcept
      { return this->m_val.fetch_add(other, this->do_order_acq_rel());  }

    template<typename otherT>
    value_type
    fetch_sub(otherT other) noexcept
      { return this->m_val.fetch_sub(other, this->do_order_acq_rel());  }

    template<typename otherT>
    value_type
    fetch_and(otherT other) noexcept
      { return this->m_val.fetch_and(other, this->do_order_acq_rel());  }

    template<typename otherT>
    value_type
    fetch_or(otherT other) noexcept
      { return this->m_val.fetch_or(other, this->do_order_acq_rel());  }

    template<typename otherT>
    value_type
    fetch_xor(otherT other) noexcept
      { return this->m_val.fetch_xor(other, this->do_order_acq_rel());  }
  };

template<typename valueT>
using atomic_relaxed = atomic<valueT, ::std::memory_order_relaxed>;

template<typename valueT>
using atomic_consume = atomic<valueT, ::std::memory_order_consume>;

template<typename valueT>
using atomic_acquire = atomic<valueT, ::std::memory_order_acquire>;

template<typename valueT>
using atomic_release = atomic<valueT, ::std::memory_order_release>;

template<typename valueT>
using atomic_acq_rel = atomic<valueT, ::std::memory_order_acq_rel>;

template<typename valueT>
using atomic_seq_cst = atomic<valueT, ::std::memory_order_seq_cst>;

}  // namespace rocket

#endif
