
// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_RECURSIVE_MUTEX_HPP_
#define ROCKET_RECURSIVE_MUTEX_HPP_

#include "assert.hpp"
#include "utilities.hpp"
#include <pthread.h>

namespace rocket {

class recursive_mutex;

#include "details/recursive_mutex.ipp"

class recursive_mutex
  {
  public:
    class unique_lock;

  private:
    ::pthread_mutex_t m_rmutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

  public:
    constexpr
    recursive_mutex()
    noexcept
      { }

    recursive_mutex(const recursive_mutex&)
      = delete;

    recursive_mutex&
    operator=(const recursive_mutex&)
      = delete;

    ~recursive_mutex()
      { details_recursive_mutex::do_rmutex_destroy(this->m_rmutex);  }
  };

class recursive_mutex::unique_lock
  {
  private:
    details_recursive_mutex::stored_pointer m_sth;

  public:
    constexpr
    unique_lock()
    noexcept
      { }

    explicit
    unique_lock(recursive_mutex& parent)
    noexcept
      { this->assign(parent);  }

    unique_lock(unique_lock&& other)
    noexcept
      { this->swap(other);  }

    unique_lock&
    operator=(unique_lock&& other)
    noexcept
      { return this->swap(other);  }

    ~unique_lock()
      { this->unlock();  }

  public:
    explicit operator
    bool()
    const noexcept
      { return this->m_sth.get() != nullptr;  }

    unique_lock&
    unlock()
    noexcept
      {
        this->m_sth.reset(nullptr);
        return *this;
      }

    unique_lock&
    assign(recursive_mutex& m)
    noexcept
      {
        // Return immediately if the same mutex is already held.
        // This is a bit faster than locking and relocking it.
        auto ptr = ::std::addressof(m.m_rmutex);
        if(ROCKET_UNEXPECT(ptr == this->m_sth.get()))
          return *this;

        // There shall be no gap between the unlock and lock operations.
        details_recursive_mutex::do_rmutex_lock(*ptr);
        this->m_sth.reset(ptr);
        return *this;
      }

    unique_lock&
    swap(unique_lock& other)
    noexcept
      {
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }
  };

inline
void
swap(recursive_mutex::unique_lock& lhs, recursive_mutex::unique_lock& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
