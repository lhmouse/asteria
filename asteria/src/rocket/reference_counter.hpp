// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFERENCE_COUNTER_HPP_
#define ROCKET_REFERENCE_COUNTER_HPP_

#include <atomic>  // std::atomic<>
#include <exception>  // std::terminate()
#include "compatibility.h"
#include "assert.hpp"

namespace rocket {

template<typename valueT>
  class reference_counter
  {
  private:
    ::std::atomic<valueT> m_nref;

  public:
    constexpr reference_counter() noexcept
      : m_nref(1)
      {
      }
    constexpr reference_counter(const reference_counter &) noexcept
      : reference_counter()
      {
      }
    reference_counter & operator=(const reference_counter &) noexcept
      {
        return *this;
      }
    ~reference_counter()
      {
        this->do_terminate_if_shared();
      }

  private:
    void do_terminate_if_shared() const
      {
        const auto old = this->m_nref.load(::std::memory_order_relaxed);
        if(old <= 1) {
          return;
        }
        ::std::terminate();
      }

  public:
    ROCKET_PURE_FUNCTION bool unique() const noexcept
      {
        const auto old = this->m_nref.load(::std::memory_order_relaxed);
        return ROCKET_EXPECT(old == 1);
      }
    valueT get() const noexcept
      {
        const auto old = this->m_nref.load(::std::memory_order_relaxed);
        return old;
      }
    bool try_increment() noexcept
      {
        auto old = this->m_nref.load(::std::memory_order_relaxed);
        do {
          if(old == 0) {
            return false;
          }
          if(this->m_nref.compare_exchange_weak(old, old + 1, ::std::memory_order_relaxed)) {
            return true;
          }
        } while(true);
      }
    void increment() noexcept
      {
        const auto old = this->m_nref.fetch_add(1, ::std::memory_order_relaxed);
        ROCKET_ASSERT(old >= 1);
      }
    bool decrement() noexcept
      {
        const auto old = this->m_nref.fetch_sub(1, ::std::memory_order_acq_rel);
        ROCKET_ASSERT(old >= 1);
        return old == 1;
      }
  };

}

#endif
