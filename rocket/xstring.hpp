// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XSTRING_
#define ROCKET_XSTRING_

#include "fwd.hpp"
namespace rocket {

#include "details/xstring.ipp"

template<typename charT>
constexpr
size_t
xstrlen(charT* str) noexcept
  {
    size_t off = 0;
    for(;;)
      if(!ROCKET_CONSTANT_P(str[off]))
        return details_xstring::xstrlen_nonconstexpr(str);
      else if(str[off] == charT())
        return off;
      else
        off++;
  }

template<typename charT>
constexpr
charT*
xstrchr(charT* str, typename identity<charT>::type target) noexcept
  {
    size_t off = 0;
    for(;;)
      if(!ROCKET_CONSTANT_P(str[off]) || !ROCKET_CONSTANT_P(target))
        return details_xstring::xstrchr_nonconstexpr(str, target);
      else if(str[off] == target)
        return str + off;
      else if(str[off] == charT())
        return nullptr;
      else
        off++;
  }

template<typename charT>
constexpr
charT*
xstrrpcpy(charT*& out, const typename identity<charT>::type* str) noexcept
  {
    size_t off = 0;
    for(;;)
      if(!ROCKET_CONSTANT_P(str[off]))
        return details_xstring::xstrrpcpy_nonconstexpr(out, str);
      else if(str[off] == charT())
        return &(*out = charT());
      else
        *(out++) = str[off++];
  }

template<typename charT>
constexpr
int
xstrcmp(const charT* lhs, const charT* rhs) noexcept
  {
    using cmp_type = typename details_xstring::xcmp_type<charT>::type;
    size_t off = 0;
    for(;;)
      if(!ROCKET_CONSTANT_P(lhs[off]) || !ROCKET_CONSTANT_P(rhs[off]))
        return details_xstring::xstrcmp_nonconstexpr(lhs, rhs);
      else if(lhs[off] != rhs[off])
        return ((cmp_type) lhs[off] < (cmp_type) rhs[off]) ? -1 : 1;
      else if(lhs[off] == charT())
        return 0;
      else
        off++;
  }

template<typename charT>
constexpr
charT*
xmemchr(charT* str, typename identity<charT>::type target, size_t len) noexcept
  {
    size_t off = 0;
    for(;;)
      if(off >= len)
        return nullptr;
      else if(!ROCKET_CONSTANT_P(str[off]) || !ROCKET_CONSTANT_P(target) || !ROCKET_CONSTANT_P(len))
        return details_xstring::xmemchr_nonconstexpr(str, target, len);
      else if(str[off] == target)
        return str + off;
      else
        off++;
  }

template<typename charT>
constexpr
charT*
xmemrpset(charT*& out, typename identity<charT>::type elem, size_t len) noexcept
  {
    size_t off = 0;
    for(;;)
      if(off >= len)
        return out;
      else if(!ROCKET_CONSTANT_P(elem) || !ROCKET_CONSTANT_P(len))
        return details_xstring::xmemrpset_nonconstexpr(out, elem, len);
      else
        *(out++) = (off++, elem);
  }

template<typename charT>
constexpr
charT*
xmemrpcpy(charT*& out, const typename identity<charT>::type* str, size_t len) noexcept
  {
    size_t off = 0;
    for(;;)
      if(off >= len)
        return out;
      else if(!ROCKET_CONSTANT_P(str[off]) || !ROCKET_CONSTANT_P(len))
        return details_xstring::xmemrpcpy_nonconstexpr(out, str, len);
      else
        *(out++) = str[off++];
  }

template<typename charT>
constexpr
int
xmemcmp(const charT* lhs, const charT* rhs, size_t len) noexcept
  {
    using cmp_type = typename details_xstring::xcmp_type<charT>::type;
    size_t off = 0;
    for(;;)
      if(off >= len)
        return 0;
      else if(!ROCKET_CONSTANT_P(lhs[off]) || !ROCKET_CONSTANT_P(rhs[off]) || !ROCKET_CONSTANT_P(len))
        return details_xstring::xmemcmp_nonconstexpr(lhs, rhs, len);
      else if(lhs[off] != rhs[off])
        return ((cmp_type) lhs[off] < (cmp_type) rhs[off]) ? -1 : 1;
      else
        off++;
  }

}  // namespace rocket
#endif
