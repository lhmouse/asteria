// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_MUTEX_HPP_
#  error Please include <rocket/mutex.hpp> instead.
#endif

namespace details_mutex {

inline
int
do_mutex_trylock(::pthread_mutex_t& mutex)
noexcept
  {
    int r = ::pthread_mutex_trylock(&mutex);
    ROCKET_ASSERT_MSG(r != EINVAL,
        "Failed to lock mutex (possible data corruption)");
    return r;
  }

inline
void
do_mutex_lock(::pthread_mutex_t& mutex)
noexcept
  {
    int r = ::pthread_mutex_lock(&mutex);
    ROCKET_ASSERT_MSG(r == 0,
        "Failed to lock mutex (possible deadlock or data corruption)");
  }

inline
void
do_mutex_unlock(::pthread_mutex_t& mutex)
noexcept
  {
    int r = ::pthread_mutex_unlock(&mutex);
    ROCKET_ASSERT_MSG(r == 0,
        "Failed to unlock mutex (possible data corruption)");
  }

inline
void
do_mutex_destroy(::pthread_mutex_t& mutex)
noexcept
  {
    int r = ::pthread_mutex_destroy(&mutex);
    ROCKET_ASSERT_MSG(r == 0,
        "Failed to destroy mutex (possible in use)");
  }

class stored_pointer
  {
  private:
    ::pthread_mutex_t* m_ptr = nullptr;

  public:
    constexpr
    stored_pointer()
    noexcept
      { };

    stored_pointer(const stored_pointer&)
      = delete;

    stored_pointer&
    operator=(const stored_pointer&)
      = delete;

    ~stored_pointer()
      { this->reset(nullptr);  }

  public:
    ::pthread_mutex_t*
    get()
    const noexcept
      { return this->m_ptr;  }

    ::pthread_mutex_t*
    release()
    noexcept
      { return ::std::exchange(this->m_ptr, nullptr);  }

    void
    reset(::pthread_mutex_t* ptr_new)
    noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, ::std::move(ptr_new));
        if(ptr)
          do_mutex_unlock(*ptr);
      }

    void
    exchange_with(stored_pointer& other)
    noexcept
      { noadl::xswap(this->m_ptr, other.m_ptr);  }
  };

}  // namespace details_mutex
