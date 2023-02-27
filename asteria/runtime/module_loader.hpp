// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_MODULE_LOADER_
#define ASTERIA_RUNTIME_MODULE_LOADER_

#include "../fwd.hpp"
#include "../../rocket/tinybuf_file.hpp"
namespace asteria {

class Module_Loader final
  : public rcfwd<Module_Loader>
  {
  public:
    class Unique_Stream;  // RAII wrapper

  private:
    cow_dictionary<::rocket::tinybuf_file> m_strms;
    using locked_stream_pair = decltype(m_strms)::value_type;

  public:
    explicit
    Module_Loader() noexcept
      { }

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
      { this->reset(loader, path);  }

    explicit
    Unique_Stream(Unique_Stream&& other) noexcept
      { this->swap(other);  }

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

  private:
    Unique_Stream&
    do_reset(const refcnt_ptr<Module_Loader>& loader, locked_stream_pair* strm) noexcept
      {
        auto qloader = ::std::exchange(this->m_loader, loader);
        auto qstrm = ::std::exchange(this->m_strm, strm);
        if(!qstrm)
          return *this;

        // Unlock the old stream if one has been assigned.
        ROCKET_ASSERT(qloader);
        qloader->do_unlock_stream(qstrm);
        return *this;
      }

  public:
    ~Unique_Stream()
      { this->do_reset(nullptr, nullptr);  }

    explicit operator
    bool() const noexcept
      { return this->m_strm != nullptr;  }

    ::rocket::tinybuf_file&
    get() const noexcept
      {
        auto qstrm = this->m_strm;
        ROCKET_ASSERT_MSG(qstrm, "no stream locked");
        return qstrm->second;
      }

    Unique_Stream&
    reset() noexcept
      { return this->do_reset(nullptr, nullptr);  }

    Unique_Stream&
    reset(const refcnt_ptr<Module_Loader>& loader, const char* path)
      {
        // Lock the stream. If an exception is thrown, there is no effect.
        ROCKET_ASSERT(loader);
        ROCKET_ASSERT(path);
        auto qstrm = loader->do_lock_stream(path);
        return this->do_reset(loader, qstrm);
      }
  };

inline
void
swap(Module_Loader::Unique_Stream& lhs, Module_Loader::Unique_Stream& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace asteria
#endif
