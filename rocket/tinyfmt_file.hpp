// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_FILE_
#define ROCKET_TINYFMT_FILE_

#include "tinyfmt.hpp"
#include "unique_posix_fd.hpp"
#include "unique_posix_file.hpp"
#include "xuchar.hpp"
#include "xthrow.hpp"
#include <fcntl.h>  // ::open()
#include <stdio.h>  // ::fdopen(), ::fclose()
#include <errno.h>  // errno
namespace rocket {

template<typename charT>
class basic_tinyfmt_file
  :
    public basic_tinyfmt<charT>
  {
  public:
    using char_type   = charT;
    using seek_dir    = tinyfmt_base::seek_dir;
    using open_mode   = tinyfmt_base::open_mode;

    using tinyfmt_type  = basic_tinyfmt<charT>;
    using file_type     = unique_posix_file;
    using handle_type   = ::FILE*;
    using closer_type   = posix_file_closer;

  private:
    file_type m_file;
    ::mbstate_t m_mbst_g = ::mbstate_t();
    ::mbstate_t m_mbst_p = ::mbstate_t();

  public:
    basic_tinyfmt_file()
      noexcept = default;

    explicit
    basic_tinyfmt_file(file_type&& file)
      noexcept
      :
        m_file(move(file))
      { }

    explicit
    basic_tinyfmt_file(handle_type fp)
      noexcept
      :
        m_file(fp)
      { }

    basic_tinyfmt_file(const char* path, open_mode mode)
      {
        this->open(path, mode);
      }

    // This move constructor is necessary because `basic_tinyfmt` is not
    // move-constructible.
    basic_tinyfmt_file(basic_tinyfmt_file&& other)
      noexcept(is_nothrow_move_constructible<file_type>::value)
      :
        m_file(move(other.m_file)),
        m_mbst_g(noadl::exchange(other.m_mbst_g)),
        m_mbst_p(noadl::exchange(other.m_mbst_p))
      { }

    basic_tinyfmt_file&
    operator=(basic_tinyfmt_file&& other)
      & noexcept(is_nothrow_move_assignable<file_type>::value)
      {
        this->m_file = move(other.m_file);
        this->m_mbst_g = noadl::exchange(other.m_mbst_g);
        this->m_mbst_p = noadl::exchange(other.m_mbst_p);
        return *this;
      }

    basic_tinyfmt_file&
    swap(basic_tinyfmt_file& other)
      noexcept(is_nothrow_swappable<file_type>::value)
      {
        this->m_file.swap(other.m_file);
        ::std::swap(this->m_mbst_g, other. m_mbst_g);
        ::std::swap(this->m_mbst_p, other. m_mbst_p);
        return *this;
      }

  public:
    virtual ~basic_tinyfmt_file() override;

    // Gets the file pointer.
    handle_type
    get_handle()
      const noexcept
      { return this->m_file.get();  }

    // Takes ownership of an existent file.
    basic_tinyfmt_file&
    reset(file_type&& file)
      noexcept
      {
        this->m_file = move(file);
        this->m_mbst_g = ::mbstate_t();
        this->m_mbst_p = ::mbstate_t();
        return *this;
      }

    basic_tinyfmt_file&
    reset(handle_type fp)
      noexcept
      {
        this->m_file.reset(fp);
        this->m_mbst_g = ::mbstate_t();
        this->m_mbst_p = ::mbstate_t();
        return *this;
      }

    // Opens a new file.
    basic_tinyfmt_file&
    open(const char* path, open_mode mode)
      {
        int flags = 0;
        char mstr[8] = "";
        size_t ml = 0;

        switch(static_cast<uint32_t>(mode & tinyfmt_base::open_read_write))
          {
          case tinyfmt_base::open_read:
            flags = O_RDONLY;
            mstr[ml++] = 'r';
            break;

          case tinyfmt_base::open_write:
            flags = O_WRONLY;
            mstr[ml++] = 'w';
            break;

          case tinyfmt_base::open_read_write:
            flags = O_RDWR;
            mstr[ml++] = 'r';
            mstr[ml++] = '+';
            break;

          default:
            noadl::sprintf_and_throw<invalid_argument>(
                "basic_tinyfmt_file: no access specified (path `%s`, mode `%u`)",
                path, mode);
          }

        if(mode & tinyfmt_base::open_binary) {
#ifdef _O_BINARY
          flags |= _O_BINARY;
#endif
          mstr[ml++] = 'b';
        }

        if(mode & tinyfmt_base::open_exclusive)
          flags |= O_EXCL;

        if(mode & tinyfmt_base::open_write) {
          // These flags make sense only when writing to a file. Should
          // we throw exceptions if they are specified with read-only
          // access? Maybe.
          if(mode & tinyfmt_base::open_append) {
            flags |= O_APPEND;
            mstr[0] = 'a';  // might be "w" or "r+"
          }

          if(mode & tinyfmt_base::open_create)
            flags |= O_CREAT;

          if(mode & tinyfmt_base::open_truncate)
            flags |= O_TRUNC;
        }

        // Open the file.
        unique_posix_fd fd(::open(path, flags, 0666));
        if(!fd)
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinyfmt_file: `open()` failed (path `%s`, flags `%u`, errno `%d`)",
              path, mode, errno);

        file_type file(::fdopen(fd, mstr));
        if(!file)
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinyfmt_file: `open()` failed (path `%s`, modes `%s`, errno `%d`)",
              path, mstr, errno);

