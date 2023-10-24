// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_MUTEX_
#define ROCKET_MUTEX_

#include "fwd.hpp"
#include "xassert.hpp"
#include <mutex>
namespace rocket {

class mutex
  {
    friend class condition_variable;

  private:
    ::std::mutex m_std_mtx;

  public:
    mutex() = default;

    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

  public:
    // This is the only public interface, other than constructors
    // and the destructor.
    class unique_lock
      {
        friend class condition_variable;

      private:
        ::pthread_mutex_t* m_mtx = nullptr;

      public:
        constexpr
        unique_lock() noexcept = default;

        unique_lock(mutex& m) noexcept
          {
            ::pthread_mutex_lock(m.m_std_mtx.native_handle());
            this->m_mtx = m.m_std_mtx.native_handle();
          }

        unique_lock(unique_lock&& other) noexcept
          {
            this->m_mtx = other.m_mtx;
            other.m_mtx = nullptr;
          }

        unique_lock&
        operator=(unique_lock&& other) & noexcept
          {
            if(this->m_mtx == other.m_mtx)
              return *this;

            this->unlock();
            this->m_mtx = other.m_mtx;
            other.m_mtx = nullptr;
            return *this;
          }

        unique_lock&
        swap(unique_lock& other) noexcept
          {
            ::std::swap(this->m_mtx, other.m_mtx);
            return *this;
          }

        ~unique_lock()
          {
            this->unlock();
          }

      public:
        explicit constexpr operator
        bool() const noexcept
          { return this->m_mtx != nullptr;  }

        void
        lock(mutex& m) noexcept
          {
            if(this->m_mtx == m.m_std_mtx.native_handle())
              return;

            ::pthread_mutex_lock(m.m_std_mtx.native_handle());  // note lock first
            this->unlock();
            this->m_mtx = m.m_std_mtx.native_handle();
          }

        void
        unlock() noexcept
          {
            if(this->m_mtx == nullptr)
              return;

            ::pthread_mutex_unlock(this->m_mtx);
            this->m_mtx = nullptr;
          }
      };
  };

inline
void
swap(mutex::unique_lock& lhs, mutex::unique_lock& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace rocket
#endif
