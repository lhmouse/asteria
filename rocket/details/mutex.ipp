// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_MUTEX_
#  error Please include <rocket/mutex.hpp> instead.
#endif

namespace details_mutex {

class stored_pointer
  {
  private:
    ::pthread_mutex_t* m_ptr = nullptr;

  public:
    constexpr
    stored_pointer() noexcept
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
    get() const noexcept
      { return this->m_ptr;  }

    ::pthread_mutex_t*
    release() noexcept
      { return ::std::exchange(this->m_ptr, nullptr);  }

    void
    reset(::pthread_mutex_t* ptr_new) noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, ::std::move(ptr_new));
        if(ptr)
          ::pthread_mutex_unlock(ptr);
      }

    void
    exchange_with(stored_pointer& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

}  // namespace details_mutex
