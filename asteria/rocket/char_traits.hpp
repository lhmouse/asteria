// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CHAR_TRAITS_HPP_
#define ROCKET_CHAR_TRAITS_HPP_

#include "utilities.hpp"
#include "throw.hpp"
#include <cstring>  // std::memcpy(), etc.
#include <cstdio>  // std::FILE, EOF
#include <cwchar>  // WEOF

namespace rocket {

template<typename charT> struct char_traits;

template<> struct char_traits<char>
  {
    using char_type  = char;
    using int_type   = int;
    using size_type  = size_t;

    static constexpr char_type to_char_type(int_type ch) noexcept
      {
        return (char)ch;
      }
    static constexpr int_type to_int_type(char_type c) noexcept
      {
        return (unsigned char)c;
      }

    static constexpr char_type& assign(char_type& r, char_type c) noexcept
      {
        return r = c;
      }
    static bool eq(char_type x, char_type y) noexcept
      {
        return x == y;
      }
    static bool lt(char_type x, char_type y) noexcept
      {
        return (unsigned char)x < (unsigned char)y;
      }

    static char_type* assign(char_type* p, size_type n, char_type c) noexcept
      {
        return (char*)::std::memset(p, c, n);
      }
    static char_type* copy(char_type* p, const char_type* s, size_type n) noexcept
      {
        return (char*)::std::memcpy(p, s, n);
      }
    static char_type* move(char_type* p, const char_type* s, size_type n) noexcept
      {
        return (char*)::std::memmove(p, s, n);
      }

    static const char_type* find(const char_type* p, size_type n, char_type c) noexcept
      {
        return (char*)::std::memchr(p, c, n);
      }
    static size_type length(const char_type* p) noexcept
      {
        return ::std::strlen(p);
      }
    static int compare(const char_type* p, const char_type* s, size_type n) noexcept
      {
        return ::std::memcmp(p, s, n);
      }

    static constexpr int_type eof() noexcept
      {
        return EOF;
      }
    static constexpr bool is_eof(int_type ch) noexcept
      {
        return ch == EOF;
      }

    static int_type fgetc(::std::FILE* fp)
      {
        return ::std::fgetc(fp);
      }
    static int_type fputc(::std::FILE* fp, char_type c)
      {
        return ::std::fputc(c, fp);
      }
    static size_type fgetn(::std::FILE* fp, char_type* p, size_type n)
      {
        return ::std::fread(p, 1, n, fp);
      }
    static size_type fputn(::std::FILE* fp, const char_type* p, size_type n)
      {
        return ::std::fwrite(p, 1, n, fp);
      }
  };

template<> struct char_traits<wchar_t>
  {
    using char_type  = wchar_t;
    using int_type   = ::std::wint_t;
    using size_type  = size_t;

    static constexpr char_type to_char_type(int_type ch) noexcept
      {
        return (wchar_t)ch;
      }
    static constexpr int_type to_int_type(char_type c) noexcept
      {
        return (wint_t)c;
      }

    static constexpr char_type& assign(char_type& r, char_type c) noexcept
      {
        return r = c;
      }
    static bool eq(char_type x, char_type y) noexcept
      {
        return x == y;
      }
    static bool lt(char_type x, char_type y) noexcept
      {
        return x < y;
      }

    static char_type* assign(char_type* p, size_type n, char_type c) noexcept
      {
        return ::std::wmemset(p, c, n);
      }
    static char_type* copy(char_type* p, const char_type* s, size_type n) noexcept
      {
        return ::std::wmemcpy(p, s, n);
      }
    static char_type* move(char_type* p, const char_type* s, size_type n) noexcept
      {
        return ::std::wmemmove(p, s, n);
      }

    static const char_type* find(const char_type* p, size_type n, char_type c) noexcept
      {
        return ::std::wmemchr(p, c, n);
      }
    static size_type length(const char_type* p) noexcept
      {
        return ::std::wcslen(p);
      }
    static int compare(const char_type* p, const char_type* s, size_type n) noexcept
      {
        return ::std::wmemcmp(p, s, n);
      }

    static constexpr int_type eof() noexcept
      {
        return WEOF;
      }
    static constexpr bool is_eof(int_type ch) noexcept
      {
        return ch == WEOF;
      }

    static int_type fgetc(::std::FILE* fp)
      {
        return ::std::fgetwc(fp);
      }
    static int_type fputc(::std::FILE* fp, char_type c)
      {
        return ::std::fputwc(c, fp);
      }
    static size_type fgetn(::std::FILE* fp, char_type* p, size_type n)
      {
        size_t k = 0;
        // Read characters one by one, as `fgetws()` doesn't like null characters.
        ::flockfile(fp);
        while(k < n) {
          ::std::wint_t ch = ::fgetwc_unlocked(fp);
          if(ROCKET_UNEXPECT(ch == WEOF))
            break;
          p[k++] = (wchar_t)ch;
        }
        ::funlockfile(fp);
        return k;
      }
    static size_type fputn(::std::FILE* fp, const char_type* p, size_type n)
      {
        size_t k = 0;
        // Write characters one by one, as `fputws()` doesn't like null characters.
        ::flockfile(fp);
        while(k < n) {
          ::std::wint_t ch = ::fputwc_unlocked(p[k], fp);
          if(ROCKET_UNEXPECT(ch == WEOF))
            break;
          k++;
        }
        ::funlockfile(fp);
        return k;
      }
  };

template<> struct char_traits<char16_t>
  {
    using char_type  = char16_t;
    using int_type   = ::std::uint_least16_t;
    using size_type  = size_t;

    static constexpr char_type to_char_type(int_type ch) noexcept
      {
        return (char16_t)ch;
      }
    static constexpr int_type to_int_type(char_type c) noexcept
      {
        return c;
      }

    static constexpr char_type& assign(char_type& r, char_type c) noexcept
      {
        return r = c;
      }
    static bool eq(char_type x, char_type y) noexcept
      {
        return x == y;
      }
    static bool lt(char_type x, char_type y) noexcept
      {
        return x < y;
      }

    static char_type* assign(char_type* p, size_type n, char_type c) noexcept
      {
        for(size_type i = 0; i != n; ++i)
          p[i] = c;
        return p;
      }
    static char_type* copy(char_type* p, const char_type* s, size_type n) noexcept
      {
        return (char16_t*)::std::memcpy(p, s, n * sizeof(char16_t));
      }
    static char_type* move(char_type* p, const char_type* s, size_type n) noexcept
      {
        return (char16_t*)::std::memmove(p, s, n * sizeof(char16_t));
      }

    static const char_type* find(const char_type* p, size_type n, char_type c) noexcept
      {
        for(size_type i = 0; i != n; ++i)
          if(p[i] == c)
            return p + i;
        return nullptr;
      }
    static size_type length(const char_type* p) noexcept
      {
        size_type k = 0;
        while(p[k] != 0)
          ++k;
        return k;
      }
    static int compare(const char_type* p, const char_type* s, size_type n) noexcept
      {
        for(size_type i = 0; i != n; ++i)
          if(p[i] != s[i])
            return (p[i] < s[i]) ? -1 : +1;
        return 0;
      }

    static constexpr int_type eof() noexcept
      {
        return UINT16_MAX;
      }
    static constexpr bool is_eof(int_type ch) noexcept
      {
        return ch >= UINT16_MAX;
      }

    [[noreturn]] static int_type fgetc(::std::FILE* /*fp*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");
      }
    [[noreturn]] static int_type fputc(::std::FILE* /*fp*/, char_type /*c*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");
      }
    [[noreturn]] static size_type fgetn(::std::FILE* /*fp*/, char_type* /*p*/, size_type /*n*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");
      }
    [[noreturn]] static size_type fputn(::std::FILE* /*fp*/, const char_type* /*p*/, size_type /*n*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char16_t` I/O not implemented");
      }
  };

template<> struct char_traits<char32_t>
  {
    using char_type  = char32_t;
    using int_type   = ::std::uint_least32_t;
    using size_type  = size_t;

    static constexpr char_type to_char_type(int_type ch) noexcept
      {
        return (char32_t)ch;
      }
    static constexpr int_type to_int_type(char_type c) noexcept
      {
        return c;
      }

    static constexpr char_type& assign(char_type& r, char_type c) noexcept
      {
        return r = c;
      }
    static bool eq(char_type x, char_type y) noexcept
      {
        return x == y;
      }
    static bool lt(char_type x, char_type y) noexcept
      {
        return x < y;
      }

    static char_type* assign(char_type* p, size_type n, char_type c) noexcept
      {
        for(size_type i = 0; i != n; ++i)
          p[i] = c;
        return p;
      }
    static char_type* copy(char_type* p, const char_type* s, size_type n) noexcept
      {
        return (char32_t*)::std::memcpy(p, s, n * sizeof(char32_t));
      }
    static char_type* move(char_type* p, const char_type* s, size_type n) noexcept
      {
        return (char32_t*)::std::memmove(p, s, n * sizeof(char32_t));
      }

    static const char_type* find(const char_type* p, size_type n, char_type c) noexcept
      {
        for(size_type i = 0; i != n; ++i)
          if(p[i] == c)
            return p + i;
        return nullptr;
      }
    static size_type length(const char_type* p) noexcept
      {
        size_type k = 0;
        while(p[k] != 0)
          ++k;
        return k;
      }
    static int compare(const char_type* p, const char_type* s, size_type n) noexcept
      {
        for(size_type i = 0; i != n; ++i)
          if(p[i] != s[i])
            return (p[i] < s[i]) ? -1 : +1;
        return 0;
      }

    static constexpr int_type eof() noexcept
      {
        return UINT32_MAX;
      }
    static constexpr bool is_eof(int_type ch) noexcept
      {
        return ch >= UINT32_MAX;
      }

    [[noreturn]] static int_type fgetc(::std::FILE* /*fp*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");
      }
    [[noreturn]] static int_type fputc(::std::FILE* /*fp*/, char_type /*c*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");
      }
    [[noreturn]] static size_type fgetn(::std::FILE* /*fp*/, char_type* /*p*/, size_type /*n*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");
      }
    [[noreturn]] static size_type fputn(::std::FILE* /*fp*/, const char_type* /*p*/, size_type /*n*/)
      {
        noadl::sprintf_and_throw<domain_error>("char_traits: `char32_t` I/O not implemented");
      }
  };

}  // namespace rocket

#endif
