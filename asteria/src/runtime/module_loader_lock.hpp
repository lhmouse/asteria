// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_MODULE_LOADER_LOCK_HPP_
#define ASTERIA_RUNTIME_MODULE_LOADER_LOCK_HPP_

#include "../fwd.hpp"
#include "../../rocket/tinybuf_file.hpp"

namespace Asteria {

class Module_Loader_Lock final : public Rcfwd<Module_Loader_Lock>
  {
  public:
    class Unique_Stream;  // RAII wrapper

  private:
    cow_dictionary<::rocket::tinybuf_file> m_strms;
    using element_type = decltype(m_strms)::value_type;

  public:
    Module_Loader_Lock() noexcept
      = default;

    ~Module_Loader_Lock() override;

    Module_Loader_Lock(const Module_Loader_Lock&)
      = delete;

    Module_Loader_Lock& operator=(const Module_Loader_Lock&)
      = delete;

  private:
    element_type* do_lock_stream(const char* path);
    void do_unlock_stream(element_type* qelem) noexcept;
  };

class Module_Loader_Lock::Unique_Stream
  {
  private:
    rcptr<Module_Loader_Lock> m_lock;
    element_type* m_elem = nullptr;

  public:
    constexpr Unique_Stream() noexcept
      = default;

    Unique_Stream(const rcptr<Module_Loader_Lock>& lock, const char* path)
      { this->reset(lock, path);  }

    Unique_Stream(Unique_Stream&& other) noexcept
      { this->swap(other);  }

    Unique_Stream& operator=(Unique_Stream&& other) noexcept
      { return this->swap(other);  }

    ~Unique_Stream()
      { this->reset();  }

  public:
    explicit operator bool () const noexcept
      { return bool(this->m_elem);  }

    ::rocket::tinybuf_file& get() const noexcept
      { return ROCKET_ASSERT(this->m_elem), this->m_elem->second;  }

    operator ::rocket::tinybuf_file& () const noexcept
      { return ROCKET_ASSERT(this->m_elem), this->m_elem->second;  }

    Unique_Stream& reset() noexcept
      {
        // Update contents of *this.
        auto qlock = ::std::exchange(this->m_lock, nullptr);
        auto qelem = ::std::exchange(this->m_elem, nullptr);

        // Unlock the old stream if one has been assigned.
        if(qelem)
          qlock->do_unlock_stream(qelem);
        return *this;
      }

    Unique_Stream& reset(const rcptr<Module_Loader_Lock>& lock, const char* path)
      {
        // Lock the stream. If an exception is thrown, there is no effect.
        ROCKET_ASSERT(lock);
        ROCKET_ASSERT(path);
        auto elem = lock->do_lock_stream(path);

        // Update contents of *this.
        auto qlock = ::std::exchange(this->m_lock, lock);
        auto qelem = ::std::exchange(this->m_elem, elem);

        // Unlock the old stream if one has been assigned.
        if(qelem)
          qlock->do_unlock_stream(qelem);
        return *this;
      }

    Unique_Stream& swap(Unique_Stream& other) noexcept
      {
        xswap(this->m_lock, other.m_lock);
        xswap(this->m_elem, other.m_elem);
        return *this;
      }
  };

inline void swap(Module_Loader_Lock::Unique_Stream& lhs, Module_Loader_Lock::Unique_Stream& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace Asteria

#endif
