// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xuchar.hpp"
#include "unique_ptr.hpp"
#include "xthrow.hpp"
#include <limits.h>  // MB_LEN_MAX
#include <wchar.h>
#ifdef HAVE_UCHAR_H
#  include <uchar.h>
#endif
namespace rocket {
namespace {

inline
void
do_throw_if_error(::FILE* fp)
  {
    if(::ferror(fp))
      sprintf_and_throw<runtime_error>(
          "uchar_io: I/O error (fileno `%d`, errno `%d`)",
          ::fileno(fp), errno);
  }

template<typename xwcharT, typename xmbrtowcT>
size_t
do_xfgetn_common(::FILE* fp, xmbrtowcT&& xmbrtowc, ::mbstate_t& mbst, xwcharT* ws, size_t n)
  {
    ::flockfile(fp);
    const unique_ptr<::FILE, void (::FILE*)> lock(fp, ::funlockfile);

    int wants_more_input = ::mbsinit(&mbst);
    xwcharT* wptr = ws;
    while(wptr != ws + n) {
      char mbc = 0;
      if(wants_more_input) {
        // Try getting the next byte if the previous character has completed.
        int ch = getc_unlocked(fp);
        if(ch == -1) {
          do_throw_if_error(fp);
          break;
        }
        mbc = static_cast<char>(ch);
      }

      // Decode one multi-byte character.
      int mblen = static_cast<int>(xmbrtowc(wptr, &mbc, 1U, &mbst));

      switch(mblen) {
        case -3:
          // trailing surrogate written; no input consumed
          wants_more_input = ::mbsinit(&mbst);
          wptr ++;
          break;

        case -2:
          // no character written; input incomplete but consumed
          wants_more_input = 1;
          break;

        case -1:
          // input invalid
          mbst = { };
          wants_more_input = 1;
          *wptr = static_cast<unsigned char>(mbc);
          wptr ++;
          break;

        case 0:
          // null character written; input byte consumed
          wants_more_input = 1;
          wptr ++;
          break;

        default:
          // multi-byte character written; input byte consumed
          wants_more_input = 1;
          wptr ++;
          break;
      }
    }
    return static_cast<size_t>(wptr - ws);
  }

template<typename xwcharT, typename xwcrtombT>
size_t
do_xfputn_common(::FILE* fp, xwcrtombT&& xwcrtomb, ::mbstate_t& mbst, const xwcharT* ws, size_t n)
  {
    ::flockfile(fp);
    const unique_ptr<::FILE, void (::FILE*)> lock(fp, ::funlockfile);

    size_t nbytes = 0;
    const xwcharT* wptr = ws;
    while(wptr != ws + n) {
      // Encode one multi-byte character.
      char mbcs[MB_LEN_MAX];
      int mblen = static_cast<int>(xwcrtomb(mbcs, *wptr, &mbst));

      switch(mblen) {
        case -1:
          // input invalid
          mbst = { };
          if(putc_unlocked('?', fp) == EOF)
            do_throw_if_error(fp);

          nbytes ++;
          wptr ++;
          break;

        case 0:
          // no character written; input incomplete but consumed
          wptr ++;
          break;

        default:
          // `mblen` bytes written; input consumed
          for(unsigned k = 0;  k < static_cast<unsigned>(mblen);  ++k)
            if(putc_unlocked(mbcs[k], fp) == EOF)
              do_throw_if_error(fp);

          nbytes += static_cast<unsigned>(mblen);
          wptr ++;
          break;
      }
    }
    return nbytes;
  }

}  // namespace

size_t
xfgetn(::FILE* fp, ::mbstate_t& /*mbst*/, char* s, size_t n)
  {
    ::flockfile(fp);
    const unique_ptr<::FILE, void (::FILE*)> lock(fp, ::funlockfile);

    char* sptr = s;
    while(sptr != s + n) {
      // Try getting one byte from the file. If the operation fails, an
      // exception will be thrown.
      int ch = getc_unlocked(fp);
      if(ch == -1) {
        do_throw_if_error(fp);
        break;
      }

      // The byte is always stored, which is essential for binary files.
      *sptr = static_cast<char>(ch);
      sptr ++;

      // For text files, we stop at end of line, like `fgets()`. For binary
      // files, this probably doesn't matter.
      if(ch == '\n')
        break;
    }
    return static_cast<size_t>(sptr - s);
  }

size_t
xfgetn(::FILE* fp, ::mbstate_t& mbst, wchar_t* s, size_t n)
  {
    size_t r = do_xfgetn_common(fp, ::mbrtowc, mbst, s, n);
    return r;
  }

size_t
xfgetn(::FILE* fp, ::mbstate_t& mbst, char16_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfgetn_common(fp, ::mbrtoc16, mbst, s, n);
    return r;
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-16 functions not available");
#endif
  }

size_t
xfgetn(::FILE* fp, ::mbstate_t& mbst, char32_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfgetn_common(fp, ::mbrtoc32, mbst, s, n);
    return r;
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-32 functions not available");
#endif
  }

int
xfgetc(::FILE* fp, ::mbstate_t& /*mbst*/, char& c)
  {
    ::flockfile(fp);
    const unique_ptr<::FILE, void (::FILE*)> lock(fp, ::funlockfile);

    int ch = getc_unlocked(fp);
    if(ch == EOF)
      do_throw_if_error(fp);
    c = static_cast<char>(ch);
    return (ch == EOF) ? -1 : ch;
  }

int
xfgetc(::FILE* fp, ::mbstate_t& mbst, wchar_t& c)
  {
    size_t r = do_xfgetn_common(fp, ::mbrtowc, mbst, &c, 1);
    return (r == 0) ? -1 : static_cast<int>(c);
  }

int
xfgetc(::FILE* fp, ::mbstate_t& mbst, char16_t& c)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfgetn_common(fp, ::mbrtoc16, mbst, &c, 1);
    return (r == 0) ? -1 : static_cast<int>(c);
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-16 functions not available");
#endif
  }

