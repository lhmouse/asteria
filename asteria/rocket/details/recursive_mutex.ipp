// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_RECURSIVE_MUTEX_HPP_
#  error Please include <rocket/recursive_mutex.hpp> instead.
#endif

namespace details_recursive_mutex {

inline
bool
do_rmutex_trylock(::pthread_mutex_t& rmutex)
noexcept
  {
    int r = ::pthread_mutex_trylock(&rmutex);
    return r == 0;
  }

inline
void
do_rmutex_lock(::pthread_mutex_t& rmutex)
noexcept
  {
    int r = ::pthread_mutex_lock(&rmutex);
    ROCKET_ASSERT(r == 0);
  }

inline
void
do_rmutex_unlock(::pthread_mutex_t& rmutex)
noexcept
  {
    int r = ::pthread_mutex_unlock(&rmutex);
    ROCKET_ASSERT(r == 0);
  }

inline
void
do_rmutex_destroy(::pthread_mutex_t& rmutex)
noexcept
  {
    int r = ::pthread_mutex_destroy(&rmutex);
    ROCKET_ASSERT(r == 0);
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
          do_rmutex_unlock(*ptr);
      }

    void
    exchange_with(stored_pointer& other)
    noexcept
      { noadl::xswap(this->m_ptr, other.m_ptr);  }
  };

}  // namespace details_recursive_mutex
