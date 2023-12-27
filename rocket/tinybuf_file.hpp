// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_FILE_
#define ROCKET_TINYBUF_FILE_

#include "tinybuf.hpp"
#include "unique_posix_file.hpp"
#include "unique_posix_fd.hpp"
#include "xuchar.hpp"
#include <fcntl.h>  // ::open()
#include <stdio.h>  // ::fdopen(), ::fclose()
#include <errno.h>  // errno
namespace rocket {

template<typename charT>
class basic_tinybuf_file
  :
    public basic_tinybuf<charT>
  {
  public:
    using char_type     = charT;
    using seek_dir      = tinybuf_base::seek_dir;
    using open_mode     = tinybuf_base::open_mode;
    using tinybuf_type  = basic_tinybuf<charT>;

    using file_type     = unique_posix_file;
    using handle_type   = typename file_type::handle_type;
    using closer_type   = typename file_type::closer_type;

  private:
    file_type m_file;
    ::mbstate_t m_mbst_g = { };
    ::mbstate_t m_mbst_p = { };

  public:
    constexpr
    basic_tinybuf_file() noexcept
      { }

    explicit
    basic_tinybuf_file(file_type&& file) noexcept
      :
        m_file(move(file))
      { }

    explicit
    basic_tinybuf_file(handle_type fp, closer_type cl) noexcept
      :
        m_file(fp, cl)
      { }

    explicit
    basic_tinybuf_file(const char* path, open_mode mode)
      {
        this->open(path, mode);
      }

    // This move constructor is necessary because `basic_tinybuf` is not
    // move-constructible.
    basic_tinybuf_file(basic_tinybuf_file&& other)
      noexcept(is_nothrow_move_constructible<file_type>::value)
      :
        m_file(move(other.m_file)),
        m_mbst_g(noadl::exchange(other.m_mbst_g)),
        m_mbst_p(noadl::exchange(other.m_mbst_p))
      { }

    basic_tinybuf_file&
    operator=(basic_tinybuf_file&& other) &
      noexcept(is_nothrow_move_assignable<file_type>::value)
      {
        this->m_file = move(other.m_file);
        this->m_mbst_g = noadl::exchange(other.m_mbst_g);
        this->m_mbst_p = noadl::exchange(other.m_mbst_p);
        return *this;
      }

    basic_tinybuf_file&
    swap(basic_tinybuf_file& other)
      noexcept(is_nothrow_swappable<file_type>::value)
      {
        this->m_file.swap(other.m_file);
        ::std::swap(this->m_mbst_g, other. m_mbst_g);
        ::std::swap(this->m_mbst_p, other. m_mbst_p);
        return *this;
      }

  public:
    virtual
    ~basic_tinybuf_file() override;

    // Gets the file pointer.
    handle_type
    get_handle() const noexcept
      { return this->m_file.get();  }

    // Gets the file closer function.
    const closer_type&
    get_closer() const noexcept
      { return this->m_file.get_closer();  }

    closer_type&
    get_closer() noexcept
      { return this->m_file.get_closer();  }

    // Takes ownership of an existent file.
    basic_tinybuf_file&
    reset(file_type&& file) noexcept
      {
        this->m_file = move(file);
        this->m_mbst_g = { };
        this->m_mbst_p = { };
        return *this;
      }

    basic_tinybuf_file&
    reset(handle_type fp, closer_type cl) noexcept
      {
        this->m_file.reset(fp, cl);
        this->m_mbst_g = { };
        this->m_mbst_p = { };
        return *this;
      }

    // Opens a new file.
    basic_tinybuf_file&
    open(const char* path, open_mode mode)
      {
        int flags = 0;
        char mstr[8] = "";
        size_t ml = 0;

        switch(static_cast<uint32_t>(mode & tinybuf_base::open_read_write)) {
          case tinybuf_base::open_read:
            flags = O_RDONLY;
            mstr[ml++] = 'r';
            break;

          case tinybuf_base::open_write:
            flags = O_WRONLY;
            mstr[ml++] = 'w';
            break;

          case tinybuf_base::open_read_write:
            flags = O_RDWR;
            mstr[ml++] = 'r';
            mstr[ml++] = '+';
            break;

          default:
            noadl::sprintf_and_throw<invalid_argument>(
                "basic_tinybuf_file: no access specified (path `%s`, mode `%u`)",
                path, mode);
        }

        if(mode & tinybuf_base::open_binary) {
#ifdef _O_BINARY
          flags |= _O_BINARY;
#endif
          mstr[ml++] = 'b';
        }

        if(mode & tinybuf_base::open_exclusive)
          flags |= O_EXCL;

        if(mode & tinybuf_base::open_write) {
          // These flags make sense only when writing to a file. Should
          // we throw exceptions if they are specified with read-only
          // access? Maybe.
          if(mode & tinybuf_base::open_append) {
            flags |= O_APPEND;
            mstr[0] = 'a';  // might be "w" or "r+"
          }

          if(mode & tinybuf_base::open_create)
            flags |= O_CREAT;

          if(mode & tinybuf_base::open_truncate)
            flags |= O_TRUNC;
        }

        // Open the file.
        unique_posix_fd fd(::open(path, flags, 0666), ::close);
        if(!fd)
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinybuf_file: `open()` failed (path `%s`, flags `%u`, errno `%d`)",
              path, mode, errno);

        file_type file(::fdopen(fd, mstr), ::fclose);
        if(!file)
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinybuf_file: `open()` failed (path `%s`, modes `%s`, errno `%d`)",
              path, mstr, errno);

