// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#define ROCKET_TINYBUF_FILE_NO_EXTERN_TEMPLATE_ 1
#include "tinybuf_file.hpp"
#include <limits.h>  // MB_LEN_MAX
#include <wchar.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifdef HAVE_UCHAR_H
#  include <uchar.h>
#endif
namespace rocket {
namespace {

class locked_FILE
  {
  private:
    ::FILE* m_file;

  public:
    explicit
    locked_FILE(::FILE* file) noexcept
      : m_file(file)
      {
        ::flockfile(this->m_file);
      }

    ~locked_FILE()
      {
        ::funlockfile(this->m_file);
      }

    locked_FILE(const locked_FILE&) = delete;
    locked_FILE& operator=(const locked_FILE&) = delete;

  private:
    void
    do_throw_exception_if_io_error() const
      {
        if(::ferror_unlocked(this->m_file))
          noadl::sprintf_and_throw<runtime_error>(
              "basic_tinybuf_file: I/O error (fileno `%d`, errno `%d`)",
              ::fileno_unlocked(this->m_file), errno);
      }

  public:
    int
    getc()
      {
        int r = ::fgetc_unlocked(this->m_file);
        if(r == EOF)
          this->do_throw_exception_if_io_error();
        return (r == EOF) ? -1 : r;
      }

    void
    putc(char c)
      {
        int r = ::fputc_unlocked((unsigned char) c, this->m_file);
        if(r == EOF)
          this->do_throw_exception_if_io_error();
      }

    void
    putn(const char* s, size_t n)
      {
        size_t r = ::fwrite_unlocked(s, 1, n, this->m_file);
        if(r != n)
          this->do_throw_exception_if_io_error();
      }

    void
    puts(const char* s)
      {
        int r = ::fputs_unlocked(s, this->m_file);
        if(r == EOF)
          this->do_throw_exception_if_io_error();
      }
  };

template<typename xwcharT, typename xmbrtowcT>
size_t
do_getn_common(xwcharT* ws, size_t n, locked_FILE& file, xmbrtowcT&& xmbrtowc, ::mbstate_t& mbst)
  {
    int more_input = ::mbsinit(&mbst);
    xwcharT* wptr = ws;
    while(wptr != ws + n) {
      // Try getting the next byte if the previous character has completed.
      int ch = more_input ? file.getc() : 0;
      if(ch == -1)
        break;

      // Decode one multi-byte character.
      char mbc = (char) ch;
      int mblen = (int) xmbrtowc(wptr, &mbc, 1U, &mbst);

      switch(mblen) {
        case -3:
          // trailing surrogate written; no input consumed
          more_input = ::mbsinit(&mbst);
          wptr ++;
          break;

        case -2:
          // no character written; input incomplete but consumed
          more_input = 1;
          break;

        case -1:
          // input invalid
          mbst = { };
          more_input = 1;
          *wptr = (unsigned char) mbc;
          wptr ++;
          break;

        case 0:
          // null character written; input byte consumed
          more_input = 1;
          wptr ++;
          break;

        default:
          // multi-byte character written; input byte consumed
          more_input = 1;
          wptr ++;
          break;
      }
    }
    return (size_t) (wptr - ws);
  }

template<typename xwcharT, typename xwcrtombT>
size_t
do_putn_common(locked_FILE& file, xwcrtombT&& xwcrtomb, ::mbstate_t& mbst, const xwcharT* ws, size_t n)
  {
    size_t ntotal = 0;
    const xwcharT* wptr = ws;
    while(wptr != ws + n) {
      // Encode one multi-byte chracter.
      char mbcs[MB_LEN_MAX];
      int mblen = (int) xwcrtomb(mbcs, *wptr, &mbst);

      switch(mblen) {
        case -1:
          // input invalid
          mbst = { };
          file.putc('?');
          ntotal ++;
          wptr ++;
          break;

        case 0:
          // no character written; input incomplete but consumed
          wptr ++;
          break;

        default:
          // `mblen` bytes written; input consumed
          file.putn(mbcs, (unsigned) mblen);
          ntotal += (unsigned) mblen;
          wptr ++;
          break;
      }
    }
    return ntotal;
  }

}  // namespace

template<>
size_t
tinybuf_file::
getn(char* s, size_t n)
  {
    locked_FILE file(this->m_file);
    char* sptr = s;
    while(sptr != s + n) {
      // Try getting one byte from the file. If the operation fails, an
      // exception will be thrown.
      int ch = file.getc();
      if(ch == -1)
        break;

      // The byte is always stored, which is essential for binary files.
      *sptr = (char) ch;
      sptr ++;

      // For text files, we stop at end of line, like `fgets()`. For binary
      // files, this probably doesn't matter.
      if(ch == '\n')
        break;
    }
    return (size_t) (sptr - s);
  }

template<>
int
tinybuf_file::
getc()
  {
    locked_FILE file(this->m_file);
    int ch = file.getc();
    return ch;
  }

template<>
tinybuf_file&
tinybuf_file::
putn(const char* s, size_t n)
  {
    locked_FILE file(this->m_file);
    file.putn(s, n);
    return *this;
  }

template<>
tinybuf_file&
tinybuf_file::
putc(char c)
  {
    locked_FILE file(this->m_file);
    file.putc(c);
    return *this;
  }

template<>
tinybuf_file&
tinybuf_file::
puts(const char* s)
  {
    locked_FILE file(this->m_file);
    file.puts(s);
    return *this;
  }

template<>
size_t
wtinybuf_file::
getn(wchar_t* s, size_t n)
  {
    locked_FILE file(this->m_file);
    size_t r = do_getn_common(s, n, file, ::mbrtowc, this->m_mbst_g);
    return r;
  }

template<>
int
wtinybuf_file::
getc()
  {
    locked_FILE file(this->m_file);
    wchar_t c;
    size_t r = do_getn_common(&c, 1, file, ::mbrtowc, this->m_mbst_g);
    return r ? (int) c : -1;
  }

template<>
wtinybuf_file&
wtinybuf_file::
putn(const wchar_t* s, size_t n)
  {
    locked_FILE file(this->m_file);
    do_putn_common(file, ::wcrtomb, this->m_mbst_p, s, n);
    return *this;
  }

template<>
wtinybuf_file&
wtinybuf_file::
putc(wchar_t c)
  {
    locked_FILE file(this->m_file);
    do_putn_common(file, ::wcrtomb, this->m_mbst_p, &c, 1);
    return *this;
  }

template<>
wtinybuf_file&
wtinybuf_file::
puts(const wchar_t* s)
  {
    locked_FILE file(this->m_file);
    do_putn_common(file, ::wcrtomb, this->m_mbst_p, s, noadl::xstrlen(s));
    return *this;
  }

template<>
size_t
u16tinybuf_file::
getn(char16_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    size_t r = do_getn_common(s, n, file, ::mbrtoc16, this->m_mbst_g);
    return r;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinybuf_file: UTF-16 functions not available");
#endif
  }

template<>
int
u16tinybuf_file::
getc()
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    char16_t c;
    size_t r = do_getn_common(&c, 1, file, ::mbrtoc16, this->m_mbst_g);
    return r ? (int) c : -1;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinybuf_file: UTF-16 functions not available");
#endif
  }