        // If and only if `fdopen()` succeeds, will it take the ownership
        // of the file descriptor.
        fd.release();

        // Set the file now.
        this->m_file = move(file);
        this->m_mbst_g = ::mbstate_t();
        this->m_mbst_p = ::mbstate_t();
        return *this;
      }

    // Closes the current file, if any.
    basic_tinyfmt_file&
    close()
      noexcept
      {
        this->m_file.reset();
        this->m_mbst_g = ::mbstate_t();
        this->m_mbst_p = ::mbstate_t();
        return *this;
      }

    // Calls `fflush()` to flush this stream.
    // If no file has been opened, nothing happens.
    virtual
    basic_tinyfmt_file&
    flush()
      override
      {
        if(!this->m_file)
          return *this;

        int r = ::fflush(this->m_file);
        if(r == EOF)
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinyfmt_file: `fflush()` failed (fileno `%d`, errno `%d`)",
              ::fileno(this->m_file), errno);

        return *this;
      }

    // Gets the current stream pointer.
    // If no file has been opened, or the file is not seekable, `-1` is returned.
    virtual
    int64_t
    tell()
      const override
      {
        if(!this->m_file)
          return -1;

        ::off_t cur = ::ftello(this->m_file);
        return cur;
      }

    // Adjusts the current stream pointer.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinyfmt_file&
    seek(int64_t off, seek_dir dir)
      override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinyfmt_file: no file opened");

        ::off_t cur = ::fseeko(this->m_file, off, (int) dir);
        if(cur == -1)
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinyfmt_file: `fseeko()` failed (fileno `%d`, errno `%d`)",
              ::fileno(this->m_file), errno);

        // FIXME: We need to report errors about corrupted shift states.
        this->m_mbst_g = ::mbstate_t();
        this->m_mbst_p = ::mbstate_t();
        return *this;
      }

    // Reads some characters from the stream. If the end of stream has been
    // reached, zero is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    size_t
    getn(char_type* s, size_t n)
      override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinyfmt_file: no file opened");

        size_t r = noadl::xfgetn(this->m_file, this->m_mbst_g, s, n);
        return r;
      }

    // Reads a single character from the stream. If the end of stream has been
    // reached, `-1` is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    int
    getc()
      override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinyfmt_file: no file opened");

        char_type c;
        int ch = noadl::xfgetc(this->m_file, this->m_mbst_g, c);
        return ch;
      }

    // Puts some characters into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinyfmt_file&
    putn(const char_type* s, size_t n)
      override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinyfmt_file: no file opened");

        noadl::xfputn(this->m_file, this->m_mbst_p, s, n);
        return *this;
      }

    // Puts a single character into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinyfmt_file&
    putc(char_type c)
      override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinyfmt_file: no file opened");

        noadl::xfputc(this->m_file, this->m_mbst_p, c);
        return *this;
      }
  };

template<typename charT>
basic_tinyfmt_file<charT>::
~basic_tinyfmt_file()
  {
  }

template<typename charT>
inline
void
swap(basic_tinyfmt_file<charT>& lhs, basic_tinyfmt_file<charT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

using tinyfmt_file     = basic_tinyfmt_file<char>;
using wtinyfmt_file    = basic_tinyfmt_file<wchar_t>;
using u16tinyfmt_file  = basic_tinyfmt_file<char16_t>;
using u32tinyfmt_file  = basic_tinyfmt_file<char32_t>;

extern template class basic_tinyfmt_file<char>;
extern template class basic_tinyfmt_file<wchar_t>;
extern template class basic_tinyfmt_file<char16_t>;
extern template class basic_tinyfmt_file<char32_t>;

}  // namespace rocket
#endif
