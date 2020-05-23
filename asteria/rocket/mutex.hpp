// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_MUTEX_HPP_
#define ROCKET_MUTEX_HPP_

#include "assert.hpp"
#include "utilities.hpp"
#include <pthread.h>

namespace rocket {

class mutex;
class condition_variable;

#include "details/mutex.ipp"

class mutex
  {
  public:
    class unique_lock;

  private:
    ::pthread_mutex_t m_mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

  public:
    constexpr
    mutex()
    noexcept
      { }

    mutex(const mutex&)
      = delete;

    mutex&
    operator=(const mutex&)
      = delete;

    ~mutex()
      { details_mutex::do_mutex_destroy(this->m_mutex);  }
  };

class mutex::unique_lock
  {
    friend condition_variable;

  private:
    details_mutex::stored_pointer m_sth;

  public:
    constexpr
    unique_lock()
    noexcept
      { }

    explicit
    unique_lock(mutex& parent)
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

    bool
    is_locking(mutex& m)
    const noexcept
      { return this->m_sth.get() == ::std::addressof(m.m_mutex);  }

    unique_lock&
    unlock()
    noexcept
      {
        this->m_sth.reset(nullptr);
        return *this;
      }

    unique_lock&
    assign(mutex& m)
    noexcept
      {
        // Return immediately if the same mutex is already held.
        // Otherwise deadlocks would occur.
        auto ptr = ::std::addressof(m.m_mutex);
        if(ROCKET_UNEXPECT(ptr == this->m_sth.get()))
          return *this;

        // There shall be no gap between the unlock and lock operations.
        details_mutex::do_mutex_lock(*ptr);
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
swap(mutex::unique_lock& lhs, mutex::unique_lock& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
