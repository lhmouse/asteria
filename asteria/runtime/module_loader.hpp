// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_MODULE_LOADER_
#define ASTERIA_RUNTIME_MODULE_LOADER_

#include "../fwd.hpp"
#include "../../rocket/tinybuf_file.hpp"
namespace asteria {

class Module_Loader
  :
    public rcfwd<Module_Loader>
  {
  public:
    class Unique_Stream;  // RAII wrapper

  private:
    cow_dictionary<::rocket::tinybuf_file> m_strms;
    using locked_stream_pair = decltype(m_strms)::value_type;

  public:
    explicit
    Module_Loader() noexcept = default;

  private:
    locked_stream_pair*
    do_lock_stream(const char* path);

    void
    do_unlock_stream(locked_stream_pair* qstrm) noexcept;

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Module_Loader);
  };

class Module_Loader::Unique_Stream
  {
  private:
    refcnt_ptr<Module_Loader> m_loader;
    locked_stream_pair* m_strm = nullptr;

  public:
    explicit constexpr
    Unique_Stream() noexcept = default;

    explicit
    Unique_Stream(const refcnt_ptr<Module_Loader>& loader, const char* path)
      {
        this->reset(loader, path);
      }

    explicit
    Unique_Stream(Unique_Stream&& other) noexcept
      {
        this->swap(other);
      }

    Unique_Stream&
    operator=(Unique_Stream&& other) & noexcept
      {
        this->swap(other);
        return *this;
      }

    Unique_Stream&
    swap(Unique_Stream& other) noexcept
      {
        this->m_loader.swap(other.m_loader);
        ::std::swap(this->m_strm, other.m_strm);
        return *this;
      }

  public:
    ~Unique_Stream()
      {
        this->reset();
      }

    explicit operator
    bool() const noexcept
      { return this->m_strm != nullptr;  }

    ::rocket::tinybuf_file&
    get() const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_strm, "no stream");
        return this->m_strm->second;
      }

    Unique_Stream&
    reset() noexcept
      {
        if(this->m_strm == nullptr)
          return *this;

        auto old_loader = ::rocket::exchange(this->m_loader);
        auto old_strm = ::rocket::exchange(this->m_strm);
        old_loader->do_unlock_stream(old_strm);
        return *this;
      }

    Unique_Stream&
    reset(const refcnt_ptr<Module_Loader>& loader, const char* path)
      {
        // If an exception is thrown, there shall be no effect.
        locked_stream_pair* strm = nullptr;
        if(loader)
          strm = loader->do_lock_stream(path);

        if(this->m_strm == nullptr) {
          this->m_loader = loader;
          this->m_strm = strm;
          return *this;
        }

        auto old_loader = ::rocket::exchange(this->m_loader, loader);
        auto old_strm = ::rocket::exchange(this->m_strm, strm);
        old_loader->do_unlock_stream(old_strm);
        return *this;
      }
  };

inline
void
swap(Module_Loader::Unique_Stream& lhs, Module_Loader::Unique_Stream& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
