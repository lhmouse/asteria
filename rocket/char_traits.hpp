// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CHAR_TRAITS_
#define ROCKET_CHAR_TRAITS_

#include "fwd.hpp"
#include "throw.hpp"
#include <string.h>  // ::memcpy() etc
#include <wchar.h>  // ::fgetwc() etc, WEOF
#include <stdio.h>  // ::fgetc() etc, ::FILE, EOF
namespace rocket {

template<typename charT>
struct char_traits;

#include "details/char_traits.ipp"

template<>
struct char_traits<char>
  {
    using char_type  = char;
    using int_type   = int;
    using size_type  = size_t;

    static constexpr
    char_type
    to_char_type(int_type ch) noexcept
      { return (char)ch;  }

    static constexpr
    int_type
    to_int_type(char_type c) noexcept
      { return (uint8_t)c;  }

    static constexpr
    char_type&
    assign(char_type& r, char_type c) noexcept
      { return r = c;  }

    static constexpr
    bool
    eq(char_type x, char_type y) noexcept
      { return x == y;  }

    static constexpr
    bool
    lt(char_type x, char_type y) noexcept
      { return (uint8_t)x < (uint8_t)y;  }

    static
    char_type*
    copy(char_type* p, const char_type* s, size_type n) noexcept
      { return (char*)::memcpy(p, s, n);  }

    static
    char_type*
    move(char_type* p, const char_type* s, size_type n) noexcept
      { return (char*)::memmove(p, s, n);  }

    static
    char_type*
    assign(char_type* p, size_type n, char_type c) noexcept
      { return (char*)::memset(p, c, n);  }

    static
    const char_type*
    find(const char_type* p, size_type n, char_type c) noexcept
      { return (char*)::memchr(p, c, n);  }

    static constexpr
    size_type
    length(const char_type* p) noexcept
      {
        return ROCKET_CONSTANT_P(*p)
                   ? details_char_traits::length<char>(p)
                   : ::strlen(p);
      }

    static
    int
    compare(const char_type* p, const char_type* s, size_type n) noexcept
      { return ::memcmp(p, s, n);  }

    static constexpr
    int_type
    eof() noexcept
      { return EOF;  }

    static constexpr
    bool
    is_eof(int_type ch) noexcept
      { return ch == EOF;  }

    static
    int_type
    fgetc(::FILE* fp)
      { return ::fgetc(fp);  }

    static
    int_type
    fputc(::FILE* fp, char_type c)
      { return ::fputc(c, fp);  }

    static
    size_type
    fgetn(::FILE* fp, char_type* p, size_type n)
      {
        size_t k = 0;
        ::flockfile(fp);
        while(k < n) {
          int ch = ::fgetc_unlocked(fp);
          if(ROCKET_UNEXPECT(ch == EOF))
            break;
          p[k++] = (char)ch;
          if((ch == 0) || (ch == '\n'))
            break;
        }
        ::funlockfile(fp);
        return k;
      }

    static
    size_type
    fputn(::FILE* fp, const char_type* p, size_type n)
      {
        size_t k = 0;
        ::flockfile(fp);
        while(k < n) {
          int ch = ::fputc_unlocked(p[k], fp);
          if(ROCKET_UNEXPECT(ch == EOF))
            break;
          k++;
        }
        ::funlockfile(fp);
        return k;
      }
  };

template<>
struct char_traits<wchar_t>
  {
    using char_type  = wchar_t;
    using int_type   = ::wint_t;
    using size_type  = size_t;

    static constexpr
    char_type
    to_char_type(int_type ch) noexcept
      { return (wchar_t)ch;  }

    static constexpr
    int_type
    to_int_type(char_type c) noexcept
      { return (::wint_t)c;  }

    static constexpr
    char_type&
    assign(char_type& r, char_type c) noexcept
      { return r = c;  }

    static constexpr
    bool
    eq(char_type x, char_type y) noexcept
      { return x == y;  }

    static constexpr
    bool
    lt(char_type x, char_type y) noexcept
      { return x < y;  }

    static
    char_type*
    copy(char_type* p, const char_type* s, size_type n) noexcept
      { return ::wmemcpy(p, s, n);  }

    static
    char_type*
    move(char_type* p, const char_type* s, size_type n) noexcept
      { return ::wmemmove(p, s, n);  }

    static
    char_type*
    assign(char_type* p, size_type n, char_type c) noexcept
      { return ::wmemset(p, c, n);  }

    static
    const char_type*
    find(const char_type* p, size_type n, char_type c) noexcept
      { return ::wmemchr(p, c, n);  }

    static constexpr
    size_type
    length(const char_type* p) noexcept
      {
        return ROCKET_CONSTANT_P(*p)
                   ? details_char_traits::length<wchar_t>(p)
                   : ::wcslen(p);
      }

    static
    int
    compare(const char_type* p, const char_type* s, size_type n) noexcept
      { return ::wmemcmp(p, s, n);  }

    static constexpr
    int_type
    eof() noexcept
      { return WEOF;  }

    static constexpr
    bool
    is_eof(int_type ch) noexcept
      { return ch == WEOF;  }

    static
    int_type
    fgetc(::FILE* fp)
      { return ::fgetwc(fp);  }

    static
    int_type
    fputc(::FILE* fp, char_type c)
      { return ::fputwc(c, fp);  }

