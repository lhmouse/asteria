// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XSTRING_
#define ROCKET_XSTRING_

#include "fwd.hpp"
namespace rocket {

#include "details/xstring.ipp"

template<typename charT>
ROCKET_PURE constexpr
size_t
xstrlen(charT* str) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ystrlen(str))
                           ? details_xstring::maybe_constexpr::ystrlen(str)
                           : details_xstring::ystrlen(str);
  }

template<typename charT>
ROCKET_PURE constexpr
charT*
xstrchr(charT* str, typename identity<charT>::type target) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ystrchr(str, target) == str)
                           ? details_xstring::maybe_constexpr::ystrchr(str, target)
                           : details_xstring::ystrchr(str, target);
  }

template<typename charT>
ROCKET_PURE constexpr
charT*
xmemchr(charT* str, typename identity<charT>::type target, size_t len) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ymemchr(str, target, len) == str)
                           ? details_xstring::maybe_constexpr::ymemchr(str, target, len)
                           : details_xstring::ymemchr(str, target, len);
  }

template<typename charT>
ROCKET_PURE constexpr
int
xstrcmp(const charT* lhs, const charT* rhs) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ystrcmp(lhs, rhs))
                           ? details_xstring::maybe_constexpr::ystrcmp(lhs, rhs)
                           : details_xstring::ystrcmp(lhs, rhs);
  }

template<typename charT>
ROCKET_PURE constexpr
bool
xstreq(const charT* lhs, const charT* rhs) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ystrcmp(lhs, rhs))
                          ? (details_xstring::maybe_constexpr::ystrcmp(lhs, rhs) == 0)
                          : (details_xstring::ystrcmp(lhs, rhs) == 0);
  }

template<typename charT>
ROCKET_PURE constexpr
int
xmemcmp(const charT* lhs, const charT* rhs, size_t len) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len))
                           ? details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len)
                           : details_xstring::ymemcmp(lhs, rhs, len);
  }

template<typename charT>
ROCKET_PURE constexpr
int
xmemcmp(const charT* lhs, size_t llen, const charT* rhs, size_t rlen) noexcept
  {
    if(llen < rlen)
      return (noadl::xmemcmp(rhs, lhs, llen) < 0) ? 1 : -1;
    else if(llen > rlen)
      return (noadl::xmemcmp(lhs, rhs, rlen) < 0) ? -1 : 1;
    else
      return noadl::xmemcmp(lhs, rhs, llen);
  }

template<typename charT>
ROCKET_PURE constexpr
bool
xmemeq(const charT* lhs, const charT* rhs, size_t len) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len))
                          ? (details_xstring::maybe_constexpr::ymemcmp(lhs, rhs, len) == 0)
                          : ((lhs == rhs) || (::memcmp(lhs, rhs, sizeof(charT) * len) == 0));
  }

template<typename charT>
ROCKET_PURE constexpr
bool
xmemeq(const charT* lhs, size_t llen, const charT* rhs, size_t rlen) noexcept
  {
    return (llen == rlen) && noadl::xmemeq(lhs, rhs, llen);
  }

template<typename charT>
constexpr
charT*
xmempset(charT* out, typename identity<charT>::type elem, size_t len) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ymempset(out, elem, len) == out)
                           ? details_xstring::maybe_constexpr::ymempset(out, elem, len)
                           : details_xstring::ymempset(out, elem, len);
  }

template<typename charT>
inline
charT*
xmemrpset(charT*& out, typename identity<charT>::type elem, size_t len) noexcept
  {
    return out = noadl::xmempset(out, elem, len);
  }

template<typename charT>
constexpr
charT*
xstrpcpy(charT* out, const typename identity<charT>::type* str) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ystrpcpy(out, str) == out)
                           ? details_xstring::maybe_constexpr::ystrpcpy(out, str)
                           : details_xstring::ystrpcpy(out, str);
  }

template<typename charT>
inline
charT*
xstrrpcpy(charT*& out, const typename identity<charT>::type* str) noexcept
  {
    return out = noadl::xstrpcpy(out, str);
  }

template<typename charT>
constexpr
charT*
xmempcpy(charT* out, const typename identity<charT>::type* str, size_t len) noexcept
  {
    return ROCKET_CONSTANT_P(details_xstring::maybe_constexpr::ymempcpy(out, str, len) == out)
                           ? details_xstring::maybe_constexpr::ymempcpy(out, str, len)
                           : details_xstring::ymempcpy(out, str, len);
  }

template<typename charT>
inline
charT*
xmemrpcpy(charT*& out, const typename identity<charT>::type* str, size_t len) noexcept
  {
    return out = noadl::xmempcpy(out, str, len);
  }

}  // namespace rocket
#endif