template<>
u16tinybuf_file&
u16tinybuf_file::
putn(const char16_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    do_putn_common(file, ::c16rtomb, this->m_mbst_p, s, n);
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinybuf_file: UTF-16 functions not available");
#endif
  }

template<>
u16tinybuf_file&
u16tinybuf_file::
putc(char16_t c)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    do_putn_common(file, ::c16rtomb, this->m_mbst_p, &c, 1);
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinybuf_file: UTF-16 functions not available");
#endif
  }

template<>
u16tinybuf_file&
u16tinybuf_file::
puts(const char16_t* s)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    do_putn_common(file, ::c16rtomb, this->m_mbst_p, s, noadl::xstrlen(s));
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinybuf_file: UTF-16 functions not available");
#endif
  }

template<>
size_t
u32tinybuf_file::
getn(char32_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    size_t r = do_getn_common(s, n, file, ::mbrtoc32, this->m_mbst_g);
    return r;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinybuf_file: UTF-32 functions not available");
#endif
  }

template<>
int
u32tinybuf_file::
getc()
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    char32_t c;
    size_t r = do_getn_common(&c, 1, file, ::mbrtoc32, this->m_mbst_g);
    return r ? (int) c : -1;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinybuf_file: UTF-32 functions not available");
#endif
  }

template<>
u32tinybuf_file&
u32tinybuf_file::
putn(const char32_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    do_putn_common(file, ::c32rtomb, this->m_mbst_p, s, n);
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinybuf_file: UTF-32 functions not available");
#endif
  }

template<>
u32tinybuf_file&
u32tinybuf_file::
putc(char32_t c)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    do_putn_common(file, ::c32rtomb, this->m_mbst_p, &c, 1);
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinybuf_file: UTF-32 functions not available");
#endif
  }

template<>
u32tinybuf_file&
u32tinybuf_file::
puts(const char32_t* s)
  {
#ifdef HAVE_UCHAR_H
    locked_FILE file(this->m_file);
    do_putn_common(file, ::c32rtomb, this->m_mbst_p, s, noadl::xstrlen(s));
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinybuf_file: UTF-32 functions not available");
#endif
  }

template class basic_tinybuf_file<char>;
template class basic_tinybuf_file<wchar_t>;
template class basic_tinybuf_file<char16_t>;
template class basic_tinybuf_file<char32_t>;

}  // namespace rocket