    static
    size_type
    fgetn(::FILE* fp, char_type* p, size_type n)
      {
        size_t k = 0;
        ::flockfile(fp);
        while(k < n) {
          ::wint_t ch = ::fgetwc_unlocked(fp);
          if(ROCKET_UNEXPECT(ch == WEOF))
            break;
          p[k++] = (wchar_t)ch;
          if((ch == 0) || (ch == L'\n'))
            break;
        }
        ::funlockfile(fp);
        return k;
      }

    static
    size_type
    fputn(::FILE* fp, const char_type* p, size_type n)
      {
        size_t k = 0;
        ::flockfile(fp);
        while(k < n) {
          ::wint_t ch = ::fputwc_unlocked(p[k], fp);
          if(ROCKET_UNEXPECT(ch == WEOF))
            break;
          k++;
        }
        ::funlockfile(fp);
        return k;
      }
  };

template<>
struct char_traits<char16_t>
  {
    using char_type  = char16_t;
    using int_type   = ::uint_least16_t;
    using size_type  = size_t;

    static constexpr
    char_type
    to_char_type(int_type ch) noexcept
      { return (char16_t)ch;  }

    static constexpr
    int_type
    to_int_type(char_type c) noexcept
      { return c;  }

    static constexpr
    char_type&
    assign(char_type& r, char_type c) noexcept
      { return r = c;  }

    static constexpr
    bool
    eq(char_type x, char_type y) noexcept
      { return x == y;  }

    static constexpr
    bool
    lt(char_type x, char_type y) noexcept
      { return x < y;  }

    static
    char_type*
    copy(char_type* p, const char_type* s, size_type n) noexcept
      { return (char16_t*)::memcpy(p, s, n * sizeof(char16_t));  }

    static
    char_type*
    move(char_type* p, const char_type* s, size_type n) noexcept
      { return (char16_t*)::memmove(p, s, n * sizeof(char16_t));  }

    static
    char_type*
    assign(char_type* p, size_type n, char_type c) noexcept
      { return details_char_traits::assign<char16_t>(p, n, c);  }

    static
    const char_type*
    find(const char_type* p, size_type n, char_type c) noexcept
      { return details_char_traits::find<char16_t>(p, n, c);  }

    static constexpr
    size_type
    length(const char_type* p) noexcept
      { return details_char_traits::length<char16_t>(p);  }

    static
    int
    compare(const char_type* p, const char_type* s, size_type n) noexcept
      { return details_char_traits::compare<char16_t>(p, s, n);  }

    static constexpr
    int_type
    eof() noexcept
      { return UINT_LEAST16_MAX;  }

    static constexpr
    bool
    is_eof(int_type ch) noexcept
      { return ch >= UINT_LEAST16_MAX;  }

    [[noreturn]] static
    int_type
    fgetc(::FILE* /*fp*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");  }

    [[noreturn]] static
    int_type
    fputc(::FILE* /*fp*/, char_type /*c*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");  }

    [[noreturn]] static
    size_type
    fgetn(::FILE* /*fp*/, char_type* /*p*/, size_type /*n*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");  }

    [[noreturn]] static
    size_type
    fputn(::FILE* /*fp*/, const char_type* /*p*/, size_type /*n*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");  }
  };

template<>
struct char_traits<char32_t>
  {
    using char_type  = char32_t;
    using int_type   = ::uint_least32_t;
    using size_type  = size_t;

    static constexpr
    char_type
    to_char_type(int_type ch) noexcept
      { return (char32_t)ch;  }

    static constexpr
    int_type
    to_int_type(char_type c) noexcept
      { return c;  }

    static constexpr
    char_type&
    assign(char_type& r, char_type c) noexcept
      { return r = c;  }

    static constexpr
    bool
    eq(char_type x, char_type y) noexcept
      { return x == y;  }

    static constexpr
    bool
    lt(char_type x, char_type y) noexcept
      { return x < y;  }

    static
    char_type*
    copy(char_type* p, const char_type* s, size_type n) noexcept
      { return (char32_t*)::memcpy(p, s, n * sizeof(char32_t));  }

    static
    char_type*
    move(char_type* p, const char_type* s, size_type n) noexcept
      { return (char32_t*)::memmove(p, s, n * sizeof(char32_t));  }

    static
    char_type*
    assign(char_type* p, size_type n, char_type c) noexcept
      { return details_char_traits::assign<char32_t>(p, n, c);  }

    static
    const char_type*
    find(const char_type* p, size_type n, char_type c) noexcept
      { return details_char_traits::find<char32_t>(p, n, c);  }

    static constexpr
    size_type
    length(const char_type* p) noexcept
      { return details_char_traits::length<char32_t>(p);  }

    static
    int
    compare(const char_type* p, const char_type* s, size_type n) noexcept
      { return details_char_traits::compare<char32_t>(p, s, n);  }

    static constexpr
    int_type
    eof() noexcept
      { return UINT_LEAST32_MAX;  }

    static constexpr
    bool
    is_eof(int_type ch) noexcept
      { return ch >= UINT_LEAST32_MAX;  }

    [[noreturn]] static
    int_type
    fgetc(::FILE* /*fp*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");  }

    [[noreturn]] static
    int_type
    fputc(::FILE* /*fp*/, char_type /*c*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");  }

    [[noreturn]] static
    size_type
    fgetn(::FILE* /*fp*/, char_type* /*p*/, size_type /*n*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");  }

    [[noreturn]] static
    size_type
    fputn(::FILE* /*fp*/, const char_type* /*p*/, size_type /*n*/)
      { noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");  }
  };

}  // namespace rocket
#endif
