// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XSTRING_
#  error Please include <rocket/xstring.hpp> instead.
#endif
namespace details_xstring {

template<typename charT>
size_t
xstrlen_nonconstexpr(charT* str) noexcept
  {
    size_t ki = 0;
    for(;;)
      if(str[ki] == charT())
        return ki;
      else
        ki++;
  }

inline
size_t
xstrlen_nonconstexpr(const char* str) noexcept
  {
    return ::strlen(str);
  }

inline
size_t
xstrlen_nonconstexpr(const unsigned char* str) noexcept
  {
    return ::strlen((const char*) str);
  }

inline
size_t
xstrlen_nonconstexpr(const wchar_t* str) noexcept
  {
    return ::wcslen(str);
  }

template<typename charT>
charT*
xstrchr_nonconstexpr(charT* str,  typename identity<charT>::type target) noexcept
  {
    size_t ki = 0;
    for(;;)
      if(str[ki] == target)
        return str + ki;
      else if(str[ki] == charT())
        return nullptr;
      else
        ki++;
  }

inline
char*
xstrchr_nonconstexpr(const char* str, char target) noexcept
  {
    return (char*) ::strchr(str, (unsigned char) target);
  }

inline
unsigned char*
xstrchr_nonconstexpr(const unsigned char* str, unsigned char target) noexcept
  {
    return (unsigned char*) ::strchr((const char*) str, target);
  }

inline
wchar_t*
xstrchr_nonconstexpr(const wchar_t* str, wchar_t target) noexcept
  {
    return (wchar_t*) ::wcschr(str, target);
  }

template<typename charT>
charT*
xmemchr_nonconstexpr(charT* str, typename identity<charT>::type target, size_t len) noexcept
  {
    size_t ki = 0;
    for(;;)
      if(ki >= len)
        return nullptr;
      else if(str[ki] == target)
        return str + ki;
      else
        ki++;
  }

inline
char*
xmemchr_nonconstexpr(const char* str, char target, size_t len) noexcept
  {
    return (char*) ::memchr(str, (unsigned char) target, len);
  }

inline
unsigned char*
xmemchr_nonconstexpr(const unsigned char* str, unsigned char target, size_t len) noexcept
  {
    return (unsigned char*) ::memchr(str, target, len);
  }

inline
wchar_t*
xmemchr_nonconstexpr(const wchar_t* str, wchar_t target, size_t len) noexcept
  {
    return (wchar_t*) ::wmemchr(str, target, len);
  }

template<typename charT>
struct xcmp_type
  : identity<charT>
  { };

template<>
struct xcmp_type<char>
  : identity<unsigned char>
  { };

#if __cpp_lib_byte + 0 >= 201603
template<>
struct xcmp_type<::std::byte>
  : identity<unsigned char>
  { };
#endif  // __cpp_lib_byte

template<typename charT>
int
xstrcmp_nonconstexpr(const charT* lhs, const charT* rhs) noexcept
  {
    using cmp_type = typename xcmp_type<charT>::type;
    size_t ki = 0;
    for(;;)
      if(lhs[ki] != rhs[ki])
        return ((cmp_type) lhs[ki] < (cmp_type) rhs[ki]) ? -1 : 1;
      else if(lhs[ki] == charT())
        return 0;
      else
        ki++;
  }

inline
int
xstrcmp_nonconstexpr(const char* lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs);
  }

inline
int
xstrcmp_nonconstexpr(const unsigned char* lhs, const unsigned char* rhs) noexcept
  {
    return ::strcmp((const char*) lhs, (const char*) rhs);
  }

inline
int
xstrcmp_nonconstexpr(const wchar_t* lhs, const wchar_t* rhs) noexcept
  {
    return ::wcscmp(lhs, rhs);
  }

template<typename charT>
int
xmemcmp_nonconstexpr(const charT* lhs, const charT* rhs, size_t len) noexcept
  {
    using cmp_type = typename details_xstring::xcmp_type<charT>::type;
    size_t ki = 0;
    for(;;)
      if(ki >= len)
        return 0;
      else if(lhs[ki] != rhs[ki])
        return ((cmp_type) lhs[ki] < (cmp_type) rhs[ki]) ? -1 : 1;
      else
        ki++;
  }

inline
int
xmemcmp_nonconstexpr(const char* lhs, const char* rhs, size_t len) noexcept
  {
    return ::memcmp(lhs, rhs, len);
  }

inline
int
xmemcmp_nonconstexpr(const unsigned char* lhs, const unsigned char* rhs, size_t len) noexcept
  {
    return ::memcmp(lhs, rhs, len);
  }

inline
int
xmemcmp_nonconstexpr(const wchar_t* lhs, const wchar_t* rhs, size_t len) noexcept
  {
    return ::wmemcmp(lhs, rhs, len);
  }

template<typename charT>
charT*
xmempset_nonconstexpr(charT* out, typename identity<charT>::type elem, size_t len) noexcept
  {
    size_t ki = 0;
    for(;;)
      if(ki >= len)
        return out + ki;
      else
        out[ki] = elem, ki ++;
  }

inline
char*
xmempset_nonconstexpr(char* out, char elem, size_t len) noexcept
  {
    return (char*) ::memset(out, (unsigned char) elem, len) + len;
  }

inline
unsigned char*
xmempset_nonconstexpr(unsigned char* out, unsigned char elem, size_t len) noexcept
  {
    return (unsigned char*) ::memset(out, elem, len) + len;
  }

inline
wchar_t*
xmempset_nonconstexpr(wchar_t* out, wchar_t elem, size_t len) noexcept
  {
    return ::wmemset(out, elem, len) + len;
  }

template<typename charT>
charT*
xstrpcpy_nonconstexpr(charT* out, const typename identity<charT>::type* str) noexcept
  {
    size_t ki = 0;
    for(;;)
      if((out[ki] = str[ki]) == charT())
        return out + ki;
      else
        ki ++;
  }

inline
char*
xstrpcpy_nonconstexpr(char* out, const char* str) noexcept
  {
    return ::stpcpy(out, str);
  }

inline
unsigned char*
xstrpcpy_nonconstexpr(unsigned char* out, const unsigned char* str) noexcept
  {
    return (unsigned char*) ::stpcpy((char*) out, (const char*) str);
  }

inline
wchar_t*
xstrpcpy_nonconstexpr(wchar_t* out, const wchar_t* str) noexcept
  {
    return ::wcpcpy(out, str);
  }

template<typename charT>
charT*
xmempcpy_nonconstexpr(charT* out, const typename identity<charT>::type* str, size_t len) noexcept
  {
    size_t ki = 0;
    for(;;)
      if(ki >= len)
        return out + ki;
      else
        out[ki] = str[ki], ki ++;
  }

inline
char*
xmempcpy_nonconstexpr(char* out, const char* str, size_t len) noexcept
  {
    return (char*) ::memcpy(out, str, len) + len;
  }

inline
unsigned char*
xmempcpy_nonconstexpr(unsigned char* out, const unsigned char* str, size_t len) noexcept
  {
    return (unsigned char*) ::memcpy(out, str, len) + len;
  }

inline
wchar_t*
xmempcpy_nonconstexpr(wchar_t* out, const wchar_t* str, size_t len) noexcept
  {
    return ::wmemcpy(out, str, len) + len;
  }

}  // namespace details_xstring
