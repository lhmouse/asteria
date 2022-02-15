// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_RECURSIVE_MUTEX_HPP_
#define ROCKET_RECURSIVE_MUTEX_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include <pthread.h>

namespace rocket {

class recursive_mutex;

#include "details/recursive_mutex.ipp"

class recursive_mutex
  {
  public:
    class unique_lock;

  private:
    ::pthread_mutex_t m_mutex[1] = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };

  public:
    constexpr
    recursive_mutex() noexcept
      { }

    recursive_mutex(const recursive_mutex&)
      = delete;

    recursive_mutex&
    operator=(const recursive_mutex&)
      = delete;

    ~recursive_mutex()
      {
        int r = ::pthread_mutex_destroy(this->m_mutex);
        ROCKET_ASSERT(r == 0);
      }
  };

class recursive_mutex::unique_lock
  {
  private:
    details_recursive_mutex::stored_pointer m_sth;

  public:
    constexpr
    unique_lock() noexcept
      { }

    explicit
    unique_lock(recursive_mutex& parent) noexcept
      { this->lock(parent);  }

    unique_lock(unique_lock&& other) noexcept
      { this->swap(other);  }

    unique_lock&
    operator=(unique_lock&& other) noexcept
      { return this->swap(other);  }

    unique_lock&
    swap(unique_lock& other) noexcept
      { this->m_sth.exchange_with(other.m_sth);
        return *this;  }

    ~unique_lock()
      { this->unlock();  }

  public:
    explicit operator
    bool() const noexcept
      { return this->m_sth.get() != nullptr;  }

    bool
    is_locking(const recursive_mutex& m) const noexcept
      { return this->m_sth.get() == m.m_mutex;  }

    bool
    is_locking(const recursive_mutex&&) const noexcept
      = delete;

    unique_lock&
    unlock() noexcept
      {
        this->m_sth.reset(nullptr);
        return *this;
      }

    unique_lock&
    try_lock(recursive_mutex& m) noexcept
      {
        // Return immediately if the same mutex is already held.
        // This is a bit faster than locking and relocking it.
        auto ptr = m.m_mutex;
        if(ROCKET_UNEXPECT(ptr == this->m_sth.get()))
          return *this;

        // There shall be no gap between the unlock and lock operations.
        // If the mutex cannot be locked, there is no effect.
        int r = ::pthread_mutex_trylock(ptr);
        ROCKET_ASSERT(r != EINVAL);
        if(r != 0)
          return *this;

        this->m_sth.reset(ptr);
        return *this;
      }

    unique_lock&
    lock(recursive_mutex& m) noexcept
      {
        // Return immediately if the same mutex is already held.
        // This is a bit faster than locking and relocking it.
        auto ptr = m.m_mutex;
        if(ROCKET_UNEXPECT(ptr == this->m_sth.get()))
          return *this;

        // There shall be no gap between the unlock and lock operations.
        int r = ::pthread_mutex_lock(ptr);
        ROCKET_ASSERT(r == 0);

        this->m_sth.reset(ptr);
        return *this;
      }
  };

inline void
swap(recursive_mutex::unique_lock& lhs, recursive_mutex::unique_lock& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
