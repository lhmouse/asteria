// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_FILE_HPP_
#define ROCKET_TINYBUF_FILE_HPP_

#include "tinybuf.hpp"
#include "unique_posix_file.hpp"
#include "unique_posix_fd.hpp"
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::close()
#include <stdio.h>  // ::fdopen(), ::fclose()
#include <errno.h>  // errno

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>>
class basic_tinybuf_file;

template<typename charT, typename traitsT>
class basic_tinybuf_file
  : public basic_tinybuf<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;

    using tinybuf_type  = basic_tinybuf<charT, traitsT>;
    using handle_type   = typename posix_file_closer::handle_type;
    using closer_type   = typename posix_file_closer::closer_type;

    using seek_dir   = typename tinybuf_type::seek_dir;
    using open_mode  = typename tinybuf_type::open_mode;
    using int_type   = typename tinybuf_type::int_type;
    using off_type   = typename tinybuf_type::off_type;
    using size_type  = typename tinybuf_type::size_type;

  private:
    unique_posix_file m_file;

  public:
    basic_tinybuf_file()
      noexcept(is_nothrow_constructible<unique_posix_file, nullptr_t, nullptr_t>::value)
      : m_file(nullptr, nullptr)
      { }

    explicit
    basic_tinybuf_file(unique_posix_file&& file) noexcept
      : m_file(::std::move(file))
      { }

    explicit
    basic_tinybuf_file(handle_type fp, closer_type cl) noexcept
      : m_file(fp, cl)
      { }

    explicit
    basic_tinybuf_file(const char* path, open_mode mode)
      : m_file(nullptr, nullptr)
      { this->open(path, mode);  }

    basic_tinybuf_file&
    swap(basic_tinybuf_file& other) noexcept(is_nothrow_swappable<unique_posix_file>::value)
      { noadl::xswap(this->m_file, other.m_file);
        return *this;  }

  protected:
    void
    do_flush() override
      {
        ::FILE* fp = this->m_file.get();
        if(!fp)
          return;

        ::fflush(fp);
      }

    off_type
    do_tell() const override
      {
        ::FILE* fp = this->m_file.get();
        if(!fp)
          noadl::sprintf_and_throw<out_of_range>("tinybuf_file: no file opened");

        ::off_t off = ::ftello(fp);
        if(off == -1)
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: could not get file position (errno `%d`, fileno `%d`)",
                errno, ::fileno(fp));

        return off;
      }

    void
    do_seek(off_type off, seek_dir dir) override
      {
        ::FILE* fp = this->m_file.get();
        if(!fp)
          noadl::sprintf_and_throw<out_of_range>("tinybuf_file: no file opened");

        if(::fseeko(fp, off, dir) == -1)
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: could not set file position (errno `%d`, fileno `%d`)",
                errno, ::fileno(fp));
      }

    size_type
    do_getn(char_type* s, size_type n) override
      {
        ::FILE* fp = this->m_file.get();
        if(!fp)
          noadl::sprintf_and_throw<out_of_range>("tinybuf_file: no file opened");

        ::size_t nread = traits_type::fgetn(fp, s, n);
        if((nread == 0) && ::ferror(fp))
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: read error (errno `%d`, fileno `%d`)",
                errno, ::fileno(this->m_file));

        return nread;
      }

    int_type
    do_getc() override
      {
        ::FILE* fp = this->m_file.get();
        if(!fp)
          noadl::sprintf_and_throw<out_of_range>("tinybuf_file: no file opened");

        int_type r = traits_type::fgetc(fp);
        if(traits_type::is_eof(r) && ::ferror(fp))
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: read error (errno `%d`, fileno `%d`)",
                errno, ::fileno(this->m_file));

        return r;
      }

    void
    do_putn(const char_type* s, size_type n) override
      {
        ::FILE* fp = this->m_file.get();
        if(!fp)
          noadl::sprintf_and_throw<out_of_range>("tinybuf_file: no file opened");

        ::size_t nwritten = traits_type::fputn(fp, s, n);
        if(nwritten != n)
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: write error (errno `%d`, fileno `%d`)",
                errno, ::fileno(this->m_file));
      }

    void
    do_putc(char_type c) override
      {
        ::FILE* fp = this->m_file.get();
        if(!fp)
          noadl::sprintf_and_throw<out_of_range>("tinybuf_file: no file opened");

        int_type r = traits_type::fputc(fp, c);
        if(traits_type::is_eof(r))
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: write error (errno `%d`, fileno `%d`)",
                errno, ::fileno(this->m_file));
      }

  public:
    ~basic_tinybuf_file() override;

    handle_type
    get_handle() const noexcept
      { return this->m_file.get();  }

    const closer_type&
    get_closer() const noexcept
      { return this->m_file.get_closer();  }

    closer_type&
    get_closer() noexcept
      { return this->m_file.get_closer();  }

    basic_tinybuf_file&
    reset() noexcept
      { this->m_file.reset();  return *this;  }

    basic_tinybuf_file&
    reset(unique_posix_file&& file) noexcept
      { this->m_file = ::std::move(file);  return *this;  }

    basic_tinybuf_file&
    reset(handle_type fp, closer_type cl) noexcept
      { this->m_file.reset(fp, cl);  return *this;  }

    basic_tinybuf_file&
    open(const char* path, open_mode mode)
      {
        int flags = 0;
        char mstr[8] = { };
        size_t mlen = 0;

        auto rwm = mode & tinybuf_base::open_read_write;
        if(rwm == tinybuf_base::open_read) {
          flags = O_RDONLY;
          mstr[mlen++] = 'r';
        }
        else if(rwm == tinybuf_base::open_write) {
          flags = O_WRONLY;
          mstr[mlen++] = 'w';
        }
        else if(rwm == tinybuf_base::open_read_write) {
          flags = O_RDWR;
          mstr[mlen++] = 'r';
          mstr[mlen++] = '+';
        }
        else
          noadl::sprintf_and_throw<invalid_argument>(
                "tinybuf_file: no desired access specified (path `%s`, mode `%u`)",
                path, mode);

        if(mode & tinybuf_base::open_append) {
          flags |= O_APPEND;
          mstr[0] = 'a';  // might be "w" or "r+"
        }

        if(mode & tinybuf_base::open_binary)
          mstr[mlen++] = 'b';

        if(mode & tinybuf_base::open_create)
          flags |= O_CREAT;

        if(mode & tinybuf_base::open_truncate)
          flags |= O_TRUNC;

        if(mode & tinybuf_base::open_exclusive)
          flags |= O_EXCL;

        // Open the file.
        unique_posix_fd fd(::open(path, flags, 0666), ::close);
        if(!fd)
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: `open()` failed (errno `%d`, path `%s`, mode `%u`)",
                errno, path, mode);

        // Convert it to a `FILE*`.
        unique_posix_file file(::fdopen(fd, mstr), ::fclose);
        if(!file)
          noadl::sprintf_and_throw<runtime_error>(
                "tinybuf_file: `fdopen()` failed (errno `%d`, path `%s`, mode `%u`)",
                errno, path, mode);

        // If `fdopen()` succeeds it will have taken the ownership of `fd`.
        fd.release();
        this->m_file = ::std::move(file);
        return *this;
      }

    basic_tinybuf_file&
    close() noexcept
      { return this->reset();  }
  };

template<typename charT, typename traitsT>
basic_tinybuf_file<charT, traitsT>::
~basic_tinybuf_file()
  { }

template<typename charT, typename traitsT>
inline void
swap(basic_tinybuf_file<charT, traitsT>& lhs, basic_tinybuf_file<charT, traitsT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

extern template
class basic_tinybuf_file<char>;

extern template
class basic_tinybuf_file<wchar_t>;

using tinybuf_file   = basic_tinybuf_file<char>;
using wtinybuf_file  = basic_tinybuf_file<wchar_t>;

}  // namespace rocket

#endif
