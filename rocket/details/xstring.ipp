// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XSTRING_
#  error Please include <rocket/xstring.hpp> instead.
#endif
namespace details_xstring {

inline namespace maybe_constexpr
  {
    template<typename charT>
    constexpr
    size_t
    ystrlen(charT* str) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(str[ki] == charT())
            return ki;
          else
            ki++;
      }
  }

inline
size_t
ystrlen(const char* str) noexcept
  {
    return ::strlen(str);
  }

inline
size_t
ystrlen(const unsigned char* str) noexcept
  {
    return ::strlen((const char*) str);
  }

inline
size_t
ystrlen(const wchar_t* str) noexcept
  {
    return ::wcslen(str);
  }

inline namespace maybe_constexpr
  {
    template<typename charT, typename ycharT>
    constexpr
    charT*
    ystrchr(charT* str,  ycharT target) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(str[ki] == target)
            return ::std::addressof(str[ki]);
          else if(str[ki] == charT())
            return nullptr;
          else
            ki++;
      }
  }

inline
char*
ystrchr(const char* str, char target) noexcept
  {
    return (char*) ::strchr(str, (unsigned char) target);
  }

inline
unsigned char*
ystrchr(const unsigned char* str, unsigned char target) noexcept
  {
    return (unsigned char*) ::strchr((const char*) str, target);
  }

inline
wchar_t*
ystrchr(const wchar_t* str, wchar_t target) noexcept
  {
    return (wchar_t*) ::wcschr(str, target);
  }

inline namespace maybe_constexpr
  {
    template<typename charT, typename ycharT>
    constexpr
    charT*
    ymemchr(charT* str, ycharT target, size_t len) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(ki >= len)
            return nullptr;
          else if(str[ki] == target)
            return ::std::addressof(str[ki]);
          else
            ki++;
      }
  }

inline
char*
ymemchr(const char* str, char target, size_t len) noexcept
  {
    return (char*) ::memchr(str, (unsigned char) target, len);
  }

inline
unsigned char*
ymemchr(const unsigned char* str, unsigned char target, size_t len) noexcept
  {
    return (unsigned char*) ::memchr(str, target, len);
  }

inline
wchar_t*
ymemchr(const wchar_t* str, wchar_t target, size_t len) noexcept
  {
    return (wchar_t*) ::wmemchr(str, target, len);
  }

inline namespace maybe_constexpr
  {
    template<typename charT, typename ycharT>
    constexpr
    int
    ystrcmp(const charT* lhs, const ycharT* rhs) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(lhs[ki] != rhs[ki])
            return (noadl::int_from(lhs[ki]) < noadl::int_from(rhs[ki])) ? -1 : 1;
          else if(lhs[ki] == charT())
            return 0;
          else
            ki++;
      }
  }

inline
int
ystrcmp(const char* lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs);
  }

inline
int
ystrcmp(const unsigned char* lhs, const unsigned char* rhs) noexcept
  {
    return ::strcmp((const char*) lhs, (const char*) rhs);
  }

inline
int
ystrcmp(const wchar_t* lhs, const wchar_t* rhs) noexcept
  {
    return ::wcscmp(lhs, rhs);
  }

inline namespace maybe_constexpr
  {
    template<typename charT, typename ycharT>
    constexpr
    int
    ymemcmp(const charT* lhs, const ycharT* rhs, size_t len) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(ki >= len)
            return 0;
          else if(lhs[ki] != rhs[ki])
            return (noadl::int_from(lhs[ki]) < noadl::int_from(rhs[ki])) ? -1 : 1;
          else
            ki++;
      }
  }

inline
int
ymemcmp(const char* lhs, const char* rhs, size_t len) noexcept
  {
    return ::memcmp(lhs, rhs, len);
  }

inline
int
ymemcmp(const unsigned char* lhs, const unsigned char* rhs, size_t len) noexcept
  {
    return ::memcmp(lhs, rhs, len);
  }

inline
int
ymemcmp(const wchar_t* lhs, const wchar_t* rhs, size_t len) noexcept
  {
    return ::wmemcmp(lhs, rhs, len);
  }

inline namespace maybe_constexpr
  {
    template<typename charT, typename ycharT>
    constexpr
    charT*
    ymempset(charT* out, ycharT elem, size_t len) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(ki >= len)
            return ::std::addressof(out[ki]);
          else
            out[ki] = elem, ki ++;
      }
  }

inline
char*
ymempset(char* out, char elem, size_t len) noexcept
  {
    return (char*) ::memset(out, (unsigned char) elem, len) + len;
  }

inline
unsigned char*
ymempset(unsigned char* out, unsigned char elem, size_t len) noexcept
  {
    return (unsigned char*) ::memset(out, elem, len) + len;
  }

inline
wchar_t*
ymempset(wchar_t* out, wchar_t elem, size_t len) noexcept
  {
    return ::wmemset(out, elem, len) + len;
  }

inline namespace maybe_constexpr
  {
    template<typename charT, typename ycharT>
    constexpr
    charT*
    ystrpcpy(charT* out, const ycharT* str) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(str[ki] == charT())
            return ::std::addressof(out[ki] = charT());
          else
            out[ki] = str[ki], ki ++;
      }
  }

inline
char*
ystrpcpy(char* out, const char* str) noexcept
  {
    return ::stpcpy(out, str);
  }

inline
unsigned char*
ystrpcpy(unsigned char* out, const unsigned char* str) noexcept
  {
    return (unsigned char*) ::stpcpy((char*) out, (const char*) str);
  }

inline
wchar_t*
ystrpcpy(wchar_t* out, const wchar_t* str) noexcept
  {
    return ::wcpcpy(out, str);
  }

inline namespace maybe_constexpr
  {
    template<typename charT, typename ycharT>
    constexpr
    charT*
    ymempcpy(charT* out, const ycharT* str, size_t len) noexcept
      {
        size_t ki = 0;
        for(;;)
          if(ki >= len)
            return ::std::addressof(out[ki]);
          else
            out[ki] = str[ki], ki ++;
      }
  }

inline
char*
ymempcpy(char* out, const char* str, size_t len) noexcept
  {
    return (char*) ::mempcpy(out, str, len);
  }

inline
unsigned char*
ymempcpy(unsigned char* out, const unsigned char* str, size_t len) noexcept
  {
    return (unsigned char*) ::mempcpy(out, str, len);
  }

inline
wchar_t*
ymempcpy(wchar_t* out, const wchar_t* str, size_t len) noexcept
  {
    return (wchar_t*) ::wmempcpy(out, str, len);
  }

}  // namespace details_xstring
