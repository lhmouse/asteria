// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ATOMIC_FLAG_HPP_
#define ROCKET_ATOMIC_FLAG_HPP_

#include "compiler.h"
#include <atomic>  // std::atomic<>

namespace rocket {

class atomic_flag
  {
  private:
    ::std::atomic<bool> m_val;

  public:
    constexpr atomic_flag() noexcept
      :
        m_val(false)
      {
      }
    explicit constexpr atomic_flag(bool val) noexcept
      :
        m_val(val)
      {
      }

  public:
    bool test_relaxed() const noexcept
      {
        return this->m_val.load(::std::memory_order_relaxed);
      }
    void set_relaxed(bool val = true) noexcept
      {
        return this->m_val.store(val, ::std::memory_order_relaxed);
      }
    void clear_relaxed() noexcept
      {
        return this->set_relaxed(false);
      }
    bool test_and_set_relaxed(bool val = true) noexcept
      {
        bool old = this->m_val.load(::std::memory_order_relaxed);
        for(;;) {
          if(old == val)
            break;
          if(this->m_val.compare_exchange_weak(old, val, ::std::memory_order_relaxed))
            break;
        }
        return old;
      }
    bool test_and_clear_relaxed() noexcept
      {
        return this->test_and_set_relaxed(false);
      }

    bool test_acquire() const noexcept
      {
        return this->m_val.load(::std::memory_order_acquire);
      }
    void set_release(bool val = true) noexcept
      {
        return this->m_val.store(val, ::std::memory_order_release);
      }
    void clear_release() noexcept
      {
        return this->set_release(false);
      }
    bool test_and_set_acq_rel(bool val = true) noexcept
      {
        return this->m_val.exchange(val, ::std::memory_order_acq_rel);
      }
    bool test_and_clear_acq_rel() noexcept
      {
        return this->test_and_set_acq_rel(false);
      }
  };

}  // namespace rocket

#endif
