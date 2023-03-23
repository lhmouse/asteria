// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XSTRING_
#  error Please include <rocket/xstring.hpp> instead.
#endif
namespace details_xstring {

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
size_t
xstrlen_nonconstexpr(charT* str) noexcept
  {
    size_t off = 0;
    for(;;)
      if(str[off] == charT())
        return off;
      else
        off++;
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
    size_t off = 0;
    for(;;)
      if(str[off] == target)
        return str + off;
      else if(str[off] == charT())
        return nullptr;
      else
        off++;
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
xstrrpcpy_nonconstexpr(charT*& out, const typename identity<charT>::type* str) noexcept
  {
    size_t off = 0;
    for(;;)
      if(str[off] == charT())
        return &(*out = charT());
      else
        *(out++) = str[off++];
  }

inline
char*
xstrrpcpy_nonconstexpr(char*& out, const char* str) noexcept
  {
    return out = ::stpcpy(out, str);
  }

inline
unsigned char*
xstrrpcpy_nonconstexpr(unsigned char*& out, const unsigned char* str) noexcept
  {
    return out = (unsigned char*) ::stpcpy((char*) out, (const char*) str);
  }

inline
wchar_t*
xstrrpcpy_nonconstexpr(wchar_t*& out, const wchar_t* str) noexcept
  {
    return out = ::wcpcpy(out, str);
  }

template<typename charT>
int
xstrcmp_nonconstexpr(const charT* lhs, const charT* rhs) noexcept
  {
    using cmp_type = typename xcmp_type<charT>::type;
    size_t off = 0;
    for(;;)
      if(lhs[off] != rhs[off])
        return ((cmp_type) lhs[off] < (cmp_type) rhs[off]) ? -1 : 1;
      else if(lhs[off] == charT())
        return 0;
      else
        off++;
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
charT*
xmemchr_nonconstexpr(charT* str, typename identity<charT>::type target, size_t len) noexcept
  {
    size_t off = 0;
    for(;;)
      if(off >= len)
        return nullptr;
      else if(str[off] == target)
        return str + off;
      else
        off++;
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
charT*
xmemrpset_nonconstexpr(charT*& out, typename identity<charT>::type elem, size_t len) noexcept
  {
    size_t off = 0;
    for(;;)
      if(off >= len)
        return out;
      else
        *(out++) = (off++, elem);
  }

inline
char*
xmemrpset_nonconstexpr(char*& out, char elem, size_t len) noexcept
  {
    return out = (char*) ::memset(out, (unsigned char) elem, len) + len;
  }

inline
unsigned char*
xmemrpset_nonconstexpr(unsigned char*& out, unsigned char elem, size_t len) noexcept
  {
    return out = (unsigned char*) ::memset(out, elem, len) + len;
  }

inline
wchar_t*
xmemrpset_nonconstexpr(wchar_t*& out, wchar_t elem, size_t len) noexcept
  {
    return out = ::wmemset(out, elem, len) + len;
  }

template<typename charT>
charT*
xmemrpcpy_nonconstexpr(charT*& out, const typename identity<charT>::type* str, size_t len) noexcept
  {
    size_t off = 0;
    for(;;)
      if(off >= len)
        return out;
      else
        *(out++) = str[off++];
  }

inline
char*
xmemrpcpy_nonconstexpr(char*& out, const char* str, size_t len) noexcept
  {
    return out = (char*) ::mempcpy(out, str, len);
  }

inline
unsigned char*
xmemrpcpy_nonconstexpr(unsigned char*& out, const unsigned char* str, size_t len) noexcept
  {
    return out = (unsigned char*) ::mempcpy(out, str, len);
  }

inline
wchar_t*
xmemrpcpy_nonconstexpr(wchar_t*& out, const wchar_t* str, size_t len) noexcept
  {
    return out = ::wmempcpy(out, str, len);
  }

template<typename charT>
int
xmemcmp_nonconstexpr(const charT* lhs, const charT* rhs, size_t len) noexcept
  {
    using cmp_type = typename details_xstring::xcmp_type<charT>::type;
    size_t off = 0;
    for(;;)
      if(off >= len)
        return 0;
      else if(lhs[off] != rhs[off])
        return ((cmp_type) lhs[off] < (cmp_type) rhs[off]) ? -1 : 1;
      else
        off++;
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

}  // namespace details_xstring
