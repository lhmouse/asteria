// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ATOMIC_
#define ROCKET_ATOMIC_

#include "fwd.hpp"
#include "xassert.hpp"
namespace rocket {

// Differences from `std::atomic`:
// 1. The default constructor value-initializes the object.
// 2. Memory order is a template argument and cannot be specified elsewhere.
template<typename valueT, memory_order memorderT = memory_order_acq_rel>
class atomic
  {
  public:
    using value_type = valueT;

  private:
    ::std::atomic<value_type> m_val;

  public:
    atomic()
      noexcept
      :
        m_val(value_type())
      { }

    explicit
    atomic(value_type val)
      noexcept
      :
        m_val(val)
      { }

    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;

  private:
    static constexpr
    memory_order
    do_order_acquire()
      noexcept
      {
        switch(memorderT)
          {
          case memory_order_consume:
            return memory_order_consume;

          case memory_order_acquire:
          case memory_order_release:
          case memory_order_acq_rel:
            return memory_order_acquire;

          case memory_order_relaxed:
          case memory_order_seq_cst:
            return memorderT;

          default:
            ROCKET_ASSERT(false);
          }
      }

    static constexpr
    memory_order
    do_order_release()
      noexcept
      {
        switch(memorderT)
          {
          case memory_order_consume:
          case memory_order_acquire:
          case memory_order_release:
          case memory_order_acq_rel:
            return memory_order_release;

          case memory_order_relaxed:
          case memory_order_seq_cst:
            return memorderT;

          default:
            ROCKET_ASSERT(false);
          }
      }

    static constexpr
    memory_order
    do_order_acq_rel()
      noexcept
      {
        switch(memorderT)
          {
          case memory_order_consume:
          case memory_order_acquire:
          case memory_order_release:
          case memory_order_acq_rel:
            return memory_order_acq_rel;

          case memory_order_relaxed:
          case memory_order_seq_cst:
            return memorderT;

          default:
            ROCKET_ASSERT(false);
          }
      }

  public:
    value_type
    load()
      const noexcept
      {
        return this->m_val.load(this->do_order_acquire());
      }

    value_type
    store(value_type val)
      noexcept
      {
        this->m_val.store(val, this->do_order_release());
        return val;
      }

    value_type
    xchg(value_type val)
      noexcept
      {
        return this->m_val.exchange(val, this->do_order_acq_rel());
      }

    bool
    cmpxchg_weak(value_type& cmp, value_type xchg)
      noexcept
      {
        return this->m_val.compare_exchange_weak(cmp, xchg, this->do_order_acq_rel());
      }

    bool
    cmpxchg(value_type& cmp, value_type xchg)
      noexcept
      {
        return this->m_val.compare_exchange_strong(cmp, xchg, this->do_order_acq_rel());
      }

    template<typename otherT>
    value_type
    xadd(otherT&& other)
      noexcept
      {
        return this->m_val.fetch_add(other, this->do_order_acq_rel());
      }

    template<typename otherT>
    value_type
    xsub(otherT&& other)
      noexcept
      {
        return this->m_val.fetch_sub(other, this->do_order_acq_rel());
      }
  };

template<typename valueT>
using atomic_relaxed = atomic<valueT, memory_order_relaxed>;

template<typename valueT>
using atomic_consume = atomic<valueT, memory_order_consume>;

template<typename valueT>
using atomic_acquire = atomic<valueT, memory_order_acquire>;

template<typename valueT>
using atomic_release = atomic<valueT, memory_order_release>;

template<typename valueT>
using atomic_acq_rel = atomic<valueT, memory_order_acq_rel>;

template<typename valueT>
using atomic_seq_cst = atomic<valueT, memory_order_seq_cst>;

}  // namespace rocket
#endif