        // If and only if `fdopen()` succeeds, will it take the ownership
        // of the file descriptor.
        fd.release();

        // Set the file now.
        this->m_file = move(file);
        this->m_mbst_g = { };
        this->m_mbst_p = { };
        return *this;
      }

    // Closes the current file, if any.
    basic_tinybuf_file&
    close() noexcept
      {
        this->m_file.reset();
        this->m_mbst_g = { };
        this->m_mbst_p = { };
        return *this;
      }

    // Calls `fflush()` to flush this stream.
    // If no file has been opened, nothing happens.
    virtual
    basic_tinybuf_file&
    flush() override
      {
        if(!this->m_file)
          return *this;

        ::fflush(this->m_file);
        return *this;
      }

    // Gets the current stream pointer.
    // If no file has been opened, or the file is not seekable, `-1` is returned.
    virtual
    int64_t
    tell() const override
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
    basic_tinybuf_file&
    seek(int64_t off, seek_dir dir) override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinybuf_file: no file opened");

        ::off_t cur = ::fseeko(this->m_file, off, (int) dir);
        if(cur == -1)
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinybuf_file: `fseeko()` failed (fileno `%d`, errno `%d`)",
              ::fileno(this->m_file), errno);

        // FIXME: We need to report errors about corrupted shift states.
        this->m_mbst_g = { };
        this->m_mbst_p = { };
        return *this;
      }

    // Reads some characters from the stream. If the end of stream has been
    // reached, zero is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    size_t
    getn(char_type* s, size_t n) override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinybuf_file: no file opened");

        size_t r = noadl::xfgetn(this->m_file, this->m_mbst_g, s, n);
        return r;
      }

    // Reads a single character from the stream. If the end of stream has been
    // reached, `-1` is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    int
    getc() override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinybuf_file: no file opened");

        char_type c;
        int ch = noadl::xfgetc(this->m_file, this->m_mbst_g, c);
        return ch;
      }

    // Puts some characters into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf_file&
    putn(const char_type* s, size_t n) override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinybuf_file: no file opened");

        noadl::xfputn(this->m_file, this->m_mbst_p, s, n);
        return *this;
      }

    // Puts a single character into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf_file&
    putc(char_type c) override
      {
        if(!this->m_file)
          noadl::sprintf_and_throw<invalid_argument>(
              "basic_tinybuf_file: no file opened");

        noadl::xfputc(this->m_file, this->m_mbst_p, c);
        return *this;
      }
  };

template<typename charT>
basic_tinybuf_file<charT>::
~basic_tinybuf_file()
  {
  }

template<typename charT>
inline
void
swap(basic_tinybuf_file<charT>& lhs, basic_tinybuf_file<charT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

using tinybuf_file     = basic_tinybuf_file<char>;
using wtinybuf_file    = basic_tinybuf_file<wchar_t>;
using u16tinybuf_file  = basic_tinybuf_file<char16_t>;
using u32tinybuf_file  = basic_tinybuf_file<char32_t>;

extern template class basic_tinybuf_file<char>;
extern template class basic_tinybuf_file<wchar_t>;
extern template class basic_tinybuf_file<char16_t>;
extern template class basic_tinybuf_file<char32_t>;

}  // namespace rocket
#endif