int
xfgetc(::FILE* fp, ::mbstate_t& mbst, char32_t& c)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfgetn_common(fp, ::mbrtoc32, mbst, &c, 1);
    return (r == 0) ? -1 : static_cast<int>(c);
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-32 functions not available");
#endif
  }

size_t
xfputn(::FILE* fp, ::mbstate_t& /*mbst*/, const char* s, size_t n)
  {
    ::flockfile(fp);
    const unique_ptr<::FILE, void (::FILE*)> lock(fp, ::funlockfile);

    size_t r = ::fwrite(s, 1, n, fp);
    if(r != n)
      do_throw_if_error(fp);
    return r;
  }

size_t
xfputn(::FILE* fp, ::mbstate_t& mbst, const wchar_t* s, size_t n)
  {
    size_t r = do_xfputn_common(fp, ::wcrtomb, mbst, s, n);
    return r;
  }

size_t
xfputn(::FILE* fp, ::mbstate_t& mbst, const char16_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfputn_common(fp, ::c16rtomb, mbst, s, n);
    return r;
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-16 functions not available");
#endif
  }

size_t
xfputn(::FILE* fp, ::mbstate_t& mbst, const char32_t* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfputn_common(fp, ::c32rtomb, mbst, s, n);
    return r;
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-32 functions not available");
#endif
  }

int
xfputc(::FILE* fp, ::mbstate_t& /*mbst*/, char c)
  {
    ::flockfile(fp);
    const unique_ptr<::FILE, void (::FILE*)> lock(fp, ::funlockfile);

    int ch = putc_unlocked(static_cast<unsigned char>(c), fp);
    if(ch == EOF)
      do_throw_if_error(fp);
    return ch;
  }

int
xfputc(::FILE* fp, ::mbstate_t& mbst, wchar_t c)
  {
    size_t r = do_xfputn_common(fp, ::wcrtomb, mbst, &c, 1);
    return (r == 0) ? -1 : static_cast<int>(c);
  }

int
xfputc(::FILE* fp, ::mbstate_t& mbst, char16_t c)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfputn_common(fp, ::c16rtomb, mbst, &c, 1);
    return (r == 0) ? -1 : static_cast<int>(c);
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-16 functions not available");
#endif
  }

int
xfputc(::FILE* fp, ::mbstate_t& mbst, char32_t c)
  {
#ifdef HAVE_UCHAR_H
    size_t r = do_xfputn_common(fp, ::c32rtomb, mbst, &c, 1);
    return (r == 0) ? -1 : static_cast<int>(c);
#else
    sprintf_and_throw<domain_error>(
        "uchar_io: UTF-32 functions not available");
#endif
  }

}  // namespace rocket
